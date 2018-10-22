#include <iostream>
#include <memory>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>
#include <string>

#include "board.h"

//thanks kalven for tips/debugging

using boost::asio::ip::tcp;

const std::string STD_OK("OK");
const std::string STD_EMPTY("");
const std::string STD_INTERRUPT("T05");
const std::string STD_ERROR("E00");
const std::string STD_IGNORE("------------------");

class GdbPacket {
public:
    GdbPacket(const char * data, size_t length)
    {
        printf("raw data: [%s]\n", std::string(data, length).c_str());
        size_t i = 0;
        for(; (data[i] == '$' || data[i] == '+') && i < length; ++i);
        if (i >= length) {
            this->command = 0;
            return;
        }

        this->command = data[i];

        int p = i + 1;
        int plen;
        for(plen = 0; data[i] != '#' && i < length; ++i,++plen);
        
        this->params = std::string(&data[p], plen-1);
        this->crc = std::string(&data[i+1], 2);

        printf("GdbPacket: command=%c params=%s crc=%s\n",
                this->command, this->params.c_str(), this->crc.c_str());
    }

    char get_command() const { return this->command; }
    std::string get_params() const { return this->params; }
    std::string get_crc() const { return this->crc; }

    static std::string calc_crc(std::string & s) 
    {
        int crc = 0;
        for (size_t i = 0; i < s.length(); ++i) {
            crc += s.at(i);
        }
        char buf[4];
        sprintf(buf, "%02x", crc & 0xff);
        return std::string(buf);
    }

private:
    char command;
    std::string params;
    std::string crc;
};

class Session : public std::enable_shared_from_this<Session>
{
public:
    Session(boost::asio::io_service& ios, Board & board)
        : socket(ios), board(board), closing(false) {}

    tcp::socket& get_socket()
    {
        return socket;
    }

    void start()
    {
        board.debugger_attached();
        board.onbreakpoint = [this](void) {
            std::string response("T05");
            std::string full = "+$" + response + "#" + 
                GdbPacket::calc_crc(response);
            strncpy(breakdata, full.c_str(), sizeof(breakdata)-1);
            printf("BREAKPOINT -- sending to GDB: [%s]\n", full.c_str());
            boost::asio::async_write(this->socket,
                    boost::asio::buffer(breakdata, full.length()),
                    boost::bind(&Session::handle_write_dummy, this,
                        shared_from_this(),
                        boost::asio::placeholders::error));
        };

        socket.async_read_some(
                boost::asio::buffer(data, max_length),
                boost::bind(&Session::handle_read, this,
                    shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
    }

    void handle_write_dummy(std::shared_ptr<Session>& s,
            const boost::system::error_code& err) 
    {
         printf("handle_write_dummy: whatever\n");
    }

    void handle_read(std::shared_ptr<Session>& s,
            const boost::system::error_code& err,
            size_t bytes_transferred)
    {
        printf("handle_read: ");
        if (!err) {
            printf("[%s]\n", std::string(data, bytes_transferred).c_str());
            if (data[0] == 3) {
                printf("^C\n");
            }

            GdbPacket packet(data, bytes_transferred);
            format_response(packet);

            if (strlen(data)) {
                boost::asio::async_write(socket,
                        boost::asio::buffer(data, strlen(data)),
                        boost::bind(&Session::handle_write, this,
                            shared_from_this(),
                            boost::asio::placeholders::error));
            } 
            else {
                // arm for reading more
                handle_write(s, err);
            }
        } 
        else {
            printf("ERROR\n");
            std::cerr << "err (recv): " << err.message() << std::endl;
        }
    }

    void handle_write(std::shared_ptr<Session>& s,
            const boost::system::error_code& err) 
    {
        if (this->closing) {
            std::cerr << "remote debugger detached" << std::endl;
            //delete this;
            return;
        }

        printf("handle_write: ");

        if (!err) {
            printf("ok -> async_read_some\n");
            socket.async_read_some(
                    boost::asio::buffer(data, max_length),
                    boost::bind(&Session::handle_read, this,
                        shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
        } 
        else {
            printf("ERROR\n");
            std::cerr << "err (recv): " << err.message() << std::endl;
        }
    }

    void format_response(GdbPacket &packet)
    {
        std::string response;
        switch(packet.get_command()) {
            case '\0':
                response = STD_IGNORE;
                break;
            case 3:
                board.debugger_break();
                response = STD_INTERRUPT;
                break;
            case 'c':
                board.debugger_continue();
                response = STD_IGNORE;
                break;
            case 'q':
                response = general_response(packet);
                break;
            case 'H':   // thread commands
                response = STD_OK;
                break;
            case '?':   // indicate reason for stopping
                response = "S05"; // ? what
                break;
            case 'g':   // read registers
                response = board.read_registers();
                break;
            case 'G':   // write registers
                response = write_registers(packet);
                break;
            case 'm':   // read memory location(s)
                response = read_memory(packet);
                break;
            case 'M':   // write memory location(s)
                response = write_memory(packet);
                break;
            case 'Z':   // insert breakpoint: type,addr,kind -> OK/E NN/''
                response = insert_breakpoint(packet);
                break;
            case 'z':   // remove breakpoint
                response = remove_breakpoint(packet);
                break;
            case 's':   // singlestep
                board.single_step(true);
                response = "T05";
                break;
            case 'D':   // debugger detach
                response = STD_OK;
                board.debugger_detached();
                this->closing = true;
                break;
        }

        if (response == STD_IGNORE) {
            data[0] = '+';
            data[1] = 0;
        } 
        else 
        {
            std::string full = "+$" + response + "#" + 
                GdbPacket::calc_crc(response);
            strcpy(data, full.c_str());
            printf("format_response: [%s]\n", data);
        }
    }

    std::string general_response(GdbPacket & packet)
    {
        if (boost::algorithm::starts_with(packet.get_params(), "Supported")) {
            return "PacketSize=4000";
        }
        else if (packet.get_params() == "C") {
            // return thread id
            return STD_EMPTY;
        }
        else if (packet.get_params() == "Attached") {
            // server attached to an existing process
            return "1"; 
        }
        else if (packet.get_params() == "TStatus") {
            return STD_EMPTY;
        }
        return STD_OK;
    }

    std::string read_memory(GdbPacket & packet)
    {
        using namespace std;
        vector<string> parts;
        string params = packet.get_params();
        boost::algorithm::split(parts, params, boost::is_any_of(","));
        
        try {
            int addr = stoul(parts.at(0), nullptr, 16);
            int len = stoul(parts.at(1), nullptr, 16);
            return board.read_memory(addr, len);
        } catch(invalid_argument&) {
            return STD_ERROR;
        }
    }

    std::string write_memory(GdbPacket & packet)
    {
        using namespace std;
        vector<string> parts;
        string params = packet.get_params();
        boost::algorithm::split(parts, params, boost::is_any_of(",:"));
        
        try {
            int addr = stoul(parts.at(0), nullptr, 16);
            int len = stoul(parts.at(1), nullptr, 16);

            if (len > 0) {
                const char * ostr = parts.at(2).c_str();
                for (int i = 0, end = strlen(ostr)/2; i < len && i < end; ++i) {
                    uint8_t bytt = stoul(string(&ostr[i*2], 2), nullptr, 16);
                    printf("W %04x,%02x", addr + i, bytt);
                    board.write_memory_byte(addr + i, bytt);
                }
            }

            return STD_OK;
        } catch(invalid_argument&) {
            return STD_ERROR;
        }
    }

    std::string write_registers(GdbPacket & packet)
    {
        uint8_t regs[26];
        const char * cstr = packet.get_params().c_str();

        for (size_t i = 0; i < sizeof(regs); ++i) {
            try {
                regs[i] = std::stoul(string(&cstr[i*2], 2), nullptr, 16);
            } catch(invalid_argument&) {
                return STD_ERROR;
            }
        }
        board.write_registers(regs);
        return STD_OK;
    }

    std::string insert_breakpoint(GdbPacket & packet)
    {
        using namespace std;
        vector<string> parts;
        string params = packet.get_params();
        boost::algorithm::split(parts, params, boost::is_any_of(","));
        try {
            int type = stoul(parts.at(0), nullptr, 10);
            int addr = stoul(parts.at(1), nullptr, 16);
            int kind = stoul(parts.at(2), nullptr, 10);

            return board.insert_breakpoint(type, addr, kind);
        } catch (invalid_argument&) {
            return STD_ERROR;
        }
    }

    std::string remove_breakpoint(GdbPacket & packet)
    {
        using namespace std;
        vector<string> parts;
        string params = packet.get_params();
        boost::algorithm::split(parts, params, boost::is_any_of(","));
        try {
            int type = stoul(parts.at(0), nullptr, 10);
            int addr = stoul(parts.at(1), nullptr, 16);
            int kind = stoul(parts.at(2), nullptr, 10);

            return board.remove_breakpoint(type, addr, kind);
        } catch (invalid_argument&) {
            return STD_ERROR;
        }
    }


private:
    tcp::socket socket;
    enum { max_length = 4096 };
    char data[max_length];
    char breakdata[max_length];
    Board & board;
    bool closing;
};

class Server {
public:
    Server(boost::asio::io_service& ios, short port, Board & board) 
        : ios(ios), acceptor(ios, tcp::endpoint(tcp::v4(), port)), board(board)
    {
        std::shared_ptr<Session> session = std::make_shared<Session>(ios, board);
        acceptor.async_accept(session->get_socket(),
                boost::bind(&Server::handle_accept, this,
                    session,
                    boost::asio::placeholders::error));
    }

    void handle_accept(std::shared_ptr<Session> session,
            const boost::system::error_code& err)
    {
        if (!err) {
            session->start();
            session = std::make_shared<Session>(ios, board);
            acceptor.async_accept(session->get_socket(),
                    boost::bind(&Server::handle_accept, this, session,
                        boost::asio::placeholders::error));
        }
        else {
            std::cerr << "err: " + err.message() << std::endl;
            session.reset();
        }
    }

private:
    boost::asio::io_service& ios;
    tcp::acceptor acceptor;
    Board & board;
};

boost::asio::io_service iosrv;
Server * srv;

void start_gdb_server(Board & board)
{
    srv = new Server(iosrv, 4000, board);
}

void poll_gdb_server()
{
    iosrv.poll_one();
}

void stop_gdb_server()
{
    delete srv;
}


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
const std::string STD_IGNORE("------------------");

class GdbPacket {
public:
    GdbPacket(const char * data, size_t length)
    {
        printf("raw data: [%s]\n", std::string(data, length).c_str());
        int i = 0;
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
        for (int i = 0; i < s.length(); ++i) {
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

        socket.async_read_some(
                boost::asio::buffer(data, max_length),
                boost::bind(&Session::handle_read, this,
                    shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
    }

    void handle_read(std::shared_ptr<Session>& s,
            const boost::system::error_code& err,
            size_t bytes_transferred)
    {
        if (!err) {
            std::cout << "recv: " << data << std::endl;
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

        if (!err) {
            socket.async_read_some(
                    boost::asio::buffer(data, max_length),
                    boost::bind(&Session::handle_read, this,
                        shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
        } 
        else {
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
            case 'g':
                response = read_registers(packet);
                break;
            case 'm':
                response = read_memory(packet);
                break;
            case 'D':
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

    std::string read_registers(GdbPacket & packet)
    {
        std::string res;
        for (int i = 0; i < 16; ++i) {
            res += get_register_hex(i); 
        }

        return res;
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
        } catch(invalid_argument) {
            return "E";
        }
    }


    std::string get_register_hex(int r) const
    {
        char buf[32];
        switch (r) {
            case 0:     sprintf(buf, "%02x", 0x0a); break; /* A */
            case 1:     sprintf(buf, "%02x", 0x0f); break; /* F */
            case 2:     sprintf(buf, "%04x", 0x0c0b); break; /* BC */
            case 3:     sprintf(buf, "%04x", 0x0e0d); break; /* DE */
            case 4:     sprintf(buf, "%04x", 0x4433); break; /* HL */
            case 5:     sprintf(buf, "0000"); break;        /* IX */
            case 6:     sprintf(buf, "0000"); break;        /* IY */
            case 7:     sprintf(buf, "%04x", 0x8967); break; /* SP */
            case 8:     sprintf(buf, "00"); break;
            case 9:     sprintf(buf, "00"); break;
            case 10:    sprintf(buf, "00"); break;
            case 11:    sprintf(buf, "00"); break;
            case 12:    sprintf(buf, "0000"); break;
            case 13:    sprintf(buf, "0000"); break;
            case 14:    sprintf(buf, "0000"); break;
            case 15:    sprintf(buf, "3412"); break;
        }
        return std::string(buf);
    }

private:
    tcp::socket socket;
    enum { max_length = 65536 };
    char data[max_length];
    bool closing;

    Board & board;
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


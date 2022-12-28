#include "debug.h"
#include "utils_string.h"
#include <string>

const std::string debug::evaluate(Board& board, const std::string packetStr)
{
    const GdbPacket& packet(packetStr);
    std::string reply;
    
    switch(packet.get_command()) {
        case '\0':
            reply = "";
            break;
        case 3: // ‘vCtrlC’
            board.debugger_attached();
            reply = REPLY_OK;
            break;
        case 'c': // continue
            board.debugger_continue();
            reply = REPLY_SIGNAL + SIGCONT;
            break; 
        case 'q':
            reply = general_response(packet);
            break;
        case 'Q':
            reply = general_response(packet);            
            break;
        case '?':   // indicate reason for stopping
            reply = board.is_break() ? REPLY_SIGNAL + SIGTRAP : REPLY_SIGNAL + SIGCONT;
            break;
        case 'g':   // read registers
            reply = board.read_registers();
            break;
        case 'G':   // write registers
            reply = write_registers(board, packet);
            break;
        case 'm':   // read memory location(s)
            reply = read_memory(board, packet);
            break;
        case 'M':   // write memory location(s)
            reply = write_memory(board, packet);
            break;
        case 'Z':   // insert breakpoint: type,addr,kind -> OK/E NN/''
            reply = insert_breakpoint(board, packet); 
            break;
        case 'z':   // remove breakpoint
            reply = remove_breakpoint(board, packet);
            break;
        case 's':   // singlestep
            board.single_step(true);
            reply = REPLY_SIGNAL + SIGTRAP;
            break;
        case 'D':   // debugger detach
            reply = REPLY_OK;
            board.debugger_detached();
            break;
    }

    const std::string response = GdbPacket::get_response(reply);
    
    printf("debug::evaluate response: [%s]\n", response.c_str());

    return response;
}

const std::string debug::general_response(const GdbPacket& packet)
{
    if (strstr(packet.get_params().c_str(), "Supported")) {
        return "PacketSize=4000";
    }
    else if (packet.get_params() == "C") {
        // return thread id
        return "";
    }
    else if (packet.get_params() == "Attached") {
        // server attached to an existing process
        return "1"; 
    }
    else if (packet.get_params() == "TStatus") {
        return "";
    }
    return REPLY_OK;
}

const std::string debug::write_registers(Board& board, const GdbPacket& packet)
{
    uint8_t regs[26];
    const char* cstr = packet.get_params().c_str();

    for (size_t i = 0; i < sizeof(regs); ++i) {
        try {
            regs[i] = std::stoul(string(&cstr[i*2], 2), nullptr, 16);
        } catch(invalid_argument&) {
            return REPLY_ERROR;
        }
    }
    board.write_registers(regs);
    return REPLY_OK;
}

const std::string debug::read_memory(Board& board, const GdbPacket& packet)
{
    auto parts = utils::split(packet.get_params(), ',');
    
    try 
    {
        int addr = std::stoul(parts[0], nullptr, 16);
        int len = std::stoul(parts[1], nullptr, 16);
        return board.read_memory(addr, len);
    } 
    catch(invalid_argument&) 
    {
        return REPLY_ERROR;
    }
}
const std::string debug::write_memory(Board& board, const GdbPacket& packet)
{
    auto parts = utils::split(packet.get_params(), ',');
    auto addrStr = parts[0];
    auto lenStr = utils::split(parts[1], ':')[0];
    auto dataStr = utils::split(parts[1], ':')[1];
    
    try {
        int addr = std::stoul(addrStr, nullptr, 16);
        int len = std::stoul(lenStr, nullptr, 16);

        if (len > 0) {
            const char* ostr = dataStr.c_str();
            for (int i = 0, end = strlen(ostr)/2; i < len && i < end; ++i) {
                uint8_t bytt = std::stoul(std::string(&ostr[i*2], 2), nullptr, 16);
                printf("W %04x,%02x", addr + i, bytt);
                board.write_memory_byte(addr + i, bytt);
            }
        }

        return REPLY_OK;
    } 
    catch(invalid_argument&) 
    {
        return REPLY_ERROR;
    }
}
const std::string debug::insert_breakpoint(Board& board, const GdbPacket& packet)
{
    auto parts = utils::split(packet.get_params(), ',');

    try {
        int type = std::stoul(parts[0], nullptr, 10);
        int addr = std::stoul(parts[1], nullptr, 16);
        int kind = std::stoul(parts[2], nullptr, 10);

        return board.insert_breakpoint(type, addr, kind);
    } 
    catch (invalid_argument&) 
    {
        return REPLY_ERROR;
    }
}
const std::string debug::remove_breakpoint(Board& board, const GdbPacket& packet)
{
    auto parts = utils::split(packet.get_params(), ',');

    try {
        int type = std::stoul(parts[0], nullptr, 10);
        int addr = std::stoul(parts[1], nullptr, 16);
        int kind = std::stoul(parts[2], nullptr, 10);

        return board.remove_breakpoint(type, addr, kind);
    } 
    catch (invalid_argument&) 
    {
        return REPLY_ERROR;
    }
}
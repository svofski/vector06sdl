#pragma once

#include "board.h"
#include <string>

namespace debug
{
    const std::string REPLY_OK("OK");
    const std::string REPLY_SIGNAL("S");
    const std::string REPLY_ERROR("E00");

    const std::string SIGTRAP("05"); // "Trace/breakpoint trap"
    const std::string SIGCONT("19"); // "Continued"

    class GdbPacket {
    public:
        GdbPacket(const std::string& packetStr)
        {
            size_t length = packetStr.length();

            printf("GdbPacket: packetStr= [%s]\n", packetStr.c_str());
            size_t i = 0;
            for(; (packetStr[i] == '$' || packetStr[i] == '+') && i < length; ++i);
            if (i >= length) {
                m_command = 0;
                return;
            }

            m_command = packetStr[i];

            int params = i + 1;
            int paramLen;
            for(paramLen = 0; packetStr[i] != '#' && i < length; ++i, ++paramLen);
            
            m_params = std::string(&packetStr[params], paramLen-1);
            m_crc = std::string(&packetStr[i+1], 2);

            printf("GdbPacket: command=%c params=%s crc=%s\n",
                    m_command, m_params.c_str(), m_crc.c_str());
        }

        static const std::string get_response(const std::string& reply)
        {
            return "+$" + reply + "#" + get_crc(reply);
        }

        const char get_command() const { return m_command; }
        const std::string get_params() const { return m_params; }
        const std::string get_crc() const { return m_crc; }
        
        static const std::string get_crc(const std::string& reply) 
        {
            int crc = 0;
            for (size_t i = 0; i < reply.length(); ++i) {
                crc += reply.at(i);
            }
            char buf[4];
            sprintf(buf, "%02x", crc & 0xff);
            return std::string(&buf[0], 2);
        }

    private:
        char m_command;
        std::string m_params;
        std::string m_crc;
    };

    const std::string evaluate(Board& board, const std::string packetStr);
    const std::string general_response(const GdbPacket& packet);
    const std::string write_registers(Board& board, const GdbPacket& packet);
    const std::string read_memory(Board& board, const GdbPacket& packet);
    const std::string write_memory(Board& board, const GdbPacket& packet);
    const std::string insert_breakpoint(Board& board, const GdbPacket& packet);
    const std::string remove_breakpoint(Board& board, const GdbPacket& packet);

}

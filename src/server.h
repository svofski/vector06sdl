#pragma once

#include "board.h"

void start_gdb_server(Board & board);
void poll_gdb_server();
void stop_gdb_server();
void notify_breakpoint();

class GdbServer {
public:
    GdbServer(Board & board) : board(board)
    {

    }
    
    ~GdbServer() 
    {
        stop_gdb_server();
    }

    void init() 
    {
        start_gdb_server(board);
    }

    void poll() 
    {
        poll_gdb_server();
    }

    void breakpoint()
    {
        notify_breakpoint();
    }

private:
    Board & board;
};

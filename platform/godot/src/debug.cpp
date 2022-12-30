#include "debug.h"

Debug::Debug(Board* _boardP, Memory* _memoryP) 
: mem_runs(), mem_reads(), mem_writes()
{
    auto read_func =
        [this](const size_t _addr) 
        {
            this->read(_addr);
        };

    auto write_func =
        [this](const size_t _addr) 
        {
            this->write(_addr);
        };
        
    auto run_func =
        [this](const size_t _addr) 
        {
            this->write(_addr);
        };

    _boardP->debug_on_single_step = run_func;
    _memoryP->debug_onread = read_func;
    _memoryP->debug_onwrite = write_func;
}

void Debug::execute(const size_t _addr)
{
    mem_runs[_addr % MEMORY_SIZE]++;
}

void Debug::read(const size_t _addr)
{   
    mem_reads[_addr % MEMORY_SIZE]++;
}

void Debug::write(const size_t _addr)
{
    mem_writes[_addr % MEMORY_SIZE]++;
}

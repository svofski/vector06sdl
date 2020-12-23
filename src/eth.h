#pragma once

#include <vector>
#include <queue>

#define UIP_CONF_BUFFER_SIZE     1024

struct TapDriver
{
    int fd;
    std::vector<uint8_t> rxbuf;
    std::queue<uint8_t> fifo;

    TapDriver() : fd(-1), rxbuf(UIP_CONF_BUFFER_SIZE) {}

    void init();
    unsigned poll();
    void send(uint8_t * buf, size_t size);
};

// PC:  CSR
//      when RX_DONE = 1
//          RX = [LEN MSB] [LEN LSB] [DATA] x LEN
//          after reading last byte RX_DONE becomes 0
//      whne TX_START = 1
//          TX <- [LEN MSB] [LEN LSB] [DATA] x LEN
// PA:  RX
// PB:  TX

class Ethernet
{
    TapDriver tap;

    uint8_t csr;

    std::vector<uint8_t> rxbuf;
    std::vector<uint8_t> txbuf;
    uint16_t txlen;
    int txstate;

public:
    Ethernet();

    void write_csr(uint8_t v);
    uint8_t read_csr() const;

    void write_data(uint8_t u8);
    uint8_t read_data();

    void poll();    // do the i/o
    void loop();
};

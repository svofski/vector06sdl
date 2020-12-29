#include <stdio.h>
#include <vector>
#include <cstdint>

// ------ tap ------
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/socket.h>

#ifdef linux
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#define DEVTAP "/dev/net/tun"
#else  /* linux */
#include <net/if.h>
#define DEVTAP "/dev/tap0"
#endif /* linux */

#include "memory.h"
#include "eth.h"

// csr bit values
#define TAP_INIT         1

// Maybe use these bits for resync? Curently if sync is lost, it's gone forever.

// a frame is received and waiting in queue
// data port will yield msb,lsb length, then length bytes of data
#define FRAME_READY 2

// host is ready to send data, after setting this bit in CSR
// write msb, lsb length, then length bytes of data
#define FRAME_SEND  8

static void dump(const char * op, uint8_t * buf, int ret)
{
    printf("HOST: %s 0x%x bytes\n", op, ret);
    for (int i = 0; i < ret + 16; i += 16) {
        for (int j = 0; j < 16; ++j) {
            if (i + j < ret) {
                printf("%02x%c", buf[i+j], j == 7 ? '-' : ' ');
            }
            else {
                printf("   ");
            }
        }
        printf("  ");
        for (int j = 0; j < 16; ++j) {
            if (i + j < ret) {
                int c = buf[i+j];
                printf("%c", (c >= 0x20 && c < 0x7f) ? c : '.');
            }
            else {
                printf(" ");
            }
        }
        printf("\n");
    }
}


void TapDriver::init()
{
    if (fd != -1) {
        close(fd);
        fd = -1;
    }

#ifdef linux
    fd = open(DEVTAP, O_RDWR);
    printf("DEVTAP is: %s, fd=%d\n", DEVTAP, fd);
    if(fd == -1) {
        perror("tapdev: tapdev_init: open");
        return;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, "tap0", sizeof(ifr.ifr_name));
    ifr.ifr_name[sizeof(ifr.ifr_name)-1] = 0; /* ensure \0 termination */

    ifr.ifr_flags = IFF_TAP|IFF_NO_PI;
    if (ioctl(fd, TUNSETIFF, (void *) &ifr) < 0) {
        perror("tapif_init: " DEVTAP " ioctl TUNSETIFF");
        close(fd);
        fd = -1;
    }
#endif

    fifo = {};
}

unsigned TapDriver::poll()
{
    if (fd == -1) return 0;

    fd_set fdset;
    struct timeval tv;
    int ret;

    // man 2 select:
    // If both fields of the timeval structure are zero, 
    // then select() returns immediately. (This is useful for polling.)
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    FD_ZERO(&fdset);
    FD_SET(fd, &fdset);

    ret = select(fd + 1, &fdset, NULL, NULL, &tv);
    if(ret == 0) {
        return fifo.size();
    }

    ret = ::read(fd, &rxbuf[0], rxbuf.size());

    if(ret == -1) {
        perror("tap_dev: tapdev_read: read");
        return fifo.size();
    }

    if (ret) {
        fifo.emplace(ret >> 8);
        fifo.emplace(ret & 255);
        for (int i = 0; i < ret; ++i) {
            fifo.emplace(rxbuf[i]);
        }
        printf("rx.queue depth: %8lu octets\r", fifo.size());
        fflush(stdout);
    }

    return fifo.size();
}

void TapDriver::send(uint8_t * srcbuf, size_t len)
{
    if (fd == -1) return;

    /*  drop++;
        if(drop % 8 == 7) {
        printf("Dropped a packet!\n");
        return;
        }*/
    int ret = ::write(fd, srcbuf, len);
    if(ret == -1) {
        perror("tap_dev: tapdev_send: writev");
        close(fd);
        fd = -1;
    }
}


Ethernet::Ethernet() : csr(0), txstate(0), rxstate(0)
{
}

void Ethernet::write_csr(uint8_t v)
{
    csr |= v;
    if (csr & TAP_INIT) {
        csr &= ~TAP_INIT;
        tap.init();
        txstate = 0;
    }
}

uint8_t Ethernet::read_csr() const
{
    return csr;
}


// rather often
void Ethernet::loop()
{
}

// once per frame
void Ethernet::poll()
{
    tap.poll();
}

uint8_t Ethernet::read_data()
{
    if (rxstate) {
        --rxstate;
    }
    if (rxstate) {
        return 0;
    }

    if (tap.fifo.empty()) {
        rxstate = 2;
        return 0;
    }

    uint8_t res = tap.fifo.front();
    tap.fifo.pop();
    return res;
}

void Ethernet::write_data(uint8_t u8)
{
    if (txstate == 0) {
        txlen = u8 << 8;
        txstate = 1;
    }
    else if (txstate == 1) {
        txlen |= u8;
        txstate = 2;
        txbuf.clear();
        if (txlen == 0) {
            printf("E: %s txlen=0\n", __PRETTY_FUNCTION__);
            txstate = 0;
        }
    }
    else {
        txbuf.emplace_back(u8);
        if (txbuf.size() == txlen) {
            tap.send(&txbuf[0], txlen);
            txstate = 0;
        }
    }
}

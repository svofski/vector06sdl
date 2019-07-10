#pragma once

#include <stdio.h>
#include <string.h>
#include <functional>
#include "keyboard.h"
#include "8253.h"
#include "ay.h"
#include "fd1793.h"
#include "wav.h"

class IO {
private:
    uint32_t palette[16];

    Memory & kvaz;
    Keyboard & keyboard;
    I8253 & timer;
    FD1793 & fdc;
    AY & ay;
    WavPlayer & tape_player;

    uint8_t CW, PA, PB, PC, PIA1_last;
    uint8_t CW2, PA2, PB2, PC2;

    int outport;
    int outbyte;
    int palettebyte;
public:
    std::function<void(int)> onborderchange;
    std::function<void(bool)> onmodechange;
    std::function<void(bool)> onruslat;
    std::function<uint32_t(uint8_t,uint8_t,uint8_t)> rgb2pixelformat;

    std::function<int(uint32_t,uint8_t)> onread;
    std::function<void(uint32_t,uint8_t)> onwrite;

public:
    IO(Memory & _memory, Keyboard & _keyboard, I8253 & _timer, FD1793 & _fdc, 
            AY & _ay, WavPlayer & _tape_player) 
        : kvaz(_memory), keyboard(_keyboard), timer(_timer), fdc(_fdc), ay(_ay),
        tape_player(_tape_player),
        CW(0x08), PA(0xff), PB(0xff), PC(0xff), CW2(0), PA2(0xff), PB2(0xff), PC2(0xff)
    {
        for (unsigned i = 0; i < sizeof(palette)/sizeof(palette[0]); ++i) {
            palette[i] = 0xff000000;
        }
        outport = outbyte = palettebyte = -1;
    }

    void yellowblue()
    {
        // Create boot-time yellow/blue yeblette using correct pixelformat
        for (int i = 0; i < 16; ++i) {
            if (i & 2) {
                this->palette[i] = rgb2pixelformat(5, 5, 0); 
            } 
            else {
                this->palette[i] = rgb2pixelformat(0, 0, 2); 
            }
        }
    }

    int input(int port)
    {
        int result = 0xff;
        
        switch(port) {
            case 0x00:
                result = 0xff;
                break;
            case 0x01:
                {
                /* PC.low input ? */
                auto pclow = (this->CW & 0x01) ? 0x0b : (this->PC & 0x0f);
                /* PC.high input ? */
                auto pcupp = (this->CW & 0x08) ? 
                    ((this->tape_player.sample() << 4) |
                     (this->keyboard.ss ? 0 : (1 << 5)) |
                     (this->keyboard.us ? 0 : (1 << 6)) |
                     (this->keyboard.rus ? 0 : (1 << 7))) : (this->PC & 0xf0);
                result = pclow | pcupp;
                }
                break;
            case 0x02:
                if ((this->CW & 0x02) != 0) {
                    result = this->keyboard.read(~this->PA); // input
                } else {
                    result = this->PB;       // output
                }
                break;
            case 0x03:
                if ((this->CW & 0x10) == 0) { 
                    result = this->PA;       // output
                } else {
                    result = 0xff;          // input
                }
                break;

            case 0x04:
                result = this->CW2;
                break;
            case 0x05:
                result = this->PC2;
                break;
            case 0x06:
                result = this->PB2;
                break;
            case 0x07:
                result = this->PA2;
                break;

                // Timer
            case 0x08:
            case 0x09:
            case 0x0a:
            case 0x0b:
                return this->timer.read(~(port & 3));

            case 0x14:
            case 0x15:
                result = this->ay.read(port & 1);
                break;

            case 0x18: // fdc data
                result = this->fdc.read(3);
                break;
            case 0x19: // fdc sector
                result = this->fdc.read(2);
                break;
            case 0x1a: // fdc track
                result = this->fdc.read(1);
                break;
            case 0x1b: // fdc status
                result = this->fdc.read(0);
                break;
            case 0x1c: // fdc control - readonly
                //result = this->fdc.read(4);
                break;
            default:
                break;
        }

        if (this->onread) {
            int hookresult = this->onread((uint32_t)port, (uint8_t)result);
            if (hookresult != -1) {
                result = hookresult;
            }
        }

        return result;
    }

    void output(int port, int w8) {
        if (this->onwrite) {
            this->onwrite((uint32_t)port, (uint8_t)w8);
        }
        this->outport = port;
        this->outbyte = w8;

        //if (port == 0x02) {
        //    this->onmodechange((w8 & 0x10) != 0);
        //}
        #if 0
        /* debug print from guest */
        switch (port) {
            case 0x77:  
                this->str1 += w8.toString(16) + " ";
                break;
            case 0x79:
                if (w8 != 0) {
                    this->str1 += String.fromCharCode(w8);
                } else {
                    console.log(this->str1);
                    this->str1 = "";
                }
        }
        #endif
    }

    void realoutput(int port, int w8) {
        bool ruslat;
        switch (port) {
            // PIA 
            case 0x00:
                this->PIA1_last = w8;
                ruslat = this->PC & 8;
                if ((w8 & 0x80) == 0) {
                    // port C BSR: 
                    //   bit 0: 1 = set, 0 = reset
                    //   bit 1-3: bit number
                    int bit = (w8 >> 1) & 7;
                    if ((w8 & 1) == 1) {
                        this->PC |= 1 << bit;
                    } else {
                        this->PC &= ~(1 << bit);
                    }
                    //this->ontapeoutchange(this->PC & 1);
                } else {
                    this->CW = w8;
                    this->realoutput(1, 0);
                    this->realoutput(2, 0);
                    this->realoutput(3, 0);
                }
                if (((this->PC & 8) != ruslat) && this->onruslat) {
                    this->onruslat((this->PC & 8) == 0);
                }
                // if (debug) {
                //     console.log("output commit cw = ", this->CW.toString(16));
                // }
                break;
            case 0x01:
                this->PIA1_last = w8;
                ruslat = this->PC & 8;
                this->PC = w8;
                //this->ontapeoutchange(this->PC & 1);
                if (((this->PC & 8) != ruslat) && this->onruslat) {
                    this->onruslat((this->PC & 8) == 0);
                }
                break;
            case 0x02:
                this->PIA1_last = w8;
                this->PB = w8;
                this->onborderchange(this->PB & 0x0f);
                this->onmodechange((this->PB & 0x10) != 0);
                break;
            case 0x03:
                this->PIA1_last = w8;
                this->PA = w8;
                break;
                // PPI2
            case 0x04:
                this->CW2 = w8;
                break;
            case 0x05:
                this->PC2 = w8;
                break;
            case 0x06:
                this->PB2 = w8;
                break;
            case 0x07:
                this->PA2 = w8;
                break;

                // Timer
            case 0x08:
            case 0x09:
            case 0x0a:
            case 0x0b:
                this->timer.write((~port & 3), w8);
                break;

            case 0x0c:
            case 0x0d:
            case 0x0e:
            case 0x0f:
                this->palettebyte = w8;
                break;
            case 0x10:
                // kvas 
                this->kvaz.control_write(w8);
                break;
            case 0x14:
            case 0x15:
                this->ay.write(port & 1, w8);
                break;

            case 0x18: // fdc data
                this->fdc.write(3, w8);
                break;
            case 0x19: // fdc sector
                this->fdc.write(2, w8);
                break;
            case 0x1a: // fdc track
                this->fdc.write(1, w8);
                break;
            case 0x1b: // fdc command
                this->fdc.write(0, w8);
                break;
            case 0x1c: // fdc control
                this->fdc.write(4, w8);
                break;
            default:
                break;
        }
    }

    void commit() 
    {
        if (this->outport != -1) {
            //printf("commit: %02x = %02x\n", this->outport, this->outbyte);
            this->realoutput(this->outport, this->outbyte);
            this->outport = this->outbyte = -1;
        }
    }

    void commit_palette(int index) 
    {
        int w8 = this->palettebyte;
        if (w8 == -1 && this->outport == 0x0c) {
            w8 = this->outbyte;
            this->outport = this->outbyte = -1;
        }
        if (w8 != -1) {
            int b = (w8 & 0xc0) >> 6;
            int g = (w8 & 0x38) >> 3;
            int r = (w8 & 0x07);

            this->palette[index] = rgb2pixelformat(r,g,b);
            //printf("commit palette: %02x = %02x\n", index, this->palette[index]);
            this->palettebyte = -1;
        }
    }

    int BorderIndex() const 
    {
        return this->PB & 0x0f;
    }

    int ScrollStart() const 
    {
        return this->PA;
    }

    bool Mode512() const 
    {
        return (this->PB & 0x10) != 0;
    }

    int TapeOut() const 
    {
        return this->PC & 1;
    }

    int Covox() const 
    {
        return this->PA2;
    }

    uint32_t Palette(int index) const
    {
        return this->palette[index];
    }

    Keyboard & the_keyboard() const
    {
        return this->keyboard;
    }
};

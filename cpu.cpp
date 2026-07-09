#include <cstdint>

class Bus {
    public:
        uint8_t read8(uint16_t addr);
        void write8(uint16_t addr, uint8_t value);

    private:
        uint8_t memory[65536];
};

// B = 000
// C = 001
// D = 010
// E = 011
// H = 100
// L = 101
// HL = 110
// A = 111
//
// F is flags register

// Little Endian
class CPU {
    private:
        union { struct{ uint8_t F, A; }; uint16_t AF; };
        union { struct{ uint8_t C, B; }; uint16_t BC; };
        union { struct{ uint8_t E, D; }; uint16_t DE; };
        union { struct{ uint8_t L, H; }; uint16_t HL;
        };
        uint16_t SP, PC;

    public:
        void rom_exec();
        void step(Bus& bus);
        uint8_t read8(uint8_t code, Bus& bus);
        void write8(uint8_t code, uint8_t value, Bus& bus);
        CPU();

};

// Initializes Program Counter (PC) to point at 0x0100, skipping boot ROM
void CPU::rom_exec() {
    PC = 0x0100;
}

uint8_t CPU::read8(uint8_t code, Bus& bus) {
    switch (code) {
        case 0b000: return B;
        case 0b001: return C;
        case 0b010: return D;
        case 0b011: return E;
        case 0b100: return H;
        case 0b101: return L;
        case 0b110: return bus.read8(HL);
        case 0b111: return A;
    }
    return 0xFF; // unreachable
}

void CPU::write8(uint8_t code, uint8_t value, Bus& bus) {
    switch (code) {
        case 0b000: B = value; break;
        case 0b001: C = value; break;
        case 0b010: D = value; break;
        case 0b011: E = value; break;
        case 0b100: H = value; break;
        case 0b101: L = value; break;
        case 0b110: bus.write8(HL, value); break;
        case 0b111: A = value; break;
    }
}

void CPU::step(Bus& bus) {
    uint8_t op = bus.read8(PC++); 
    uint8_t dst = (op >> 3) & 0b111;
    uint8_t src = op & 0b111;
    switch (op) {
        case 0x76: // HALT (not LD (HL),(HL))
            // TODO: halt handling
            break;
        default:
            if (op >= 0x40 && op <= 0x7F) {
                write8(dst, read8(src, bus), bus);
            } 
            else if (src == 0b110 && (op >> 6) == 0b00) {
                uint8_t n = bus.read8(PC++);
                write8(dst, n, bus);
            }
            else if(op == 0x0A){          // LD A,(BC)
                A = bus.read8(BC);
            }
            else if(op == 0x1A){          // LD A,(DE)
                A = bus.read8(DE);
            }
            else if(op == 0x02){          // LD (BC),A
                bus.write8(BC, A);
            }
            else if(op == 0x12){          // LD (DE),A
                bus.write8(DE, A);
            }
            else if(op == 0xFA){          // LD A,(nn)
                uint8_t nn_lsb = bus.read8(PC++);
                uint8_t nn_msb = bus.read8(PC++);

                uint16_t nn = ((static_cast<uint16_t>(nn_msb) << 8) | nn_lsb);
                A = bus.read8(nn);
            }
            else if(op == 0xEA){          // LD (nn),A
                uint8_t nn_lsb = bus.read8(PC++);
                uint8_t nn_msb = bus.read8(PC++);

                uint16_t nn = ((static_cast<uint16_t>(nn_msb) << 8) | nn_lsb);
                bus.write8(nn, A);
            }
            else if(op == 0xF2){          // LD A,(C)
                A = bus.read8(0xFF00 | C);
            }
            else if(op == 0xE2){          // LD (C),A
                bus.write8(0xFF00 | C, A);
            }
            else if(op == 0xF0){          // LDH A,(n)
                uint8_t n = bus.read8(PC++);
                A = bus.read8(0xFF00 | n);
            }
            else if(op == 0xE0){          // LDH (n),A
                uint8_t n = bus.read8(PC++);
                bus.write8(0xFF00 | n, A);
            }
            else if(op == 0x3A){          // LD A,(HL-)
                A = bus.read8(HL);
                HL--;
            }
            else if(op == 0x32){          // LD (HL-),A
                bus.write8(HL, A);
                HL--;
            }
            else if(op == 0x2A){          // LD A,(HL+)
                A = bus.read8(HL);
                HL++;
            }
            else if(op == 0x22){          // LD (HL+),A
                bus.write8(HL, A);
                HL++;
            }
            break;
    }           
}

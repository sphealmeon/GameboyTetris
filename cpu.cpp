#include <cstdint>

class Bus {
    public:
        uint8_t read8(uint16_t addr);
        uint16_t read16(uint16_t addr);
        void write8(uint16_t addr, uint8_t value);
        void write16(uint16_t addr, uint16_t value);

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
        uint16_t read16(uint8_t code, Bus& bus);
        uint16_t read16_af(uint8_t code, Bus& bus);
        void write8(uint8_t code, uint8_t value, Bus& bus);
        void write16(uint8_t code, uint16_t value, Bus& bus);
        void write16_af(uint8_t code, uint16_t value, Bus& bus);
        void push(uint16_t value, Bus& bus);
        uint16_t pop(Bus& bus);
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

uint16_t CPU::read16(uint8_t code, Bus& bus){
    switch (code){
        case 0b00: return BC;
        case 0b01: return DE;
        case 0b10: return HL;
        case 0b11: return SP;
    }
    return 0xFF; // unreachable
}

// PUSH/POP use rr==0b11 for AF instead of SP
uint16_t CPU::read16_af(uint8_t code, Bus& bus){
    switch (code){
        case 0b00: return BC;
        case 0b01: return DE;
        case 0b10: return HL;
        case 0b11: return AF;
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

void CPU::write16(uint8_t code, uint16_t value, Bus& bus){
    switch (code) {
        case 0b00: BC = value; break;
        case 0b01: DE = value; break;
        case 0b10: HL = value; break;
        case 0b11: SP = value; break;
    }
}

// PUSH/POP use rr==0b11 for AF instead of SP; low nibble of F is hardwired to 0
void CPU::write16_af(uint8_t code, uint16_t value, Bus& bus){
    switch (code) {
        case 0b00: BC = value; break;
        case 0b01: DE = value; break;
        case 0b10: HL = value; break;
        case 0b11: AF = value & 0xFFF0; break;
    }
}

void CPU::step(Bus& bus) {
    uint8_t op = bus.read8(PC++); 
    uint8_t dst = (op >> 3) & 0b111;
    uint8_t src = op & 0b111;
    uint8_t st_dst = (op >> 4) & 0b11; // st = sixteen

    switch (op) {
        case 0x76: // HALT (not LD (HL),(HL))
            // TODO: halt handling
            break;
        default:
            // 8-bit load instructions
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
            // 16-bit load instructions
            else if((op & 0b1111) == 0b0001 && (op >> 6) == 0b00){ // LD rr, nn
                uint8_t nn_lsb = bus.read8(PC++);
                uint8_t nn_msb = bus.read8(PC++);
                uint16_t nn = ((static_cast<uint16_t> (nn_msb) << 8) | nn_lsb);
                write16(st_dst, nn, bus);
            }
            else if(op == 0x08){ // LD (nn), SP
                uint8_t nn_lsb = bus.read8(PC++);
                uint8_t nn_msb = bus.read8(PC++);
                uint16_t nn = ((static_cast<uint16_t> (nn_msb) << 8) | nn_lsb);
                bus.write16(nn, SP);
            }
            else if(op == 0xF9){ // LD SP, HL
                write16(0b11, read16(0b10, bus), bus);
            }
            else if((op & 0b1111) == 0b0101 && (op >> 6) == 0b11){ // PUSH rr
                uint16_t value = read16_af(st_dst, bus);
                bus.write8(--SP, static_cast<uint8_t>(value >> 8));
                bus.write8(--SP, static_cast<uint8_t>(value & 0xFF));
            }
            else if((op & 0b1111) == 0b0001 && (op >> 6) == 0b11){ // POP rr
                uint8_t lsb = bus.read8(SP++);
                uint8_t msb = bus.read8(SP++);
                write16_af(st_dst, (static_cast<uint16_t>(msb) << 8) | lsb, bus);
            }
            else if(op == 0xF8){ // LD HL, SP+e
                int8_t e = bus.read8(PC++);
                uint16_t result = SP + e;
                
                bool half_carry = ((SP & 0xF) + (e & 0xF) > 0xF);
                bool full_carry = ((SP & 0xFF) + (e & 0xFF) > 0xFF);

                HL = result;
                F = 0;
                if(half_carry) F |= 0x20;
                if(full_carry) F |= 0x10;
            }
            // 8-bit arithmetic instructions
            else if((op >> 3) == 0b10000){ // ADD r and ADD (HL)
                uint8_t operand = read8(src, bus);
                uint16_t result = static_cast<uint16_t>(A) + static_cast<uint16_t> (operand);
                bool half_carry = (((A & 0xF) + (read8(src, bus) & 0xF)) > 0xF);
                bool full_carry = (result > 0xFF);

                A = static_cast<uint8_t>(result);
                F = 0;
                if(A == 0) F |= 0x80;
                if(half_carry) F |= 0x20;
                if(full_carry) F |= 0x10;
            }
            else if(op == 0xC6){ // ADD n
                uint8_t n = bus.read8(PC++);
                uint16_t result = static_cast<uint16_t>(A) + static_cast<uint16_t> (n);
                bool half_carry = (((A & 0xF) + (n & 0xF)) > 0xF);
                bool full_carry = (result > 0xFF);

                A = static_cast<uint8_t>(result);
                F = 0;
                if(A == 0) F |= 0x80;
                if(half_carry) F |= 0x20;
                if(full_carry) F |= 0x10;
            }
            else if((op >> 3) == 0b10001){ // ADC r
                uint8_t operand = read8(src, bus);
                uint8_t carry_in = (F >> 4) & 0b1;
                uint16_t result = static_cast<uint16_t> (A) + static_cast<uint16_t> (operand) + (carry_in);
                bool half_carry = (((A & 0xF) + (operand & 0xF) + (carry_in)) > 0xF);
                bool full_carry = (result > 0xFF);

                A = static_cast<uint8_t>(result);
                F = 0;
                if(A == 0) F |= 0x80;
                if(half_carry) F |= 0x20;
                if(full_carry) F |= 0x10;
            }
            else if(op == 0xCE){ // ADC n
                uint8_t n = bus.read8(PC++);
                uint8_t carry_in = (F >> 4) & 0b1;
                uint16_t result = static_cast<uint16_t> (A) + static_cast<uint16_t> (n) + (carry_in);
                bool half_carry = (((A & 0xF) + (n & 0xF) + (carry_in)) > 0xF);
                bool full_carry = (result > 0xFF);

                A = static_cast<uint8_t>(result);
                F = 0;
                if(A == 0) F |= 0x80;
                if(half_carry) F |= 0x20;
                if(full_carry) F |= 0x10;
            }
            else if((op >> 3) == 0b10010){ //SUB r and SUB (HL)
                uint8_t operand = read8(src, bus);
                uint8_t result = A - operand;
                bool half_carry = (A & 0xF) < (operand & 0xF);
                bool full_carry = (A < operand);

                A = result;
                F = 0x40;
                if(A == 0) F |= 0x80;
                if(half_carry) F |= 0x20;
                if(full_carry) F |= 0x10;
            }
            else if(op == 0xD6){ // SUB n
                uint8_t n = bus.read8(PC++);
                uint8_t result = A - n;
                bool half_carry = (A & 0xF) < (n & 0xF);
                bool full_carry = (A < n);

                A = result;
                F = 0x40;
                if(A == 0) F |= 0x80;
                if(half_carry) F |= 0x20;
                if(full_carry) F |= 0x10;
            }
            else if((op >> 3) == 0b10011){ // SBC r
                uint8_t operand = read8(src, bus);
                uint8_t carry_in = (F >> 4) & 0b1;
                uint8_t result = A - operand - carry_in;

                bool half_carry = (A & 0xF) < ((operand & 0xF) + carry_in);
                bool full_carry = A < (static_cast<uint16_t>(operand) + carry_in);

                A = result;
                F = 0x40;
                if(A == 0) F |= 0x80;
                if(half_carry) F |= 0x20;
                if(full_carry) F |= 0x10;
            }
            else if(op == 0xDE){ // SBC n
                uint8_t n = bus.read8(PC++);
                uint8_t carry_in = (F >> 4) & 0b1;
                uint8_t result = A - n - carry_in;

                bool half_carry = (A & 0xF) < ((n & 0xF) + carry_in);
                bool full_carry = A < (static_cast<uint16_t>(n) + carry_in);

                A = result;
                F = 0x40;
                if(A == 0) F |= 0x80;
                if(half_carry) F |= 0x20;
                if(full_carry) F |= 0x10;
            }
            else if((op >> 3) == 0b10111){ // CP r and CP (HL)
                uint8_t operand = read8(src, bus);
                uint8_t result = A - operand;
                bool half_carry = (A & 0xF) < (operand & 0xF);
                bool full_carry = (A < operand);

                F = 0x40;
                if(result == 0) F |= 0x80;
                if(half_carry) F |= 0x20;
                if(full_carry) F |= 0x10;
            }
            else if(op == 0xFE) { // CP n
                uint8_t n = bus.read8(PC++);
                uint8_t result = A - n;
                bool half_carry = (A & 0xF) < (n & 0xF);
                bool full_carry = (A < n);

                F = 0x40;
                if(result == 0) F |= 0x80;
                if(half_carry) F |= 0x20;
                if(full_carry) F |= 0x10;
            }  
            else if((op & 0b111) == 0b100 && (op >> 6) == 0){ // INC r and INC (HL)
                uint8_t operand = read8(dst, bus);
                uint8_t result = operand + 1;

                bool half_carry = (operand == 0x0F);
                write8(dst, result, bus);
                F = 0;
                if(result == 0) F |= 0x80;
                if(half_carry) F |= 0x20;
            } 
            else if((op & 0b111) == 0b101 && (op >> 6) == 0){ // DEC r and DEC (HL)
                uint8_t operand = read8(dst, bus);
                uint8_t result = operand - 1;

                bool half_carry = (operand == 0x10);
                write8(dst, result, bus);
                F = 0x40;
                if(result == 0) F |= 0x80;
                if(half_carry) F |= 0x20;
            }
            else if((op >> 3) == 0b10100){ // AND r and AND (HL)
                uint8_t operand = read8(src, bus);
                uint8_t result = A & operand;
                A = result;
                F = 0x20;
                if(result == 0) F |= 0x80;
            }
            else if(op == 0xE6){ // AND n
                uint8_t n = bus.read8(PC++);
                uint8_t result = A & n;
                A = result;
                F = 0x20;
                if(result == 0) F |= 0x80;
            }
            else if((op >> 3) == 0b10110){ // OR r and OR (HL)
                uint8_t operand = read8(src, bus);
                uint8_t result = A | operand;
                A = result;
                F = 0;
                if(result == 0) F |= 0x80;
            }
            else if(op == 0xF6){ // OR n
                uint8_t n = bus.read8(PC++);
                uint8_t result = A | n;
                A = result;
                F = 0;
                if(result == 0) F |= 0x80;
            }
            else if((op >> 3) == 0b10101){ // XOR r and XOR (HL)
                uint8_t operand = read8(src, bus);
                uint8_t result = A ^ operand;
                A = result;
                F = 0;
                if(result == 0) F |= 0x80;
            }
            else if(op == 0xEE){ // XOR n
                uint8_t n = bus.read8(PC++);
                uint8_t result = A ^ n;
                A = result;
                F = 0;
                if(result == 0) F |= 0x80;
            }
            else if(op == 0x3F){ // CCF
                F &= 0b10011111;
                F ^= (1 << 5);
            }
            else if(op == 0x37){ // SCF
                F &= 0b10011111;
                F |= (1 << 5);
            }
            else if(op == 0x27){ // DAA (wtf is the point of this)
                uint8_t correction = 0;
                bool FLAG_Z = F & (1 << 7);
                bool FLAG_N = F & (1 << 6);
                bool FLAG_H = F & (1 << 5);
                bool FLAG_C = F & (1 << 4);

                if(!FLAG_N){
                    if(FLAG_H || (A & 0x0F) > 9){
                        correction |= 0x06;
                    }
                    if(FLAG_C || A > 0x99){
                        correction |= 0x60;
                        F |= (1 << 4);
                    }
                    A += correction;
                }
                else{
                    if(FLAG_H){
                        correction |= 0x06;
                    }
                    if(FLAG_C){
                        correction |= 0x60;
                    }
                    A -= correction;
                }
                F &= ~(1 << 5);
                if(A == 0) F |= (1 << 7);
                else F &= ~(1 << 7);
                F &= 0xF0;
            }
            else if(op == 0x2F){ // CPL
                A = ~A;
                F |= (1 << 6);
                F |= (1 << 7);
            }
            break;
    }           
}

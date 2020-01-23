#include <stdint.h>
#include <string.h>
#include <stdexcept>
#include <fstream>
#include <stdlib.h>
#include <random>

#include "chip8.h"

#define VX V[((instrukcija & 0x0F00) >> 8)]
#define VY V[((instrukcija & 0x00F0) >> 4)]

uint8_t font[80] = 
{
    0xF0, 0x90, 0x90, 0x90, 0xF0, //0
    0x20, 0x60, 0x20, 0x20, 0x70, //1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
    0x90, 0x90, 0xF0, 0x10, 0x10, //4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
    0xF0, 0x10, 0x20, 0x40, 0x40, //7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
    0xF0, 0x90, 0xF0, 0x90, 0x90, //A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
    0xF0, 0x80, 0x80, 0x80, 0xF0, //C
    0xE0, 0x90, 0x90, 0x90, 0xE0, //D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
    0xF0, 0x80, 0xF0, 0x80, 0x80  //F
};

Chip8::Chip8() {
    gen = new std::mt19937(rd());
    dis = new std::uniform_int_distribution<> (0, 255);
};
Chip8::~Chip8() {
    delete gen;
    delete dis;
};

void Chip8::pocisti(){
    PC = 0x200;
    I = instrukcija = SP = DT= ST = 0;
    memset(graf, 0, 2048);
    memset(memorija, 0,  4096);
    for(int i = 0; i < 16; i++)
        stek[i] = tast[i] = V[i] = 0;
    memcpy(memorija, font, 80);
}

void Chip8::ucitaj_program(std::string fajl_putanja){
    pocisti(); 

    std::ifstream ifs(fajl_putanja, std::ios::in | std::ios::ate | std::ios::binary);
    if(!ifs)
        throw std::runtime_error("Fajl putanja nije validna");

    int velicina_fajla = ifs.tellg();
    ifs.clear();

    char *program_buffer = new char[velicina_fajla];
    ifs.read(program_buffer, velicina_fajla);
    if(velicina_fajla <= CHIP8_MAX_FAJL_VELICINA)
        memcpy(memorija+512, program_buffer, velicina_fajla);
    else
        throw std::runtime_error("Velicina odabranog programa prelazi memorijski limit");
   
    ifs.close(); 
    delete[] program_buffer;  
}

uint8_t Chip8::nasumican_bajt(){
    return (*dis)(*gen); 
};

void Chip8::emuliraj_ciklus(){
    instrukcija = memorija[PC] << 8 | memorija[PC+1]; // Poveze 2 susjedna bajta u memoriji u jednu 16-bitnu instrukciju
    uint8_t opkod = instrukcija & 0xF000;
    switch (opkod){
    
        case OP_00E_:
            switch (instrukcija & SOPMASK_00E_){
                
                case SOP_00E0: // CLS - Ocisti ekran
                    memset(graf, 0, CHIP8_VELICINA_GRAFICKE_MEMORIJE);
                    crtaj = true;
                    PC += 2;
                    break;
            
                case SOP_00EE: // RET - Povrat iz subrutine
                    --SP; // Prethodni stack pointer
                    PC = stek[SP];
                    PC+=2;
                    break;

                default:
                    throw std::runtime_error("Opkod nije validan");
                    break;
            }
            break;

        case OP_1NNN: // JMP addr - Skoci na adresu
            PC = instrukcija & 0x0FFF;
            break;
       
        case OP_2NNN: // CALL addr - Zapocni subrutinu na adresi. 
            stek[SP] = PC;
            ++SP;
            PC = instrukcija & 0xFFF;
            break;

        case OP_3XNN:// SE Vx, byte - Preskoci sljedecu instrukciju ako VX = NN
            uint8_t NN = (instrukcija & 0x00FF);
            if(VX == NN)
                PC += 2;
            else
                PC += 4;
            break;

        case OP_4XNN: // SNE Vx, byte - Preskoci sljedecu instrukciju ako VX != NN
            uint8_t NN = (instrukcija & 0x00FF);
            if(VX != NN)
                PC += 2;
            else
                PC += 4;
            break;

        case OP_5XY0: // SE Vx, Vy - Preskoci sljedec instrukciju ako VX = VY
            if(VX == VY)
                PC += 2;
            else
                PC += 4;
            break;

        case OP_6XNN: // LD Vx, byte - Postavi VX = NN
            VX = instrukcija & 0x00FF;
            PC += 2;
            break;

        case OP_7XNN: // ADD Vx, byte - Dodaj NN na VX
            VX += instrukcija & 0x00FF;
            PC += 2;
            break;

        case OP_8XY_: // Operacije nad 2 registra
            switch (instrukcija & SOPMASK_8XY_){
                case SOP_8XY0: // LD Vx, Vy - Postavi Vx = Vy
                    VX = VY;
                    PC += 2;
                    break;

                case SOP_8XY1: // OR Vx, Vy 
                    VX |= VY;
                    break;

                case SOP_8XY2: // AND Vx, Vy
                    VX &= VY;
                    break;
            
                case SOP_8XY3: // XOR Vx, Vy
                    VX ^= VY;
                    break;
            
                case SOP_8XY4: // ADD Vx, Vy
                    VX += VY;
                    V[0xF] = VY > (255 - VX);
                    PC += 2;
                    break;
            
                case SOP_8XY5: // SUB Vx, Vy
                    V[0xF] = VY <= VX;
                    VX -= VY;
                    PC += 2;
                    break;
            
                case SOP_8XY6: // SHR Vx, Vy - Pomjeri sve bite Vx udesno za 1
                    V[0xF] = VX & 1;
                    VX >>= 1;
                    PC += 2;
                    break;
            
                case SOP_8XY7: // SUBN Vx, Vy
                    V[0xF] = VX <= VY;
                    VX -= VY;
                    PC += 2;
                    break;
            
                case SOP_8XYE: // SHL
                    V[0xF] = VX >> 7;
                    VX <<= 1;
                    PC += 2;
                    break;
            
                default:
                    throw std::runtime_error("Opkod nije validan");
                    break;
            }
            break;

        case OP_9XY0: // SNE Vx, Vy - Preskoci sljedecu instrukciju ako Vx != Vy
            if(VX != VY)
                PC += 4;
            else
                PC += 2;
            break;

        case OP_ANNN: // LD I, addr - Postavi vrjednost registra I na adresu
            I = instrukcija & 0x0FFF;
            PC += 2;
            break;

        case OP_BNNN: // JP V0, addr - Skoci na adresu + vrijednost u V0
            PC = (instrukcija & 0x0FFF) + V[0];
            PC += 2;
            break;

        case OP_CXNN: // RND Vx, byte - Postavi Vx = [nasumican broj] AND NN
            VX = nasumican_bajt() & (instrukcija & 0x00FF);
            PC += 2;
            break;

        // Crta sprajt na koordinatama Vx, Vy sireine 8 piksela i visine N piksela
        // Sprajt se cita pocevsi od memorijske lokacije sacuvane u registru I
        // Ako se makar jedan piksel promjeni sa 1 u 0, VF registar postaviti na 1
        case OP_DXNN: 
            uint8_t visina = instrukcija & 0x000F; // Visina sprajta
            V[0xF] = 0;
            uint8_t red;
            for(uint8_t i_reda = 0; i_reda < visina; i++){
                red = memorija[I + i_reda];
                for(uint8_t i_bajta = 0; i_bajta < 8; i++)
                    if(red & (128 >> i_bajta)){
                        if(graf[(VY + i_reda)*64 + VX + i_bajta])
                            V[0xF] = 1;
                        graf[(VY + i_reda)*64 + VX + i_bajta] ^= 1;
                    }
                crtaj = true;
                PC += 2;
            }
            break;

        case OP_EX__:
            switch(opkod & SOPMASK_EX__){
                case SOP_EX9E: // Preskoci sljedecu instrukciju ako je tipka sa vrijednosti Vx trenutno pritisnuta
                    if(tast[VX])
                        PC += 4;
                    else
                        PC += 2;
                    break;
                
                case SOP_EX1A: // Preskoci sljedecu instrukciju ako tipka sa vrijednosti Vx nije trenutno pritisnuta
                    if(!tast[VX])
                        PC += 4;
                    else
                        PC += 2;
                    break;
                default:
                   throw std::runtime_error("Opkod nije validan");
                    break;
            }
            break;

        case OP_FX__:
            break;
    
        default:
            break;
    }
}
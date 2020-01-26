#include <stdint.h>
#include <string.h>
#include <stdexcept>
#include <fstream>
#include <stdlib.h>
#include <random>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <SFML/Graphics.hpp>

#include "chip8.h"

#define VX V[((instrukcija & 0x0F00) >> 8)]
#define VY V[((instrukcija & 0x00F0) >> 4)]

std::random_device rd;
std::mt19937 gen = std::mt19937(rd());
std::uniform_int_distribution<> dis = std::uniform_int_distribution<> (0, 255);

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
    tastmap[0] = sf::Keyboard::Num7;       // 1
    tastmap[1] = sf::Keyboard::Num8;       // 2
    tastmap[2] = sf::Keyboard::Num9;       // 3
    tastmap[3] = sf::Keyboard::Num0;       // C
    tastmap[4] = sf::Keyboard::I;          // 4
    tastmap[5] = sf::Keyboard::O;          // 5
    tastmap[6] = sf::Keyboard::P;          // 6
    tastmap[7] = sf::Keyboard::LBracket;   // D
    tastmap[8] = sf::Keyboard::J;          // 7
    tastmap[9] = sf::Keyboard::K;          // 8
    tastmap[10] = sf::Keyboard::L;         // 9
    tastmap[11] = sf::Keyboard::Semicolon; // E
    tastmap[12] = sf::Keyboard::M;         // A
    tastmap[13] = sf::Keyboard::Comma;     // 0
    tastmap[14] = sf::Keyboard::Period;    // B
    tastmap[15] = sf::Keyboard::Slash;     // F
};
Chip8::~Chip8() {};

void Chip8::pocisti(){
    PC = 0x200;
    I = instrukcija = SP = DT= ST = 0;
    memset(graf, 0, 2048*4);
    memset(memorija, 0,  4096);
    for(int i = 0; i < 16; i++)
        stek[i] = tast[i] = V[i] = 0;
    memcpy(memorija, font, 80);
}

void Chip8::ucitaj_program(std::string fajl_putanja){
    pocisti();

    FILE* program = fopen(fajl_putanja.c_str(), "rb");

    fseek(program, 0, SEEK_END);
    long program_velicina = ftell(program);
    rewind(program);

    char* program_buffer = (char*) malloc(sizeof(char) * program_velicina);

    fread(program_buffer, sizeof(char), (size_t)program_velicina, program);

    for(int i = 0; i < program_velicina; ++i)
        memorija[i + 512] = (uint8_t)program_buffer[i];   // Load into memory starting
    
    fclose(program);
    free(program_buffer);
}

uint8_t Chip8::nasumican_bajt(){
    return dis(gen); 
};

void Chip8::emuliraj_ciklus(){
    instrukcija = memorija[PC] << 8 | memorija[PC+1]; // Poveze 2 susjedna bajta u memoriji u jednu 16-bitnu instrukciju
    uint16_t NN;

    switch (instrukcija & 0xF000){
        case OP_00E_:
            switch (instrukcija & SOPMASK_00E_){
                
                case SOP_00E0: // CLS - Ocisti ekran
                    memset(graf, 0, CHIP8_VELICINA_GRAFICKE_MEMORIJE * 4);
                    crtaj = true;
                    PC += 2;
                    break;
            
                case SOP_00EE: // RET - Povrat iz subrutine

                    PC = stek[--SP]; // Prethodni stack pointer
                    PC+=2;
                    break;

                default:
                    throw std::runtime_error("instrukcija nije validna");
                    break;
            }
            break;

        case OP_1NNN: // JMP addr - Skoci na adresu
            PC = instrukcija & 0x0FFF;
            break;
       
        case OP_2NNN: // CALL addr - Zapocni subrutinu na adresi. 
            stek[SP++] = PC;
            PC = instrukcija & 0x0FFF;
            break;

        case OP_3XNN:// SE Vx, byte - Preskoci sljedecu instrukciju ako VX = NN
            if(VX == (instrukcija & 0x00FF))
                PC += 4;
            else
                PC += 2;
            break;

        case OP_4XNN: // SNE Vx, byte - Preskoci sljedecu instrukciju ako VX != NN
            NN = (instrukcija & 0x00FF);
            if(VX != NN)
                PC += 4;
            else
                PC += 2;
            break;

        case OP_5XY0: // SE Vx, Vy - Preskoci sljedec instrukciju ako VX = VY
            if(VX == VY)
                PC += 4;
            else
                PC += 2;
            break;

        case OP_6XNN: // LD Vx, byte - Postavi VX = NN
            VX = (instrukcija & 0x00FF);
            PC += 2;
            break;

        case OP_7XNN: // ADD Vx, byte - Dodaj NN na VX
            VX += (instrukcija & 0x00FF);
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
                    PC += 2;
                    break;

                case SOP_8XY2: // AND Vx, Vy
                    VX &= VY;
                    PC += 2;
                    break;
            
                case SOP_8XY3: // XOR Vx, Vy
                    VX ^= VY;
                    PC += 2;
                    break;
            
                case SOP_8XY4: // ADD Vx, Vy
                    VX += VY;
                    V[0xF] = VY > (0xff - VX);
                    PC += 2;
                    break;
            
                case SOP_8XY5: // SUB Vx, Vy
                    V[0xF] = !(VY > VX);
                    VX -= VY;
                    PC += 2;
                    break;
            
                case SOP_8XY6: // SHR Vx, Vy - Pomjeri sve bite Vx udesno za 1
                    V[0xF] = VX & 0x1;
                    VX >>= 1;
                    PC += 2;
                    break;
            
                case SOP_8XY7: // SUBN Vx, Vy
                    V[0xF] = !(VX > VY);
                    VX = VY - VX;
                    PC += 2;
                    break;
            
                case SOP_8XYE: // SHL
                    V[0xF] = VX >> 7;
                    VX <<= 1;
                    PC += 2;
                    break;
            
                default:
                    throw std::runtime_error("instrukcija nije validna");
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

        case OP_CXNN: { // RND Vx, byte - Postavi Vx = [nasumican broj] AND NN
            VX = nasumican_bajt() & (instrukcija & 0x00FF);
            PC += 2;
            break;
        }

        // Crta sprajt na koordinatama Vx, Vy sireine 8 piksela i visine N piksela
        // Sprajt se cita pocevsi od memorijske lokacije sacuvane u registru I
        // Ako se makar jedan piksel promjeni sa 1 u 0, VF registar postaviti na 1
        case OP_DXYN:{
            uint8_t visina = instrukcija & 0x000F; // Visina sprajta
            V[0xF] = 0;
            uint16_t red;
            uint16_t pix;
            for(uint8_t i_reda = 0; i_reda < visina; i_reda++){
                red = memorija[I + i_reda];
                for(uint8_t i_bajta = 0; i_bajta < 8; i_bajta++){
                    if((red >> i_bajta) & 1){
                        pix = (VX + 7 - i_bajta + ((VY + i_reda) * 64))*4;
                        V[0xF] = graf[pix] == 255;
                        graf[pix+1] = \
                        graf[pix+2] = \
                        graf[pix+3] = \
                        graf[pix] ^= 255;
                    }
                }
            }
            crtaj = true;
            PC += 2;
            break;
        }

        case OP_EX__:{
            switch(instrukcija & SOPMASK_EX__){
                case SOP_EX9E: // Preskoci sljedecu instrukciju ako je tipka sa vrijednosti Vx trenutno pritisnuta
                    if(tast[VX] != 0)
                        PC += 4;
                    else
                        PC += 2;
                    break;
                
                case SOP_EXA1: // Preskoci sljedecu instrukciju ako tipka sa vrijednosti Vx nije trenutno pritisnuta
                    if(tast[VX] == 0)
                        PC += 4;
                    else
                        PC += 2;
                    break;
                default:
                    throw std::runtime_error("instrukcija nije validna(1)");
                    break;
            }
            break;
        }

        case OP_FX__:
            switch (instrukcija & SOPMASK_FX__){
            case SOP_FX07: // LD Vx, DT - Postavi Vx registar na vrijednost DT
                VX = DT;
                PC += 2;
                break;
            case SOP_FX0A:{
                bool tipka_pritisnuta = false;
                for(int i = 0; i < 16 && !tipka_pritisnuta; i++)
                    if(tast[i] != 0)
                        VX = i, tipka_pritisnuta = true;
                if(!tipka_pritisnuta)
                    return; // Ne povecava PC tako da ce opet izvrsiti ovu instrukciju u sljedecem ciklusu
                PC += 2; 
                break;
            }
            case SOP_FX15: // Postavi DT na vrijednost Vx
                DT = VX;
                PC += 2;
                break;
            case SOP_FX18: // Postavi DT na vrijednost Vx
                ST = VX;
                PC += 2;
                break;  
            case SOP_FX1E:
                V[0xF] = (I + VX > 0xFFF);
                I += VX;
                PC += 2;
                break;
            case SOP_FX29: // Postavi I registar na memorijsku lokaciju sprajta Vx-tog karakera u fontu
                I = VX * 0x5; // Svaki sprajt zauzima 5 bajti i pocinju od memorijske lokacije 0
                PC += 2;
                break;
            case SOP_FX33: // Sacuvaj binarnu reprezentaciju decimalnog oblika broja u Vx na adresama I, I+1, I+2
                // Vrijednosti se cuvaju na taj nacin da u memorjiskoj lokaciji na adresi I
                // cuvamo broj stotica, na adresi I+1 cuvamo broj desetica i na I+2 broj jedinica
                memorija[I] = VX / 100;
                memorija[I+1] = (VX / 10) % 10;
                memorija[I+2] = VX % 10;
                PC += 2;
                break;
            case SOP_FX55:{ // Spasava vrijednost svih registara od V0 do Vx na memorijskim adresama pocevsi od I
                uint16_t X = (instrukcija & 0x0F00) >> 8;
                for(int i = 0; i <= X; i++)
                    memorija[I+i] = V[i];
                // Pomjerzi I za X+1 lokacija u memoriji
                I += X+1;
                PC += 2;
                break;
            }
            case SOP_FX65:{ // Spasava vrijednost svih memoriskih lokacija od I do I+X u registrima pocevsi od V0
                uint16_t X = (instrukcija & 0x0F00) >> 8;
                for(int i = 0; i <= X; i++)
                     V[i] = memorija[I+i];
                // Pomjeri I za X+1 lokacija u memoriji
                I += X+1;
                PC += 2;
                break;
            }
            default:
                throw std::runtime_error("instrukcija nije validna(2)");
                break;
            }
            break;
    
        default:
            throw std::runtime_error("instrukcija nije validna(3)");
            break;
    }
}
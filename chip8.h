
#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>
#include <random>

#define CHIP8_MAX_FAJL_VELICINA 3584
#define CHIP8_VELICINA_GRAFICKE_MEMORIJE 2048
#define CHIP8_VELICINA_MEMORIJE 4096

enum {
    OP_00E_ = 0x0000,
    OP_1NNN = 0x1000,
    OP_2NNN = 0x2000,
    OP_3XNN = 0x3000,
    OP_4XNN = 0x4000,
    OP_5XY0 = 0x5000,
    OP_6XNN = 0x6000,
    OP_7XNN = 0x7000,
    OP_8XY_ = 0x8000,
    OP_9XY0 = 0x9000,
    OP_ANNN = 0xA000,
    OP_BNNN = 0xB000,
    OP_CXNN = 0xC000,
    OP_DXYN = 0xD000,
    OP_EX__ = 0xE000,
    OP_FX__ = 0xF000
};

enum {
    SOPMASK_00E_ = 0x000F,
    SOP_00E0 = 0x0000,
    SOP_00EE = 0x000E,

    SOPMASK_8XY_ = 0x000F,
    SOP_8XY0 = 0x0000,
    SOP_8XY1 = 0x0001,
    SOP_8XY2 = 0x0002,
    SOP_8XY3 = 0x0003,
    SOP_8XY4 = 0x0004,
    SOP_8XY5 = 0x0005,
    SOP_8XY6 = 0x0006,
    SOP_8XY7 = 0x0007,
    SOP_8XYE = 0x000E,

    SOPMASK_EX__ = 0x00FF,
    SOP_EX9E = 0x009E,
    SOP_EXA1 = 0x00A1,

    SOPMASK_FX__ = 0x00FF,
    SOP_FX07 = 0x0007,
    SOP_FX0A = 0x000A,
    SOP_FX15 = 0x0015,
    SOP_FX18 = 0x0018,
    SOP_FX1E = 0x001E,
    SOP_FX29 = 0x0029,
    SOP_FX33 = 0x0033,
    SOP_FX55 = 0x0055,
    SOP_FX65 = 0x0065
};

class Chip8 {
private: 
    uint8_t V[16]; // Registri V1 – VF
    uint16_t stek[16]; // 16 bajti stack memorije = 64 bita
    uint16_t SP; // 16-bitni Stack Pointer registar
    uint16_t PC; // 16-bitni Program Counter registar
    uint16_t I; // 16-bitni Index registar
    uint16_t instrukcija; // Trenutna instrukcija
   
    uint8_t memorija[CHIP8_VELICINA_MEMORIJE]; // Radna memorija ~4kb
    
public:
    uint8_t DT; // Delay Timer zvučni registar
    uint8_t ST; // Sound Timer zvučni registar

    uint8_t graf[CHIP8_VELICINA_GRAFICKE_MEMORIJE*4]; // Graficka memorija 64*32 bytes
    uint8_t tast[16]; // Tastatura
    sf::Keyboard::Key tastmap[16];
    bool crtaj;   
    Chip8();
    ~Chip8();

    void pocisti();
    void ucitaj_program(std::string fajl_putanja); // Ucitava program
    void emuliraj_ciklus(); // Emulira jedan ciklus procesora
    uint8_t nasumican_bajt();
};

#endif
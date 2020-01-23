#include <iostream>
#include "chip8.h"

int main(int argc, char **argv){
    if(argc != 2){
        std::cout << "Argumenti nisu validni" << std::endl;
        return 1;
    }

    Chip8 chip8 = Chip8();
    try{
        chip8.ucitaj_program(argv[1]);
    }catch(std::exception e){
        return 2;
    }
    std::cout << "YAY\n";
}
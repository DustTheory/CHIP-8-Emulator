#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <unistd.h>
#include <thread>
#include <map>
#include <cstdlib>
#include <string.h>

#include "chip8.h"

int main(int argc, char **argv){
    if(argc < 2){
        std::cout << "Argumenti nisu validni" << std::endl;
        return 1;
    }

    int frekvencija = 600; // Hz
    int zvukFrekvencija = 60; // Hz

    if(argc >= 3)
        frekvencija = std::atoi(argv[2]);
    if(argc >= 4)
        zvukFrekvencija = std::atoi(argv[3]);
    
    Chip8 chip8 = Chip8();
    chip8.ucitaj_program(argv[1]);
    int cpuInterval = 1000000/frekvencija;
    int zvukInterval = 1000000/zvukFrekvencija; 
    bool emulacijaTraje = true;
    bool zvuk_traje = false;

    sf::Sound Zvuk;
    const unsigned SAMPLES = 44100;
	const unsigned SAMPLE_RATE = 44100;
	const unsigned AMPLITUDE = 25000;
	
	sf::Int16 raw[SAMPLES];

	const double TWO_PI = 6.28318;
	const double increment = 440./44100;
	double x = 0;
	for (unsigned i = 0; i < SAMPLES; i++) {
		raw[i] = AMPLITUDE * sin(x*TWO_PI);
		x += increment;
	}
	
	sf::SoundBuffer Buffer;
	Buffer.loadFromSamples(raw, SAMPLES, 1, SAMPLE_RATE);
    Zvuk.setBuffer(Buffer);
	Zvuk.setLoop(true);

    sf::RenderWindow window(sf::VideoMode(640, 320), "Chip8 Emulator");
    sf::Texture texture;
    texture.create(64, 32);
    sf::Sprite sprite(texture); 
    sprite.setScale({10.0, 10.0});
    std::thread soundThread([&chip8, &emulacijaTraje, &zvukInterval, &Zvuk, &zvuk_traje](){
        while (emulacijaTraje){
            if(chip8.DT)
                chip8.DT--;
            if(chip8.ST){
                if(!zvuk_traje)
                    Zvuk.play(), zvuk_traje = true;
                 chip8.ST--;
            }else
                Zvuk.pause(), zvuk_traje = false;
            std::this_thread::sleep_for(std::chrono::microseconds(zvukInterval));
        }
    });
    
    while(window.isOpen()){
        sf::Event event;
        while(window.pollEvent(event)){
            if(event.type == sf::Event::Closed)
                window.close(), emulacijaTraje = false;
            for(int i = 0; i < 16; i++)
                chip8.tast[i] = sf::Keyboard::isKeyPressed(chip8.tastmap[i]);
            
        }
        chip8.emuliraj_ciklus();
        if(chip8.crtaj){
            texture.update(chip8.graf);
            window.clear();
            window.draw(sprite);
            chip8.crtaj = false;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(cpuInterval));
        window.display();
    }
    soundThread.join();
    return 0;
}
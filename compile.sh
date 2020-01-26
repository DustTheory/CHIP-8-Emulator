g++ -std=c++11 -c chip8.cpp
g++ -std=c++11 -c main.cpp
g++ -std=c++11 -o main main.o chip8.o -pthread -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio 
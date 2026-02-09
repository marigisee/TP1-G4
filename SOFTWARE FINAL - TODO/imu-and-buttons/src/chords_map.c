#include "chords_map.h"

const ButtonChord buttonChordMap[16] = {
    {"Nada", 0, CHORD_NONE},        // 0000

    {"DO mayor", 60, CHORD_MAJOR},  // 0001
    {"RE mayor", 62, CHORD_MAJOR},  // 0010
    {"MI mayor", 64, CHORD_MAJOR},  // 0011
    {"FA mayor", 65, CHORD_MAJOR},  // 0100
    {"SOL mayor", 67, CHORD_MAJOR}, // 0101
    {"LA mayor", 69, CHORD_MAJOR},  // 0110
    {"SI mayor", 71, CHORD_MAJOR},  // 0111

    {"Nada", 0, CHORD_NONE},        // 1000 (no nota válida, menor sin base)             
    
//do = 1000
//sol = 1010
//lam = 0111
//fa = 0010    
    
//lam = 0111
//fa = 0010 
//do = 1000
//sol = 1010
//fa = 0010 



    {"DO menor", 60, CHORD_MINOR},  // 1001
    {"RE menor", 62, CHORD_MINOR},  // 1010
    {"MI menor", 64, CHORD_MINOR},  // 1011
    {"FA menor", 65, CHORD_MINOR},  // 1100
    {"SOL menor", 67, CHORD_MINOR}, // 1101
    {"LA menor", 69, CHORD_MINOR},  // 1110
    {"SI menor", 71, CHORD_MINOR},  // 1111
};

uint8_t obtenerValorBotones(const uint8_t botones[4])
{
    uint8_t b = ((botones[3] & 1) << 3) |
                ((botones[2] & 1) << 2) |
                ((botones[1] & 1) << 1) |
                ((botones[0] & 1) << 0);
    return b;
}

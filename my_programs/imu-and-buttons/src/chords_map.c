#include "chords_map.h"

const ButtonChord buttonChordMap[16] = {
    {"Nada", 0, CHORD_NONE},        // 0000 - sin botones presionados
    {"DO mayor", 60, CHORD_MAJOR},  // 0001
    {"DO menor", 60, CHORD_MINOR},  // 0010
    {"RE mayor", 62, CHORD_MAJOR},  // 0011
    {"RE menor", 62, CHORD_MINOR},  // 0100
    {"MI mayor", 64, CHORD_MAJOR},  // 0101
    {"MI menor", 64, CHORD_MINOR},  // 0110
    {"FA mayor", 65, CHORD_MAJOR},  // 0111
    {"FA menor", 65, CHORD_MINOR},  // 1000
    {"SOL mayor", 67, CHORD_MAJOR}, // 1001
    {"SOL menor", 67, CHORD_MINOR}, // 1010
    {"LA mayor", 69, CHORD_MAJOR},  // 1011
    {"LA menor", 69, CHORD_MINOR},  // 1100
    {"SI mayor", 71, CHORD_MAJOR},  // 1101
    {"SI menor", 71, CHORD_MINOR},  // 1110
    {"DO#/Reb mayor", 61, CHORD_MAJOR}  // 1111
};

uint8_t obtenerValorBotones(const uint8_t botones[4])
{
    uint8_t b = ((botones[3] & 1) << 3) |
                ((botones[2] & 1) << 2) |
                ((botones[1] & 1) << 1) |
                ((botones[0] & 1) << 0);
    return b;
}

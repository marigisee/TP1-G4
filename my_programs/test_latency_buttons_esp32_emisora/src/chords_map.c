#ifndef BUTTONS_MAP_H
#define BUTTONS_MAP_H

#include "chords_map.h" // usa los tipos CHORD_MAJOR / CHORD_MINOR

/* ==========================================================
 * Tabla que asocia combinaciones de botones (b3 b2 b1 b0)
 * con un acorde (nombre, nota raíz y tipo mayor/menor).
 *
 * Cada entrada del arreglo corresponde a un valor binario
 * entre 0000 y 1111 (0 a 15), donde cada bit representa el
 * estado de un boton.
 *
 * Ejemplo:
 *   b3 b2 b1 b0 = 0b1010 --> i�ndice 10 -> "SOL menor"
 * ========================================================== */
const ButtonChord buttonChordMap[16] = {
    {"Nada", 0, CHORD_NONE},        // 0000 - sin botones presionados
    {"Nada", 0, CHORD_NONE},        // 0001
    {"DO menor", 60, CHORD_MINOR},  // 0010
    {"DO mayor", 60, CHORD_MAJOR},  // 0011
    {"RE menor", 62, CHORD_MINOR},  // 0100
    {"RE mayor", 62, CHORD_MAJOR},  // 0101
    {"MI menor", 64, CHORD_MINOR},  // 0110
    {"MI mayor", 64, CHORD_MAJOR},  // 0111
    {"FA menor", 65, CHORD_MINOR},  // 1000
    {"FA mayor", 65, CHORD_MAJOR},  // 1001
    {"SOL menor", 67, CHORD_MINOR}, // 1010
    {"SOL mayor", 67, CHORD_MAJOR}, // 1011
    {"LA menor", 69, CHORD_MINOR},  // 1100
    {"LA mayor", 69, CHORD_MAJOR},  // 1101
    {"SI menor", 71, CHORD_MINOR},  // 1110
    {"SI mayor", 71, CHORD_MAJOR}   // 1111
};

/**
 * @brief Convierte el arreglo de botones de la estructura IMUMessage
 *        en un valor binario (nibble) de 4 bits.
 *
 * Ejemplo:
 *   botones = [1,0,1,0] -> b = 0b0101 (5)
 *
 * @param msg  Estructura IMUMessage con los botones [0..3].
 * @return     Entero entre 0 y 15 (nibble que representa los botones).
 */
uint8_t obtenerValorBotones(const uint8_t botones[4])
{
    uint8_t b = ((botones[3] & 1) << 3) |
                ((botones[2] & 1) << 2) |
                ((botones[1] & 1) << 1) |
                ((botones[0] & 1) << 0);
    return b;
}
#endif /* BUTTONS_MAP_H */

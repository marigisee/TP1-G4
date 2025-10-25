#ifndef CHORDS_MAP_H
#define CHORDS_MAP_H

#include <stdint.h>

/* ==========================================================
 * Enumeración de tipos de acordes disponibles en el sistema.
 * ========================================================== */
typedef enum
{
    CHORD_NONE = 0, // sin acorde (ningún botón presionado)
    CHORD_MINOR,    // acorde menor
    CHORD_MAJOR     // acorde mayor
} chord_quality_t;

/* ==========================================================
 * Estructura general que representa un acorde MIDI.
 *
 *  - nombre: texto descriptivo ("SOL mayor")
 *  - root:   nota raíz (C4 = 60, D = 62, ...)
 *  - tipo:   mayor / menor / ninguno
 * ========================================================== */
typedef struct
{
    const char *nombre;   // Ejemplo: "SOL mayor"
    uint8_t root;         // Nota MIDI raíz (0–127)
    chord_quality_t tipo; // Tipo de acorde (enum)
} ButtonChord;

/* ==========================================================
 * Declaración externa de la tabla de 16 acordes definida en chords_map.c
 * ========================================================== */
extern const ButtonChord buttonChordMap[16];

/**
 * @brief Devuelve un puntero al acorde correspondiente al valor b.
 *
 * @param b Valor entre 0 y 15.
 * @return  Puntero a una entrada válida dentro de buttonChordMap.
 */
static inline const ButtonChord *getChordFromButtons(uint8_t b)
{
    return &buttonChordMap[(uint8_t)(b & 0x0F)];
}

#endif /* CHORDS_MAP_H */

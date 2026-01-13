#ifndef CHORDS_MAP_H
#define CHORDS_MAP_H

#include <stdint.h>

typedef enum
{
    CHORD_NONE = 0,
    CHORD_MINOR,
    CHORD_MAJOR
} chord_quality_t;

typedef struct
{
    const char *nombre;
    uint8_t root;
    chord_quality_t tipo;
} ButtonChord;

extern const ButtonChord buttonChordMap[16];

static inline const ButtonChord *getChordFromButtons(uint8_t b)
{
    return &buttonChordMap[(uint8_t)(b & 0x0F)];
}

uint8_t obtenerValorBotones(const uint8_t botones[4]);

#endif /* CHORDS_MAP_H */

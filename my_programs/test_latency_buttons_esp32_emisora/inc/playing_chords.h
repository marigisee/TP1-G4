#pragma once
#include <stdint.h>
#include "chords_map.h"

void playChord(const ButtonChord *chord);
void stopChord(const ButtonChord *chord);
void debugPrintChord(const ButtonChord *chord);

// NUEVO: rasgueo (dispara acorde con timings)
void strumChord(const ButtonChord *chord, uint8_t velocity, uint32_t now_ms);

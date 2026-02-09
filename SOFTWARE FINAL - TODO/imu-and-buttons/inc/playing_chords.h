#pragma once
#include <stdint.h>
#include "chords_map.h"

void playChord(const ButtonChord *chord, uint8_t velocity);
void stopChord(const ButtonChord *chord);
void debugPrintChord(const ButtonChord *chord);

void strumChord(const ButtonChord *chord, uint8_t velocity, uint32_t now_ms);

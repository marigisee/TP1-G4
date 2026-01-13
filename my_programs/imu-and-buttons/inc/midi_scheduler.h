#pragma once
#include <stdint.h>
#include <stdbool.h>

void midiSchedInit(void);
void midiSchedProcess(uint32_t now_ms);

bool midiSchedNoteOn (uint32_t t_ms, uint8_t ch, uint8_t note, uint8_t vel);
bool midiSchedNoteOff(uint32_t t_ms, uint8_t ch, uint8_t note, uint8_t vel);

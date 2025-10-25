#pragma once
#include "lowlevel.h"

/* Cada byte MIDI por SDI: 0x00 + byte (formato plugin RT-MIDI) */
static inline void sendMIDI(uint8_t b){
   sdiWrite16(0x00, b);
}

static inline void midiProgramChange(uint8_t ch, uint8_t prog){
   sendMIDI(0xC0 | (ch & 0x0F));
   sendMIDI(prog & 0x7F);
}

static inline void midiNoteOn(uint8_t ch, uint8_t note, uint8_t vel){
   sendMIDI(0x90 | (ch & 0x0F));
   sendMIDI(note & 0x7F);
   sendMIDI(vel  & 0x7F);
}

static inline void midiNoteOff(uint8_t ch, uint8_t note, uint8_t vel){
   sendMIDI(0x80 | (ch & 0x0F));
   sendMIDI(note & 0x7F);
   sendMIDI(vel  & 0x7F);
}



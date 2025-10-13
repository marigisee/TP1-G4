#ifndef VS1053B_MIDI_H
#define VS1053B_MIDI_H
#pragma once

#include <stdint.h>
#include "vs1053b_sapi.h" // usa vs1053b_sdi_write()

/* Status base (MSB=1) */
#define MIDI_NOTE_OFF 0x80
#define MIDI_NOTE_ON 0x90
#define MIDI_CONTROL_CHANGE 0xB0
#define MIDI_PROGRAM_CHANGE 0xC0
#define MIDI_PITCH_BEND 0xE0

/* MIDI-over-SDI requiere 0x00 + byte (status o data) */
static inline void sendMIDI(uint8_t b)
{
    uint8_t w[2] = {0x00, b};
    vs1053b_sdi_write(w, 2);
}

/* Helpers mínimos (sanitizan canal y data a 7 bits) */
static inline void midiNoteOn(uint8_t ch, uint8_t note, uint8_t vel)
{
    sendMIDI((uint8_t)(MIDI_NOTE_ON | (ch & 0x0F)));
    sendMIDI((uint8_t)(note & 0x7F));
    sendMIDI((uint8_t)(vel & 0x7F));
}
static inline void midiNoteOff(uint8_t ch, uint8_t note, uint8_t vel)
{
    sendMIDI((uint8_t)(MIDI_NOTE_OFF | (ch & 0x0F)));
    sendMIDI((uint8_t)(note & 0x7F));
    sendMIDI((uint8_t)(vel & 0x7F));
}
static inline void midiProgramChange(uint8_t ch, uint8_t prog)
{
    sendMIDI((uint8_t)(MIDI_PROGRAM_CHANGE | (ch & 0x0F)));
    sendMIDI((uint8_t)(prog & 0x7F));
}
static inline void midiControlChange(uint8_t ch, uint8_t ctrl, uint8_t val)
{
    sendMIDI((uint8_t)(MIDI_CONTROL_CHANGE | (ch & 0x0F)));
    sendMIDI((uint8_t)(ctrl & 0x7F));
    sendMIDI((uint8_t)(val & 0x7F));
}
static inline void midiPitchBend(uint8_t ch, uint16_t value14)
{
    uint16_t v = value14 & 0x3FFF; // 14 bits
    sendMIDI((uint8_t)(MIDI_PITCH_BEND | (ch & 0x0F)));
    sendMIDI((uint8_t)(v & 0x7F));        // LSB
    sendMIDI((uint8_t)((v >> 7) & 0x7F)); // MSB
}

/* Atajos */
static inline void midiAllNotesOff(uint8_t ch)
{
    midiControlChange(ch, 123, 0);
}

#endif /* VS1053B_MIDI_H */

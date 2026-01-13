#include "playing_chords.h"
#include "midi_sdi.h"
#include "midi_scheduler.h"
#include "sapi.h"

#define MIDI_CH 0
#define MIDI_VELOCITY_OFF 64

#define STRUM_GAP_MS  10
#define NOTE_LEN_MS   180

static void buildTriad(const ButtonChord *chord, uint8_t notes[3], uint8_t *n)
{
    if (chord->tipo == CHORD_MAJOR)
    {
        notes[0] = chord->root + 0;
        notes[1] = chord->root + 4;
        notes[2] = chord->root + 7;
        *n = 3;
    }
    else if (chord->tipo == CHORD_MINOR)
    {
        notes[0] = chord->root + 0;
        notes[1] = chord->root + 3;
        notes[2] = chord->root + 7;
        *n = 3;
    }
    else
    {
        *n = 0;
    }
}

void playChord(const ButtonChord *chord, uint8_t velocity)
{
    if (!chord || chord->tipo == CHORD_NONE)
        return;

    uint8_t notes[3], n = 0;
    buildTriad(chord, notes, &n);
    for (uint8_t i = 0; i < n; i++)
        midiNoteOn(MIDI_CH, notes[i], velocity);
}

void stopChord(const ButtonChord *chord)
{
    if (!chord || chord->tipo == CHORD_NONE)
        return;

    uint8_t notes[3], n = 0;
    buildTriad(chord, notes, &n);
    for (uint8_t i = 0; i < n; i++)
        midiNoteOff(MIDI_CH, notes[i], MIDI_VELOCITY_OFF);
}

void strumChord(const ButtonChord *chord, uint8_t velocity, uint32_t now_ms)
{
    if (!chord || chord->tipo == CHORD_NONE)
        return;

    uint8_t notes[3], n = 0;
    buildTriad(chord, notes, &n);
    if (n == 0) return;

    for (uint8_t i = 0; i < n; i++)
    {
        uint32_t t_on  = now_ms + (uint32_t)i * STRUM_GAP_MS;
        uint32_t t_off = t_on  + NOTE_LEN_MS;

        midiSchedNoteOn (t_on,  MIDI_CH, notes[i], velocity);
        midiSchedNoteOff(t_off, MIDI_CH, notes[i], MIDI_VELOCITY_OFF);
    }
}

void debugPrintChord(const ButtonChord *chord)
{
    if (!chord || chord->tipo == CHORD_NONE) {
        uartWriteString(UART_USB, "DEBUG chord: NONE\r\n");
        return;
    }

    uint8_t notes[3], n = 0;
    buildTriad(chord, notes, &n);

    char line[96];
    int len = 0;
    len += snprintf(line+len, sizeof(line)-len,
                    "DEBUG chord: %s (root=%u tipo=%d) -> notes:",
                    chord->nombre, chord->root, chord->tipo);
    for (uint8_t i = 0; i < n; i++) {
        len += snprintf(line+len, sizeof(line)-len, " %u", notes[i]);
    }
    len += snprintf(line+len, sizeof(line)-len, "\r\n");
    uartWriteString(UART_USB, line);
}

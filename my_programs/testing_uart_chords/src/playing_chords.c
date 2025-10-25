/** @file playing_chords.c
 *  @brief Construcción y ejecución de acordes (triadas) vía MIDI sobre VS1053.
 *
 *  Explicación rápida:
 *  - `buildTriad()` arma las notas (root, 3ra, 5ta) según mayor/menor.
 *  - `playChord()` envía NOTE ON de todas las notas de la triada.
 *  - `stopChord()` envía NOTE OFF de esas mismas notas.
 *
 *  Requiere:
 *   - `chords_map.h` para conocer `ButtonChord` (root y tipo).
 *   - `vs1053_midi.h` para las primitivas `midiNoteOn/Off`.
 */

#include "playing_chords.h"
#include "vs1053_midi.h"

#define MIDI_CH 0            // Canal MIDI usado (0..15)
#define MIDI_VELOCITY_ON 100 // Velocidad para NOTE ON (0..127)
#define MIDI_VELOCITY_OFF 64 // Velocidad para NOTE OFF (0..127)
#define MIDI_MAX_NOTE 127    // Nota MIDI máxima válida

/**
 * @brief Construye una triada (3 notas) según el tipo de acorde.
 * @param[in]  chord  Acorde con root y tipo (mayor/menor/none).
 * @param[out] notes  Buffer con hasta 3 notas MIDI resultantes.
 * @param[out] n      Cantidad de notas válidas cargadas en `notes`.
 *
 * Notas:
 * - Para mayor: 0, +4, +7 semitonos.
 * - Para menor: 0, +3, +7 semitonos.
 * - Si el tipo es NONE, no devuelve notas (n=0).
 */
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

/**
 * @brief Toca el acorde (NOTE ON para sus 3 notas).
 */
void playChord(const ButtonChord *chord)
{
    if (!chord || chord->tipo == CHORD_NONE)
        return;
    uint8_t notes[3], n = 0;
    buildTriad(chord, notes, &n);
    for (uint8_t i = 0; i < n; i++)
        midiNoteOn(MIDI_CH, notes[i], MIDI_VELOCITY_ON);
}

/**
 * @brief Detiene el acorde (NOTE OFF para sus 3 notas).
 */
void stopChord(const ButtonChord *chord)
{
    if (!chord || chord->tipo == CHORD_NONE)
        return;
    uint8_t notes[3], n = 0;
    buildTriad(chord, notes, &n);
    for (uint8_t i = 0; i < n; i++)
        midiNoteOff(MIDI_CH, notes[i], MIDI_VELOCITY_OFF);
}

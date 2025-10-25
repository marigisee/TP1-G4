/** @file playing_chords.h
 *  @brief API para reproducir/detener acordes basados en `ButtonChord`.
 */

#ifndef PLAYING_CHORDS_H
#define PLAYING_CHORDS_H

#include <stdint.h>
#include "chords_map.h"

/**
 * @brief Envía NOTE ON de la triada correspondiente al acorde.
 * @param chord Acorde con root y tipo (ignora si es NULL o NONE).
 */
void playChord(const ButtonChord *chord);

/**
 * @brief Envía NOTE OFF de la triada correspondiente al acorde.
 * @param chord Acorde con root y tipo (ignora si es NULL o NONE).
 */
void stopChord(const ButtonChord *chord);

#endif /* PLAYING_CHORDS_H */

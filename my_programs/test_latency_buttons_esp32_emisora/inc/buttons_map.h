#ifndef BUTTONS_MAP_H
#define BUTTONS_MAP_H

#include <stdint.h>

/* Construye el nibble b (b3..b0) a partir del array botones[4].
   LSB = botones[0]. Invertí el orden si tu cableado lo requiere. */
static inline uint8_t obtenerValorBotones(const uint8_t botones[4]) {
    return ((botones[3] & 1u) << 3)
         | ((botones[2] & 1u) << 2)
         | ((botones[1] & 1u) << 1)
         | ((botones[0] & 1u) << 0);
}

#endif /* BUTTONS_MAP_H */
    
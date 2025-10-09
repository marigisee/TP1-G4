/*=============================================================================
 * Copyright (c) 2025, Marina Cuello <marina.cuello@alu.ing.unlp.edu.ar>
 * All rights reserved.
 * License:  (see LICENSE.txt)
 * Date: 2025/10/09
 * Version: 1
 *===========================================================================*/

/*=====[Avoid multiple inclusion - begin]====================================*/

#ifndef __TESTING_VS1053B_H
#define __TESTING_VS1053B_H

/*=====[Inclusions of public function dependencies]==========================*/

#include <stdint.h>
#include <stddef.h>



/*=====[Definition macros of public constants]===============================*/

#define TESTING_VS1053B_ID 0x5A5A


/*=====[Public function-like macros]=========================================*/

static inline uint16_t testing_vs1053b_get_id(void)
{
    return (uint16_t)TESTING_VS1053B_ID;
}

/*=====[Definitions of public data types]====================================*/

/*=====[Prototypes (declarations) of public functions]=======================*/

/*=====[Prototypes (declarations) of public interrupt functions]=============*/




/*=====[Avoid multiple inclusion - end]======================================*/

#endif /* __TESTING_VS1053B_H__ */

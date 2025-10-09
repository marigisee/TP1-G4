/*=============================================================================
 * Copyright (c) 2025, Marina Cuello <marina.cuello@alu.ing.unlp.edu.ar>
 * All rights reserved.
 * License:  (see LICENSE.txt)
 * Date: 2025/10/09
 * Version: 1
 *===========================================================================*/

/*=====[Inclusions of function dependencies]=================================*/

#include "testing_vs1053b.h"
#include "sapi.h"

/*=====[Definition macros of private constants]==============================*/

/*=====[Definitions of extern global variables]==============================*/

/*=====[Definitions of public global variables]==============================*/

/*=====[Definitions of private global variables]=============================*/

/*=====[Main function, program entry point after power on or reset]==========*/

int main( void )
{
   // ----- Setup -----------------------------------
   boardInit();

   uartConfig(UART_USB, 115200);

   printf("Header OK: TESTING_VS1053B_ID=0x%04X, get_id()=0x%04X\r\n",
          TESTING_VS1053B_ID, testing_vs1053b_get_id());

   while (TRUE)
   {
      delay(1000);
   }

   // YOU NEVER REACH HERE, because this program runs directly or on a
   // microcontroller and is not called by any Operating System, as in the 
   // case of a PC program.
   return 0;
}

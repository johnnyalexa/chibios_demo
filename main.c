/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "ch.h"
#include "hal.h"

//#include "test.h"

extern void start_threads(void);
extern void start_tcp_threads(void);
/*
 * SPI TX and RX buffers.
 */
//static uint8_t txbuf[]={1,2,3,4,5,6,7,8,9,10};
//static uint8_t rxbuf[10];


/*
 * Application entry point.
 */
int main(void) {

	static uint8_t rx_buf[10]={0};
	static uint8_t tx_buf[10]={1,5,7,2,76,3,43,76,87,98};
  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();



	start_threads();
	start_tcp_threads();
	
//	chThdCreateStatic(spi_thread_1_wa, sizeof(spi_thread_1_wa), NORMALPRIO + 1, spi_thread_1, NULL);
  /*
   * Normal main() thread activity, in this demo it does nothing except
   * sleeping in a loop and check the button state.
   */
  while (TRUE) {
		
		//SPI2->DR = 0x0F;
//spiExchange(&SPID2, 10, tx_buf, rx_buf);
   // if (palReadPad(GPIOA, GPIOA_BUTTON))
    //  TestThread(&SD2);
    chThdSleepMicroseconds(10);
  }
}

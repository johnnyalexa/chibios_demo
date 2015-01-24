#include "ch.h"
#include "hal.h"



/*
 * Red LED blinker thread, times are in milliseconds.
 */
static WORKING_AREA(waThread1, 128);
static msg_t Thread1(void *arg) {

  (void)arg;
  chRegSetThreadName("blinker");
  while (TRUE) {
    palClearPad(GPIOA, LED1);
    chThdSleepMilliseconds(40);
    palSetPad(GPIOA, LED1);
    chThdSleepMilliseconds(40);
  }
}

/*
 * Red LED blinker thread, times are in milliseconds.
 */
static WORKING_AREA(waThread2, 128);
static msg_t Thread2(void *arg) {

  (void)arg;
  chRegSetThreadName("blinker");
  while (TRUE) {
    palClearPad(GPIOA, LED2);
    chThdSleepMilliseconds(10);
    palSetPad(GPIOA, LED2);
    chThdSleepMilliseconds(10);
  }
}

/*
 * Red LED blinker thread, times are in milliseconds.
 */
static WORKING_AREA(waThread3, 128);
static msg_t Thread3(void *arg) {

  (void)arg;
  chRegSetThreadName("blinker");
  while (TRUE) {
    palClearPad(GPIOA, LED3);
    chThdSleepMilliseconds(20);
    palSetPad(GPIOA, LED3);
    chThdSleepMilliseconds(20);
  }
}

void start_threads(void){
 /*
   * Creates the blinker thread.
   */
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);
	chThdCreateStatic(waThread2, sizeof(waThread2), NORMALPRIO, Thread2, NULL);
	chThdCreateStatic(waThread3, sizeof(waThread3), NORMALPRIO, Thread3, NULL);


}


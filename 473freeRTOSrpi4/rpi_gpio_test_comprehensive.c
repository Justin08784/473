/* GPIO test, currently GPIO ports 2 - 11 and 13 thru 26 are verified */


/**< C libraries includes*/
#include <stddef.h>
#include <stdint.h>
//#include <stdio.h>

/**< FreeRTOS port includes*/
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"

/**< Drivers includes*/
#include "gpio.h"
/* this task runs every 1m */
void task1(void *pParam) {
(void) pParam;
  portTickType xLastWakeTime;
  const portTickType xFrequency = 1;//1ms task frequency

     // Initialise the xLastWakeTime variable with the current time.
     xLastWakeTime = xTaskGetTickCount();

     for( ;; )
     {
         
        vTaskDelayUntil(&xLastWakeTime,xFrequency);
     	//shiting 1 write test
	//int i;
 	//for (i=0; i<28; i++) {
        //	gpio_pin_set(i, 0);
        //	gpio_pin_set(i, 1);
        //	gpio_pin_set(i, 0);
 	//}

	//shifting read test
	int i, x;
        for (i=0; i<14; i++) {
		x = gpio_pin_read(i);
        	gpio_pin_set(i+14, x);
        }
    }
}


void main(void)
{
		
	
	//gpio init
	int i;
	//initialize pins 0 thru 27 as outputs for write test. 
 	//for (i=0; i<28; i++) {
        //	 gpio_pin_init(i, OUT, GPIO_PIN_PULL_UP);
 	//}

	//for read write test 
	//initialize pins 0 thru 13 as inputs
 	for (i=0; i<14; i++) {
                gpio_pin_init(i, IN, GPIO_PIN_PULL_UP);
        }
	//initialize pins 14 thru 27 as outputs
 	for (i=14; i<28; i++) {
        	gpio_pin_init(i, OUT, GPIO_PIN_PULL_UP);
 	}

	// task creation	    	 
    	xTaskCreate(task1, "GPIO21", 128, NULL, 1, NULL);//priority 1
    	
	//start task
	vTaskStartScheduler();
	
	while(1);
}

void vApplicationIdleHook( void ){}
void vApplicationTickHook( void ){}

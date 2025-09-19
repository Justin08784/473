
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
int change = 0;
int counter = 0;
/* this task runs every 1m toggling gpio21 */
void task1(void *pParam) {
(void) pParam;
  portTickType xLastWakeTime;
  const portTickType xFrequency = 1;//1ms task frequency

     // Initialize the xLastWakeTime variable with the current time.
     xLastWakeTime = xTaskGetTickCount();

     for( ;; )
     {
         
        vTaskDelayUntil(&xLastWakeTime,xFrequency);
        //counter input test
	//jumper 26 to 21, 19 to 20, 13 to 16
	//observe count pattern on 2, 3 and 4
		gpio_pin_set(26, counter & 1);
		gpio_pin_set(19, (counter & 2) >> 1);
		gpio_pin_set(13, (counter & 4) >> 2);
		if (counter == 7)
			counter = 0;
		else	
			counter++;
		gpio_pin_set(2, gpio_pin_read(21));
		gpio_pin_set(3, gpio_pin_read(20));
		gpio_pin_set(4, gpio_pin_read(16));

	     	//counter output test
		/*
		gpio_pin_set(21, counter & 1);
		gpio_pin_set(20, (counter & 2) >> 1);
		gpio_pin_set(16, (counter & 4) >> 2);
		if (counter == 7)
			counter = 0;
		else	
			counter++;
		*/
		
    }
}


void main(void)
{
		
	
	//gpio init
	//gpio 21, 20, 16 count output test
	/*
	gpio_pin_init(GPIO_21, OUT, GPIO_PIN_PULL_UP);
	gpio_pin_init(GPIO_20, OUT, GPIO_PIN_PULL_UP);
	gpio_pin_init(GPIO_16, OUT, GPIO_PIN_PULL_UP);
	*/
	//gpio 21, 20, 16 count input test
	gpio_pin_init(GPIO_21, IN, GPIO_PIN_PULL_UP);
	gpio_pin_init(GPIO_20, IN, GPIO_PIN_PULL_UP);
	gpio_pin_init(GPIO_16, IN, GPIO_PIN_PULL_UP);
	
	gpio_pin_init(GPIO_26, OUT, GPIO_PIN_PULL_UP);
	gpio_pin_init(GPIO_19, OUT, GPIO_PIN_PULL_UP);
	gpio_pin_init(GPIO_13, OUT, GPIO_PIN_PULL_UP);
	
	gpio_pin_init(GPIO_2, OUT, GPIO_PIN_PULL_UP);
	gpio_pin_init(GPIO_3, OUT, GPIO_PIN_PULL_UP);
	gpio_pin_init(GPIO_4, OUT, GPIO_PIN_PULL_UP);
	
	// task creation	    	 
    	xTaskCreate(task1, "GPIO21", 128, NULL, 1, NULL);//priority 1
    	
	//start task
	vTaskStartScheduler();
	
	while(1);
}

void vApplicationIdleHook( void ){}
void vApplicationTickHook( void ){}




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
/* this task runs every 1m toggling gpio21 */
void task1(void *pParam) {
(void) pParam;
  portTickType xLastWakeTime;
  const portTickType xFrequency = 1;//1ms task frequency

     // Initialise the xLastWakeTime variable with the current time.
     xLastWakeTime = xTaskGetTickCount();

     for( ;; )
     {
         
        vTaskDelayUntil(&xLastWakeTime,xFrequency);
        gpio_pin_toggle(21); //toggle gpio
        //gpio_pin_set(21, 1); //to make gpio high
        //gpio_pin_set(21, 0); //to make gpio low
        
    }
}


void main(void)
{
		
	
	//gpio init
	gpio_pin_init(GPIO_21, OUT, GPIO_PIN_PULL_UP);
	
	// task creation	    	 
    	xTaskCreate(task1, "GPIO21", 128, NULL, 1, NULL);//priority 1
    	
	//start task
	vTaskStartScheduler();
	
	while(1);
}

void vApplicationIdleHook( void ){}
void vApplicationTickHook( void ){}



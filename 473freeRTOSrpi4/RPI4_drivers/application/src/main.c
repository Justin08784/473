//starting code for robot chasses control using RPI4B with freeRTOS
//authored by Jared Hull
//converted by MS to current RPI4B freeRTOS port MS 7/28/22
//note you cannot use GPIO (pins) 14 and 15. They are used as a UART port counsel connection.
//pins means GPIO below, they are not the connector pins.

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

/*
 * Input commands and pin mounts
 */

#define FORWARD 'F'
#define LEFT    'L'
#define BACK    'B'
#define RIGHT   'R'
#define STOP    'S'

//Adjust to add intermediate speeds
#define IDLE    0
#define SPEED   1

//Adjust to reverse motor polarity
int LEFT_MOTOR  = 1;
int RIGHT_MOTOR = 0;

//Testing for RPI4B

//Motor Control 
#define EN1 20  //pwm pin, left motor
#define EN2 21  //pwm pin, right motor
#define A_1 6   //Y1, left motor positive
#define A_2 13  //Y2, left motor negative
#define A_3 19  //Y3, right motor positive
#define A_4 26  //Y4, right motor negative

//Pins for your distance sensor
#define TRIG 9
#define ECHO 11

//Task monitor trace pins for tasks 1 thru 3

#define T1_PIN 16
#define T2_PIN 5
#define T3_PIN 0

#define LED_PIN 2

//moveRobot as used in lab 1
void moveRobot(char command);

//Helper function for moveRobot as used in lab 1
void motorControl(int ifLeftMotor, char command);

//

/****
		TODO: LCD function declarations. See GPIO pin settings above.






	****/

int DISTANCE_IN_TICKS = 0;
SemaphoreHandle_t semaphore_dist = NULL;



//Task 1 is implemented for you. It interfaces with the distance sensor and 
//calculates the number of ticks the echo line is high after trigger, storing that value in
//the global variable DISTANCE_IN_TICKS

//You may need to alter some values as described in the lab documentation.

void task1() {
	portTickType xLastWakeTime;
	const portTickType xFrequency = 50 / portTICK_RATE_MS; // TODO: shouldn't this be 50 / portTick_RATE_MS and not 200? In the lab its 50ms not 200ms.
	
	xLastWakeTime = xTaskGetTickCount();

	while(1) {
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
		
		//IN TASK
		gpio_pin_set(T1_PIN, 1);
		gpio_pin_set(TRIG, 1);
		vTaskDelay(1);
		gpio_pin_set(TRIG, 0);
		while (xSemaphoreTake(semaphore_dist, portMAX_DELAY) != pdTRUE); // don't override the semaphore

		while(gpio_pin_read(ECHO) == 0);
		
		portTickType curr = xTaskGetTickCount();
		while(gpio_pin_read(ECHO) == 1);
		portTickType traveltime_in_ticks = xTaskGetTickCount() - curr;

		DISTANCE_IN_TICKS = traveltime_in_ticks;

		xSemaphoreGive(semaphore_dist);
		gpio_pin_set(T1_PIN, 0);
	}
}

/*
Task 2: Read the global variable from task1 when it is ready and move as needed
*/
void task2() {
	const portTickType xFrequency = 100 / portTICK_RATE_MS;
	const int STOP_DISTANCE_TICKS = 5;
	portTickType xLastWakeTime = xTaskGetTickCount();

	while(1) {

		vTaskDelayUntil(&xLastWakeTime, xFrequency); // Wait 100ms between drive adjustments

		gpio_pin_set(T2_PIN, 1);
		while (xSemaphoreTake(semaphore_dist, portMAX_DELAY) != pdTRUE); // don't override the semaphore

		// now safe to read the distance variable
		if (DISTANCE_IN_TICKS <= STOP_DISTANCE_TICKS) {
			moveRobot(STOP);
		} else {
			moveRobot(FORWARD);
		}

		xSemaphoreGive(semaphore_dist);
		gpio_pin_set(T2_PIN, 0);

	}
}

/*
Task 3: Flash an LED based on distance measurement. This task has no assigned period, runs whenever possible.
*/
void task3() {
	uint32_t distance_cm = 0;
	// int pulse_frequency; // in Hz
    uint32_t pulse_period_ms = 0;
    uint32_t dist_in_ticks_local;

	while(1){
        vTaskDelay(pulse_period_ms);

		gpio_pin_set(T3_PIN, 1);
        dist_in_ticks_local = DISTANCE_IN_TICKS;
        dist_in_ticks_local = dist_in_ticks_local == 0 ? 1 : dist_in_ticks_local;
		distance_cm = dist_in_ticks_local * portTICK_RATE_MS * 17; // 17 comes from physics: sound travels 343 m/s and travels 2*distance for the round trip
        // pulse_period_ms = distance_cm * (1000 / 10); // this is the correct frequency but it is hard to observe...
        pulse_period_ms = distance_cm * (100 / 10);     // ...so we use this instead

		// 25ms is enough to see a blink
		gpio_pin_set(LED_PIN, 1);
		vTaskDelay(25 / portTICK_RATE_MS);
		gpio_pin_set(LED_PIN, 0);

		gpio_pin_set(T3_PIN, 0);
	}
}


int main(void) {
	gpio_pin_init(T1_PIN, OUT, GPIO_PIN_PULL_UP);
	gpio_pin_init(T2_PIN, OUT, GPIO_PIN_PULL_UP);
	gpio_pin_init(T3_PIN, OUT, GPIO_PIN_PULL_UP);
	gpio_pin_set(T1_PIN, 0);
	gpio_pin_set(T2_PIN, 0);
	gpio_pin_set(T3_PIN, 0);

	gpio_pin_init(LED_PIN, OUT, GPIO_PIN_PULL_UP);
	gpio_pin_init(EN1, OUT, GPIO_PIN_PULL_UP);
	gpio_pin_init(EN2, OUT, GPIO_PIN_PULL_UP);
	gpio_pin_init(A_1, OUT, GPIO_PIN_PULL_UP);
	gpio_pin_init(A_2, OUT, GPIO_PIN_PULL_UP);
	gpio_pin_init(A_3, OUT, GPIO_PIN_PULL_UP);
	gpio_pin_init(A_4, OUT, GPIO_PIN_PULL_UP);

	gpio_pin_init(TRIG, OUT, GPIO_PIN_PULL_UP);
	gpio_pin_init(ECHO, IN, GPIO_PIN_PULL_NON);
	gpio_pin_set(TRIG, 0);

    semaphore_dist = xSemaphoreCreateMutex();

	xTaskCreate(task1, "t1", 128, NULL, 3, NULL);
	// xTaskCreate(task2, "t2", 128, NULL, 2, NULL);
	xTaskCreate(task3, "t3", 128, NULL, 1, NULL);

	vTaskStartScheduler();

	/*
	 *	We should never get here, but just in case something goes wrong,
	 *	we'll place the CPU into a safe loop.
	 */
	while(1) {
		;
	}
}

void moveRobot(char command) {
    switch(command) {
        case FORWARD:
            motorControl(LEFT_MOTOR, FORWARD);
            motorControl(RIGHT_MOTOR, FORWARD);
            break;
        case LEFT:
            motorControl(LEFT_MOTOR, STOP);
            motorControl(RIGHT_MOTOR, FORWARD);
            break; 
        case BACK:
            motorControl(LEFT_MOTOR, BACK);
            motorControl(RIGHT_MOTOR, BACK);
            break; 
        case RIGHT:
            motorControl(LEFT_MOTOR, FORWARD);
            motorControl(RIGHT_MOTOR, STOP);
            break;
        case STOP:
            motorControl(LEFT_MOTOR, STOP);
            motorControl(RIGHT_MOTOR, STOP);
            break;
        default:
            break;
    }   
}

void motorControl(int ifLeftMotor, char command) {
    int enable      = ifLeftMotor ? EN1 : EN2;
    int motorPos    = ifLeftMotor ? A_1 : A_3;
    int motorNeg    = ifLeftMotor ? A_2 : A_4;

    switch (command) {
        case FORWARD:
            gpio_pin_set(enable, SPEED);
            gpio_pin_set(motorPos, 1);
            gpio_pin_set(motorNeg, 0);
            break;
        case BACK:
            gpio_pin_set(enable, SPEED);
            gpio_pin_set(motorPos, 0);
            gpio_pin_set(motorNeg, 1);
            break;    
        case STOP:
            gpio_pin_set(motorPos, 0);
            gpio_pin_set(motorNeg, 0);
            break;
        default:
            break;           
    }
}

void vApplicationIdleHook( void ){}
void vApplicationTickHook( void ){}

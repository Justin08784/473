#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include "task.h"

// define two tasks for Blink & AnalogRead
void TaskBlink(void *pvParameters);
void TaskAnalogRead(void *pvParameters);
static inline void cpu_work(u32 ms)
{
    // 60000 u32 increments corresponds to 132.05ms
    #define U32_INCREMENTS_PER_MILLISECOND 454
    u32 iter_limit = U32_INCREMENTS_PER_MILLISECOND * ms;
    for (volatile u32 i = 0; i < iter_limit; ++i);
}

#define MS_TO_TICKS(ms) ((ms)+portTICK_PERIOD_MS-1)/portTICK_PERIOD_MS


void TaskBlink(void *pvParameters)  // This is a task.
{
    (void) pvParameters;

    // initialize digital LED_BUILTIN on pin 13 as an output.
    // pinMode(LED_BUILTIN, OUTPUT);
    // pinMode(12, OUTPUT);
    // for (;;) {
    //     digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    //     digitalWrite(12, HIGH);
    //     vTaskDelay(949 / portTICK_PERIOD_MS ); // wait for one second
    //     digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    //     digitalWrite(12, LOW);
    //     vTaskDelay(949 / portTICK_PERIOD_MS ); // wait for one second
    // }

    // pinMode(LED_BUILTIN, OUTPUT);
    // volatile int i = 0;
    // for (;;) {
    //     PORTB |= (1 << 5);
    //     // digitalWrite(LED_BUILTIN, HIGH);
    //     for(i=0;i<30000;i++);
    //     PORTB &= ~(1 << 5);
    //     // digitalWrite(LED_BUILTIN, LOW);
    //     for(i=0;i<30000;i++);
    // }

    Serial.println("startgin");
    // demo g1
    pinMode(LED_BUILTIN, OUTPUT);
    for (;;) {
        PORTB |= (1 << 5);
        cpu_work(900);
        PORTB &= ~(1 << 5);
        cpu_work(100);
    }

}

void TaskAnalogRead(void *pvParameters)  // This is a task.
{
    (void) pvParameters;

    for (;;) {
        // read the input on analog pin 0:
        int sensorValue = analogRead(A0);
        // print out the value you read:
        Serial.println(sensorValue);
        vTaskDelay(1);  // one tick delay (15ms) in between reads for stability
    }
}

void partb_main()
{
    // initialize serial communication at 9600 bits per second:
    Serial.begin(9600);
    while (!Serial) {
        ; // wait for serial port to connect. Needed for native USB, on LEONARDO, MICRO, YUN, and other 32u4 based boards.
    }

    // Now set up two tasks to run independently.
    xTaskCreate(
        TaskBlink
        , "Blink"   // A name just for humans
        , 128  // This stack size can be checked & adjusted by reading the Stack Highwater
        , NULL
        , 2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
        , NULL
      );

    xTaskCreate(
        TaskAnalogRead
        , "AnalogRead"
        , 128  // Stack size
        , NULL
        , 1  // Priority
        , NULL
        );

    // Now the task scheduler, which takes over control of scheduling individual tasks, is automatically started.
}


void partc_task1(void *param)  // This is a task.
{
    // toggle pin 12
    pinMode(12, OUTPUT);
    TickType_t prev = 0;
    for (;;) {
        PORTB |=   1 << 4;
        // cpu_work(30);
        cpu_work(55);
        PORTB &= ~(1 << 4);
        vTaskDelayUntil(&prev, 5);
    }
}

void partc_task2(void *param)  // This is a task.
{
    // toggle pin 13
    pinMode(13, OUTPUT);
    TickType_t prev = 0;
    for (;;) {
        PORTB |=   1 << 5;
        cpu_work(10);
        PORTB &= ~(1 << 5);
        vTaskDelayUntil(&prev, 2);
    }
}


void partc_main()
{
    // initialize serial communication at 9600 bits per second:
    Serial.begin(9600);
    while (!Serial);

    xTaskCreate(partc_task1,
        "t1",
        128,
        NULL,
        2,
        NULL
    );

    xTaskCreate(partc_task2,
        "t2",
        128,
        NULL,
        3,
        NULL
    );
}

// PART D //
SemaphoreHandle_t semaphore;

void partd_task1(void* param)  // This is a task.
{
    // toggle pin 12
    for (;;) {
        while (xSemaphoreTake(semaphore, (TickType_t) 1000) != pdPASS); // wait until the semaphore passes
        cpu_work(20);
        // cpu_work(55);
        PORTB &= ~(1 << 4);
    }
}

void partd_task2(void* param)  // This is a task.
{
    // toggle pin 13
    TickType_t prev = 0;
    for (;;) {
        PORTB |=   1 << 5;
        cpu_work(10);
        PORTB &= ~(1 << 5);
        vTaskDelayUntil(&prev, 2);
    }
}

void partd_task3(void* param)
{ // Task 3
    TickType_t prev = 0;
    for (;;) {
        vTaskDelayUntil(&prev, 5);
        PORTB |=   1 << 4;
        xSemaphoreGive(semaphore);
    }
}

void partd_main()
{
    Serial.begin(9600);
    while (!Serial);

    semaphore = xSemaphoreCreateBinary();
    pinMode(12, OUTPUT);
    pinMode(13, OUTPUT);

    xTaskCreate(
        partd_task1,
        "t1",
        128,
        NULL,
        1,
        NULL
    );

    xTaskCreate(
        partd_task2,
        "t2",
        128,
        NULL,
        2,
        NULL
    );

    xTaskCreate(
        partd_task3,
        "t3",
        128,
        NULL,
        3,
        NULL
    );
}


SemaphoreHandle_t e_sem = NULL;
void parte_task1(void* param)
{
    // toggles pin 12
    TickType_t prev = 0;
    uint8_t t2_val = 0;

    for (;;) {
        vTaskDelayUntil(&prev, 2); // ~30ms
        t2_val = PORTB & (1 << 5);
        PORTB &= ~(1 << 5); // save t2's pin

        PORTB |=   1 << 4;
        cpu_work(20);
        PORTB &= ~(1 << 4);

        PORTB |= t2_val;    // restore t2's pin
    }
}

void parte_task2(void* param)
{
    for (;;) {
        while (xSemaphoreTake(e_sem, 1000) != pdPASS);
        PORTB |=   1 << 5;
        cpu_work(100);
        PORTB &= ~(1 << 5);
    }
}

void parte_isr_pin2()
{
    // v1: non-deferred
    // cpu_work(100);

    // v2: deferred
    xSemaphoreGiveFromISR(e_sem, NULL);
}

void parte_main()
{
    // initialize serial communication at 9600 bits per second:
    Serial.begin(9600);
    while (!Serial);

    e_sem = xSemaphoreCreateBinary();

    pinMode(2,  INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(2), parte_isr_pin2, RISING);
    pinMode(12, OUTPUT);
    pinMode(13, OUTPUT);

    xTaskCreate(
        parte_task1,
        "t1",
        128,
        NULL,
        2,
        NULL
    );

    xTaskCreate(
        parte_task2,
        "t2",
        128,
        NULL,
        1,
        NULL
    );
}

void loop() {}


// the setup function runs once when you press reset or power the board
void setup()
{
    // partb_main();
    parte_main();
}

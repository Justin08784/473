#include "Arduino.h"

#define PORT_E      PORTB
#define PORT_RW     PORTD
#define PORT_RS     PORTD
#define PORT_DB7    PORTD
#define PORT_DB6    PORTD
#define PORT_DB5    PORTD
#define PORT_DB4    PORTD

#define DDR_E       DDRB
#define DDR_RW      DDRD
#define DDR_RS      DDRD
#define DDR_DB7     DDRD
#define DDR_DB6     DDRD
#define DDR_DB5     DDRD
#define DDR_DB4     DDRD

// #define PIN_E   8
// #define PIN_RW  7
// #define PIN_RS  6
// #define PIN_DB7 5
// #define PIN_DB6 4
// #define PIN_DB5 3
// #define PIN_DB4 2

// #define PIN_RW      (((uint8_t)1u) << 1)
// #define PIN_RS      (((uint8_t)1u) << 0)
// #define PIN_DB7     (((uint8_t)1u) << 3)
// #define PIN_DB6     (((uint8_t)1u) << 2)
// #define PIN_DB5     (((uint8_t)1u) << 1)
// #define PIN_DB4     (((uint8_t)1u) << 0)

#define PIN_E       0   // ddrb
#define PIN_RW      7   // ddrd rest...
#define PIN_RS      6
#define PIN_DB7     5
#define PIN_DB6     4
#define PIN_DB5     3
#define PIN_DB4     2

#define SET_BIT(r, i)   do {( (r) |= (1u << (i))    );} while(0)
#define CLR_BIT(r, i)   do {( (r) &= ~(1u << (i))   );} while(0)
#define ADD_BIT(r, i, b)do {( (r) |= ((b) << (i))   );} while(0)
// void lcd_set_rwrs (uint8_t d)
// {
//     digitalWrite(PIN_RW, !!(d & PIN_RW));
//     digitalWrite(PIN_RS, !!(d & PIN_RS));
// }
// void lcd_send_data(uint8_t d)
// {
//     digitalWrite(PIN_DB7, !!(d & PIN_DB7));
//     digitalWrite(PIN_DB6, !!(d & PIN_DB6));
//     digitalWrite(PIN_DB5, !!(d & PIN_DB5));
//     digitalWrite(PIN_DB4, !!(d & PIN_DB4));

//     digitalWrite(PIN_E, 1);
//     delay(1);
//     digitalWrite(PIN_E, 0);
// }


void pulse_E()
{
    SET_BIT(PORT_E, PIN_E);
    delayMicroseconds(40);
    CLR_BIT(PORT_E, PIN_E);
    // digitalWrite(8, 0);
}

void lcd_set_nibble(uint8_t nib)
{
    CLR_BIT(PORT_DB7, PIN_DB7);
    CLR_BIT(PORT_DB6, PIN_DB6);
    CLR_BIT(PORT_DB5, PIN_DB5);
    CLR_BIT(PORT_DB4, PIN_DB4);

    ADD_BIT(PORT_DB7, PIN_DB7, !!(nib & 0x8));
    ADD_BIT(PORT_DB6, PIN_DB6, !!(nib & 0x4));
    ADD_BIT(PORT_DB5, PIN_DB5, !!(nib & 0x2));
    ADD_BIT(PORT_DB4, PIN_DB4, !!(nib & 0x1));

}

void lcd_send_insn_wait_time(uint8_t d)
{
    CLR_BIT(PORT_RW, PIN_RW);
    CLR_BIT(PORT_RS, PIN_RS);

    lcd_set_nibble(d >> 4);
    pulse_E();
    delayMicroseconds(40);

    lcd_set_nibble(d);
    pulse_E();
    delay(10);

}

void lcd_send_data_wait_time(uint8_t d)
{
    CLR_BIT(PORT_RW, PIN_RW);
    SET_BIT(PORT_RS, PIN_RS);

    lcd_set_nibble(d >> 4);
    pulse_E();
    delayMicroseconds(40);

    lcd_set_nibble(d);
    pulse_E();
    delay(1);

}

void lcd_send_insn_wait_busy(uint8_t d)
{
    CLR_BIT(PORT_RW, PIN_RW);
    CLR_BIT(PORT_RS, PIN_RS);

    lcd_set_nibble(d >> 4);
    pulse_E();
    delayMicroseconds(40);

    lcd_set_nibble(d);
    pulse_E();
    // delay(1);
    // return;


    // set data bus to input >>
    CLR_BIT(DDR_DB7,    PIN_DB7);
    CLR_BIT(DDR_DB6,    PIN_DB6);
    CLR_BIT(DDR_DB5,    PIN_DB5);
    CLR_BIT(DDR_DB4,    PIN_DB4);
    CLR_BIT(PORT_DB7,   PIN_DB7);
    CLR_BIT(PORT_DB6,   PIN_DB6);
    CLR_BIT(PORT_DB5,   PIN_DB5);
    CLR_BIT(PORT_DB4,   PIN_DB4);

    SET_BIT(PORT_RW,    PIN_RW);
    // <<
    delayMicroseconds(40);

    uint8_t busy = 1;
    while (busy) {
        SET_BIT(PORT_E, PIN_E);
        delayMicroseconds(40);
        busy = PIND & (1u << PIN_DB7);
        CLR_BIT(PORT_E, PIN_E);
        delayMicroseconds(40);

        SET_BIT(PORT_E, PIN_E);
        delayMicroseconds(40);
        CLR_BIT(PORT_E, PIN_E);
        delayMicroseconds(40);

    }

    // why???
    CLR_BIT(PORT_DB7,   PIN_DB7);
    CLR_BIT(PORT_DB6,   PIN_DB6);
    CLR_BIT(PORT_DB5,   PIN_DB5);
    CLR_BIT(PORT_DB4,   PIN_DB4);

    // set data bus to output
    SET_BIT(DDR_DB7,    PIN_DB7);
    SET_BIT(DDR_DB6,    PIN_DB6);
    SET_BIT(DDR_DB5,    PIN_DB5);
    SET_BIT(DDR_DB4,    PIN_DB4);

}

void lcd_read_busy(uint8_t d)
{
    CLR_BIT(PORT_RS, PIN_RS);

    lcd_set_nibble(d >> 4);
    pulse_E();
    delay(1);
    // delayMicroseconds(40);

    lcd_set_nibble(d);
    pulse_E();
    delay(1);
    // delayMicroseconds(40);

}

void init_lcd()
{
    delay(40);
    SET_BIT(DDR_E,      PIN_E);
    SET_BIT(DDR_RW,     PIN_RW);
    SET_BIT(DDR_RS,     PIN_RS);
    SET_BIT(DDR_DB7,    PIN_DB7);
    SET_BIT(DDR_DB6,    PIN_DB6);
    SET_BIT(DDR_DB5,    PIN_DB5);
    SET_BIT(DDR_DB4,    PIN_DB4);

    CLR_BIT(PORT_E,     PIN_E);
    CLR_BIT(PORT_RW,    PIN_RW);
    CLR_BIT(PORT_RS,    PIN_RS);
    CLR_BIT(PORT_DB7,   PIN_DB7);
    CLR_BIT(PORT_DB6,   PIN_DB6);
    CLR_BIT(PORT_DB5,   PIN_DB5);
    CLR_BIT(PORT_DB4,   PIN_DB4);

    // lcd_send_insn(0b1010);
    // digitalWrite(2, 0);
    // digitalWrite(3, 1);
    // digitalWrite(4, 0);
    // digitalWrite(5, 1);


    SET_BIT(PORT_DB5,   PIN_DB5);
    SET_BIT(PORT_DB4,   PIN_DB4);
    pulse_E();
    delay(5);
    pulse_E();
    delay(1);
    pulse_E();
    delay(1);

    CLR_BIT(PORT_DB4,   PIN_DB4);
    pulse_E();
    delay(1);

    lcd_send_insn_wait_time(0b00101000);
    lcd_send_insn_wait_time(0b00001000);
    lcd_send_insn_wait_time(0b00000001);
    lcd_send_insn_wait_time(0b00000110);


    delay(1);

    lcd_send_insn_wait_time(0b00001111);

    char *str = "ARDUINOOO";
    size_t len = strlen(str);
    for (size_t i = 0; i < len; ++i)
        lcd_send_data_wait_time(str[i]);

    // lcd_send_data_wait_time(0b00100000);
    // str = "RULES";
    // len = strlen(str);
    // for (size_t i = 0; i < len; ++i)
    //     lcd_send_data_wait_time(str[i]);

    // lcd_send_data_wait_time(0b01001101);
    return;




    // lcd_send_data(PIN_DB5 | PIN_DB4);

    // lcd_send_data(PIN_DB5 | PIN_DB4);
    // delay(1);

    // lcd_send_data(PIN_DB5 | PIN_DB4);

    // digitalWrite(PIN_RW, 0);

}


int main(int argc, char *argv[])
{
    init();
    // Serial.begin(9600);
    init_lcd();

    return 0;




    delay(1000);
    SET_BIT(DDRD, 5);

    for (int i = 0; i < 100; ++i) {
        if (i % 2 == 0)
            lcd_set_nibble(0b1000);
        else
            lcd_set_nibble(0b0000);
        delayMicroseconds(100);
    }

}


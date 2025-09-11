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

#define INSN_CLEAR_DISPLAY              ((uint8_t) 0x1)
#define INSN_RETURN_HOME                ((uint8_t) 0x2)

#define INSN_ENTRY_MODE_SET             ((uint8_t) 0x4)
#define ENTRY_MODE_SET_I_INCREMENT      ((uint8_t) 0b10)
#define ENTRY_MODE_SET_I_DECREMENT      ((uint8_t) 0)
#define ENTRY_MODE_SET_S_DISPLAY_SHIFT  ((uint8_t) 0b01)
#define ENTRY_MODE_SET_S_CURSOR_MOVE    ((uint8_t) 0)

#define INSN_DISPLAY_ONOFF              ((uint8_t) 0x8)
#define DISPLAY_ONOFF_D_DISPLAY_ON      ((uint8_t) 0b100)
#define DISPLAY_ONOFF_D_DISPLAY_OFF     ((uint8_t) 0)
#define DISPLAY_ONOFF_C_CURSOR_ON       ((uint8_t) 0b010)
#define DISPLAY_ONOFF_C_CURSOR_OFF      ((uint8_t) 0)
#define DISPLAY_ONOFF_B_BLINK_ON        ((uint8_t) 0b001)
#define DISPLAY_ONOFF_B_BLINK_OFF       ((uint8_t) 0)

#define INSN_CD_SHIFT                   ((uint8_t) 0x10)

#define INSN_FUNCTION_SET               ((uint8_t) 0x20)
#define FUNCTION_SET_DL_8BITS           ((uint8_t) 0b10000)
#define FUNCTION_SET_DL_4BITS           ((uint8_t) 0)
#define FUNCTION_SET_N_LINES_2          ((uint8_t) 0b01000)
#define FUNCTION_SET_N_LINES_1          ((uint8_t) 0)
#define FUNCTION_SET_F_DOTS_5x10        ((uint8_t) 0b00100)
#define FUNCTION_SET_F_DOTS_5x8         ((uint8_t) 0)

#define INSN_SET_DDRAM_ADDRESS          ((uint8_t) 0xC0)


struct Lcd_Config {
    uint8_t dcb;        // display onoff config 
    uint8_t func_set;   // function set config

    // pin indices
    uint8_t rs, e, rw;
    uint8_t d4, d5, d6, d7;
        // note: d0-d3 unused (we only support 4-bit mode)
};

struct Lcd_Connection {
    // ddr
    volatile uint8_t *ddr_rs;
    volatile uint8_t *ddr_e;
    volatile uint8_t *ddr_rw;
    volatile uint8_t *ddr_db4;
    volatile uint8_t *ddr_db5;
    volatile uint8_t *ddr_db6;
    volatile uint8_t *ddr_db7;

    // port
    volatile uint8_t *port_rs;
    volatile uint8_t *port_e;
    volatile uint8_t *port_rw;
    volatile uint8_t *port_db4;
    volatile uint8_t *port_db5;
    volatile uint8_t *port_db6;
    volatile uint8_t *port_db7;

    // pin
    uint8_t pin_rs;
    uint8_t pin_e;
    uint8_t pin_rw;
    uint8_t pin_db4;
    uint8_t pin_db5;
    uint8_t pin_db6;
    uint8_t pin_db7;

};

// Return: 0 on success, nonzero on failure. Define multiple failure codes if necessary.
int lcd_init_connection             (const struct Lcd_Config *cfg, struct Lcd_Connection *rv);
    // 1. power-up wait. [2. 4-bit handshake], 3. function set, 4. display off, 5. clear, 6. entry mode set, 7. display control, 8. zeroize DDRAM
int lcd_send_clear_display          (const struct Lcd_Connection *con);
int lcd_send_return_home            (const struct Lcd_Connection *con);
int lcd_send_set_cursor             (const struct Lcd_Connection *con, uint8_t row, uint8_t col);
int lcd_send_display_char           (const struct Lcd_Connection *con, uint8_t c);
int lcd_send_display_span           (const struct Lcd_Connection *con, const uint8_t *src, size_t nbytes);
    // ^ if       c ≤ 7   , display custom glyph slot CGRAM[c]. Slot depends on font size.
    //   else             , display predefined glyph from ROM
int lcd_send_store_glyph_5x8        (const struct Lcd_Connection *con, const uint8_t src[8], uint8_t slot);
    // ^ assert 0 ≤ slot ≤ 7
int lcd_send_store_glyph_5x10       (const struct Lcd_Connection *con, const uint8_t src[10], uint8_t slot);
    // ^ assert 0 ≤ slot ≤ 3

int lcd_send_insn                   (const struct Lcd_Connection *con, const uint8_t insn);
int lcd_send_data                   (const struct Lcd_Connection *con, const uint8_t data);
    // ^ "backdoor"


void pulse_E                        (const struct Lcd_Connection *con);
void lcd_set_nibble                 (const struct Lcd_Connection *con, uint8_t nib);

int lcd_init_connection(const struct Lcd_Config *cfg, struct Lcd_Connection *rv)
{
    if (!cfg) {
        // NULL config. Use defaults

        *rv = {
            .ddr_rs     = &DDR_RS,
            .ddr_e      = &DDR_E,
            .ddr_rw     = &DDR_RW,
            .ddr_db4    = &DDR_DB4,
            .ddr_db5    = &DDR_DB5,
            .ddr_db6    = &DDR_DB6,
            .ddr_db7    = &DDR_DB7,

            .port_rs    = &PORT_RS,
            .port_e     = &PORT_E,
            .port_rw    = &PORT_RW,
            .port_db4   = &PORT_DB4,
            .port_db5   = &PORT_DB5,
            .port_db6   = &PORT_DB6,
            .port_db7   = &PORT_DB7,

            .pin_rs     = PIN_RS,
            .pin_e      = PIN_E,
            .pin_rw     = PIN_RW,
            .pin_db4    = PIN_DB4,
            .pin_db5    = PIN_DB5,
            .pin_db6    = PIN_DB6,
            .pin_db7    = PIN_DB7

        };

    } else {
        /* TODO: general config handling */

    }


    delay(40);
    SET_BIT(*rv->ddr_e,      rv->pin_e);
    SET_BIT(*rv->ddr_rw,     rv->pin_rw);
    SET_BIT(*rv->ddr_rs,     rv->pin_rs);
    SET_BIT(*rv->ddr_db7,    rv->pin_db7);
    SET_BIT(*rv->ddr_db6,    rv->pin_db6);
    SET_BIT(*rv->ddr_db5,    rv->pin_db5);
    SET_BIT(*rv->ddr_db4,    rv->pin_db4);

    CLR_BIT(*rv->port_e,     rv->pin_e);
    CLR_BIT(*rv->port_rw,    rv->pin_rw);
    CLR_BIT(*rv->port_rs,    rv->pin_rs);
    CLR_BIT(*rv->port_db7,   rv->pin_db7);
    CLR_BIT(*rv->port_db6,   rv->pin_db6);
    CLR_BIT(*rv->port_db5,   rv->pin_db5);
    CLR_BIT(*rv->port_db4,   rv->pin_db4);


    SET_BIT(*rv->port_db5,   rv->pin_db5);
    SET_BIT(*rv->port_db4,   rv->pin_db4);
    pulse_E(rv);
    delay(5);
    pulse_E(rv);
    delay(1);
    pulse_E(rv);
    delay(1);

    CLR_BIT(*rv->port_db4,   rv->pin_db4);
    pulse_E(rv);
    delay(1);

    lcd_send_insn(rv, INSN_FUNCTION_SET
                    | FUNCTION_SET_DL_4BITS
                    | FUNCTION_SET_N_LINES_2
                    | FUNCTION_SET_F_DOTS_5x8);
    lcd_send_insn(rv, INSN_DISPLAY_ONOFF);
    lcd_send_insn(rv, INSN_CLEAR_DISPLAY);
    lcd_send_insn(rv, INSN_ENTRY_MODE_SET
                    | ENTRY_MODE_SET_I_INCREMENT
                    | ENTRY_MODE_SET_S_CURSOR_MOVE);
    // lcd_send_insn(rv, 0b00101000);
    // lcd_send_insn(rv, 0b00001000);
    // lcd_send_insn(rv, 0b00000001);
    // lcd_send_insn(rv, 0b00000110);

    lcd_send_insn(rv, INSN_DISPLAY_ONOFF
                    | DISPLAY_ONOFF_D_DISPLAY_ON);
    // lcd_send_insn(rv, 0b00001111);




    // lcd_send_data(0b00100000);
    // str = "RULES";
    // len = strlen(str);
    // for (size_t i = 0; i < len; ++i)
    //     lcd_send_data(str[i]);

    // lcd_send_data(0b01001101);
    return;


}

void pulse_E(const struct Lcd_Connection *con)
{
    SET_BIT(*con->port_e, con->pin_e);
    delayMicroseconds(1);
    CLR_BIT(*con->port_e, con->pin_e);
}

void lcd_set_nibble(const struct Lcd_Connection *con, uint8_t nib)
{
    CLR_BIT(*con->port_db7, con->pin_db7);
    CLR_BIT(*con->port_db6, con->pin_db6);
    CLR_BIT(*con->port_db5, con->pin_db5);
    CLR_BIT(*con->port_db4, con->pin_db4);

    ADD_BIT(*con->port_db7, con->pin_db7, !!(nib & 0x8));
    ADD_BIT(*con->port_db6, con->pin_db6, !!(nib & 0x4));
    ADD_BIT(*con->port_db5, con->pin_db5, !!(nib & 0x2));
    ADD_BIT(*con->port_db4, con->pin_db4, !!(nib & 0x1));

}

int lcd_send_clear_display(const struct Lcd_Connection *con)
{
    return lcd_send_insn(con, INSN_CLEAR_DISPLAY);
}

int lcd_send_return_home(const struct Lcd_Connection *con)
{
    return lcd_send_insn(con, INSN_RETURN_HOME);
}

int lcd_send_set_cursor(const struct Lcd_Connection *con, uint8_t row, uint8_t col)
{
    // TODO
    uint8_t insn =
        INSN_SET_DDRAM_ADDRESS
    |   ((row & 1u)         << 6)
    |   (col & 0b111111);
    return lcd_send_insn(con, insn);
}

int lcd_send_display_char(const struct Lcd_Connection *con, uint8_t c)
{
    return lcd_send_data(con, c);
}

int lcd_send_display_span(const struct Lcd_Connection *con, const uint8_t *src, size_t nbytes)
{
    for (size_t i = 0; i < nbytes; ++i)
        lcd_send_data(con, src[i]);
    return 0;
}

int lcd_send_insn(const struct Lcd_Connection *con, uint8_t d)
{
    CLR_BIT(*con->port_rw, con->pin_rw);
    CLR_BIT(*con->port_rs, con->pin_rs);

    lcd_set_nibble(con, d >> 4);
    pulse_E(con);
    delayMicroseconds(1);

    lcd_set_nibble(con, d);
    pulse_E(con);
    delay(1);

    return 0;
}

int lcd_send_data(const struct Lcd_Connection *con, uint8_t d)
{
    CLR_BIT(*con->port_rw, con->pin_rw);
    SET_BIT(*con->port_rs, con->pin_rs);

    lcd_set_nibble(con, d >> 4);
    pulse_E(con);
    delayMicroseconds(1);

    lcd_set_nibble(con, d);
    pulse_E(con);
    delay(1);

    return 0;
}

// void lcd_send_insn_wait_busy(uint8_t d)
// {
//     CLR_BIT(PORT_RW, PIN_RW);
//     CLR_BIT(PORT_RS, PIN_RS);

//     lcd_set_nibble(d >> 4);
//     pulse_E();
//     delayMicroseconds(40);

//     lcd_set_nibble(d);
//     pulse_E();
//     // delay(1);
//     // return;


//     // set data bus to input >>
//     CLR_BIT(DDR_DB7,    PIN_DB7);
//     CLR_BIT(DDR_DB6,    PIN_DB6);
//     CLR_BIT(DDR_DB5,    PIN_DB5);
//     CLR_BIT(DDR_DB4,    PIN_DB4);
//     CLR_BIT(PORT_DB7,   PIN_DB7);
//     CLR_BIT(PORT_DB6,   PIN_DB6);
//     CLR_BIT(PORT_DB5,   PIN_DB5);
//     CLR_BIT(PORT_DB4,   PIN_DB4);

//     SET_BIT(PORT_RW,    PIN_RW);
//     // <<
//     delayMicroseconds(40);

//     uint8_t busy = 1;
//     while (busy) {
//         SET_BIT(PORT_E, PIN_E);
//         delayMicroseconds(40);
//         busy = PIND & (1u << PIN_DB7);
//         CLR_BIT(PORT_E, PIN_E);
//         delayMicroseconds(40);

//         SET_BIT(PORT_E, PIN_E);
//         delayMicroseconds(40);
//         CLR_BIT(PORT_E, PIN_E);
//         delayMicroseconds(40);

//     }

//     // why???
//     CLR_BIT(PORT_DB7,   PIN_DB7);
//     CLR_BIT(PORT_DB6,   PIN_DB6);
//     CLR_BIT(PORT_DB5,   PIN_DB5);
//     CLR_BIT(PORT_DB4,   PIN_DB4);

//     // set data bus to output
//     SET_BIT(DDR_DB7,    PIN_DB7);
//     SET_BIT(DDR_DB6,    PIN_DB6);
//     SET_BIT(DDR_DB5,    PIN_DB5);
//     SET_BIT(DDR_DB4,    PIN_DB4);

// }


// void init_lcd()
// {
//     delay(40);
//     SET_BIT(DDR_E,      PIN_E);
//     SET_BIT(DDR_RW,     PIN_RW);
//     SET_BIT(DDR_RS,     PIN_RS);
//     SET_BIT(DDR_DB7,    PIN_DB7);
//     SET_BIT(DDR_DB6,    PIN_DB6);
//     SET_BIT(DDR_DB5,    PIN_DB5);
//     SET_BIT(DDR_DB4,    PIN_DB4);

//     CLR_BIT(PORT_E,     PIN_E);
//     CLR_BIT(PORT_RW,    PIN_RW);
//     CLR_BIT(PORT_RS,    PIN_RS);
//     CLR_BIT(PORT_DB7,   PIN_DB7);
//     CLR_BIT(PORT_DB6,   PIN_DB6);
//     CLR_BIT(PORT_DB5,   PIN_DB5);
//     CLR_BIT(PORT_DB4,   PIN_DB4);

//     // lcd_send_insn(0b1010);
//     // digitalWrite(2, 0);
//     // digitalWrite(3, 1);
//     // digitalWrite(4, 0);
//     // digitalWrite(5, 1);


//     SET_BIT(PORT_DB5,   PIN_DB5);
//     SET_BIT(PORT_DB4,   PIN_DB4);
//     pulse_E();
//     delay(5);
//     pulse_E();
//     delay(1);
//     pulse_E();
//     delay(1);

//     CLR_BIT(PORT_DB4,   PIN_DB4);
//     pulse_E();
//     delay(1);

//     lcd_send_insn(0b00101000);
//     lcd_send_insn(0b00001000);
//     lcd_send_insn(0b00000001);
//     lcd_send_insn(0b00000110);


//     delay(1);

//     lcd_send_insn(0b00001111);

//     char *str = "ARDUINOOO";
//     size_t len = strlen(str);
//     for (size_t i = 0; i < len; ++i)
//         lcd_send_data(str[i]);

//     // lcd_send_data(0b00100000);
//     // str = "RULES";
//     // len = strlen(str);
//     // for (size_t i = 0; i < len; ++i)
//     //     lcd_send_data(str[i]);

//     // lcd_send_data(0b01001101);
//     return;




//     // lcd_send_data(PIN_DB5 | PIN_DB4);

//     // lcd_send_data(PIN_DB5 | PIN_DB4);
//     // delay(1);

//     // lcd_send_data(PIN_DB5 | PIN_DB4);

//     // digitalWrite(PIN_RW, 0);

// }


int main(int argc, char *argv[])
{
    init();
    // Serial.begin(9600);
    // init_lcd();

    struct Lcd_Connection con;
    lcd_init_connection(NULL, &con);

    char *str = "papap";
    size_t len = strlen(str);
    lcd_send_set_cursor(&con, 1, 0);
    lcd_send_display_span(&con, str, strlen(str));
    // char *str = "aparaphernelia";
    // size_t len = strlen(str);
    // for (size_t i = 0; i < len; ++i)
    //     lcd_send_data(&con, str[i]);


    return 0;




    delay(1000);
    SET_BIT(DDRD, 5);

    for (int i = 0; i < 100; ++i) {
        if (i % 2 == 0)
            lcd_set_nibble(&con, 0b1000);
        else
            lcd_set_nibble(&con, 0b0000);
        delayMicroseconds(100);
    }

}


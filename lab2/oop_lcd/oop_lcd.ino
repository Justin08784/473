#include "Arduino.h"

#define SET_BIT(r, i)   do {( (r) |= (1u << (i))    );} while(0)
#define CLR_BIT(r, i)   do {( (r) &= ~(1u << (i))   );} while(0)
#define ADD_BIT(r, i, b)do {( (r) |= ((b) << (i))   );} while(0)

#define ABS_PIN_TO_PORT_INDEX(i)        (((i) >> 3) & 1u)   // [0..13] -> {&DDRD, &DDRB}
#define ABS_PIN_TO_PORT_PIN(i)          ((i) & 0b111)       // [0..13] -> [0..7]

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

#define INSN_SET_DDRAM_ADDRESS          ((uint8_t) 0x80)

volatile uint8_t *const DDR_REGS[2] = {&DDRD,   &DDRB};
volatile uint8_t *const PORT_REGS[2]= {&PORTD,  &PORTB};

struct Lcd_Config {
    // absolute pins [0..13]
    uint8_t rs;
    // uint8_t rw;              // we only support writes. rw should be tied to low.
    uint8_t e;
    uint8_t db4, db5, db6, db7; // db0-db3 unused (we only support 4-bit mode)
};

const struct Lcd_Config default_config = {
    .rs = 13,
    // .rw = 13,
    .e  = 12,
    .db4= 11,
    .db5= 10,
    .db6= 9,
    .db7= 8
};

struct Lcd_Connection {
    // ddr pointers
    volatile uint8_t *ddr_rs;
    // volatile uint8_t *ddr_rw;
    volatile uint8_t *ddr_e;
    volatile uint8_t *ddr_db4;
    volatile uint8_t *ddr_db5;
    volatile uint8_t *ddr_db6;
    volatile uint8_t *ddr_db7;

    // port pointers
    volatile uint8_t *port_rs;
    // volatile uint8_t *port_rw;
    volatile uint8_t *port_e;
    volatile uint8_t *port_db4;
    volatile uint8_t *port_db5;
    volatile uint8_t *port_db6;
    volatile uint8_t *port_db7;

    // in-port pins [0..7]
    uint8_t pin_rs;
    // uint8_t pin_rw;
    uint8_t pin_e;
    uint8_t pin_db4;
    uint8_t pin_db5;
    uint8_t pin_db6;
    uint8_t pin_db7;

};

// Return: 0 on success, nonzero on failure. Define multiple failure codes if necessary.
/* high-level helpers */
int lcd_init_connection             (const struct Lcd_Config *cfg, struct Lcd_Connection *rv);
    // 1. power-up wait. [2. 4-bit handshake], 3. function set, 4. display off, 5. clear, 6. entry mode set, 7. display control, 8. zeroize DDRAM
int lcd_send_clear_display          (const struct Lcd_Connection *con);
int lcd_send_return_home            (const struct Lcd_Connection *con);
int lcd_send_set_cursor             (const struct Lcd_Connection *con, uint8_t row, uint8_t col);
int lcd_send_display_char           (const struct Lcd_Connection *con, uint8_t c);
int lcd_send_display_span           (const struct Lcd_Connection *con, const uint8_t *src, size_t nbytes);
/* "backdoor"/low-level helpers */
int lcd_send_insn                   (const struct Lcd_Connection *con, const uint8_t insn);
int lcd_send_data                   (const struct Lcd_Connection *con, const uint8_t data);
    // ^ "backdoor"
static inline void pulse_E          (const struct Lcd_Connection *con);
static inline void lcd_set_nibble   (const struct Lcd_Connection *con, uint8_t nib);


/*
class Lcd: 
- some OOP syntactic sugar (the methods in this class merely forward to the procedural API)
- This class does not use the private keyword; it is up to the user to decide how much of the
internals with which they wish to interact. The helpers are clearly divided into
easier-to-use/"high-level" and "expert"/"backdoor" categories.
*/
class Lcd
{
public:
    Lcd_Connection con;

    /* high-level helpers */
    int init_connection     (const struct Lcd_Config *cfg)
    {
        return ::lcd_init_connection(cfg, &con);
    }

    int send_clear_display  ()
    {
        return ::lcd_send_clear_display(&con);
    }

    int send_return_home    ()
    {
        return ::lcd_send_return_home(&con);
    }

    int send_set_cursor     (uint8_t row, uint8_t col)
    {
        return ::lcd_send_set_cursor(&con, row, col);
    }

    int send_display_char   (uint8_t c)
    {
        return ::lcd_send_display_char(&con, c);
    }

    int send_display_span   (const uint8_t *src, size_t nbytes)
    {
        return ::lcd_send_display_span(&con, src, nbytes);
    }


    /* "backdoor"/low-level helpers */
    int send_insn           (const uint8_t insn)
    {
        return ::lcd_send_insn(&con, insn);
    }

    int send_data           (const uint8_t data)
    {
        return ::lcd_send_data(&con, data);
    }

    void pulse_E            ()
    {
        return ::pulse_E(&con);

    }
    void set_nibble     (uint8_t nib)
    {
        return ::lcd_set_nibble(&con, nib);
    }


};

int lcd_init_connection(const struct Lcd_Config *cfg, struct Lcd_Connection *rv)
{
    if (!cfg) // NULL config. Use defaults
        cfg = &default_config;

    uint8_t pi_rs   = ABS_PIN_TO_PORT_INDEX(cfg->rs );
    uint8_t pi_e    = ABS_PIN_TO_PORT_INDEX(cfg->e  );
    // uint8_t pi_rw   = ABS_PIN_TO_PORT_INDEX(cfg->rw );
    uint8_t pi_db4  = ABS_PIN_TO_PORT_INDEX(cfg->db4);
    uint8_t pi_db5  = ABS_PIN_TO_PORT_INDEX(cfg->db5);
    uint8_t pi_db6  = ABS_PIN_TO_PORT_INDEX(cfg->db6);
    uint8_t pi_db7  = ABS_PIN_TO_PORT_INDEX(cfg->db7);

    *rv = {
        .ddr_rs     = DDR_REGS[pi_rs ],
        // .ddr_rw     = DDR_REGS[pi_rw ],
        .ddr_e      = DDR_REGS[pi_e  ],
        .ddr_db4    = DDR_REGS[pi_db4],
        .ddr_db5    = DDR_REGS[pi_db5],
        .ddr_db6    = DDR_REGS[pi_db6],
        .ddr_db7    = DDR_REGS[pi_db7],

        .port_rs    = PORT_REGS[pi_rs ],
        // .port_rw    = PORT_REGS[pi_rw ],
        .port_e     = PORT_REGS[pi_e  ],
        .port_db4   = PORT_REGS[pi_db4],
        .port_db5   = PORT_REGS[pi_db5],
        .port_db6   = PORT_REGS[pi_db6],
        .port_db7   = PORT_REGS[pi_db7],

        .pin_rs     = ABS_PIN_TO_PORT_PIN(cfg->rs ),
        // .pin_rw     = ABS_PIN_TO_PORT_PIN(cfg->rw ),
        .pin_e      = ABS_PIN_TO_PORT_PIN(cfg->e  ),
        .pin_db4    = ABS_PIN_TO_PORT_PIN(cfg->db4),
        .pin_db5    = ABS_PIN_TO_PORT_PIN(cfg->db5),
        .pin_db6    = ABS_PIN_TO_PORT_PIN(cfg->db6),
        .pin_db7    = ABS_PIN_TO_PORT_PIN(cfg->db7)

    };

    delay(40);
    SET_BIT(*rv->ddr_e,      rv->pin_e);
    // SET_BIT(*rv->ddr_rw,     rv->pin_rw);
    SET_BIT(*rv->ddr_rs,     rv->pin_rs);
    SET_BIT(*rv->ddr_db7,    rv->pin_db7);
    SET_BIT(*rv->ddr_db6,    rv->pin_db6);
    SET_BIT(*rv->ddr_db5,    rv->pin_db5);
    SET_BIT(*rv->ddr_db4,    rv->pin_db4);

    CLR_BIT(*rv->port_e,     rv->pin_e);
    // CLR_BIT(*rv->port_rw,    rv->pin_rw);
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


    return 0;
}

static inline void pulse_E(const struct Lcd_Connection *con)
{
    SET_BIT(*con->port_e, con->pin_e);
    delayMicroseconds(1);
    CLR_BIT(*con->port_e, con->pin_e);
}

static inline void lcd_set_nibble(const struct Lcd_Connection *con, uint8_t nib)
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
    uint8_t insn =
        INSN_SET_DDRAM_ADDRESS
    |   ((row & 1u) << 6)
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
    // CLR_BIT(*con->port_rw, con->pin_rw);
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
    // CLR_BIT(*con->port_rw, con->pin_rw);
    SET_BIT(*con->port_rs, con->pin_rs);

    lcd_set_nibble(con, d >> 4);
    pulse_E(con);
    delayMicroseconds(1);

    lcd_set_nibble(con, d);
    pulse_E(con);
    delay(1);

    return 0;
}


int demo_g2()
{
    struct Lcd_Connection con;
    lcd_init_connection(NULL, &con);

    char *line1 = "ARDUINO RULES!";
    size_t line1_len = strlen(line1);

    char *line2_variants[] = {
        "Roomba!  ", // spaces to equalize length (and zero out last 2 chars of "BLE[...]"
        "BLE 4ever"
    };
    size_t line2_len = strlen(line2_variants[0]);
    if (line2_len != strlen(line2_variants[1]))
        return 1;

    size_t cur = 0;
    lcd_send_display_span(&con, line1, line1_len);
    while (1) {
        lcd_send_set_cursor(&con, 1, 0);
        lcd_send_display_span(&con, line2_variants[cur], line2_len);
        delay(2000);
        cur = !cur;

    }

}


int main(int argc, char *argv[])
{
    init();
    demo_g2();
    return 0;

}


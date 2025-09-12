#include "Arduino.h"

#define SET_BIT(r, i)   do {( (r) |= (1u << (i))    );} while(0)
#define CLR_BIT(r, i)   do {( (r) &= ~(1u << (i))   );} while(0)
#define ADD_BIT(r, i, b)do {( (r) |= ((b) << (i))   );} while(0)

#define PIN_TO_PORT_INDEX(i)            (((i) >> 3) & 1u)
#define PIN_TO_PORT_BIT(i)              ((i) & 0b111)

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
    // uint8_t dcb;        // display onoff config 
    // uint8_t func_set;   // function set config

    // pin indices
    uint8_t rs, rw, e;
    uint8_t db4, db5, db6, db7;
        // note: db0-db3 unused (we only support 4-bit mode)
};

const struct Lcd_Config default_config = {
    .rs = 13,
    .rw = 13,
    .e  = 12,
    .db4= 11,
    .db5= 10,
    .db6= 9,
    .db7= 8
};

struct Lcd_Connection {
    // ddr
    volatile uint8_t *ddr_rs;
    volatile uint8_t *ddr_rw;
    volatile uint8_t *ddr_e;
    volatile uint8_t *ddr_db4;
    volatile uint8_t *ddr_db5;
    volatile uint8_t *ddr_db6;
    volatile uint8_t *ddr_db7;

    // port
    volatile uint8_t *port_rs;
    volatile uint8_t *port_rw;
    volatile uint8_t *port_e;
    volatile uint8_t *port_db4;
    volatile uint8_t *port_db5;
    volatile uint8_t *port_db6;
    volatile uint8_t *port_db7;

    // pin
    uint8_t pin_rs;
    uint8_t pin_rw;
    uint8_t pin_e;
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
// int lcd_send_store_glyph_5x8        (const struct Lcd_Connection *con, const uint8_t src[8], uint8_t slot);
    // ^ assert 0 ≤ slot ≤ 7
// int lcd_send_store_glyph_5x10       (const struct Lcd_Connection *con, const uint8_t src[10], uint8_t slot);
    // ^ assert 0 ≤ slot ≤ 3

int lcd_send_insn                   (const struct Lcd_Connection *con, const uint8_t insn);
int lcd_send_data                   (const struct Lcd_Connection *con, const uint8_t data);
    // ^ "backdoor"


static inline void pulse_E                        (const struct Lcd_Connection *con);
static inline void lcd_set_nibble                 (const struct Lcd_Connection *con, uint8_t nib);

int lcd_init_connection(const struct Lcd_Config *cfg, struct Lcd_Connection *rv)
{
    if (!cfg) // NULL config. Use defaults
        cfg = &default_config;

    uint8_t pi_rs   = PIN_TO_PORT_INDEX(cfg->rs );
    uint8_t pi_e    = PIN_TO_PORT_INDEX(cfg->e  );
    uint8_t pi_rw   = PIN_TO_PORT_INDEX(cfg->rw );
    uint8_t pi_db4  = PIN_TO_PORT_INDEX(cfg->db4);
    uint8_t pi_db5  = PIN_TO_PORT_INDEX(cfg->db5);
    uint8_t pi_db6  = PIN_TO_PORT_INDEX(cfg->db6);
    uint8_t pi_db7  = PIN_TO_PORT_INDEX(cfg->db7);

    *rv = {
        .ddr_rs     = DDR_REGS[pi_rs ],
        .ddr_rw     = DDR_REGS[pi_rw ],
        .ddr_e      = DDR_REGS[pi_e  ],
        .ddr_db4    = DDR_REGS[pi_db4],
        .ddr_db5    = DDR_REGS[pi_db5],
        .ddr_db6    = DDR_REGS[pi_db6],
        .ddr_db7    = DDR_REGS[pi_db7],

        .port_rs    = PORT_REGS[pi_rs ],
        .port_rw    = PORT_REGS[pi_rw ],
        .port_e     = PORT_REGS[pi_e  ],
        .port_db4   = PORT_REGS[pi_db4],
        .port_db5   = PORT_REGS[pi_db5],
        .port_db6   = PORT_REGS[pi_db6],
        .port_db7   = PORT_REGS[pi_db7],

        .pin_rs     = PIN_TO_PORT_BIT(cfg->rs ),
        .pin_rw     = PIN_TO_PORT_BIT(cfg->rw ),
        .pin_e      = PIN_TO_PORT_BIT(cfg->e  ),
        .pin_db4    = PIN_TO_PORT_BIT(cfg->db4),
        .pin_db5    = PIN_TO_PORT_BIT(cfg->db5),
        .pin_db6    = PIN_TO_PORT_BIT(cfg->db6),
        .pin_db7    = PIN_TO_PORT_BIT(cfg->db7)

    };
    // Serial.println(">>");
    // Serial.println((int) rv->ddr_rs  ); delay(1);
    // Serial.println((int) rv->ddr_rw  ); delay(1);
    // Serial.println((int) rv->ddr_e   ); delay(1);
    // Serial.println((int) rv->ddr_db4 ); delay(1);
    // Serial.println((int) rv->ddr_db5 ); delay(1);
    // Serial.println((int) rv->ddr_db6 ); delay(1);
    // Serial.println((int) rv->ddr_db7 ); delay(1);

    // Serial.println("0!"); delay(1);

    // Serial.println((int) rv->port_rs ); delay(1);
    // Serial.println((int) rv->port_rw ); delay(1);
    // Serial.println((int) rv->port_e  ); delay(1);
    // Serial.println((int) rv->port_db4); delay(1);
    // Serial.println((int) rv->port_db5); delay(1);
    // Serial.println((int) rv->port_db6); delay(1);
    // Serial.println((int) rv->port_db7); delay(1);

    // Serial.println("1!");

    // Serial.println(rv->pin_rs  ); delay(1);
    // Serial.println(rv->pin_rw  ); delay(1);
    // Serial.println(rv->pin_e   ); delay(1);
    // Serial.println(rv->pin_db4 ); delay(1);
    // Serial.println(rv->pin_db5 ); delay(1);
    // Serial.println(rv->pin_db6 ); delay(1);
    // Serial.println(rv->pin_db7 ); delay(1);

    // Serial.println("<<");


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

/**
 * @brief Control commands available for the robot.
 */

enum Action_Code {
    FWD         = 0,
    REV         = 1,
    FWD_LEFT    = 2,
    FWD_RIGHT   = 3,
    REV_LEFT    = 4,
    REV_RIGHT   = 5,
    ROTL        = 6,
    ROTR        = 7,

    SPEED_INC   = 8,
    SPEED_DEC   = 9,
    SPEED_MAX   = 10,

    INSERT      = 11,
    ESCAPE      = 12,
    STOP        = 13
};

void set_motor_left (bool fwd, size_t speed);
void set_motor_right(bool fwd, size_t speed);

/**
 * @brief Start and end delimiters
 * @details Ideally these would be high ASCII characters and MUST
 * be something that doesn't occur inside of a legal packet!
 */
const char SoP = 'C';
const char EoP = 'E';

/**
 * @brief Other constants and variables for communication
 */
const char nullTerminator = '\0';
unsigned char inByte;
#define MESSAGE_MAX_SIZE 5
char message[MESSAGE_MAX_SIZE];
char command;

bool    insert_mode = 0;
bool    fwd_left = 1;
bool    fwd_right= 1;
size_t  speed_left= 150;
size_t  speed_right= 150;
size_t speed_index = 3;
const size_t speeds[4] = {25, 50, 100, 150};


/**
 * @brief Definitions of different speed levels.
 * @details Here we define idle as 0 and full speed as 150. For now,
 * we don't have intermediate speed levels in the program. But you
 * are welcome to add them and change the code.
 */
const int IDLE  = 0;
const int SPEED = 150;
// const int SPEED = 50;

/**
 * @brief H bridge (SN754410) pin connections to Arduino.
 * @details The main ideas is to drive two enable signals with PWMs to
 * control the speed. Connect the 4 logic inputs to any 4 Arduino
 * digital pins. We can also connect enables to vcc to save pins, but
 * then we need 2 or 4 pwm pins. These methods are all feasible, but you
 * need to adjust the setup and the function 'motorControl' accordingly.
 */
/// pwm pin, control left motor
const int EN1 = 2;
/// pwm pin, control right motor
const int EN2 = 3;
/// control Y1 (left motor positive)
const int A_1 = 4;
/// control Y2 (left motor negative)
const int A_2 = 5;
/// control Y3 (right motor positive)
const int A_3 = 6;
/// control Y4 (right motor negative)
const int A_4 = 7;
#define LEFT_MOTOR  true
#define RIGHT_MOTOR false

/**
 * @brief Packet parser for serial communication
 * @details Called in the main loop to get legal message from serial port.
 * @return true on success, false on failure
 */
bool parsePacket();

/**
 * @brief Control the robot
 * @param command FORWARD, LEFT, BACKWARD or STOP.
 */
void moveRobot(enum Action_Code action);

/**
 * @brief Control motor
 * @details Called by `moveRobot` to break down high level command
 * to each motor.
 *
 * @param ifLeftMotor Either Left motor or right motor.
 * @param command FORWARD, STOP or BACKWARD
 */
void motorControl(bool ifLeftMotor, char command);

struct Lcd_Connection con;

/**
 * @brief Pin setup and initialize state
 * @details Enable serial communication, set up H bridge connections,
 * and make robot stop at the beginning.
 */
void setup() {
    Serial.begin(9600);
    lcd_init_connection(NULL, &con);
    lcd_send_display_span(&con, "fuck", 4);
    Serial.println("START");
    pinMode(EN1, OUTPUT);
    pinMode(EN2, OUTPUT);
    pinMode(A_1, OUTPUT);
    pinMode(A_2, OUTPUT);
    pinMode(A_3, OUTPUT);
    pinMode(A_4, OUTPUT);

    moveRobot(STOP);
    delay(10);
    Serial.println("Ready, Steady, Go");
    delay(10);
}

/**
 * @brief Main loop of the program
 * @details In this function, we get messages from serial port and
 * execute them accordingly.
 */
// u32 time_since_last_control = 0;
// u32 cur_time = 0;
void loop() {
    /// 1. get legal message
    // cur_time = millis();
    // Serial.print("times: ");
    // Serial.print(time_since_last_control);
    // Serial.print(" ");
    // Serial.println(cur_time);
    if (!parsePacket()) {
        // if ((cur_time - time_since_last_control) >= 10) {
        //     speed_index = 0;
        //     speed_left = 0;
        //     speed_right= 0;
        //     set_motor_left  (fwd_left,  speed_left);
        //     set_motor_right (fwd_right, speed_right);
        // }

        return;

    }

    // time_since_last_control = cur_time;
    

    /// 2. action, for now we only use option 1
    if (message[0] == '1') {

        command = message[1];
        enum Action_Code action;
        switch (command) {
        case 'F':
            action = FWD;
            break;
        case 'B':
            action = REV;
            break;
        case 'L':
            action = FWD_LEFT;
            break;
        case 'R':
            action = FWD_RIGHT;
            break;
        case 'l':
            action = REV_LEFT;
            break;
        case 'r':
            action = REV_RIGHT;
            break;
        case 'Z':
            action = ROTL;
            break;
        case 'X':
            action = ROTR;
            break;
        case 'S':
            action = STOP;
            break;
        case ',':
            action = SPEED_DEC;
            break;
        case '.':
            action = SPEED_INC;
            break;
        case ' ':
            action = SPEED_MAX;
            break;

        case 'I':
            action = INSERT;
            lcd_send_set_cursor(&con, 0, 0);
            break;

        default:
            action = STOP;
            break;

        }

        // Move command
        // command = message[1];
        moveRobot(action);
    }
    else if (message[0] == '2') {
        // Display Read
        // ...
        command = message[1];
        enum Action_Code action;
        switch (command) {
            case 0:
                Serial.println("stonks");
                action = ESCAPE;
                insert_mode = 0;
                break;
            default:
                lcd_send_display_char(&con, command);
                break;
        }
    }
    else if (message[0] == '3') {
        // Distance Read
        // ...
    }
    else if (message[0] == '4') {
        // Display Write
        Serial.println(message);
        return;
    }
    else {
        Serial.println("ERROR: unknown message");
        return;
    }
}


/**
 * @brief FUNCTION IMPLEMENTATIONS BELOW
 */


bool parsePacket() {
    /// step 1. get SoP
    while (Serial.available() < 1) {};
    inByte = Serial.read();
    if (inByte != SoP) {
        Serial.print("ERROR: Expected SOP, got: ");
        Serial.write((byte)inByte);
        Serial.print("\n");
        return false;
    }

    /// step 2. get message length
    while (Serial.available() < 1) {};
    inByte = Serial.read();
    if (inByte == EoP || inByte == SoP) {
        Serial.println("ERROR: SoP/EoP in length field");
        return false;
    }
    int message_size = inByte - '0';
    if (message_size > MESSAGE_MAX_SIZE || message_size < 0) {
        Serial.println("ERROR: Packet Length out of range");
        return false;
    }

    /// step 3. get message
    for (int i = 0; i < message_size; i++) {
        while (Serial.available() < 1) {};
        inByte = Serial.read();
        if ((inByte == EoP || inByte == SoP)) {
            Serial.println("ERROR: SoP/EoP in command field");
            return false;
        }
        message[i] = (char)inByte;
    }
    message[message_size] = nullTerminator;

    /// step 4. get EoP
    while (Serial.available() < 1) {};
    inByte = Serial.read();
    if (inByte != EoP) {
        Serial.println("EoP not found");
        return false;
    } else {
        return true;
    }
}

void moveRobot(enum Action_Code action) {
    Serial.print("action: ");
    Serial.println(action);
    switch(action) {
    case FWD      :
        fwd_left = 1;
        fwd_right= 1;
        speed_left = speeds[speed_index];
        speed_right= speeds[speed_index];
        break;
    case REV      :
        fwd_left = 0;
        fwd_right= 0;
        speed_left = speeds[speed_index];
        speed_right= speeds[speed_index];
        break;

    case FWD_LEFT :
        fwd_left = 1;
        fwd_right= 1;
        speed_left = 0;
        speed_right= speeds[speed_index];
        break;
    case FWD_RIGHT:
        fwd_left = 1;
        fwd_right= 1;
        speed_left = speeds[speed_index];
        speed_right= 0;
        break;

    case REV_LEFT :
        fwd_left = 0;
        fwd_right= 0;
        speed_left = 0;
        speed_right= speeds[speed_index];
        break;
    case REV_RIGHT:
        fwd_left = 0;
        fwd_right= 0;
        speed_left = speeds[speed_index];
        speed_right= 0;
        break;

    case ROTL     :
        fwd_left = 0;
        fwd_right= 1;
        speed_left = speeds[speed_index];
        speed_right= speeds[speed_index];
        break;
    case ROTR     :
        fwd_left = 1;
        fwd_right= 0;
        speed_left = speeds[speed_index];
        speed_right= speeds[speed_index];
        break;

    case SPEED_INC:
        speed_index = speed_index == 3 ? 3 : speed_index + 1;
        speed_left = speed_left     ? speeds[speed_index] : 0;
        speed_right= speed_right    ? speeds[speed_index] : 0;
        break;
    case SPEED_DEC:
        speed_index = speed_index == 0 ? 0 : speed_index - 1;
        speed_left = speed_left     ? speeds[speed_index] : 0;
        speed_right= speed_right    ? speeds[speed_index] : 0;
        break;
    case SPEED_MAX:
        speed_left = speeds[3];
        speed_right= speeds[3];
        break;

    case INSERT   :
        insert_mode = 1;
    case STOP     :
        speed_left = 0;
        speed_right= 0;
        break;
    case ESCAPE   :
        insert_mode = 0;
        break;


        // case FORWARD:
        //     Serial.println("FORWARD");
        //     motorControl(LEFT_MOTOR, FORWARD);
        //     motorControl(RIGHT_MOTOR, FORWARD);
        //     break;
        // case LEFT:
        //     Serial.println("LEFT");
        //     motorControl(LEFT_MOTOR, STOP);
        //     motorControl(RIGHT_MOTOR, FORWARD);
        //     break;
        // case BACKWARD:
        //     Serial.println("BACKWARD");
        //     motorControl(LEFT_MOTOR, BACKWARD);
        //     motorControl(RIGHT_MOTOR, BACKWARD);
        //     break;
        // case RIGHT:
        //     Serial.println("RIGHT");
        //     motorControl(LEFT_MOTOR, FORWARD);
        //     motorControl(RIGHT_MOTOR, STOP);
        //     break;
        // case STOP:
        //     Serial.println("STOP");
        //     motorControl(LEFT_MOTOR,STOP);
        //     motorControl(RIGHT_MOTOR,STOP);
        //     break;

        // case S_IDLE:
        //     Serial.println("IDLE");
        //     motorControl(LEFT_MOTOR,    STOP);
        //     motorControl(RIGHT_MOTOR,   STOP);
        //     break;

        default:
            Serial.println("ERROR: Unknown command in legal packet");
            break;
    }

    set_motor_left  (fwd_left, speed_left);
    set_motor_right (fwd_right, speed_right);
}

/**
 * @brief Please rewrite the function if you change H bridge connections.
 */

void set_motor_left(bool fwd, size_t speed)
{
    analogWrite (EN1, speed);
    digitalWrite(A_1, fwd);
    digitalWrite(A_2, !fwd);
}

void set_motor_right(bool fwd, size_t speed)
{
    analogWrite (EN2, speed);
    digitalWrite(A_3, fwd);
    digitalWrite(A_4, !fwd);
}

// void motorControl(bool ifLeftMotor, char command) {
//     int enable   = ifLeftMotor ? EN1 : EN2;
//     int motorPos = ifLeftMotor ? A_1 : A_3;
//     int motorNeg = ifLeftMotor ? A_2 : A_4;
//     switch (command) {
//         case FORWARD:
//             analogWrite(enable, SPEED);
//             digitalWrite(motorPos, HIGH);
//             digitalWrite(motorNeg, LOW);
//             break;
//         case BACKWARD:
//             analogWrite(enable, SPEED);
//             digitalWrite(motorPos, LOW);
//             digitalWrite(motorNeg, HIGH);
//             break;
//         case STOP:
//             digitalWrite(motorPos, LOW);
//             digitalWrite(motorNeg, LOW);
//             break;
//         default:
//             break;
//     }
// }

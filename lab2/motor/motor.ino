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
const int EN1 = 3;
/// pwm pin, control right motor
const int EN2 = 5;
/// control Y1 (left motor positive)
const int A_1 = 7;
/// control Y2 (left motor negative)
const int A_2 = 8;
/// control Y3 (right motor positive)
const int A_3 = 9;
/// control Y4 (right motor negative)
const int A_4 = 13;
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
void moveRobot(char command);

/**
 * @brief Control motor
 * @details Called by `moveRobot` to break down high level command
 * to each motor.
 *
 * @param ifLeftMotor Either Left motor or right motor.
 * @param command FORWARD, STOP or BACKWARD
 */
void motorControl(bool ifLeftMotor, char command);

/**
 * @brief Pin setup and initialize state
 * @details Enable serial communication, set up H bridge connections,
 * and make robot stop at the beginning.
 */
void setup() {
    Serial.begin(9600);
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
            break;
        case 'e':
            action = ESCAPE;
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

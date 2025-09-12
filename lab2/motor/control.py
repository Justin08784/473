# import msvcrt
# import serial
# ser = serial.Serial('COM38', 9600)
# while (True):
#     # Poll keyboard
#     if msvcrt.kbhit():
#         key = msvcrt.getch()
#         if key == b'f':
#             ser.write(str.encode('C21FE'))
#         elif key == b's':
#             ser.write(str.encode('C21SE'))


from pynput import keyboard
import serial
import time

PORT = "/dev/tty.usbserial-A5XK3RJT"   # ← change this to your device
BAUD = 9600

insert_mode = False

def key_to_str(key):
    if isinstance(key, keyboard.KeyCode) and key.char:
        return key.char.lower()
    else:
        return str(key)

def send(s):
    print("sending: " + str(s));
    ser.write(s.encode())

last_speed_change_time      = 0.0
last_speed_change_fwd       = True
def on_press(key):
    global last_speed_change_time
    global last_speed_change_fwd
    global insert_mode

    k = key_to_str(key)
    print("detected " + k)
    if k not in pressed:
        pressed.add(k)

    if (insert_mode):
        if  'Key.esc' in pressed:
            send("C22\0E")
            insert_mode = False
            return
        elif 'Key.space' in pressed:
            send("C22 E")
            return
        else:
            send("C22" + str(k) + "E")
            return
    # print("pressed: " + str(key))

    if      {'w', 'a'} <= pressed:
        send("C21LE")
    elif    {'w', 'd'} <= pressed:
        send("C21RE")
    elif    {'s', 'a'} <= pressed:
        send("C21rE")
    elif    {'s', 'd'} <= pressed:
        send("C21lE")


    elif    'a' in pressed:
        send("C21ZE") # rot left
    elif    'd' in pressed:
        send("C21XE") # rot right


    elif    'i' in pressed:
        send("C21IE")
        insert_mode = True


    elif    'w' in pressed:
        send("C21FE")
    elif    's' in pressed:
        send("C21BE")
    elif    'q' in pressed:
        send("C21SE")
    elif    'Key.space' in pressed:
        send("C21 E");
    # elif    'a' <= pressed:
    #     send("BACK LEFT")
    # elif    'd' <= pressed:
    #     send("BACK LEFT")
    # else:
    #     send("C21" + k + "S");

    if      ',' in pressed:
        cur_time = time.time()
        if ((cur_time - last_speed_change_time) >= 0.1) or (last_speed_change_fwd):
            send("C21,E");
            last_speed_change_time = cur_time
            last_speed_change_fwd  = False
    elif    '.' in pressed:
        cur_time = time.time()
        if ((cur_time - last_speed_change_time) >= 0.1) or (not last_speed_change_fwd):
            send("C21.E");
            last_speed_change_time = cur_time
            last_speed_change_fwd  = True

def on_release(key):
    k = key_to_str(key)
    if (k in pressed):
        pressed.remove(k)
    # print("released: " + str(key))

    # Press Esc to exit cleanly
    # if key == keyboard.Key.esc:
    #     # ser.close()
    #     return False

if __name__ == "__main__":
    ser = serial.Serial(PORT, BAUD, timeout=0)
    pressed = set()

    with keyboard.Listener(on_press=on_press, on_release=on_release) as listener:
        listener.join()

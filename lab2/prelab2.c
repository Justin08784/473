#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#define DISPLAY_ONOFF_DISPLAY_ON    (((uint8_t)1u) << 2)    // D
#define DISPLAY_ONOFF_CURSOR_ON     (((uint8_t)1u) << 1)    // C
#define DISPLAY_ONOFF_BLINK_ON      (((uint8_t)1u) << 0)    // B

#define FUNCTION_SET_DL_8BIT        (((uint8_t)1u) << 4)    // 1 = 8-bit,       0 = 4-bit mode
#define FUNCTION_SET_N_DOUBLE       (((uint8_t)1u) << 3)    // 1 = 2 lines,     0 = 1 line
#define FUNCTION_SET_FONT_5x10      (((uint8_t)1u) << 2)    // 1 = 5x10 dots,   0 = 5x8 dots

struct Lcd_Tx_Cfg {
    uint8_t dcb;        // display onoff config 
    uint8_t func_set;   // function set config

    // pin indices
    uint8_t rs, e, rw;
    uint8_t d0, d1, d2, d3, d4, d5, d6, d7;
        // note: d0-d3 used iff DL_8BIT set
};

// Return: 0 on success, nonzero on failure. Define multiple failure codes if necessary.
int lcd_init                        (const struct Lcd_Tx_Cfg *cfg);
    // 1. power-up wait. [2. 4-bit handshake], 3. function set, 4. display off, 5. clear, 6. entry mode set, 7. display control, 8. zeroize DDRAM
int lcd_send_clear_display          (const struct Lcd_Tx_Cfg *cfg);
int lcd_send_return_home            (const struct Lcd_Tx_Cfg *cfg);
int lcd_send_set_cursor             (const struct Lcd_Tx_Cfg *cfg, uint8_t row, uint8_t col);
int lcd_send_display_char           (const struct Lcd_Tx_Cfg *cfg, uint8_t c);
int lcd_send_display_span           (const struct Lcd_Tx_Cfg *cfg, const uint8_t *src, size_t nbytes);
    // ^ if       c ≤ 7   , display custom glyph slot CGRAM[c]. Slot depends on font size.
    //   else             , display predefined glyph from ROM
int lcd_send_store_glyph_5x8        (const struct Lcd_Tx_Cfg *cfg, const uint8_t src[8], uint8_t slot);
    // ^ assert 0 ≤ slot ≤ 7
int lcd_send_store_glyph_5x10       (const struct Lcd_Tx_Cfg *cfg, const uint8_t src[10], uint8_t slot);
    // ^ assert 0 ≤ slot ≤ 3

int lcd_send_command                (const struct Lcd_Tx_Cfg *cfg, const uint8_t cmd);
int lcd_send_data                   (const struct Lcd_Tx_Cfg *cfg, const uint8_t data);
    // ^ "backdoor"

int main (int argc, char *argv[])
{

}

// int ring_buf_write(Lcd_Cmd_Ring *dst, uint8_t *src, size_t nbytes);
// int ring_buf_read(Lcd_Cmd_Ring *src, size_t nbytes);

// struct Lcd_Cmd_Ring {
//     size_t              cap;
//     volatile    size_t  used;
//     volatile    size_t  head;
//     volatile    size_t  tail;
//     uint8_t             *rs_rw; // packed 2-bit [RS, R/W] tuples
//     uint8_t             *data;  // 8-bit payloads
// };

// int lcd_cmd_ring_init               (struct Lcd_Cmd_Ring **rv, size_t cap); // assert(cap is power of 2);
// int lcd_cmd_ring_free               (struct Lcd_Cmd_Ring *ring);
// int lcd_cmd_ring_enqueue            (struct Lcd_Cmd_Ring *dst, const uint8_t *src_rs_rw, const uint8_t *src_data, size_t ninsns);
// int lcd_cmd_ring_dequeue            (struct Lcd_Cmd_Ring *src, size_t ninsns);

// struct Lcd_Cmd_Ring;

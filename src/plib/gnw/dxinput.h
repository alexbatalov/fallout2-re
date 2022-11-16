#ifndef FALLOUT_PLIB_GNW_DXINPUT_H_
#define FALLOUT_PLIB_GNW_DXINPUT_H_

#include "win32.h"

typedef struct dxinput_mouse_state {
    int delta_x;
    int delta_y;
    unsigned char left_button;
    unsigned char right_button;
} dxinput_mouse_state;

typedef struct dxinput_key_data {
    unsigned char code;
    unsigned char state;
} dxinput_key_data;

bool dxinput_init();
void dxinput_exit();
bool dxinput_acquire_mouse();
bool dxinput_unacquire_mouse();
bool dxinput_get_mouse_state(dxinput_mouse_state* mouse_state);
bool dxinput_acquire_keyboard();
bool dxinput_unacquire_keyboard();
bool dxinput_flush_keyboard_buffer();
bool dxinput_read_keyboard_buffer(dxinput_key_data* key_data);

#endif /* FALLOUT_PLIB_GNW_DXINPUT_H_ */

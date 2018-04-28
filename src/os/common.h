#include "math_m.h"

// Structs
struct Keyboard { //@Cleanup, should probably break up Keyboard and Mouse
    bool key_left  = false;
    bool key_right = false;
    bool key_up    = false;
    bool key_down  = false;

    bool key_shift = false;
    bool key_space = false;

    bool key_F1 = false;
    bool key_F2 = false;
    bool key_F3 = false;

    bool key_ESC = false;

    // Mouse input
    Vector2f mouse_position;

    bool mouse_left = false;
    bool mouse_right = false;

    Vector2f mouse_left_pressed_position;
    Vector2f mouse_right_pressed_position;
};

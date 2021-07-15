
#ifndef _HOST_KEY_H
#define _HOST_KEY_H

// Keyboard Keys
typedef enum MapleKey
{
    Key_Alt,
    Key_Shift,
    Key_Ctrl,
    Key_Escape,
    Key_Return,
    Key_Space,
    
    // Arrow keys
    Key_Left,
    Key_Up,
    Key_Right,
    Key_Down,
    
    // Number keys
    Key_Zero,
    Key_One,
    Key_Two,
    Key_Three,
    Key_Four,
    Key_Five,
    Key_Size,
    Key_Seven,
    Key_Eight,
    Key_Nine,
    
    Key_A,
    Key_B,
    Key_C,
    Key_D,
    Key_E,
    Key_F,
    Key_G,
    Key_H,
    Key_I,
    Key_J,
    Key_K,
    Key_L,
    Key_M,
    Key_N,
    Key_O,
    Key_P,
    Key_Q,
    Key_R,
    Key_S,
    Key_T,
    Key_U,
    Key_V,
    Key_W,
    Key_X,
    Key_Y,
    Key_Z,
    
    // TODO(Dustin): FN keys
    
    // TODO(Dustin): Keypad keys
    
    Key_Count,
    Key_Unknown = Key_Count,
} MapleKey;

#endif //_HOST_KEY_H

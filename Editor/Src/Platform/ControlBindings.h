
#ifndef _CONTROL_BINDINGS_H
#define _CONTROL_BINDINGS_H

/*

Specifies the bindings of importance for an
application.

To add bindings, a user should add to the Control
enum. For example, if I wanted to have a binding for
crouch, I would add a field called "Control_Crouch"
 to the enum "Control". Then add the key mapping
in the control_bindings array. Let's say I wanted
crouch to be "Control", I would add "MapleKey::Ctrl"
to the control_bindings array. Please note the index
you add the binding to is VERY important. Unfortunately,
C++ does not support out-of-order array initializations.

The array "control_states" will maintain the state of
your bindings and can be updated using UpdateControlBindings(host_wnd_t wnd);

Example Usage: Let's say Control_CloseApp was a control
binding mapped to MapleKey::Escape

// Update the state of the current bindings
        UpdateControlBindings(host_window);
        
// If the Escape key was pressed, quit the application
if (control_states[Control_CloseApp].pressed)
        {
            g_app_is_running = false;
        }

*/

typedef enum Control
{
    // Engine Controls
    Control_RenderWireframe,
    
    // Application controls
    Control_CloseApp,

    // Game controls
    Control_MoveLeft,
    Control_MoveRight,
    Control_MoveForward,
    Control_MoveBack,
    Control_LookUp,
    Control_LookDown,
    Control_LookLeft,
    Control_LookRight,
    Control_Jump,
    
    Control_Count,
    Control_Unknown = Control_Count,
} Control;

// Application bindings
file_global const MapleKey g_control_bindings[Control_Count] = 
{
    [Control_RenderWireframe] = Key_One,
    [Control_CloseApp]     = Key_Escape, 
    [Control_MoveLeft]     = Key_A,      
    [Control_MoveRight]    = Key_D, 
    [Control_MoveForward]  = Key_W,
    [Control_MoveBack]     = Key_S,
    [Control_LookUp]       = Key_Up,
    [Control_LookDown]     = Key_Down,
    [Control_LookLeft]     = Key_Left,
    [Control_LookRight]    = Key_Right,
    [Control_Jump]         = Key_Space,  
};

struct ControlState
{
    u8 pressed:1;
    u8 down:1;
    u8 released:1;
    u8 pad0:5;
} file_global g_control_states[Control_Count];

// Defined X11Main.c
static void update_ctrl_bindings(struct HostWnd *wnd);

#endif //_CONTROL_BINDINGS_H

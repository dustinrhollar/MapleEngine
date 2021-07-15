#ifndef _INPUT_LAYER_H
#define _INPUT_LAYER_H

namespace input_layer
{
    enum EventType
    {
        Event_KeyPressed,
        Event_KeyRelease,
        Event_ButtonPress,
        Event_ButtonRelease,
        Event_MouseMove,
        Event_MouseWheel,
        Event_MouseHWheel,
        Event_KillFocus,
        Event_Char,
        Event_SetCursor,
        Event_DeviceChange,
        Event_DisplayChange,
        Event_Close,
        Event_Size,
        Event_Fullscreen,
        // Similar to a ProcHandler, passes all of the OS
        // args to the callback. If a user wants to not process
        // anymore messages, return true.
        Event_AllEvents,
        
        Event_Count,
    };
    
    // No data
    struct Close
    {
    };
    
    // No data
    struct Size
    {
        u32 width;
        u32 height;
    };
    
    struct ButtonPress
    {
        u8  mod_ctrl:1;
        u8  mod_shift:1;
        u8  mod_menu:1;
        u8  mod_pad0:5;
        int button;
    };
    
    struct ButtonRelease
    {
        u8  mod_ctrl:1;
        u8  mod_shift:1;
        u8  mod_menu:1;
        u8  mod_pad0:5;
        int button;
    };
    
    struct MouseWheel
    {
        r32 delta;
    };
    
    // Horizontal wheel
    struct MouseHWheel
    {
        r32 delta;
    };
    
    struct KeyPress
    {
        u8  mod_ctrl:1;
        u8  mod_shift:1;
        u8  mod_menu:1;
        u8  mod_pad0:5;
        int key;
    };
    
    struct KeyRelease
    {
        u8  mod_ctrl:1;
        u8  mod_shift:1;
        u8  mod_menu:1;
        u8  mod_pad0:5;
        int key;
    };
    
    // No data
    struct KillFocus
    {
    };
    
    struct SetCursor
    {
        u64 c;
    };
    
    struct DeviceChange
    {
        u64 c;
    };
    
    // No data
    struct DisplayChange
    {
    };
    
    struct MouseMove
    {
        i32 x;
        i32 y;
    };
    
    struct WChar
    {
        u64 c;
    };
    
    struct Fullscreen
    {
        // TODO(Dustin):
    };
    
    struct AllEvents
    {
        void *hwnd;
        u32   message;
        u64   wparam;
        u64   lparam;
    };
    
    struct Event
    {
        EventType type = Event_Count;
        union
        {
            KeyPress      key_press; 
            KeyRelease    key_release; 
            ButtonPress   button_press;
            ButtonRelease button_release;
            MouseMove     mouse_move;
            MouseWheel    mouse_wheel;
            MouseHWheel   mouse_hwheel;
            KillFocus     kill_focus;
            WChar         wchar;
            SetCursor     set_cursor;
            DeviceChange  device_change;
            DisplayChange display_change;
            Close         close;
            Size          size;
            Fullscreen    fullscreen;
            AllEvents     all_events;
        };
    };
    
    typedef bool (*EventCallback)(Event event, void *args);
    
    enum LayerType
    {
        Layer_Game,        // Priority = 2
        
        // TODO(Dustin): These other layers would normally not be
        // active in a full game build, and optional when playing the
        // game from the editor
        
        Layer_ImGui,       // Priority = 1
        Layer_ImGuiWindow, // Priority = 0
        
        Layer_LayerCount,
    };
    
    struct Listener
    {
        EventCallback callback;
        void         *args;
    };
    
    struct Layer
    {
        Listener *_listeners[Event_Count];
        
        void Init()
        {
            for (u32 i = 0; i < Event_Count; ++i) _listeners[i] = 0;
        }
        
        void AddListener(EventType type, EventCallback callback, void *args)
        {
            Listener l = {callback, args};
            arrput(_listeners[type], l);
        }
        
        // Return true if the event should not be propagated to the next layer
        // Return fakle if the event should be propagated to the next layer
        bool DispatchEvent(Event event)
        {
            for (u32 i = 0; i < arrlen(_listeners[event.type]); ++i) 
            {
                if (_listeners[event.type][i].callback(event, _listeners[event.type][i].args)) 
                    return true;
            }
            return false;
        }
        
        void Free()
        {
            for (u32 i = 0; i < Event_Count; ++i) 
                arrfree(_listeners[i]);
        }
    };
};

#endif //_INPUT_LAYER_H

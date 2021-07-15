#ifndef _HOST_WINDOW_H
#define _HOST_WINDOW_H

union WND_ID 
{
    struct 
    {
        u32 idx:24;
        u32 gen:8;
    };
    u32 val;
};
static const WND_ID INVALID_WND_ID = {0xFFFFFFFF};

FORCE_INLINE bool 
operator==(WND_ID left, WND_ID right)
{
    return left.val == right.val;
}

FORCE_INLINE bool 
operator!=(WND_ID left, WND_ID right)
{
    return left.val != right.val;
}



typedef void (*HostWndResizeCallback)(WND_ID wnd, u32 width, u32 height);
typedef void (*HostWndKeyPressCallback)(MapleKey key);
typedef void (*HostWndKeyReleaseCallback)(MapleKey key);
typedef i64  (*HostWndEventProcCallback)(void *wnd, u32 msg, i64 wParam, i64 lParam);

struct HostWndCallbacks 
{
    HostWndResizeCallback     resize   = 0;
    HostWndKeyPressCallback   press    = 0;
    HostWndKeyReleaseCallback release  = 0;
    HostWndEventProcCallback  msg_proc = 0;
};

WND_ID HostWndInit(u32 width, u32 height, const char *title, const char *window_class = 0);
void  HostWndFree(WND_ID wnd);
void  HostWndSetCallbacks(WND_ID wnd, HostWndCallbacks *callbacks);
void  HostWndSetActive(WND_ID wnd);
bool  HostWndMsgLoop(WND_ID wnd);
bool  HostWndIsKeyPressed(WND_ID wnd, MapleKey key);
bool  HostWndIsKeyReleased(WND_ID wnd, MapleKey key);
void  HostWndGetDims(WND_ID wnd, u32 *width, u32 *height);
void* HostWndGetHandle(WND_ID wnd);

// @param layer:      layer the event is for (@see InputLayer.h)
// @param type:       type of event (@see InputLayer.h)
// @param callback:   callback function for the event (@see InputLayer.h)
// @param event_args: any arguments the callback might need
void HostWndAttachListener(WND_ID                     id, 
                           input_layer::LayerType     layer, 
                           input_layer::EventType     type, 
                           input_layer::EventCallback callback, 
                           void                      *event_args = 0);


//------------------------------------------------------------------------------------
// GUI Multiple Viewports API

struct PlatformViewportData
{
    WND_ID  Wnd;
    bool    HwndOwned;
    u32     DwStyle;
    u32     DwExStyle;
    
    PlatformViewportData() { Wnd = INVALID_WND_ID; HwndOwned = false;  DwStyle = DwExStyle = 0; }
    ~PlatformViewportData() { assert(Wnd == INVALID_WND_ID); }
};

#endif // _HOST_WINDOW_H

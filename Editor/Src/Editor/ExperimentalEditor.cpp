// Editor for experimental rendering features

namespace experimental
{
#include "Experimental/Lighting.h"
#include "Experimental/Material.h"
#include "Experimental/EnvironmentMapping.cpp"
#include "Experimental/PhongLighting.cpp"
#include "Experimental/HDR.cpp"
    
    // Scene List
    
    struct SceneInterface
    {
        const char *name;
        void (*OnRender)(CommandList *command_list, v2 dims);
        void (*OnFree)();
        ViewportCamera* (*GetViewportCamera)();
        void (*OnDrawSceneData)();
        void (*OnDrawSelectedObject)();
    };
    
    SceneInterface g_scene_list[] = {
        { "Environment Mapping", environ_mapping::OnRender, environ_mapping::OnFree, environ_mapping::GetViewportCamera,
            environ_mapping::OnDrawSceneData, environ_mapping::OnDrawSelectedObject },
        { "Phong Lighting",      phong_lighting::OnRender,  phong_lighting::OnFree,  phong_lighting::GetViewportCamera,
            phong_lighting::OnDrawSceneData, phong_lighting::OnDrawSelectedObject },
        { "HDR", HDR::OnRender, HDR::OnFree, HDR::GetViewportCamera, HDR::OnDrawSceneData, HDR::OnDrawSelectedObject },
    };
    
    static int g_active_scene_idx = 2;
    
    // Viewport Information
    
    static bool              g_viewport_focused;
    static bool              g_viewport_hovered;
    
    // Window Panels
    
    const char* dockspace_name        = "ExeperimentalDockSpace";
    const char* viewport_panel        = "Experimental Viewport";
    const char* content_browser_panel = "Experimental Content Browser";
    const char* scene_list_panel      = "Experimental Scene List";
    const char* scene_panel           = "Experimental Scene";
    const char* settings_panel        = "Experimental Settings";
    
    static void BuildExperimentalDocker(ImGuiID dockspace_id, ImGuiDockNodeFlags dockspace_flags);
    static void DrawDocker();
    
    static void InitializeWindow(u32 width, u32 height);
    static void FreeWindow();
    static void DrawWindow();
    
    static void DrawScene();
    static void DrawContentBrowser();
    
    // Callbacks
    
    static void WindowRegisterCallbacks(WND_ID wnd);
    static bool ButtonPressCallback(input_layer::Event event, void *args);
    static bool ButtonReleaseCallback(input_layer::Event event, void *args);
    static bool KeyPressCallback(input_layer::Event event, void *args);
    static bool KeyReleaseCallback(input_layer::Event event, void *args);
    static bool CloseCallback(input_layer::Event event, void *args);
    
};

static void 
experimental::BuildExperimentalDocker(ImGuiID dockspace_id, ImGuiDockNodeFlags dockspace_flags)
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    
    // Sets central node
    ImGui::DockBuilderRemoveNode(dockspace_id); // clear any previous layout
    ImGui::DockBuilderAddNode(dockspace_id, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);
    
    // split the dockspace into 2 nodes -- DockBuilderSplitNode takes in the following args in the following order
    //   window ID to split, direction, fraction (between 0 and 1), the final two setting let's us choose which id we want (which ever one we DON'T set as NULL, will be returned by the function)
    //                                                              out_id_at_dir is the id of the node in the direction we specified earlier, out_id_at_opposite_dir is in the opposite direction
    auto dock_id_left     = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.20f, nullptr, &dockspace_id);
    auto dock_id_right    = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.20f, nullptr, &dockspace_id);
    auto dock_id_down     = ImGui::DockBuilderSplitNode(dockspace_id,  ImGuiDir_Down, 0.250f, nullptr, &dockspace_id);
    
    // we now dock our windows into the docking node we made above
    //ImGui::DockBuilderDockWindow("Viewport", dockspace_id);
    ImGui::DockBuilderDockWindow(scene_list_panel, dock_id_left);
    ImGui::DockBuilderDockWindow(scene_panel, dock_id_right);
    ImGui::DockBuilderDockWindow(content_browser_panel, dock_id_down);
    ImGui::DockBuilderDockWindow(viewport_panel, dockspace_id);
    
    ImGui::DockBuilderFinish(dockspace_id);
}

static void experimental::DrawDocker()
{
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    
    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;
    
    ImGuiID dockspace_id = ImGui::GetID(dockspace_name);
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    
    static bool first_time = true;
    if (first_time)
    {
        BuildExperimentalDocker(dockspace_id, dockspace_flags);
        first_time = false;
    }
    
    ImGui::Begin(content_browser_panel);
    ImGui::End();
    
    ImGui::Begin(scene_list_panel);
    DrawScene();
    ImGui::End();
    
    bool renderable = ImGui::Begin(viewport_panel);
    //ImGui::Text("Hello viewport");
    if (renderable) experimental::DrawWindow();
    ImGui::End();
    
    ImGui::Begin(scene_panel);
    g_scene_list[g_active_scene_idx].OnDrawSceneData();
    ImGui::End();
}

static void 
experimental::InitializeWindow(u32 width, u32 height)
{
}

static void 
experimental::DrawWindow()
{
    g_viewport_focused = ImGui::IsWindowFocused();
    g_viewport_hovered = ImGui::IsWindowHovered();
    
    static bool first_time = true;
    
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 viewport_panel_size = ImGui::GetContentRegionAvail();
    
    if (first_time)
    {
        ImGuiViewport *viewport = ImGui::GetWindowViewport();
        PlatformViewportData *viewport_data = (PlatformViewportData*)viewport->PlatformUserData;
        if (viewport_data && viewport_panel_size.x > 0 && viewport_panel_size.y > 0)
        {
            first_time = false;
            WindowRegisterCallbacks(viewport_data->Wnd);
            InitializeWindow((u32)viewport_panel_size.x, (u32)viewport_panel_size.y);
        }
        else
        { // No window to draw into?
            return;
        }
    }
    
    CommandList *command_list = RendererGetActiveCommandList();
    g_scene_list[g_active_scene_idx].OnRender(command_list, { viewport_panel_size.x, viewport_panel_size.y });
}

static void 
experimental::FreeWindow()
{
}

static void 
experimental::DrawScene()
{
    if (ImGui::BeginCombo("Active Scene", g_scene_list[g_active_scene_idx].name))
    {
        for (u32 i = 0; i < _countof(g_scene_list); ++i)
        {
            bool is_selected = g_active_scene_idx == i;
            const char *label = g_scene_list[i].name;;
            if (ImGui::Selectable(label, is_selected))
            {
                if (g_active_scene_idx != i)
                {
                    device::Flush(); // NOTE(Dustin): Is there a way to avoid this?
                    g_scene_list[g_active_scene_idx].OnFree();
                    g_active_scene_idx = i;
                }
            }
        }
        ImGui::EndCombo();
    }
}

static void 
experimental::DrawContentBrowser()
{
    ImGui::Text("Hello content browser");
}

static bool 
experimental::KeyPressCallback(input_layer::Event event, void *args)
{
    ViewportCamera *camera = g_scene_list[g_active_scene_idx].GetViewportCamera();
    if (g_viewport_focused)
    {
        camera->OnKeyPress((MapleKey)event.key_press.key);
    }
    return false;
}

static bool 
experimental::KeyReleaseCallback(input_layer::Event event, void *args)
{
    ViewportCamera *camera = g_scene_list[g_active_scene_idx].GetViewportCamera();
    if (g_viewport_focused)
    {
        camera->OnKeyRelease((MapleKey)event.key_press.key);
    }
    return false;
}

static bool 
experimental::ButtonPressCallback(input_layer::Event event, void *args)
{
    ViewportCamera *camera = g_scene_list[g_active_scene_idx].GetViewportCamera();
    if (experimental::g_viewport_focused && experimental::g_viewport_hovered)
    {
        camera->OnMouseButtonPress((MapleKey)event.key_press.key);
    }
    return false;
}

static bool 
experimental::ButtonReleaseCallback(input_layer::Event event, void *args)
{
    ViewportCamera *camera = g_scene_list[g_active_scene_idx].GetViewportCamera();
    camera->OnMouseButtonRelease((MapleKey)event.key_press.key);
    return false;
}

static bool 
experimental::CloseCallback(input_layer::Event event, void *args)
{
    return false;
}

static void
experimental::WindowRegisterCallbacks(WND_ID wnd)
{
    HostWndAttachListener(wnd, 
                          input_layer::Layer_ImGuiWindow, 
                          input_layer::Event_ButtonPress, ButtonPressCallback);
    HostWndAttachListener(wnd, 
                          input_layer::Layer_ImGuiWindow, 
                          input_layer::Event_ButtonRelease, ButtonReleaseCallback);
    HostWndAttachListener(wnd, 
                          input_layer::Layer_ImGuiWindow, 
                          input_layer::Event_KeyPressed, KeyPressCallback);
    HostWndAttachListener(wnd, 
                          input_layer::Layer_ImGuiWindow, 
                          input_layer::Event_KeyRelease, KeyReleaseCallback);
    HostWndAttachListener(wnd,
                          input_layer::Layer_ImGuiWindow, 
                          input_layer::Event_Close, CloseCallback);
}


#include "../../ext/imgui/imgui_internal.h"
#include "ViewportCamera.cpp"
#include "ExperimentalEditor.cpp"

// TODO(Dustin): 
// - Allow for Camera Viewports to be used as separate "ImGUI Viewports"

namespace editor
{
    //-----------------------------------------------------------------------------------//
    // General Input Tracking
    
    enum MouseMask
    {
        Button_0 = BIT(0),
        Button_1 = BIT(1),
        Button_2 = BIT(2),
        Button_3 = BIT(3),
        Button_4 = BIT(4),
    };
    
    // Track the pressed buttons the mask. Will only be set if ImGui or another viewport
    // does not consume the button press event.
    static u32                g_mouse_button_mask = 0;
    
    //-----------------------------------------------------------------------------------//
    // Main Viewport data
    
    static v2                 g_viewport_bounds[2];
    static v2                 g_viewport_size = V2_ZERO;
    static bool               g_viewport_focused = false;
    static bool               g_viewport_hovered = false;
    static RenderTarget       g_rt_viewport;
    static TEXTURE_ID         g_color_texture = INVALID_TEXTURE_ID;
    static TEXTURE_ID         g_depth_texture = INVALID_TEXTURE_ID;
    static ViewportCamera     g_main_camera;
    static ImGuiID            g_main_viewport_id;
    
    //-----------------------------------------------------------------------------------//
    // Terrain Gen Viewport Data
    
    static ViewportCamera     g_terrain_camera;
    static ImGuiID            g_terrain_viewport_id;
    // Node Editor
    namespace ed = ax::NodeEditor;
    static ed::EditorContext *g_editor_context = 0;
    
    
    //-----------------------------------------------------------------------------------//
    // Active terrain
    
    static TerrainTileInfo    g_tile_settings = {};
    static Terrain            g_terrain;
    // The terrain needs to be re-generated
    static bool               g_terrain_dirty = false;
    
    static TEXTURE_ID         g_heightmap = INVALID_TEXTURE_ID;
    static terrain::Noise_CB  g_terrain_sim;
    static bool               g_terrain_gen_dirty = false;
    
    static const u32            g_downsampled_image_dims = 128;
    static HeightmapDownsampler g_hm_downsampler;
    static TEXTURE_ID           g_downsampled_heightmap = INVALID_TEXTURE_ID;
    
    //-----------------------------------------------------------------------------------//
    // Options
    static bool               g_show_imgui_metrics = false;
    
    //-----------------------------------------------------------------------------------//
    // Function Pre-decs
    
    // Editor Setup
    static void Initialize();
    static void Shutdown();
    
    static void Render();
    
    // Dockspace windows
    static void DrawMainWindow();
    static void DrawTerrainWindow();
    
    // Window Set #1
    static void DrawMainViewport();
    static void DrawContentPanel();
    static void DrawSettingsPanel();
    
    // Window Set #2
    static void DrawTerrainGraph();
    static void DrawTerrainViewport();
    static void DrawTerrainSettings();
    
    // MainViewport callbacks
    static void WindowRegisterCallbacks(WND_ID wnd);
    static bool ButtonPressCallback(input_layer::Event event, void *args);
    static bool ButtonReleaseCallback(input_layer::Event event, void *args);
    static bool KeyPressCallback(input_layer::Event event, void *args);
    static bool KeyReleaseCallback(input_layer::Event event, void *args);
    static bool CloseCallback(input_layer::Event event, void *args);
    
};

static bool 
editor::KeyPressCallback(input_layer::Event event, void *args)
{
    if (g_viewport_focused)
    {
        g_main_camera.OnKeyPress((MapleKey)event.key_press.key);
    }
    if (experimental::g_viewport_focused)
    {
        experimental::g_camera.OnKeyPress((MapleKey)event.key_press.key);
    }
    return false;
}

static bool 
editor::KeyReleaseCallback(input_layer::Event event, void *args)
{
    if (g_viewport_focused)
    {
        g_main_camera.OnKeyRelease((MapleKey)event.key_press.key);
    }
    if (experimental::g_viewport_focused)
    {
        experimental::g_camera.OnKeyRelease((MapleKey)event.key_press.key);
    }
    return false;
}

static bool 
editor::ButtonPressCallback(input_layer::Event event, void *args)
{
    if (g_viewport_focused && g_viewport_hovered)
    {
        g_main_camera.OnMouseButtonPress(event.button_press.button);
    }
    if (experimental::g_viewport_focused && experimental::g_viewport_hovered)
    {
        experimental::g_camera.OnMouseButtonPress((MapleKey)event.key_press.key);
    }
    
    BIT32_TOGGLE_1(g_mouse_button_mask, event.button_press.button);
    return false;
}

static bool 
editor::ButtonReleaseCallback(input_layer::Event event, void *args)
{
    g_main_camera.OnMouseButtonRelease(event.button_press.button);
    experimental::g_camera.OnMouseButtonRelease((MapleKey)event.key_press.key);
    BIT32_TOGGLE_0(g_mouse_button_mask, event.button_press.button);
    return false;
}

static bool 
editor::CloseCallback(input_layer::Event event, void *args)
{
    LogDebug("Window has been closed!");
    return false;
}

static void editor::Initialize()
{
    g_rt_viewport.Init();
    g_viewport_bounds[0] = V2_ZERO;
    g_viewport_bounds[1] = V2_ZERO;
    
    ed::Config config{};
    g_editor_context = ed::CreateEditor(&config);
    
    // Main Viewport Camera
    v3 eye_pos = { 0, 0, -10 };
    v3 look_at = { 0, 0,   0 };
    v3 up_dir  = { 0, 1,   0 };
    g_main_camera.Init(eye_pos, up_dir);
    g_terrain_camera.Init(eye_pos, up_dir);
    
    g_terrain.Init();
    
    // Generate a simple terrain grid
    
    g_tile_settings.tile_x           = 1;
    g_tile_settings.tile_y           = 1;
    g_tile_settings.vertex_y         = 16;
    g_tile_settings.vertex_x         = 16;
    g_tile_settings.scale            = 1.0f;
    g_tile_settings.texture_tiling   = 1.0f;
    g_tile_settings.meshing_strategy = TerrainMeshType::TriangleStrip;
    
    CommandQueue *command_queue = device::GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
    CommandList *command_list = command_queue->GetCommandList();
    g_terrain.Generate(command_list, &g_tile_settings, NULL);
    command_queue->ExecuteCommandLists(&command_list, 1);
    g_terrain_dirty = false;
    
    // Create a heightmap for noisemap testing
    
    const u32 texture_width  = 4096;
    const u32 texture_height = 4096;
    
    DXGI_FORMAT back_buffer_format = DXGI_FORMAT_R8_UNORM;
    D3D12_RESOURCE_DESC heightmap_desc = d3d::GetTex2DDesc(back_buffer_format, 
                                                           texture_width, 
                                                           texture_height);
    heightmap_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    heightmap_desc.MipLevels = 1;
    
    g_heightmap = texture::Create(&heightmap_desc, 0);
    
    
    back_buffer_format  = DXGI_FORMAT_R8G8B8A8_UNORM;
    heightmap_desc = d3d::GetTex2DDesc(back_buffer_format, 
                                       g_downsampled_image_dims, 
                                       g_downsampled_image_dims);
    heightmap_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    heightmap_desc.MipLevels = 1;
    g_downsampled_heightmap = texture::Create(&heightmap_desc, 0);
    
    g_hm_downsampler.Init(g_downsampled_image_dims, g_downsampled_image_dims);
    
    command_queue = device::GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    command_list = command_queue->GetCommandList();
    
    g_terrain_sim.scale      = 0.002f;
    g_terrain_sim.seed       = 534864359;
    g_terrain_sim.octaves    = 8;
    g_terrain_sim.lacunarity = 2.0f;
    g_terrain_sim.decay      = 0.5f;
    g_terrain_sim.threshold  = -1.0f;
    
    terrain::ExecuteFunction(terrain::Function_Perlin, &g_terrain_sim, command_list, g_heightmap);
    
    g_hm_downsampler.Downsample(command_list, g_heightmap, g_downsampled_heightmap);
    
    command_queue->ExecuteCommandLists(&command_list, 1);
    device::Flush();
}

static void editor::Shutdown()
{
    texture::Free(g_color_texture);
    texture::Free(g_depth_texture);
    g_rt_viewport.Free();
    g_terrain.Free();
}

static void 
editor::DrawTerrainGraph()
{
    ImGui::Begin("Graph");
    
    ed::SetCurrentEditor(g_editor_context);
    ed::Begin("My Editor", ImVec2(0.0, 0.0f));
    int uniqueId = 1;
    // Start drawing nodes.
    ed::BeginNode(uniqueId++);
    ImGui::Text("Node A");
    ed::BeginPin(uniqueId++, ed::PinKind::Input);
    ImGui::Text("-> In");
    ed::EndPin();
    ImGui::SameLine();
    ed::BeginPin(uniqueId++, ed::PinKind::Output);
    ImGui::Text("Out ->");
    ed::EndPin();
    ed::EndNode();
    ed::End();
    ed::SetCurrentEditor(nullptr);
    
    ImGui::End();
}

static void 
editor::DrawTerrainViewport()
{
    ImGui::Begin("Terrain Viewport");
    ImGui::End();
}

static void 
editor::DrawTerrainSettings()
{
    ImGui::Begin("Terrain Settings");
    ImGui::End();
}

static void
editor::WindowRegisterCallbacks(WND_ID wnd)
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

static void 
editor::DrawMainViewport()
{
    ImGuiIO& io = ImGui::GetIO();
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
    bool renderable = ImGui::Begin("Main Viewport");
    
    auto viewport_min_region = ImGui::GetWindowContentRegionMin();
    auto viewport_max_region = ImGui::GetWindowContentRegionMax();
    auto viewportOffset = ImGui::GetWindowPos();
    g_viewport_bounds[0] = { viewport_min_region.x + viewportOffset.x, viewport_min_region.y + viewportOffset.y };
    g_viewport_bounds[1] = { viewport_max_region.x + viewportOffset.x, viewport_max_region.y + viewportOffset.y };
    
    g_viewport_focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootWindow);
    g_viewport_hovered = ImGui::IsWindowHovered();
    
    ImVec2 viewport_panel_size = ImGui::GetContentRegionAvail();
    g_viewport_size = { viewport_panel_size.x, viewport_panel_size.y };
    
    static bool first_time = true;
    
    if (first_time)
    {
        ImGuiViewport *viewport = ImGui::GetWindowViewport();
        ImGuiID viewport_id = viewport->ID;
        // The viewport has changed, so need to register callbacks...
        PlatformViewportData *viewport_data = (PlatformViewportData*)viewport->PlatformUserData;
        if (viewport_data)
        {
            first_time = false;
            WindowRegisterCallbacks(viewport_data->Wnd);
            g_main_viewport_id = viewport_id;
            
            // Resize the depth texture.
            D3D12_RESOURCE_DESC depthTextureDesc = d3d::GetTex2DDesc(DXGI_FORMAT_D32_FLOAT, 
                                                                     (u32)g_viewport_size.x, 
                                                                     (u32)g_viewport_size.y);
            // Must be set on textures that will be used as a depth-stencil buffer.
            depthTextureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
            
            // Specify optimized clear values for the depth buffer.
            D3D12_CLEAR_VALUE optimizedClearValue = {};
            optimizedClearValue.Format            = DXGI_FORMAT_D32_FLOAT;
            optimizedClearValue.DepthStencil      = { 1.0F, 0 };
            
            g_depth_texture = texture::Create(&depthTextureDesc, &optimizedClearValue);
            
            DXGI_FORMAT back_buffer_format  = DXGI_FORMAT_R8G8B8A8_UNORM;
            D3D12_RESOURCE_DESC color_tex_desc = d3d::GetTex2DDesc(back_buffer_format, 
                                                                   (u32)g_viewport_size.x, 
                                                                   (u32)g_viewport_size.y);
            color_tex_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            color_tex_desc.MipLevels = 1;
            
            D3D12_CLEAR_VALUE color_clear_value;
            color_clear_value.Format   = color_tex_desc.Format;
            color_clear_value.Color[0] = 0.4f;
            color_clear_value.Color[1] = 0.6f;
            color_clear_value.Color[2] = 0.9f;
            color_clear_value.Color[3] = 1.0f;
            
            g_color_texture = texture::Create(&color_tex_desc, &color_clear_value);
            
            g_rt_viewport.Resize((u32)g_viewport_size.x, (u32)g_viewport_size.y);
            g_rt_viewport.AttachTexture(AttachmentPoint::Color0, g_color_texture);
            g_rt_viewport.AttachTexture(AttachmentPoint::DepthStencil, g_depth_texture);
        }
        else
        {
            renderable = false;
        }
    }
    
    if (renderable)
    {
        // Update the camera (if it needs to)
        g_main_camera.OnUpdate(io.DeltaTime, { io.MouseDelta.x, io.MouseDelta.y });
        g_main_camera.OnMouseScroll(io.MouseWheel);
        
#if 1
        
        // Setup render target
        g_rt_viewport.Resize((u32)g_viewport_size.x, (u32)g_viewport_size.y);
        
        // Setup view projection matrix
        r32 aspect_ratio;
        if (g_viewport_size.x >= g_viewport_size.y)
        {
            aspect_ratio = (r32)g_viewport_size.x / (r32)g_viewport_size.y;
        }
        else 
        {
            aspect_ratio = (r32)g_viewport_size.y / (r32)g_viewport_size.x;
        }
        
        m4 projection_matrix = m4_perspective(g_main_camera._zoom, aspect_ratio, 0.1f, 100.0f);
        m4 view_matrix = g_main_camera.LookAt();
        m4 view_proj = m4_mul(projection_matrix, view_matrix);
        // Draw!
        
        // TODO(Dustin): Move this into a "DrawScene"-like function
        
        CommandList *command_list = RendererGetActiveCommandList();
        
        // Update terrain Mesh, conditions:
        // ---- LMB is not pressed
        // ---- Terrain has been marked as dirty
        // NOTE(Dustin): Should this go on the COPY command queue instead?
        if (((g_mouse_button_mask & Button_0) == 0) && g_terrain_dirty)
        {
            // @FIXME
            // this isn't a great thing to do, but we cannot release a Resource
            // when it is potentially in use on a CommandList
            device::Flush();
            
            g_terrain.Generate(command_list, &g_tile_settings, NULL);
            g_terrain_dirty = false;
        }
        
        if (((g_mouse_button_mask & Button_0) == 0) && g_terrain_gen_dirty)
        {
            terrain::ExecuteFunction(terrain::Function_Perlin, &g_terrain_sim, command_list, g_heightmap);
            g_hm_downsampler.Downsample(command_list, g_heightmap, g_downsampled_heightmap);
            g_terrain_gen_dirty = false;
        }
        
        D3D12_VIEWPORT viewport = g_rt_viewport.GetViewport();
        command_list->SetViewport(viewport);
        
        D3D12_RECT scissor_rect = {};
        scissor_rect.left   = 0;
        scissor_rect.top    = 0;
        scissor_rect.right  = (LONG)viewport.Width;
        scissor_rect.bottom = (LONG)viewport.Height;
        command_list->SetScissorRect(scissor_rect);
        
        r32 clear_color[] = { 0.0f, 0.2f, 0.4f, 1.0f };
        command_list->ClearTexture(g_rt_viewport.GetTexture(AttachmentPoint::Color0), clear_color);
        command_list->ClearDepthStencilTexture(g_rt_viewport.GetTexture(AttachmentPoint::DepthStencil), D3D12_CLEAR_FLAG_DEPTH);
        
        command_list->SetRenderTarget(&g_rt_viewport);
        
        // Draw the terrain
        g_terrain.Render(command_list, view_proj, g_heightmap);
        
        TEXTURE_ID texture = g_rt_viewport.GetTexture(AttachmentPoint::Color0);
        
#else
        
        CommandList *command_list = RendererGetActiveCommandList();
        
        if (((g_mouse_button_mask & Button_0) == 0) && g_terrain_gen_dirty)
        {
            terrain::ExecuteFunction(terrain::Function_Perlin, &g_terrain_sim, command_list, g_heightmap);
            g_terrain_gen_dirty = false;
        }
        
        TEXTURE_ID texture = g_heightmap;
        
#endif
        
        ImGui::Image((ImTextureID)((uptr)texture.val), 
                     ImVec2{ g_viewport_size.x, g_viewport_size.y }, 
                     ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
    }
    
    ImGui::End(); // Viewport end
    ImGui::PopStyleVar();
}

static void 
editor::DrawContentPanel()
{
    ImGui::Begin("Content Browser");
    ImGui::End();
}

static void
editor::DrawSettingsPanel()
{
    ImGui::Begin("Global Settings");
    ImGui::End();
}

static void
editor::DrawMainWindow()
{
    
    DrawMainViewport();
    DrawContentPanel();
    DrawSettingsPanel();
    
}

static void
editor::DrawTerrainWindow()
{
}

static void 
editor::Render()
{
    ImGuiIO& io = ImGui::GetIO();
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("TabTest", nullptr, window_flags);
    ImGui::PopStyleVar();
    ImGui::PopStyleVar(2);
    {
        //ImGui::Text("Hello, left!");
        
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                // Disabling fullscreen would allow the window to be moved to the front of other windows, 
                // which we can't undo at the moment without finer window depth/z control.
                //ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen_persistant);1
                if (ImGui::MenuItem("New", "Ctrl+N")) {}
                //NewScene();
                
                if (ImGui::MenuItem("Open...", "Ctrl+O")) {}
                //OpenScene();
                
                if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {}
                //SaveSceneAs();
                
                if (ImGui::MenuItem("Exit")) 
                    PlatformCloseApplication();
                
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Options"))
            {
                if (!g_show_imgui_metrics)
                {
                    if (ImGui::MenuItem("Show Metrics", "Ctrl+M")) 
                        g_show_imgui_metrics = true;
                }
                else
                {
                    if (ImGui::MenuItem("Hide Metrics", "Ctrl+M")) 
                        g_show_imgui_metrics = false;
                }
                ImGui::EndMenu();
            }
            
            ImGui::EndMenuBar();
        }
        
        ImGui::BeginTabBar("TabBar");
        {
            
            
            //-----------------------------------------------------------------------------------//
            // Terrain Generator Tab
            
            if (ImGui::BeginTabItem("Experimental"))
            {
                static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
                // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
                if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
                    window_flags |= ImGuiWindowFlags_NoBackground;
                
                ImGuiID dockspace_id = ImGui::GetID("ExeperimentalDockSpace");
                ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
                
                static auto first_time = true;
                if (first_time)
                {
                    first_time = false;
                    
                    // Sets central node?
                    ImGui::DockBuilderRemoveNode(dockspace_id); // clear any previous layout
                    ImGui::DockBuilderAddNode(dockspace_id, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
                    ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);
                    
                    // split the dockspace into 2 nodes -- DockBuilderSplitNode takes in the following args in the following order
                    //   window ID to split, direction, fraction (between 0 and 1), the final two setting let's us choose which id we want (which ever one we DON'T set as NULL, will be returned by the function)
                    //                                                              out_id_at_dir is the id of the node in the direction we specified earlier, out_id_at_opposite_dir is in the opposite direction
                    auto dock_id_left     = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.20f, nullptr, &dockspace_id);
                    auto dock_id_down     = ImGui::DockBuilderSplitNode(dockspace_id,  ImGuiDir_Down, 0.250f, nullptr, &dockspace_id);
                    
                    // we now dock our windows into the docking node we made above
                    //ImGui::DockBuilderDockWindow("Viewport", dockspace_id);
                    ImGui::DockBuilderDockWindow("Scene", dock_id_left);
                    ImGui::DockBuilderDockWindow("Content Browser", dock_id_down);
                    ImGui::DockBuilderDockWindow("Experimental Viewport", dockspace_id);
                    
                    ImGui::DockBuilderFinish(dockspace_id);
                }
                
                
                ImGui::Begin("Scene");
                //experimental::DrawScene();
                ImGui::End();
                
                ImGui::Begin("Content Browser");
                //experimental::DrawContentBrowser();
                ImGui::End();
                
                bool renderable = ImGui::Begin("Experimental Viewport");
                //ImGui::Text("Hello viewport");
                if (renderable) experimental::DrawWindow();
                ImGui::End();
                
                ImGui::EndTabItem();
            }
            
            //-----------------------------------------------------------------------------------//
            // Main Viewport Tab
            
            if (ImGui::BeginTabItem("Viewport"))
            {
                static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
                // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
                if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
                    window_flags |= ImGuiWindowFlags_NoBackground;
                
                ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
                ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
                
                static auto first_time = true;
                if (first_time)
                {
                    first_time = false;
                    
                    // Sets central node?
                    ImGui::DockBuilderRemoveNode(dockspace_id); // clear any previous layout
                    ImGui::DockBuilderAddNode(dockspace_id, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
                    ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);
                    
                    // split the dockspace into 2 nodes -- DockBuilderSplitNode takes in the following args in the following order
                    //   window ID to split, direction, fraction (between 0 and 1), the final two setting let's us choose which id we want (which ever one we DON'T set as NULL, will be returned by the function)
                    //                                                              out_id_at_dir is the id of the node in the direction we specified earlier, out_id_at_opposite_dir is in the opposite direction
                    auto dock_id_left     = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.20f, nullptr, &dockspace_id);
                    auto dock_id_down     = ImGui::DockBuilderSplitNode(dockspace_id,  ImGuiDir_Down, 0.250f, nullptr, &dockspace_id);
                    
                    // we now dock our windows into the docking node we made above
                    //ImGui::DockBuilderDockWindow("Viewport", dockspace_id);
                    ImGui::DockBuilderDockWindow("Scene", dock_id_left);
                    ImGui::DockBuilderDockWindow("Content Browser", dock_id_down);
                    ImGui::DockBuilderDockWindow("Main Viewport", dockspace_id);
                    
                    ImGui::DockBuilderFinish(dockspace_id);
                }
                
                ImGui::Begin("Scene");
                
                ImGui::Value("Viewport \"Scene\" ID: ", ImGui::GetWindowViewport()->ID);
                
                ImGuiInputTextFlags input_flags = ImGuiInputTextFlags_EnterReturnsTrue;
                if (ImGui::InputInt("Tile X Count", (int*)&g_tile_settings.tile_x, 1, 5, input_flags))
                {
                    g_terrain_dirty = true;
                }
                
                if (ImGui::InputInt("Tile Y Count", (int*)&g_tile_settings.tile_y, 1, 5, input_flags))
                {
                    g_terrain_dirty = true;
                }
                
                if (ImGui::InputInt("Tile Width", (int*)&g_tile_settings.vertex_x, 1, 5, input_flags))
                {
                    g_terrain_dirty = true;
                }
                
                if (ImGui::InputInt("Tile Height", (int*)&g_tile_settings.vertex_y, 1, 5, input_flags))
                {
                    g_terrain_dirty = true;
                }
                
                if (ImGui::InputFloat("Tile Scale", &g_tile_settings.scale, 1.0f, 5.0f, "%.3f", input_flags))
                {
                    g_terrain_dirty = true;
                }
                
                if (ImGui::InputFloat("Texture Tiling", &g_tile_settings.texture_tiling, 1.0f, 5.0f, "%.3f", input_flags))
                {
                    g_terrain_dirty = true;
                }
                
                static bool set_wireframe = true;
                if (ImGui::Checkbox("Wireframe", &set_wireframe))
                {
                    g_terrain.SetWireframe(set_wireframe);
                }
                
                
                if (ImGui::InputFloat("Terrain Scale", &g_terrain_sim.scale, 1.0f, 5.0f, "%.3f", input_flags))
                {
                    g_terrain_gen_dirty = true;
                }
                
                if (ImGui::InputInt("Terrain Seed", &g_terrain_sim.seed, 1, 5, input_flags))
                {
                    g_terrain_gen_dirty = true;
                }
                
                if (ImGui::InputInt("Terrain Octaves", &g_terrain_sim.octaves, 1, 5, input_flags))
                {
                    g_terrain_gen_dirty = true;
                }
                
                if (ImGui::InputFloat("Terrain Lacunarity", &g_terrain_sim.lacunarity, 1.0f, 5.0f, "%.3f", input_flags))
                {
                    g_terrain_gen_dirty = true;
                }
                
                if (ImGui::InputFloat("Terrain Decay", &g_terrain_sim.decay, 1.0f, 5.0f, "%.3f", input_flags))
                {
                    g_terrain_gen_dirty = true;
                }
                
                if (ImGui::InputFloat("Terrain Threshold", &g_terrain_sim.threshold, 1.0f, 5.0f, "%.3f", input_flags))
                {
                    g_terrain_gen_dirty = true;
                }
                
                ImGui::Image((ImTextureID)((uptr)g_downsampled_heightmap.val), 
                             ImVec2{ 256, 256 }, 
                             ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
                
                ImGui::End();
                
                ImGui::Begin("Content Browser");
                ImGui::Text("Hello, Content Browser!");
                ImGui::End();
                
                
                DrawMainViewport();
                
                ImGui::EndTabItem();
            }
            
            //-----------------------------------------------------------------------------------//
            // Terrain Generator Tab
            
            if (ImGui::BeginTabItem("Terrain Gen"))
            {
                static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
                // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
                if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
                    window_flags |= ImGuiWindowFlags_NoBackground;
                
                ImGuiID dockspace_id = ImGui::GetID("MyOtherDockSpace");
                ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
                
                static auto first_time = true;
                if (first_time)
                {
                    first_time = false;
                    
                    // Sets central node?
                    ImGui::DockBuilderRemoveNode(dockspace_id); // clear any previous layout
                    ImGui::DockBuilderAddNode(dockspace_id, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
                    ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);
                    
                    // split the dockspace into 2 nodes -- DockBuilderSplitNode takes in the following args in the following order
                    //   window ID to split, direction, fraction (between 0 and 1), the final two setting let's us choose which id we want (which ever one we DON'T set as NULL, will be returned by the function)
                    //                                                              out_id_at_dir is the id of the node in the direction we specified earlier, out_id_at_opposite_dir is in the opposite direction
                    
                    auto dock_id_viewport    = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.20f, nullptr, &dockspace_id);
                    auto dock_id_settings    = ImGui::DockBuilderSplitNode(dock_id_viewport, ImGuiDir_Down, 0.60f, nullptr, &dock_id_viewport);
                    auto dock_id_console     = ImGui::DockBuilderSplitNode(dockspace_id,  ImGuiDir_Down, 0.250f, nullptr, &dockspace_id);
                    
                    // we now dock our windows into the docking node we made above
                    //ImGui::DockBuilderDockWindow("Viewport", dockspace_id);
                    ImGui::DockBuilderDockWindow("Viewport", dock_id_viewport);
                    ImGui::DockBuilderDockWindow("Settings", dock_id_settings);
                    ImGui::DockBuilderDockWindow("Console",  dock_id_console);
                    ImGui::DockBuilderDockWindow("Graph",    dockspace_id);
                    
                    ImGui::DockBuilderFinish(dockspace_id);
                }
                
                // ImGuiViewport *viewport = ImGui::GetWindowViewport();
                
                ImGui::Begin("Viewport");
                ImGui::Text("Hello, viewport!");
                
                ImGui::Value("Viewport \"Viewport\" ID: ", ImGui::GetWindowViewport()->ID);
                
                ImGui::End();
                
                ImGui::Begin("Settings");
                ImGui::Text("Hello, settings!");
                
                ImGui::Value("Viewport \"Settings\" ID: ", ImGui::GetWindowViewport()->ID);
                
                ImGui::End();
                
                ImGui::Begin("Console");
                ImGui::Text("Hello, console!");
                
                ImGui::Value("Viewport \"Console\" ID: ", ImGui::GetWindowViewport()->ID);
                
                ImGui::End();
                
                DrawTerrainGraph();
                
                //ImGui::Text("Hello, other tab item!");
                ImGui::EndTabItem();
            }
            
        }
        ImGui::EndTabBar();
        
        if (g_show_imgui_metrics)
            ImGui::ShowMetricsWindow(&g_show_imgui_metrics);
    }
    ImGui::End(); // TabTest
    
    //ImGui::End(); // MainWindow
    
    // End imgui frame
    ImGui::Render();
}

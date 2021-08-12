
#define MAPLE_MEMORY_IMPLEMENTATION
#define MAPLE_STRING_IMPLEMENTATION
#define MAPLE_STR_POOL_IMPLEMENTATION
#define MAPLE_MATH_IMPLEMENTATION
#define STB_DS_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION

//#define TOML_ASSERT Assert

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h> 
#include <stdarg.h>
#include <math.h>
#include <float.h>
#include <stdlib.h>

#include "Common/PlatformTypes.h"
#include "Common/Core.h"
#include "Common/Util/String.h"
#include "Platform/Platform.h" // platform API
//#include "EngineApi.h"
//#include "RendererApi.h"

#include "Core/SysMemory.h"

#include "Common/Util/Memory.h"
#include "Common/Util/stb_ds.h"
#include "Common/Util/stb_image.h"
//#include "Common/Util/StrPool.h"
#include "Common/Util/MapleMath.h"
#include "Common/Util/String.cpp"

#include "Common/Util/Parsers/TomlParser.h"
#include "Common/Util/Parsers/TomlParser.cpp"

#include "Platform/HostKey.h"
#include "Platform/Timer.h"
#include "Events/InputLayer.h"
#include "Platform/HostWindow.h"

#include "Core/SysMemory.cpp"

// Load ImGui Library
#include "../ext/imgui/imgui.h"
#include "../ext/imgui-node-editor/imgui_node_editor.h"
#pragma comment(lib, "imgui.lib")

#include "Platform/Platform.cpp"

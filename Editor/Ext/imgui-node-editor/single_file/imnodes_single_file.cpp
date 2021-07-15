
// Node editor implementation

# include <cstdio> // snprintf
# include <string>
# include <fstream>
# include <bitset>
# include <climits>
# include <algorithm>
# include <sstream>
# include <streambuf>
# include <type_traits>
#include <imgui/imgui.h>

#define IMGUI_DEFINE_MATH_OPERATORS

#include <imgui/imgui_internal.h>
#include "../imgui_node_editor.h"

#include "../imgui_extra_math.h"
#include "../imgui_bezier_math.h"
#include "../imgui_canvas.h"

#include "../imgui_node_editor_internal.h"


#ifdef _IMGUI_NODE_IMPLEMENTATION
#include "../imgui_canvas.cpp"
#include "../imgui_node_editor.cpp"
#include "../imgui_node_editor_api.cpp"
#include "../crude_json.cpp"
#endif

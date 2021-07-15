
namespace maple 
{
#include "Loader/EngineFunctions.h"
#include "Loader/EngineFunctions.cpp"
    
    static void LoadGlobalFunctions(HMODULE lib)
    {
#define MP_GLOBAL_LEVEL_FUNCTION(fn)         \
        fn = (PFN_##fn)GetProcAddress(lib, #fn); \
              assert(fn);
              
#include "Loader/EngineFunctions.inl"
    }
};

namespace samara  
{
#include "RendererApi.h"
#include "Loader/RendererFunctions.h"
#include "Loader/RendererFunctions.cpp"
    
    static void LoadGlobalFunctions(HMODULE lib)
    {
#define MP_GLOBAL_LEVEL_FUNCTION(fn)         \
        fn = (PFN_##fn)GetProcAddress(lib, #fn); \
              assert(fn);
              
#include "Loader/RendererFunctions.inl"
    }
};


#ifndef _RENDERER_FUNCTIONS_H
#define _RENDERER_FUNCTIONS_H

#define MP_GLOBAL_LEVEL_FUNCTION(fn) extern PFN_##fn fn;
#include "RendererFunctions.inl"

#endif //_RENDERER_FUNCTIONS_H

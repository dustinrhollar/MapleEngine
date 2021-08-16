#ifndef _TOML_PARSER_H
#define _TOML_PARSER_H

// For malloc/free
#include <stdlib.h>
// For uint64_t
#include <stdint.h>

// TODO(Dustin): 
// - (BUG) Spaces should be allowed in object names 
// - (Inline) Table support
// - (MAYBE) - custom array and hashtable to remove stb_ds as a depedency

enum TomlType
{
    Toml_String,
    Toml_Int,
    Toml_Bool,
    Toml_Float,
    Toml_Table,
    Toml_Array,
    
    Toml_Count,
};

struct TomlData
{
    TomlType type;
    union
    {
        int        i;                // integer type
        bool       b;                // boolean type
        float      f;                // float type
        TomlData  *a;                // array type, inline table
        struct { char *s; int sl; }; // string type
    };
};

struct TomlKeyValue
{
    char     *key;
    TomlData  value;
};

struct TomlObject
{
    TomlKeyValue *data_table;
};

struct TomlObjectKeyValue
{
    char       *key;
    TomlObject  value;
};

struct TomlCallbacks
{
    void* (*Alloc)(uint64_t size);
    void  (*Free)(void *ptr);
    
    void  (*LoadFile)(const char* file_path, u8** buffer, int* size);
    void  (*FreeFile)(void *ptr);
};

struct Toml
{
    char               *title;
    TomlObjectKeyValue *table;
    // NOTE(Dustin): Should we store the data after parsing the file?
    // - Reasons keeping data: strings are persistent as long as the 
    //   data is held in memory. Can just point to their loc in 
    void               *file_data;
};

enum TomlResult
{
    TomlResult_Success,
    TomlResult_ParseError,
    TomlResult_UnknownLexeme,
    TomlResult_UnknownToken,
    TomlResult_UnexpectedDataType,
    TomlResult_InvalidSyntax,
    
    TomlResult_Count,
};

static TomlResult  TomlLoad(Toml *toml, const char *filepath);
static void        TomlFree(Toml *toml);
static void        TomlSetCallbacks(TomlCallbacks *callbacks);

/* Read Object API */
static TomlObject  TomlGetObject(Toml *toml, const char *name);
/* Read Basic Data Types API */
static int         TomlGetInt(TomlObject *obj, const char *name);
static bool        TomlGetBool(TomlObject *obj, const char *name);
static float       TomlGetFloat(TomlObject *obj, const char *name);
static const char* TomlGetString(TomlObject *obj, const char *name);
static int         TomlGetStringLen(TomlObject *obj, const char *name);
/* Read Array API */
static TomlData*   TomlGetArray(TomlObject *obj, const char *name);
static int         TomlGetArrayLen(TomlData *data);
static int         TomlGetIntArrayElem(TomlData *data, int idx);
static bool        TomlGetBoolArrayElem(TomlData *data, int idx);
static float       TomlGetFloatArrayElem(TomlData *data, int idx);
static float       TomlGetFloatArrayElem(TomlData *data, int idx);
static const char* TomlGetStringArrayElem(TomlData *data, int idx);
static int         TomlGetStringLenArrayElem(TomlData *data, int idx);
/* Gets a sub-array from an index. Ex. [[1, 2, 3], [1, 2, 3]] */
static TomlData*   TomlGetArrayElem(TomlData *data, int idx);

static Toml        TomlCreate();
static TomlObject  TomlCreateObject();
static TomlData    TomlDataInt(int val);
static TomlData    TomlDataBool(bool val);
static TomlData    TomlDataFloat(float val);
// NOTE(Dustin): Shoulds strings be copied to internal storage?
static TomlData    TomlDataString(char *str, int len);
static TomlData    TomlDataArray();

static void        TomlArrayAddInt(TomlData *data, int val);
static void        TomlArrayAddBool(TomlData *data, bool val);
static void        TomlArrayAddFloat(TomlData *data, float val);
static void        TomlArrayAddString(TomlData *data, char *str, int len);
static void        TomlArrayAddArray(TomlData *data, TomlData *array);

static void        TomlArrayAddInt(TomlData *data, TomlData *i);
static void        TomlArrayAddBool(TomlData *data, TomlData *b);
static void        TomlArrayAddFloat(TomlData *data, TomlData *f);
static void        TomlArrayAddString(TomlData *data, TomlData *s);

static void        TomlObjectAddData(TomlObject *obj, TomlData *data, char *name);
static void        TomlAddObject(Toml *toml, TomlObject *obj, char *name);

static void        TomlToString(Toml *toml, char **str, int *len);
// Free a string allocated by the fn call to TomlToString
static void        TomlFreeString(char **str);

#endif //_TOML_PARSER_H

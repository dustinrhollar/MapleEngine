
//
// Several Files need to be generated:
//
// - Core Metadata file, this will contain the metadata of all structs that have been tagged
// - MDF -> Header File
// --- Foo.mdf -> Foo_Gen.h
//
// Generated print has the format:
// - PrintFoo() or... Print(struct_data, struct_name)
//
//
// single include header: MetaGen.h
// --- Defines for each tag
// --- Print tag:
// ------ See extern_print_fn_signature
//

// TODO(Dustin): 
// - MetaProperty output metadata for internal types 
// - Preserve includes within a MDF file and output them to H file
// - CONST and PTR qualifiers

static const char *metadata_include_section =
"#include <stdlib.h> // malloc/free\n"
"#include <stdint.h>\n"
"#include <stdio.h>  // default callbacks, vprintf\n"
"#include <assert.h> // assert\n"
"#include <stdarg.h> // default callbacks, va_list\n"
"\n";

static const char *metadata_global_section =
"struct ReflectionMetadata\n"
"{\n"
"\tMetaRegistry      registry;\n"
"\tMetadataCallbacks callbacks;\n"
"} g_reflection_system = {};\n\n";

static const char *metadata_callbacks =
"struct MetadataCallbacks\n"
"{\n"
"\tvoid (*Log)(char *fmt, ...);\n"
"\tvoid (*FileRead)(const char* file_path, uint8_t** buffer, int* size);\n"
"\tvoid (*FileWrite)(const char* file_path, uint8_t* buffer, uint64_t size, bool append);\n"
"};\n\n";


static const char *metadata_fn_header_pre_decs =
"static void ReflectionMetadataInit();\n"
"static void ReflectionMetadataFree();\n"
"static void ReflectionMetadataSetCallbacks(MetadataCallbacks *callbacks);\n"
"\n";

static const char *metadata_fn_src_pre_decs =
"static MetaRegistry MetadataRegisteryInit();\n"
"static void MetadataRegisteryFree(MetaRegistry *registry);\n"
"static void MetaLogger_Internal(char *fmt, ...);\n"
"static void MetaFileReader_Internal(const char* file_path, uint8_t** buffer, int* size);\n"
"static void MetaFileWriter_Internal(const char* file_path, uint8_t* buffer, uint64_t size, bool append = false);\n"
"static void PrintObj_Internal(MetaRegistry *registry, void *obj, MetaClass *cls, u32 depth);\n"
"\n";

static const char *metadata_refl_meta_init_impl = 
"static void ReflectionMetadataInit()\n"
"{\n"
"\tg_reflection_system.registry = MetadataRegisteryInit();\n"
"\tMetadataCallbacks cb = { MetaLogger_Internal, MetaFileReader_Internal, MetaFileWriter_Internal };\n"
"\tReflectionMetadataSetCallbacks(&cb);\n"
"}\n\n";

static const char *metadata_refl_meta_free_impl = 
"static void ReflectionMetadataFree()\n"
"{\n"
"\tMetadataRegisteryFree(&g_reflection_system.registry);\n"
"}\n\n";

static const char *metadata_set_callbacks_function_impl =
"static void ReflectionMetadataSetCallbacks(MetadataCallbacks *callbacks)\n"
"{\n"
"\tif (callbacks->Log) g_reflection_system.callbacks.Log = callbacks->Log;\n"
"\tif (callbacks->FileRead && callbacks->FileWrite)\n"
"\t{\n"
"\t\tg_reflection_system.callbacks.FileRead  = callbacks->FileRead;\n"
"\t\tg_reflection_system.callbacks.FileWrite = callbacks->FileWrite;\n"
"\t}\n"
"}\n\n";

static const char *print_state_machine =
"\tif (!(cls->tags & META_PROPERTY_TAG_PRINT))\n"
"\t{\n"
"\t\tg_reflection_system.callbacks.Log(\"WARNING: Attempting to print the class, %s, without the tag, META_PROPERTY_TAG_PRINT.\\n\", cls->name);\n"
"\t\treturn;\n"
"\t}\n\n"
"\tfor (u32 i = 0; i < depth; ++i) g_reflection_system.callbacks.Log(\"\\t\");\n"
"\tdepth++;\n"
"\tg_reflection_system.callbacks.Log(\"%s {\\n\", cls->name);\n"
"\tfor (uint32_t i = 0; i < cls->count; ++i)\n\t{\n"
"\t\tMetaProperty *prop = &cls->properties[i];\n"
"\t\tif (!(prop->tags & META_PROPERTY_TAG_PRINT)) continue;\n\n"
"\t\tswitch (prop->type.id)\n\t\t{\n"
"\t\t\tcase META_PROPERTY_TYPE_S8:\n"
"\t\t\tcase META_PROPERTY_TYPE_S16:\n"
"\t\t\tcase META_PROPERTY_TYPE_S32:\n"
"\t\t\tcase META_PROPERTY_TYPE_S64:\n"
"\t\t\tcase META_PROPERTY_TYPE_U8:\n"
"\t\t\tcase META_PROPERTY_TYPE_U16:\n"
"\t\t\tcase META_PROPERTY_TYPE_U32:\n"
"\t\t\tcase META_PROPERTY_TYPE_U64:\n"
"\t\t\tcase META_PROPERTY_TYPE_SIZE_T:\n\t\t\t{\n"
"\t\t\t\tfor (u32 i = 0; i < depth; ++i) g_reflection_system.callbacks.Log(\"\\t\");\n"
"\t\t\t\tuint64_t *v = meta_registry_getvp(obj, uint64_t, prop);\n"
"\t\t\t\tg_reflection_system.callbacks.Log(\"%s (%s): %d\\n\", prop->name, prop->type.name, *v);\n"
"\t\t\t} break;\n\n"
"\t\t\tcase META_PROPERTY_TYPE_F32:\n\t\t\t{\n"
"\t\t\t\tfor (u32 i = 0; i < depth; ++i) g_reflection_system.callbacks.Log(\"\\t\");\n"
"\t\t\t\tfloat *v = meta_registry_getvp(obj, float, prop);\n"
"\t\t\t\tg_reflection_system.callbacks.Log(\"%s (%s): %lf\\n\", prop->name, prop->type.name, *v);\n"
"\t\t\t} break;\n\n"
"\t\t\tcase META_PROPERTY_TYPE_F64:\n\t\t\t{\n"
"\t\t\t\tfor (u32 i = 0; i < depth; ++i) g_reflection_system.callbacks.Log(\"\\t\");\n"
"\t\t\t\tdouble *v = meta_registry_getvp(obj, double, prop);\n"
"\t\t\t\tg_reflection_system.callbacks.Log(\"%s (%s): %f\\n\", prop->name, prop->type.name, *v);\n"
"\t\t\t} break;\n\n"
"\t\t\tcase META_PROPERTY_TYPE_STR:\n\t\t\t{\n"
"\t\t\t\tfor (u32 i = 0; i < depth; ++i) g_reflection_system.callbacks.Log(\"\\t\");\n"
"\t\t\t\tchar *v = meta_registry_getv(obj, char*, prop);\n"
"\t\t\t\tg_reflection_system.callbacks.Log(\"%s (%s): %s\\n\", prop->name, prop->type.name, v);\n"
"\t\t\t} break;\n\n";

static const char *default_logger =
"static void MetaLogger_Internal(char *fmt, ...)\n"
"{\n"
"\tva_list args;\n"
"\tva_start(args, fmt);\n"
"\tvprintf(fmt, args);\n"
"\tva_end(args);\n"
"}\n\n";

static const char *default_file_reader =
"static void MetaFileReader_Internal(const char* filepath, uint8_t** buffer, int* size)\n"
"{\n"
"\tFILE *fp = fopen(filepath, \"r\");\n"
"\tassert(fp);\n"
"\tfseek(fp, 0, SEEK_END); // seek to end of file\n"
"\tuint64_t fsize = ftell(fp); // get current file pointer\n"
"\tfseek(fp, 0, SEEK_SET); // seek back to beginning of file\n"
"\t*buffer = (uint8_t*)malloc(fsize+1);\n"
"\tuint64_t read = fread(*buffer, 1, fsize, fp);\n"
"\t*size   = (int)read;\n"
"\t(*buffer)[read] = 0;\n"
"}\n\n";

static const char *default_file_writer =
"static void MetaFileWriter_Internal(const char* filepath, uint8_t* buffer, uint64_t size, bool append)\n"
"{\n"
"\tFILE *fp = (append) ? fopen(filepath, \"a\") : fopen(filepath, \"w\");\n"
"\tassert(fp);\n"
"\tfwrite (buffer, 1, size, fp);\n"
"\tfclose(fp);\n"
"}\n\n";

static const char *extern_print_fn_signature = 
"extern void MetaPrint_Impl(void *obj, const char *name);\n"
"#define MetaPrint(VAL, TYPE) MetaPrint_Impl((void*)(VAL), meta_to_str(TYPE))\n"
"\n";

static const char *intern_print_fn_signature = 
"void MetaPrint_Impl(void *obj, const char *name)\n"
"{\n"
"\tMetaClass *cls = _MetaClassGetPImpl(&g_reflection_system.registry, name);\n"
"\tPrintObj_Internal(&g_reflection_system.registry, obj, cls, 0);\n"
"}\n\n";


struct StrAllocator
{
    void *start;
    char *end;
    char *brkp;
};

struct CodeGenerator
{
    // Core registry collected across all files
    MetaRegistry registry;
    
    // Hash lookups for new structs that need to be registered as a type
    u32 type_lut_id;
    struct { char* key; u32 value; } *type_lut;
    
    // Parser strings are not null terminated. Use a 
    // list of linear allocators to store names of all struct names.
    StrAllocator *allocator_list = 0;
    StrAllocator *allocator = 0; // current allocator in the list
};

static void FormatNestedParserUnnamedClass(CodeGenerator *generator, PrettyBuffer *buffer, ParserClass *cls, ParserProperty *property, int depth);
static void FormatNestedParserClass(CodeGenerator *generator, PrettyBuffer *buffer, ParserClass *cls, int depth);

static void 
StrAllocatorInit(StrAllocator *a)
{
    // TODO(Dustin): MALLOC
    const u64 size = 4096;
    a->start = malloc(size);
    a->end = (char*)a->start + size;
    a->brkp = (char*)a->start;
}

static void 
StrAllocatorFree(StrAllocator *a)
{
    // TODO(Dustin): FREE
    free(a->start);
    a->brkp = a->end = 0;
}

static void*
StrAllocatorAlloc(StrAllocator *a, u64 len)
{
    char *result = 0;
    if (a->brkp + len < a->end)
    {
        result = a->brkp;
        a->brkp += len;
    }
    return result;
}

static CodeGenerator
CodeGeneratorInit()
{
    CodeGenerator gen = {};
    
    gen.registry = MetaRegistryNew();
    gen.type_lut_id = META_PROPERTY_TYPE_COUNT + 1;
    gen.type_lut = 0;
    shdefault(gen.type_lut, META_PROPERTY_TYPE_COUNT);
    
    StrAllocator str_alloc;
    StrAllocatorInit(&str_alloc);
    arrput(gen.allocator_list, str_alloc);
    gen.allocator = gen.allocator_list;
    
    return gen;
}

static void
CodeGeneratorFree(CodeGenerator *generator)
{
    MetaRegistryFree(&generator->registry);
    
    for (u32 i = 0; i < (u32)arrlen(generator->allocator_list); ++i)
        StrAllocatorFree(&generator->allocator_list[i]);
    arrfree(generator->allocator_list);
    
    // TODO(Dustin): Free the LUT
}

static char*
CodeGeneratorAllocStr(CodeGenerator *generator, char *str, u64 size)
{
    char *ptr = (char*)StrAllocatorAlloc(generator->allocator, size+1);
    if (!ptr)
    {
        StrAllocator a = {};
        StrAllocatorInit(&a);
        arrput(generator->allocator_list, a);
        generator->allocator = &generator->allocator_list[arrlen(generator->allocator_list) - 1];
        
        ptr = (char*)StrAllocatorAlloc(generator->allocator, size);
    }
    
    if (str)
        memcpy(ptr, str, size);
    ptr[size] = 0;
    
    return ptr;
}


static MetaPropertyTypeInfo
ParserPropertyToMetaProperty(CodeGenerator *generator, ParserPropertyTypeInfo info)
{
    MetaPropertyTypeInfo result;
    switch (info.id)
    {
        case PARSER_PROPERTY_TYPE_S8:     result = META_PROPERTY_TYPE_INFO_S8;     break;
        case PARSER_PROPERTY_TYPE_S16:    result = META_PROPERTY_TYPE_INFO_S16;    break;
        case PARSER_PROPERTY_TYPE_S32:    result = META_PROPERTY_TYPE_INFO_S32;    break;
        case PARSER_PROPERTY_TYPE_S64:    result = META_PROPERTY_TYPE_INFO_S64;    break;
        //case PARSER_PROPERTY_TYPE_BOOL:   result = META_PROPERTY_TYPE_INFO_U8;     break;
        case PARSER_PROPERTY_TYPE_U8:     result = META_PROPERTY_TYPE_INFO_U8;     break;
        case PARSER_PROPERTY_TYPE_U16:    result = META_PROPERTY_TYPE_INFO_U16;    break;
        case PARSER_PROPERTY_TYPE_U32:    result = META_PROPERTY_TYPE_INFO_U32;    break;
        case PARSER_PROPERTY_TYPE_U64:    result = META_PROPERTY_TYPE_INFO_U64;    break;
        case PARSER_PROPERTY_TYPE_F32:    result = META_PROPERTY_TYPE_INFO_F32;    break;
        case PARSER_PROPERTY_TYPE_F64:    result = META_PROPERTY_TYPE_INFO_F64;    break;
        case PARSER_PROPERTY_TYPE_SIZE_T: result = META_PROPERTY_TYPE_INFO_SIZE_T; break;
        case PARSER_PROPERTY_TYPE_STR:    result = META_PROPERTY_TYPE_INFO_STR;    break;
        //case PARSER_PROPERTY_TYPE_CLASS:
        case PARSER_PROPERTY_TYPE_CUSTOM:
        {
            char* name = CodeGeneratorAllocStr(generator, (char*)info.name, info.name_len);
            
            u32 id = shget(generator->type_lut, name);
            if (id == META_PROPERTY_TYPE_COUNT)
            { // the entry does not currently exist
                id = shput(generator->type_lut, name, generator->type_lut_id);
                ++generator->type_lut_id;
            }
            
            result = _MetaPropertyTypeInfoDeclImpl(name, id);
        } break;
    }
    return result;
}

static void
StrToUpperCase(char *str, u32 len)
{
    for (u32 i = 0; i < len; ++i)
    {
        if (str[i] >= 'a' && str[i] <= 'z')
            str[i] = 'A' + (str[i] - 'a');
        else if (str[i] == '.' || str[i] == '/' || str[i] == '\\')
            str[i] = '_';
    }
}

static void 
FormatIndent(PrettyBuffer *buffer, int count)
{
    for (int  i = 0; i < count; ++i) pb_write(buffer, "\t");
}

static void 
FormatStructProperty(CodeGenerator *generator, PrettyBuffer *buffer, ParserProperty *property, int indent)
{
    char* type_name;
    if (property->type.id == PARSER_PROPERTY_TYPE_CUSTOM)
        type_name = CodeGeneratorAllocStr(generator, (char*)property->type.name, property->type.name_len);
    else if (property->type.id == PARSER_PROPERTY_TYPE_CLASS)
        type_name = CodeGeneratorAllocStr(generator, (char*)property->type.name, property->type.name_len);
    else
        type_name = CodeGeneratorAllocStr(generator, (char*)property->type.name, strlen(property->type.name));
    
    char* field_name = CodeGeneratorAllocStr(generator, (char*)property->name, property->name_len);
    
    // TODO(Dustin): Handle const and pointer types 
    
    FormatIndent(buffer, indent);
    pb_write(buffer, "%s %s;\n", type_name, field_name);
}

static void
FormatNestedParserUnnamedClass(CodeGenerator *generator, PrettyBuffer *buffer, ParserClass *cls, ParserProperty *property, int depth)
{
    char* cls_name = CodeGeneratorAllocStr(generator, (char*)cls->name, cls->name_len);
    
    FormatIndent(buffer, depth);
    pb_write(buffer, "struct\n");
    
    FormatIndent(buffer, depth);
    pb_write(buffer, "{\n");
    
    depth += 1;
    
    // Emit nested types first
    for (u32 i = 0; i < (u32)arrlen(cls->nested_cls); ++i)
    {
        FormatNestedParserClass(generator, buffer, &cls->nested_cls[i], depth);
    }
    
    // Emit struct properties
    for (u32 c = 0; c < (u32)arrlen(cls->properties); ++c)
    {
        ParserProperty *property = &cls->properties[c];
        if (property->type.name)
            FormatStructProperty(generator, buffer, property, depth);
        else
        {
            int idx = property->type.name_len; 
            FormatNestedParserUnnamedClass(generator, buffer, cls->nested_cls + idx, property, depth);
        }
    }
    
    char* field_name = CodeGeneratorAllocStr(generator, (char*)property->name, property->name_len);
    
    FormatIndent(buffer, depth-1);
    pb_write(buffer, "} %s;\n\n", field_name);
}

static void
FormatNestedParserClass(CodeGenerator *generator, PrettyBuffer *buffer, ParserClass *cls, int depth)
{
    // unnamed classes are emitted with the property
    if (!cls->name) return;
    
    char* cls_name = CodeGeneratorAllocStr(generator, (char*)cls->name, cls->name_len);
    
    FormatIndent(buffer, depth);
    pb_write(buffer, "struct %s\n", cls_name);
    
    FormatIndent(buffer, depth);
    pb_write(buffer, "{\n");
    
    depth += 1;
    
    // Emit nested types first
    for (u32 i = 0; i < (u32)arrlen(cls->nested_cls); ++i)
    {
        FormatNestedParserClass(generator, buffer, &cls->nested_cls[i], depth);
    }
    
    // Emit struct properties
    for (u32 c = 0; c < (u32)arrlen(cls->properties); ++c)
    {
        ParserProperty *property = &cls->properties[c];
        if (property->type.name)
            FormatStructProperty(generator, buffer, property, depth);
        else
        {
            int idx = property->type.name_len; 
            FormatNestedParserUnnamedClass(generator, buffer, cls->nested_cls + idx, property, depth);
        }
    }
    
    FormatIndent(buffer, depth-1);
    pb_write(buffer, "};\n\n");
}

static void
FormatParserClass(CodeGenerator *generator, PrettyBuffer *buffer, ParserClass *cls)
{
    // names are not null terminated!
    // TODO(Dustin): Use a better allocation scheme?
    // NOTE(Dustin): When I move to a multithreaded generator, use a per-thread
    // string allocator. Should help some...
    char* cls_name = CodeGeneratorAllocStr(generator, (char*)cls->name, cls->name_len);
    pb_write(buffer, "struct %s\n{\n", cls_name);
    
    // Emit nested types first
    for (u32 i = 0; i < (u32)arrlen(cls->nested_cls); ++i)
    {
        FormatNestedParserClass(generator, buffer, &cls->nested_cls[i], 1);
    }
    
    for (u32 c = 0; c < (u32)arrlen(cls->properties); ++c)
    {
        ParserProperty *property = &cls->properties[c];
        
        if (property->type.name)
            FormatStructProperty(generator, buffer, property, 1);
        else
        {
            int idx = property->type.name_len; 
            FormatNestedParserUnnamedClass(generator, buffer, cls->nested_cls + idx, property, 1);
        }
    }
    
    pb_write(buffer, "};\n\n");
}

static void
MDFToHFile(CodeGenerator *generator, MdfFile *mdf_file)
{
    ParserRegistry *parser = &mdf_file->parsed_file;
    
    // Get just the filename (no extension)
    char *filename = StrGetString(&mdf_file->fullpath);
    u32 filepath_len = (u32)strlen(filename);
    {
        char *iter_end = filename + filepath_len;
        char *iter = iter_end - 1;
        
        while (iter != filename && *iter != '/' && *iter != '\\')
            --iter;
        
        if (*iter != '/' || *iter != '\\')
            ++iter;
        
        filename = iter;
        filepath_len = (u32)(iter_end - iter);
    }
    
    char *filepath_cpy = CodeGeneratorAllocStr(generator, filename, filepath_len);
    StrToUpperCase(filepath_cpy, filepath_len);
    
    PrettyBuffer buffer = {};
    pb_init(&buffer, 4096);
    
    char *to_upper = buffer.current;
    pb_write(&buffer, "#ifndef _%s\n", filepath_cpy);
    pb_write(&buffer, "#define _%s\n\n", filepath_cpy);
    
    // Generate Meta data for the struct
    for (u32 i = 0; i < (u32)arrlen(parser->classes); ++i)
    {
        ParserClass *pcls = &parser->classes[i];
        FormatParserClass(generator, &buffer, pcls);
    }
    
    pb_write(&buffer, "#endif //_%s\n", filepath_cpy);
    
    // TODO(Dustin): Use native file write instead...
    
    FILE *fp = fopen(StrGetString(&mdf_file->fullpath), "w");
    fwrite (buffer.start, 1, buffer.size, fp);
    fclose(fp);
    
    pb_free(&buffer);
}

static i32
FindMetaClassIdxByName(MetaClass *classes, u32 count, const char *name, u32 nlen)
{
    i32 result = -1;
    for (u32 i = 0; i < count; ++i)
    {
        if (classes[i].name && (strncmp(classes[i].name, name, nlen) == 0))
        {
            result = i;
            break;
        }
    }
    return result;
}

static MetaClassDecl
CodeGeneratorParseMetaClassDecl(CodeGenerator *generator, ParserClass *pcls)
{
    //
    // Register nested classes
    //
    
    MetaClass *nested_classes = 0; 
    arrsetcap(nested_classes, arrlen(pcls->nested_cls));
    
    for (u32 i = 0; i < (u32)arrlen(pcls->nested_cls); ++i)
    {
        ParserClass *child_cls = &pcls->nested_cls[i];
        MetaClass mcls = {};
        mcls.tags  = META_PROPERTY_TAG_NONE;
        mcls.name  = 0;
        mcls.count = 0;
        
        if (child_cls->tags == META_PROPERTY_TAG_NONE) 
        { // insert empty class into name_par
            arrput(nested_classes, mcls);
            continue;
        }
        
        if (child_cls->name)
            mcls.name = CodeGeneratorAllocStr(generator, (char*)child_cls->name, child_cls->name_len);
        
        MetaClassDecl idecl = CodeGeneratorParseMetaClassDecl(generator, child_cls);
        
        u32 cnt = (uint32_t)idecl.size / (u32)sizeof(MetaProperty);
        u32 cnt_nested = (uint32_t)idecl.nested_size / (u32)sizeof(MetaClass);
        
        mcls.tags = idecl.tags;
        mcls.count = cnt;
        mcls.nested_count = cnt_nested;
        
        mcls.properties = (MetaProperty*)malloc(idecl.size);
        memcpy(mcls.properties, idecl.properties, idecl.size);
        
        mcls.nested_classes = (MetaClass*)malloc(idecl.nested_size);
        memcpy(mcls.nested_classes, idecl.nested_classes, idecl.nested_size);
        
        arrput(nested_classes, mcls);
        
        arrfree(idecl.properties);
        arrfree(idecl.nested_classes);
    }
    
    u32 nested_cls_count = (u32)arrlen(nested_classes);
    
    //
    // Parse class properties
    //
    
    MetaProperty *property_list = 0;
    for (u32 c = 0; c < (u32)arrlen(pcls->properties); ++c)
    {
        ParserProperty *property = &pcls->properties[c];
        if (property->tags == META_PROPERTY_TAG_NONE) continue;
        
        MetaPropertyTypeInfo type;
        if (property->type.id == PARSER_PROPERTY_TYPE_CLASS)
        {
            i32 cls_idx = FindMetaClassIdxByName(nested_classes, nested_cls_count, 
                                                 property->type.name, property->type.name_len);
            Assert(cls_idx >= 0);
            
            type.id   = META_PROPERTY_TYPE_CLASS;
            type.name = (const char*)((uptr)cls_idx); // index into class list for type name
        }
        else if (property->type.id == PARSER_PROPERTY_TYPE_CLASS_UNNAMED)
        {
            type.id   = META_PROPERTY_TYPE_CLASS_UNNAMED;
            type.name = (const char*)((uptr)property->type.name_len); // index into class list for type name
        }
        else
        {
            type = ParserPropertyToMetaProperty(generator, property->type);
        }
        
        char* prop_name = CodeGeneratorAllocStr(generator, (char*)property->name, property->name_len);
        
        MetaProperty prop = _MetaPropertyImpl(prop_name, property->offset, type, property->tags);
        arrput(property_list, prop);
    }
    
    MetaClassDecl decl = { 
        property_list,  arrlen(property_list)  * sizeof(MetaProperty), 
        pcls->tags,
        nested_classes, arrlen(nested_classes) * sizeof(MetaClass),
    };
    
    return decl;
}

static void 
CodeGeneratorParserClassToMetaClass(CodeGenerator *generator, ParserClass *pcls, const char *class_name)
{
    if (pcls->tags == META_PROPERTY_TAG_NONE) return;
    
    MetaClassDecl decl = CodeGeneratorParseMetaClassDecl(generator, pcls);
    _MetaRegisterClassImpl(&generator->registry, class_name, &decl);
    
    arrfree(decl.properties);
    arrfree(decl.nested_classes);
}

// Takes a set of struct definitions from a file, generates the meta registry information from it
// and write the *.h file for the struct definitions without tags
static void
CodeGeneratorGenCode(CodeGenerator *generator, MdfFile *mdf_file)
{
    ParserRegistry *parser = &mdf_file->parsed_file;
    
    // Generate Meta data for the struct
    for (u32 i = 0; i < (u32)arrlen(parser->classes); ++i)
    {
        ParserClass *pcls = &parser->classes[i];
        
        char* cls_name = CodeGeneratorAllocStr(generator, (char*)pcls->name, pcls->name_len);
        CodeGeneratorParserClassToMetaClass(generator, pcls, cls_name);
    }
    
    MDFToHFile(generator, mdf_file);
}

static char*
MetaTypeToStr(u32 id)
{
    char *result;
    switch (id)
    {
        case META_PROPERTY_TYPE_U8:     result = "META_PROPERTY_TYPE_INFO_U8";     break;
        case META_PROPERTY_TYPE_U16:    result = "META_PROPERTY_TYPE_INFO_U16";    break;
        case META_PROPERTY_TYPE_U32:    result = "META_PROPERTY_TYPE_INFO_U32";    break;
        case META_PROPERTY_TYPE_U64:    result = "META_PROPERTY_TYPE_INFO_U64";    break;
        case META_PROPERTY_TYPE_S8:     result = "META_PROPERTY_TYPE_INFO_S8";     break;
        case META_PROPERTY_TYPE_S16:    result = "META_PROPERTY_TYPE_INFO_S16";    break;
        case META_PROPERTY_TYPE_S32:    result = "META_PROPERTY_TYPE_INFO_S32";    break;
        case META_PROPERTY_TYPE_S64:    result = "META_PROPERTY_TYPE_INFO_S64";    break;
        case META_PROPERTY_TYPE_SIZE_T: result = "META_PROPERTY_TYPE_INFO_SIZE_T"; break;
        case META_PROPERTY_TYPE_F32:    result = "META_PROPERTY_TYPE_INFO_F32";    break;
        case META_PROPERTY_TYPE_F64:    result = "META_PROPERTY_TYPE_INFO_F64";    break;
        case META_PROPERTY_TYPE_STR:    result = "META_PROPERTY_TYPE_INFO_STR";    break;
    }
    return result;
}

static char*
MetaCustomTypeToStr(CodeGenerator *generator, u32 id)
{
    char *result = "";
    for (u32 i = 0; i < (u32)shlen(generator->type_lut); ++i)
    {
        if (generator->type_lut[i].value == id)
        {
            char* type_name = CodeGeneratorAllocStr(generator, (char*)generator->type_lut[i].key, 
                                                    (u32)strlen(generator->type_lut[i].key));
            StrToUpperCase(type_name, (u32)strlen(generator->type_lut[i].key));
            result = type_name;
            break;
        }
    }
    return result;
}

static void 
CodeGeneratorFormatPropertyClass(CodeGenerator *generator, PrettyBuffer *buffer, MetaClass *cls, 
                                 const char *parent, const char *vars, bool found_unnamed)
{
    if (cls->tags == META_PROPERTY_TAG_NONE) return;
    
    for (u32 i = 0; i < cls->nested_count; ++i)
    {
        MetaClass *ncls = &cls->nested_classes[i];
        if (ncls->tags == META_PROPERTY_TAG_NONE) continue;
        
        if (found_unnamed || !ncls->name)
        {
            found_unnamed = true;
            
            if (!vars)
            { // need to find the variable associated with this unnamed class
                for (u32 p = 0; p < cls->count; ++p)
                {
                    MetaProperty *prop = &cls->properties[p];
                    if (prop->type.id == META_PROPERTY_TYPE_CLASS_UNNAMED && 
                        (u32)((uptr)prop->type.name) == p)
                    {
                        CodeGeneratorFormatPropertyClass(generator, buffer, ncls, parent, prop->name, found_unnamed);
                        break;
                    }
                }
            }
            else
            {
                CodeGeneratorFormatPropertyClass(generator, buffer, ncls, parent, vars, found_unnamed);
            }
        }
        else
        {
            u64 parent_len = strlen(parent);
            u64 child_len = strlen(ncls->name);
            u64 len = parent_len + child_len + 2;
            char* name = CodeGeneratorAllocStr(generator, NULL, len);
            
            memcpy(name + 0, parent, parent_len);
            memcpy(name + parent_len, "::", 2);
            memcpy(name + parent_len + 2, ncls->name, child_len);
            
            CodeGeneratorFormatPropertyClass(generator, buffer, ncls, name, vars, found_unnamed);
        }
    }
    
    pb_write(buffer, "\t{\n\t\tMetaProperty properties[] = {\n");
    
    for (u32 p = 0; p < cls->count; ++p)
    {
        MetaProperty *prop = &cls->properties[p];
        
        char *type_str = 0;
        if (prop->type.id < META_PROPERTY_TYPE_COUNT)
            type_str = MetaTypeToStr(prop->type.id);
        else
            type_str = MetaCustomTypeToStr(generator, prop->type.id);
        
        char *var_name = (char*)prop->name;
        if (found_unnamed && vars)
        {
            u64 var_len = strlen(var_name);
            u64 parent_var_len = strlen(vars);
            u64 len = parent_var_len + 1 + parent_var_len;
            char* name = CodeGeneratorAllocStr(generator, NULL, len);
            
            memcpy(name + 0, vars, parent_var_len);
            memcpy(name + parent_var_len, ".", 1);
            memcpy(name + parent_var_len + 1, var_name, var_len);
            
            var_name = name;
        }
        
        pb_write(buffer, "\t\t\tmeta_property(%s, %s, ", parent, var_name);
        
        // TODO(Dustin): Handle internal class types
        
        if (prop->type.id < META_PROPERTY_TYPE_CLASS)
            pb_write(buffer, "%s, ", type_str);
        else if (prop->type.id == META_PROPERTY_TYPE_CLASS)
        {
            MetaClass *type_class = cls->nested_classes + (int)((uptr)prop->type.name);
            
            if (!parent) parent = type_class->name;
            
            u32 parent_len = (u32)strlen(parent);
            u32 type_len = (u32)strlen(type_class->name);
            u32 len = parent_len + type_len + 2;
            
            char* type_name = CodeGeneratorAllocStr(generator, NULL, len);
            
            memcpy(type_name, parent, parent_len);
            memcpy(type_name + parent_len, "__", 2);
            memcpy(type_name + parent_len + 2, type_class->name, type_len);
            
            StrToUpperCase(type_name, len);
            // Convert all ':' to '_'
            for (u32 i = 0; i < len; ++i) if (type_name[i] == ':') type_name[i] = '_';
            
            pb_write(buffer, "META_PROPERTY_TYPE_INFO_%s, ", type_name);
        }
        else if (prop->type.id == META_PROPERTY_TYPE_CLASS_UNNAMED)
        {// TODO(Dustin): 
        }
        else
            pb_write(buffer, "META_PROPERTY_TYPE_INFO_%s, ", type_str);
        
        pb_write(buffer, "%d),\n", prop->tags);
    }
    
    pb_write(buffer, "\t\t};\n");
    pb_write(buffer, "\t\tMetaClassDecl decl = { properties, %d * sizeof(MetaProperty), %d };\n", cls->count, cls->tags);
    pb_write(buffer, "\t\tmeta_registry_register_class(registry, %s, &decl);\n", cls->name);
    pb_write(buffer, "\t}\n\n");
}

static void
CodeGeneratorGenMetaFile(CodeGenerator *generator, const char *outdir, MdfFile *files, u32 file_count)
{
    PrettyBuffer buffer = {};
    pb_init(&buffer, 4096);
    
    pb_write(&buffer, "#ifndef _REFLECTION_METADATA_H\n");
    pb_write(&buffer, "#define _REFLECTION_METADATA_H\n\n");
    
    pb_write(&buffer, "%s", metadata_include_section);
    
    // Write the custom meta types
    {
        for (u32 i = 0; i < (u32)shlen(generator->type_lut); ++i)
        {
            pb_write(&buffer, "#define META_PROPERTY_TYPE_%s %d\n", (char*)generator->type_lut[i].key, generator->type_lut[i].value);
        }
        pb_write(&buffer, "\n");
        
        for (u32 i = 0; i < (u32)shlen(generator->type_lut); ++i)
        {
            char* type_name = CodeGeneratorAllocStr(generator, (char*)generator->type_lut[i].key, 
                                                    (u32)strlen(generator->type_lut[i].key));
            StrToUpperCase(type_name, (u32)strlen(generator->type_lut[i].key));
            
            pb_write(&buffer, "#define META_PROPERTY_TYPE_INFO_%s ", type_name);
            pb_write(&buffer, "meta_property_type_info_decl(%s, META_PROPERTY_TYPE_%s)\n", 
                     generator->type_lut[i].key, generator->type_lut[i].key);
        }
        pb_write(&buffer, "\n");
    }
    
    pb_write(&buffer, "%s", metadata_callbacks);
    pb_write(&buffer, "%s", extern_print_fn_signature);
    pb_write(&buffer, "%s", metadata_fn_header_pre_decs);
    
    pb_write(&buffer, "#ifdef MAPLE_REFLECTION_META_IMPLEMENTATION\n\n");
    
    // Write include paths for each refl file
    {
        for (u32 i = 0; i < file_count; ++i)
        {
            MdfFile *file = &files[i];
            pb_write(&buffer, "#include <%s>\n", file->fullpath);
        }
        pb_write(&buffer, "\n");
    }
    
    pb_write(&buffer, "%s", metadata_global_section);
    pb_write(&buffer, "%s", metadata_fn_src_pre_decs);
    
    pb_write(&buffer, "%s", intern_print_fn_signature);
    pb_write(&buffer, "%s", metadata_refl_meta_init_impl);
    pb_write(&buffer, "%s", metadata_refl_meta_free_impl);
    pb_write(&buffer, "%s", metadata_set_callbacks_function_impl);
    
    // Function to register all meta classes
    pb_write(&buffer, "static void RegisterMetaClasses(MetaRegistry *registry)\n{\n");
    {
        MetaRegistry *registry = &generator->registry;
        for (u32 i = 0; i < (u32)shlen(registry->classes); ++i)
        {
            MetaClass *cls = &registry->classes[i].value;
            CodeGeneratorFormatPropertyClass(generator, &buffer, cls, cls->name, NULL, false);
        }
    }
    pb_write(&buffer, "}\n\n");
    
    // Function to print registered classes
    pb_write(&buffer, "static MetaRegistry MetadataRegisteryInit()\n{\n");
    {
        pb_write(&buffer, "\tMetaRegistry registry = MetaRegistryNew();\n");
        pb_write(&buffer, "\tRegisterMetaClasses(&registry);\n");
        pb_write(&buffer, "\treturn registry;\n");
    }
    pb_write(&buffer, "}\n\n");
    
    // Free the metadata
    pb_write(&buffer, "static void MetadataRegisteryFree(MetaRegistry *registry)\n{\n");
    {
        pb_write(&buffer, "\tMetaRegistryFree(registry);\n");
    }
    pb_write(&buffer, "}\n\n");
    
    pb_write(&buffer, "%s", default_logger);
    pb_write(&buffer, "%s", default_file_reader);
    pb_write(&buffer, "%s", default_file_writer);
    
    // Function to print registered classes
    pb_write(&buffer, "static void PrintObj_Internal(MetaRegistry *registry, void *obj, MetaClass *cls, u32 depth)\n{\n");
    {
        pb_write(&buffer, "%s", print_state_machine);
        
        for (u32 i = 0; i < (u32)shlen(generator->type_lut); ++i)
        {
            char *key = generator->type_lut[i].key;
            pb_write(&buffer, "\t\t\tcase META_PROPERTY_TYPE_%s:\n\t\t\t{\n", key);
            pb_write(&buffer, "\t\t\t\t%s *v = meta_registry_getvp(obj, %s, prop);\n", key, key);
            pb_write(&buffer, "\t\t\t\tMetaClass *ocls = meta_registry_class_getp(registry, %s);\n", key);
            pb_write(&buffer, "\t\t\t\tPrintObj_Internal(registry, (void*)v, ocls, depth);\n");
            pb_write(&buffer, "\t\t\t} break;\n\n");
        }
        
        pb_write(&buffer, "\t\t}\n");
        pb_write(&buffer, "\t}\n");
    }
    pb_write(&buffer, "\tfor (u32 i = 0; i < depth-1; ++i) g_reflection_system.callbacks.Log(\"\\t\");\n");
    pb_write(&buffer, "\tg_reflection_system.callbacks.Log(\"}\\n\");\n}\n\n");
    
    pb_write(&buffer, "#endif //MAPLE_REFLECTION_META_IMPLEMENTATION\n");
    pb_write(&buffer, "#endif //_REFLECTION_METADATA_H\n\n");
    
    char* filepath;
    {
        const char *filename = "ReflectionMetadata.h";
        u32 filename_len = (u32)strlen(filename);
        
        u32 dir_len  = (u32)strlen(outdir);
        bool ends_with_slash = (dir_len > 0) ? outdir[dir_len-1] == '/' || outdir[dir_len-1] == '\\' : false;
        
        u32 filepath_len = dir_len + filename_len;
        if (!ends_with_slash) filepath_len++;
        
        filepath = CodeGeneratorAllocStr(generator, NULL, filepath_len);
        
        u32 offset = 0;
        memcpy(filepath + offset, outdir, dir_len);
        offset += dir_len;
        
        if (!ends_with_slash) filepath[offset++] = '/';
        
        memcpy(filepath + offset, filename, filename_len);
    }
    
    FILE *fp = fopen(filepath, "w");
    fwrite (buffer.start, 1, buffer.size, fp);
    fclose(fp);
    
    pb_free(&buffer);
}
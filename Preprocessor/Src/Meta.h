#ifndef _META_H
#define _META_H

#include <stdlib.h>
#include <stdint.h>

enum MetaPropertyType
{
    META_PROPERTY_TYPE_U8 = 0x00,
    META_PROPERTY_TYPE_U16,
    META_PROPERTY_TYPE_U32,
    META_PROPERTY_TYPE_U64,
    
    META_PROPERTY_TYPE_S8,
    META_PROPERTY_TYPE_S16,
    META_PROPERTY_TYPE_S32,
    META_PROPERTY_TYPE_S64,
    
    META_PROPERTY_TYPE_SIZE_T,
    
    META_PROPERTY_TYPE_F32,
    META_PROPERTY_TYPE_F64,
    
    META_PROPERTY_TYPE_STR,
    
    META_PROPERTY_TYPE_CLASS,         // for nested custom types
    META_PROPERTY_TYPE_CLASS_UNNAMED, // for nested custom unnamed types
    
    META_PROPERTY_TYPE_COUNT,
};

enum MetaPropertyTag
{
    // Default: No tags
    META_PROPERTY_TAG_NONE         = 0<<0,
    // General meta tag - will only create metadata
    META_PROPERTY_TAG_META         = 1<<0,
    
    // Enable tags for structs/enums
    META_PROPERTY_TAG_PRINT        = 1<<1,
    META_PROPERTY_TAG_SERIALIZE    = 1<<2,
};

struct MetaPropertyTypeInfo
{
    const char *name; // display name
    uint32_t    id;   // Matches to property type
};

struct MetaProperty
{
    const char          *name;   // Display name
    size_t               offset; // offset in bytes in the struct
    uint32_t             tags;   // tags {Meta, Serialize, Print}
    MetaPropertyTypeInfo type;   // type info
};

struct MetaClass
{
    const char   *name;  // display name
    uint32_t      count; // count of all props in list
    uint32_t      tags;  // tags {Meta, Serialize, Print}
    MetaProperty *properties;
    
    // A Meta Class can contain various nested classes.
    // MetaProperty.type.id is set to META_PROPERTY_TYPE_CLASS(_UNNAMED)
    // MetaProperty.type.name is coerced to the be index of the class within
    // the nested_cls array.
    uint32_t      nested_count;   // number of unnamed classes within this class  
    MetaClass    *nested_classes;
};

struct MetaRegistry
{
    struct { uint64_t key; MetaClass value; } *classes;
};

struct MetaClassDecl
{
    MetaProperty *properties; // array of properties
    size_t        size;       // size of array in bytes
    uint32_t      tags;       // tags for metadata
    
    MetaClass    *nested_classes;
    size_t        nested_size;   // number of unnamed classes within this class  
};

static MetaRegistry MetaRegistryNew();
static void MetaRegistryFree(MetaRegistry *registry);
static uint64_t _MetaRegisterClassImpl(MetaRegistry *registry, const char *name, const MetaClassDecl *decl);
static MetaProperty _MetaPropertyImpl(const char *name, size_t offset, MetaPropertyTypeInfo type, uint32_t tags);
static MetaPropertyTypeInfo _MetaPropertyTypeInfoDeclImpl(const char *name, uint32_t id);
static MetaClass* _MetaClassGetPImpl(MetaRegistry *registry, const char *name);

#define meta_to_str(T) (const char *)#T
#define meta_offset(T, E) (size_t)(&(((T*)(0))->E))

#define meta_registry_register_class(META, T, DECL) \
_MetaRegisterClassImpl((META), meta_to_str(T), (DECL))

#define meta_property(CLS, FIELD, TYPE, FLAGS) \
_MetaPropertyImpl(meta_to_str(FIELD), meta_offset(CLS, FIELD), TYPE, (FLAGS))

#define meta_property_type_info_decl(T, PROP_TYPE) \
_MetaPropertyTypeInfoDeclImpl(meta_to_str(T), (PROP_TYPE))

#define meta_registry_class_getp(META, T) \
_MetaClassGetPImpl(META, meta_to_str(T))

#define meta_registry_getvp(OBJ, T, PROP) \
((T*)((uint8_t*)(OBJ) + (PROP)->offset))

#define meta_registry_getv(OBJ, T, PROP) \
(*((T*)((uint8_t*)(OBJ) + (PROP)->offset)))

#define META_PROPERTY_TYPE_INFO_U8     meta_property_type_info_decl(uint8_t,      META_PROPERTY_TYPE_U8)
#define META_PROPERTY_TYPE_INFO_U16    meta_property_type_info_decl(uint16_t,     META_PROPERTY_TYPE_U16)
#define META_PROPERTY_TYPE_INFO_U32    meta_property_type_info_decl(uint32_t,     META_PROPERTY_TYPE_U32)
#define META_PROPERTY_TYPE_INFO_U64    meta_property_type_info_decl(uint64_t,     META_PROPERTY_TYPE_U64)
#define META_PROPERTY_TYPE_INFO_S8     meta_property_type_info_decl(int8_t,       META_PROPERTY_TYPE_S8)
#define META_PROPERTY_TYPE_INFO_S16    meta_property_type_info_decl(int16_t,      META_PROPERTY_TYPE_S16)
#define META_PROPERTY_TYPE_INFO_S32    meta_property_type_info_decl(int32_t,      META_PROPERTY_TYPE_S32)
#define META_PROPERTY_TYPE_INFO_S64    meta_property_type_info_decl(int64_t,      META_PROPERTY_TYPE_S64)
#define META_PROPERTY_TYPE_INFO_F32    meta_property_type_info_decl(float,        META_PROPERTY_TYPE_F32)
#define META_PROPERTY_TYPE_INFO_F64    meta_property_type_info_decl(double,       META_PROPERTY_TYPE_F64)
#define META_PROPERTY_TYPE_INFO_SIZE_T meta_property_type_info_decl(size_t,       META_PROPERTY_TYPE_SIZE_T)
#define META_PROPERTY_TYPE_INFO_STR    meta_property_type_info_decl(char *,       META_PROPERTY_TYPE_STR)

#if defined(MAPLE_META_IMPLEMENTATION)

static MetaRegistry 
MetaRegistryNew()
{
    MetaRegistry result = {};
    return result;
}

static void 
MetaRegistryFree(MetaRegistry *registry)
{
    // TODO(Dustin):
}

static uint64_t 
_MetaRegisterClassImpl(MetaRegistry *registry, const char *name, const MetaClassDecl *decl)
{
    MetaClass cls = {};
    
    uint32_t cnt = (uint32_t)decl->size / (uint32_t)sizeof(MetaProperty);
    uint32_t cnt_nested = (uint32_t)decl->nested_size / (uint32_t)sizeof(MetaClass);
    
    cls.name = name;
    cls.tags = decl->tags;
    cls.count = cnt;
    cls.nested_count = cnt_nested;
    
    cls.properties = (MetaProperty*)malloc(decl->size);
    memcpy(cls.properties, decl->properties, decl->size);
    
    cls.nested_classes = (MetaClass*)malloc(decl->nested_size);
    memcpy(cls.nested_classes, decl->nested_classes, decl->nested_size);
    
    uint64_t id = (uint64_t)stbds_hash_string((char*)name, 0);
    hmput(registry->classes, id, cls);
    
    return id;
}

static MetaProperty 
_MetaPropertyImpl(const char *name, size_t offset, MetaPropertyTypeInfo type, uint32_t tags)
{
    MetaProperty result = {};
    result.name = name;
    result.offset = offset;
    result.type = type;
    result.tags = tags;
    return result;
}

static MetaPropertyTypeInfo 
_MetaPropertyTypeInfoDeclImpl(const char *name, uint32_t id)
{
    MetaPropertyTypeInfo result = {};
    result.name = name;
    result.id = id;
    return result;
}

static MetaClass* 
_MetaClassGetPImpl(MetaRegistry *registry, const char *name)
{
    uint64_t key = (uint64_t)stbds_hash_string((char*)name, 0);
    return &hmgetp(registry->classes, key)->value;
}

#endif // MAPLE_META_IMPLEMENTATION
#endif //_META_H



static void
SerializeMetafile(AssetMetadata metadata)
{
    Toml toml = TomlCreate();
    toml.title = StrGetString(&metadata.virtual_name);
    
    TomlObject obj = TomlCreateObject();
    
    Str guid = PlatformGuidToString(metadata.guid);
    TomlObjectAddData(&obj, TomlDataString(StrGetString(&guid), StrLen(&guid)), "GUID");
    
    if (PlatformIsGuidValid(metadata.icon))
    {
        Str icon = PlatformGuidToString(metadata.icon);
        TomlObjectAddData(&obj, TomlDataString(StrGetString(&icon), StrLen(&icon)), "Icon");
    }
    else
    {
        TomlObjectAddData(&obj, TomlDataString(NULL, 0), "Icon");
    }
    
    Str dep_str_array = 0;
    arrsetcap(dep_str_array, arrlen(metadata.depenencies));
    
    TomlData dep_array = TomlDataArray();
    for (u32 i = 0; i < (u32)arrlen(metadata.depenencies); ++i)
    {
        Str str = PlatformGuidToString(metadata.depenencies[i]);
        TomlArrayAddString(dep_array, TomlDataString(StrGetString(&str), StrLen(&str)));
        arrput(dep_str_array, str);
    }
    
    TomlObjectAddData(&obj, &dep_array, "Dependencies");
    TomlAddObject(&toml, &obj, "Metadata");
    
    char *str; int len;
    TomlToString(&toml, &str, &len);
    
    PlatformFile *file = PlatformGetFile(metadata.file);
    PlatformWriteBufferToFile(StrGetString(&file->physical_name), file_path, (u8*)buffer, (u64)len);
    
    TomlFreeString(&str);
    TomlFree(&toml);
    
    for (u32 i = 0; i < (u32)arrlen(metadata.depenencies); ++i)
        StrFree(&dep_str_array[i]);
    arrfree(dep_str_array);
    
    StrFree(&guid);
    StrFree(&icon);
}

static void
DeserializeMetafile(AssetMetadata *metadata, FILE_ID fid)
{
    PlatformFile *file = PlatformGetFile(metadata.file);
    Assert(file);
    
    Toml toml;
    TomlResult result = TomlLoad(&toml, StrGetString(&file->physical_name));
    Assert(result == TomlResult_Success);
    
    metadata->file = fid;
    metadata->virtual_name = StrInit(strlen(toml.title), toml.title);
    
    TomlObject obj = TomlGetObject(&toml, "Metadata");
    
    const char* guid = TomlGetString(&obj, "GUID");
    Assert(guid);
    metadata->guid = PlatformStringToGuid(guid);
    
    if (TomlGetStringLen(&obj, "Icon") > 0)
    {
        const char* icon = TomlGetString(&obj, "Icon");
        metadata->icon = PlatformStringToGuid(icon);
    }
    else
        metadata->icon = PlatformGetInvalidGuid();
    
    metadata->depenencies = 0;
    
    TomlData* dep_array = TomlGetArray(&obj, "Dependencies");
    int dep_count = TomlGetArrayLen(dep_array);
    for (int i = 0; i < (int)dep_count; ++i)
    {
        const char *dep_guid = TomlGetStringArrayElem(dep_array, i);
        arrput(metadata->depenencies, PlatformStringToGuid(dep_guid));
    }
    
    TomlFree(&toml);
}
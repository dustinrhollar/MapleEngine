
static Str PlatformNormalizePath(const char* path);
static Str Win32GetExeFilepath();

#if 0
PlatformError PlatformCreateDirectory(const char *abs_path)
{
    PlatformError result = PlatformError::Success;
    
    BOOL err = CreateDirectoryA(abs_path, NULL);
    if (err == 0)
    {
        DWORD last_err = GetLastError();
        if (last_err == ERROR_ALREADY_EXISTS)
        {
            result = PlatformError::DirectoryAlreadyExists;
        }
        else if (last_err == ERROR_PATH_NOT_FOUND)
        {
            result = PlatformError::PathNotFound;
        }
    }
    
    return result;
}
#endif

Str PlatformGetFullExecutablePath()
{
    char buf[MAX_PATH]; // hmmm, max_path usage...
    if (GetCurrentDirectory(MAX_PATH, buf) > MAX_PATH) return {};
    for (u32 i = 0; buf[i]; ++i) if (buf[i] == '\\') buf[i] = '/';
    
    Str result = StrInit((u32)strlen(buf), buf);
    return result;
}

PlatformErrorType 
PlatformReadFileToBuffer(const char* file_path, u8** buffer, u32* size)
{
    PlatformErrorType result = PlatformError_Success;
    
    *size = 0;
    *buffer = 0;
    HANDLE handle = CreateFileA(file_path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 0);
    
    if (handle == INVALID_HANDLE_VALUE) 
    {
        //LogError("Unable to open file!\n");
        return PlatformError_FileOpenFailure;
    }
	
    u32 size_or_error = GetFileSize(handle, 0);
	if (size_or_error == INVALID_FILE_SIZE) 
    {
        //LogError("Unable to open file: size failure!\n");
        return PlatformError_FileOpenFailure;
    }
    else *size = size_or_error;
    
	*buffer = (u8*)SysAlloc(*size + 1);
    
	DWORD dummy;
    if (!ReadFile(handle, *buffer, *size, &dummy, 0))
    {
        //LogError("Unable to read file!\n");
        CloseHandle(handle);
        SysFree(*buffer);
		*size = 0;
        
        result = PlatformError_FileReadFailure;
    }
    CloseHandle(handle);
    
    (*buffer)[*size] = 0;
    
    return result;
}

PlatformErrorType 
PlatformWriteBufferToFile(const char* file_path, u8* buffer, u64 size, bool append)
{
    PlatformErrorType result = PlatformError_Success;
    Str long_path = PlatformNormalizePath(file_path);
    
    HANDLE handle = 0;
    if (append)
    {
        handle = CreateFileA(StrGetString(&long_path), GENERIC_WRITE, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 0);
    }
    else
    {
        handle = CreateFileA(StrGetString(&long_path), GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 0);
        
        //if (handle == INVALID_HANDLE_VALUE)
        //{
		//handle = CreateFileA(long_path, GENERIC_WRITE, 0, 0, CREATE_NEW, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 0);
        //}
    }
    if (handle == INVALID_HANDLE_VALUE) return PlatformError_FileOpenFailure;
    
    DWORD dummy;
    u32 count = (u32)(size / U32_MAX);
    u32 mod = size % U32_MAX;
    for (u32 i = 0; i < count; ++i)
    {
        if (!WriteFile(handle, buffer + (u64)(i * U32_MAX), U32_MAX, &dummy, 0))
        {
            CloseHandle(handle);
            return PlatformError_FileWriteFailure;
        }
    }
    
    if (!WriteFile(handle, buffer + (u64)(U32_MAX * count), mod, &dummy, 0))
    {
        CloseHandle(handle);
        return PlatformError_FileWriteFailure;
    }
    CloseHandle(handle);
    return result;
}

static Str 
Win32GetExeFilepath()
{
    char buf[MAX_PATH];
    if (GetCurrentDirectory(MAX_PATH, buf) > MAX_PATH) 
    {
        Str result = {};
        return result;
    }
    
    for (u32 i = 0; buf[i]; ++i) if (buf[i] == '\\') buf[i] = '/';
    
    u32 buf_len = (u32)strlen(buf);
    if (buf[buf_len-1] != '/') buf[buf_len++] = '/';
    buf[buf_len] = 0;
    
    Str result = StrInit(buf_len, buf);
    return result;
}

static Str 
PlatformNormalizePath(const char* path)
{
    // If the string is null or has length < 2, just return an empty one.
    if (!path || !path[0] || !path[1]) 
    {
        Str err = {};
        return err;
    }
    
    // Start with our relative path appended to the full executable path.
    // TODO(Dustin): Don't do this in the long run. Let the caller set this up.
#if 0
    Str exe_path = Win32GetExeFilepath();
    
    Str result = StrAdd(&exe_path, path, (u32)strlen(path));
#else
    
    Str result = StrInit((u32)strlen(path), path);
    
#endif
    
    char *tmp = StrGetString(&result);
    u64 result_len = StrLen(&result);
    
    // Swap any back slashes for forward slashes.
    for (u32 i = 0; i < (u32)result_len; ++i) if (tmp[i] == '\\') tmp[i] = '/';
    
    // Strip double separators.
    for (u32 i = 0; i < (u32)result_len - 1; ++i)
    {
        if (tmp[i] == '/' && tmp[i + 1] == '/')
        {
            for (u32 j = i; j < (u32)result_len; ++j) tmp[j] = tmp[j + 1];
            --result_len;
            --i;
        }
    }
    
    // Evaluate any relative specifiers (./).
    if (tmp[0] == '.' && tmp[1] == '/')
    {
        for (u32 i = 0; i < (u32)result_len - 1; ++i) tmp[i] = tmp[i + 2];
        result_len -= 2;
    }
    for (u32 i = 0; i < (u32)result_len - 1; ++i)
    {
        if (tmp[i] != '.' && tmp[i + 1] == '.' && tmp[i + 2] == '/')
        {
            for (u32 j = i + 1; tmp[j + 1]; ++j) tmp[j] = tmp[j + 2];
            result_len -= 2;
        }
    }
    
    // Evaluate any parent specifiers (../).
    u32 last_separator = 0;
    for (u32 i = 0; (i < (u32)result_len - 1); ++i)
    {
        if (tmp[i] == '.' && tmp[i + 1] == '.' && tmp[i + 2] == '/')
        {
            u32 base = i + 2;
            u32 count = (u32)result_len - base;
            
            for (u32 j = 0; j <= count; ++j)
            {
                tmp[last_separator + j] = tmp[base + j];
            }
            
            result_len -= base - last_separator;
            i = last_separator;
            
            if (i > 0)
            {
                bool has_separator = false;
                for (i32 j = last_separator - 1; j >= 0; --j)
                {
                    if (tmp[j] == '/')
                    {
                        last_separator = j;
                        has_separator = true;
                        break;
                    }
                }
                if (!has_separator) 
                {
                    Str r = {};
                    return r;
                }
            }
        }
        if (i > 0 && tmp[i - 1] == '/') last_separator = i - 1;
    }
    
    // Update length of the string
    StrSetLen(&result, result_len);
    
    //StrFree(&exe_path);
    return result;
}

#if 0
static Str Win32ShowBasicFileDialog(i32 dialog_type, i32 resource_type)
{
	Str result;
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | 
								COINIT_DISABLE_OLE1DDE);
	if (SUCCEEDED(hr))
	{
		switch (dialog_type)
		{
			case 0: // Open dialog
			{
				IFileOpenDialog *pFileOpen;
				
				// Create the FileOpenDialog object.
				hr = CoCreateInstance((const IID *const)CLSID_FileOpenDialog, NULL, CLSCTX_ALL, 
									  IID_IFileOpenDialog, (void**)&pFileOpen);
				
				if (SUCCEEDED(hr))
				{
					if (resource_type == 0)
					{
						COMDLG_FILTERSPEC rgSpec[] =
						{ 
							{ L"region file", L"*.region" }
						};
						pFileOpen->SetFileTypes(ARRAYCOUNT(rgSpec), rgSpec);
						pFileOpen->SetDefaultExtension(L"region");
					}
					else if (resource_type == 1)
					{
						COMDLG_FILTERSPEC rgSpec[] =
						{
							{ L"any image", L"*.png;*.bmp;*.jpg;*.jpeg" },
							{ L"png image", L"*.png" },
							{ L"bmp image", L"*.bmp" },
							{ L"jpeg image", L"*.jpg;*.jpeg" },
						};
						pFileOpen->SetFileTypes(ARRAYCOUNT(rgSpec), rgSpec);
						pFileOpen->SetDefaultExtension(L"png");
					}
					
					// Show the Open dialog box.
					hr = pFileOpen->Show(NULL);
					
					// Get the file name from the dialog box.
					if (SUCCEEDED(hr))
					{
						IShellItem *pItem;
						hr = pFileOpen->GetResult(&pItem);
						if (SUCCEEDED(hr))
						{
							PWSTR pszFilePath;
							hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
							
							// Display the file name to the user.
							if (SUCCEEDED(hr))
							{
								int size = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, 0, 0, 0, 0);
								result = Str8(size);
								WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, result, size, 0, 0);
								result[size] = 0;
								result.size = size;
								CoTaskMemFree(pszFilePath);
							}
							pItem->Release();
						}
					}
					pFileOpen->Release();
				}
			}
			break;
			case 1: // Save dialog
			{
				IFileSaveDialog *pFileSave;
				
				// Create the FileOpenDialog object.
				hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL, 
									  IID_IFileSaveDialog, (void**)&pFileSave);
				
				if (SUCCEEDED(hr))
				{
					if (resource_type == 0)
					{
						COMDLG_FILTERSPEC rgSpec[] =
						{ 
							{ L"region file", L"*.region" }
						};
						pFileSave->SetFileTypes(ARRAYCOUNT(rgSpec), rgSpec);
						pFileSave->SetDefaultExtension(L"region");
					}
					else if (resource_type == 1)
					{
						COMDLG_FILTERSPEC rgSpec[] =
						{
							{ L"any image", L"*.png;*.bmp;*.jpg;*.jpeg" },
							{ L"png image", L"*.png" },
							{ L"bmp image", L"*.bmp" },
							{ L"jpeg image", L"*.jpg;*.jpeg" },
						};
						pFileSave->SetFileTypes(ARRAYCOUNT(rgSpec), rgSpec);
						pFileSave->SetDefaultExtension(L"png");
					}
					
					// Show the Open dialog box.
					hr = pFileSave->Show(NULL);
					
					// Get the file name from the dialog box.
					if (SUCCEEDED(hr))
					{
						IShellItem *pItem;
						hr = pFileSave->GetResult(&pItem);
						if (SUCCEEDED(hr))
						{
							PWSTR pszFilePath;
							hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
							
							// Display the file name to the user.
							if (SUCCEEDED(hr))
							{
								int size = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, 0, 0, 0, 0);
								result = Str8(size);
								WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, result, size, 0, 0);
								result[size] = 0;
								result.size = size;
								CoTaskMemFree(pszFilePath);
							}
							pItem->Release();
						}
					}
					pFileSave->Release();
				}
			}
			break;
			//default: Assert(false);
		}
		
		CoUninitialize();
	}
	return result;
}

Str PlatformShowBasicFileDialog(int type, int resource_type)
{
	return Win32ShowBasicFileDialog(type, resource_type);
}
#endif
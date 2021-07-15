#ifndef LOG_BUFFER_SIZE
#define LOG_BUFFER_SIZE 512
#endif

#define CONSOLE_COLOR_BLACK 0

typedef enum
{
    ConsoleColor_Black = 0,  ConsoleColor_White,
    ConsoleColor_DarkGrey,   ConsoleColor_Grey,
    ConsoleColor_DarkRed,    ConsoleColor_Red,
    ConsoleColor_DarkGreen,  ConsoleColor_Green,
    ConsoleColor_DarkBlue,   ConsoleColor_Blue,
    ConsoleColor_DarkCyan,   ConsoleColor_Cyan,
    ConsoleColor_DarkPurple, ConsoleColor_Purple,
    ConsoleColor_DarkYellow, ConsoleColor_Yellow,
} EConsoleColor;

file_global const char *g_log_level_strings[] = {
    "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

file_global const WORD g_log_level_colors[] = {
    ConsoleColor_Blue,   // Trace: blue
    ConsoleColor_Green,  // Debug: Green
    ConsoleColor_White,  // Info: White
    ConsoleColor_Yellow, // Warn: Yellow
    ConsoleColor_Red,    // Error: Red 
    ConsoleColor_DarkRed // Fatal: Dark Red
};

typedef struct
{
    char *start;
    char *brkp;
} Win32InternalAlloc;
static const u32 InternalAllocDefaultSize = _1MB;

typedef struct
{
    HANDLE handle; // Stream handle (STD_OUTPUT_HANDLE or STD_ERROR_HANDLE).
    bool is_redirected; // True if redirected to file.
    bool is_wide; // True if appending to a UTF-16 file.
    bool is_little_endian; // True if file is UTF-16 little endian.
} Win32StandardStream;

constexpr int SCRATCH_SIZE = 4096;
struct Win32VirtualConsole
{
    Win32InternalAlloc internal_allocator;
    char    scratch_buffer[SCRATCH_SIZE];
    wchar_t wlog_buffer[LOG_BUFFER_SIZE];
    
    Win32StandardStream output_stream;
    Win32StandardStream error_stream;
    
    CRITICAL_SECTION cs_lock;
};

static Win32VirtualConsole g_virtual_console;

file_internal void* Win32LocalAlloc(Win32InternalAlloc *internal_allocator, u32 size);
file_internal Win32StandardStream Win32GetStandardStream(u32 stream_type);
file_internal i32 __Win32FormatString(char *buff, i32 len, const char *fmt, va_list list);
file_internal WORD Win32TranslateConsoleColors(EConsoleColor text_color, EConsoleColor background_color);
file_internal void Win32PrintToStream(const char* message, Win32StandardStream stream, EConsoleColor text_color, EConsoleColor background_color);

void 
PlatformLoggerInit()
{
    g_virtual_console = {0};
    
    g_virtual_console.internal_allocator.start = (char*)VirtualAlloc(NULL,                   
                                                                     InternalAllocDefaultSize,
                                                                     MEM_COMMIT|MEM_RESERVE,
                                                                     PAGE_READWRITE);
    g_virtual_console.internal_allocator.brkp = g_virtual_console.internal_allocator.start;
    
    g_virtual_console.output_stream = Win32GetStandardStream(STD_OUTPUT_HANDLE);
    g_virtual_console.error_stream = Win32GetStandardStream(STD_ERROR_HANDLE);
    
    InitializeCriticalSectionAndSpinCount(&g_virtual_console.cs_lock, 0x00001000);
}

void
PlatformLoggerFree()
{
    DeleteCriticalSection(&g_virtual_console.cs_lock);
    PlatformVirtualFree(g_virtual_console.internal_allocator.start);
    g_virtual_console.internal_allocator.start = 0;
    g_virtual_console.internal_allocator.brkp = 0;
}

// An internal allocation scheme that allocates a small amount of memory. 
// Memory is treated as a linear allocator.
// this functions is primarily used by print/formatting functions that need temporary,
// dynamic memory. When the heap is filled, the allocator is reset.
file_internal void* 
Win32LocalAlloc(Win32InternalAlloc *internal_allocator, u32 size)
{        
    u64 current_size = internal_allocator->brkp - internal_allocator->start;
    if (current_size + size > InternalAllocDefaultSize)
    {
        internal_allocator->brkp = internal_allocator->start;
    }
    
    void *result = internal_allocator->brkp;
    internal_allocator->brkp += size;
    
    return result;
}

// Sets up a standard stream (stdout or stderr).
file_internal Win32StandardStream 
Win32GetStandardStream(u32 stream_type)
{
    Win32StandardStream result{0};
    
    // If we don't have our own stream and can't find a parent console,
    // allocate a new console.
    result.handle = GetStdHandle(stream_type);
    if (!result.handle || result.handle == INVALID_HANDLE_VALUE)
    {
        if (!AttachConsole(ATTACH_PARENT_PROCESS)) AllocConsole();
        result.handle = GetStdHandle(stream_type);
    }
    
    // Check if the stream is redirected to a file. If it is, check if
    // the file already exists. If so, parse the encoding.
    if (result.handle != INVALID_HANDLE_VALUE)
    {
        DWORD type = GetFileType(result.handle) & (~FILE_TYPE_REMOTE);
        DWORD dummy;
        result.is_redirected = (type == FILE_TYPE_CHAR) ? !GetConsoleMode(result.handle, &dummy) : true;
        if (type == FILE_TYPE_DISK)
        {
            LARGE_INTEGER file_size;
            GetFileSizeEx(result.handle, &file_size);
            if (file_size.QuadPart > 1)
            {
                LARGE_INTEGER int_def;
                int_def.QuadPart = 0;
                
                u16 bom = 0;
                SetFilePointerEx(result.handle, int_def, 0, FILE_BEGIN);
                ReadFile(result.handle, &bom, 2, &dummy, 0);
                SetFilePointerEx(result.handle, int_def, 0, FILE_END);
                result.is_wide = (bom == (u16)0xfeff || bom == (u16)0xfffe);
                result.is_little_endian = (bom == (u16)0xfffe);
            }
        }
    }
    return result;
}

// Translates foreground/background color into a WORD text attribute.
file_internal WORD 
Win32TranslateConsoleColors(EConsoleColor text_color, EConsoleColor background_color)
{
    WORD result = 0;
    switch (text_color)
    {
        case ConsoleColor_White:
        result |=  FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        break;
        case ConsoleColor_DarkGrey:
        result |= FOREGROUND_INTENSITY;
        break;
        case ConsoleColor_Grey:
        result |= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        break;
        case ConsoleColor_DarkRed:
        result |= FOREGROUND_RED;
        break;
        case ConsoleColor_Red:
        result |= FOREGROUND_RED | FOREGROUND_INTENSITY;
        break;
        case ConsoleColor_DarkGreen:
        result |= FOREGROUND_GREEN;
        break;
        case ConsoleColor_Green:
        result |= FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        break;
        case ConsoleColor_DarkBlue:
        result |= FOREGROUND_BLUE;
        break;
        case ConsoleColor_Blue:
        result |= FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        break;
        case ConsoleColor_DarkCyan:
        result |= FOREGROUND_GREEN | FOREGROUND_BLUE;
        break;
        case ConsoleColor_Cyan:
        result |= FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        break;
        case ConsoleColor_DarkPurple:
        result |= FOREGROUND_RED | FOREGROUND_BLUE;
        break;
        case ConsoleColor_Purple:
        result |= FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        break;
        case ConsoleColor_DarkYellow:
        result |= FOREGROUND_RED | FOREGROUND_GREEN;
        break;
        case ConsoleColor_Yellow:
        result |= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        break;
        default:
        break;
    }
    
    switch (background_color)
    {
        case ConsoleColor_White:
        result |=  FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        break;
        case ConsoleColor_DarkGrey:
        result |=  FOREGROUND_INTENSITY;
        break;
        case ConsoleColor_Grey:
        result |= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        break;
        case ConsoleColor_DarkRed:
        result |= FOREGROUND_RED;
        break;
        case ConsoleColor_Red:
        result |= FOREGROUND_RED | FOREGROUND_INTENSITY;
        break;
        case ConsoleColor_DarkGreen:
        result |= FOREGROUND_GREEN;
        break;
        case ConsoleColor_Green:
        result |= FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        break;
        case ConsoleColor_DarkBlue:
        result |= FOREGROUND_BLUE;
        break;
        case ConsoleColor_Blue:
        result |= FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        break;
        case ConsoleColor_DarkCyan:
        result |= FOREGROUND_GREEN | FOREGROUND_BLUE;
        break;
        case ConsoleColor_Cyan:
        result |= FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        break;
        case ConsoleColor_DarkPurple:
        result |= FOREGROUND_RED | FOREGROUND_BLUE;
        break;
        case ConsoleColor_Purple:
        result |= FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        break;
        case ConsoleColor_DarkYellow:
        result |= FOREGROUND_RED | FOREGROUND_GREEN;
        break;
        case ConsoleColor_Yellow:
        result |= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        break;
        default:
        break;
    }
    
    return result;
}

// Prints a message to a platform stream. If the stream is a console, uses
// supplied colors.
file_internal void 
Win32PrintToStream(const char* message, Win32StandardStream stream, EConsoleColor text_color, EConsoleColor background_color)
{
    // If redirected, write to a file instead of console.
    DWORD dummy;
    if (stream.is_redirected)
    {
        if (stream.is_wide)
        {
            
            i32 required_size = MultiByteToWideChar(CP_UTF8, 0, message, -1, 0, 0) - 1;
            i32 offset;
            for (offset = 0; offset + LOG_BUFFER_SIZE , required_size; offset += LOG_BUFFER_SIZE)
            {
                // TODO(Matt): Little endian BOM.
                MultiByteToWideChar(CP_UTF8, 0, &message[offset], LOG_BUFFER_SIZE, 
                                    g_virtual_console.wlog_buffer, LOG_BUFFER_SIZE);
                WriteFile(stream.handle, g_virtual_console.wlog_buffer, LOG_BUFFER_SIZE * 2, &dummy, 0);
            }
            i32 mod = required_size % LOG_BUFFER_SIZE;
            i32 size = MultiByteToWideChar(CP_UTF8, 0, &message[offset], mod, 
                                           g_virtual_console.wlog_buffer, LOG_BUFFER_SIZE) * 2;
            WriteFile(stream.handle, g_virtual_console.wlog_buffer, size, &dummy, 0);
        }
        else
        {
            WriteFile(stream.handle, message, (DWORD)strlen(message), &dummy, 0);
        }
    }
    else
    {
        WORD attribute = Win32TranslateConsoleColors(text_color, background_color);
        SetConsoleTextAttribute(stream.handle, attribute);
        WriteConsole(stream.handle, message, (DWORD)strlen(message), &dummy, 0);
        attribute = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        SetConsoleTextAttribute(stream.handle, attribute);
    }
}

file_internal i32 
__Win32FormatString(char *buff, i32 len, const char *fmt, va_list list)
{
    // if a caller doesn't actually know the length of the
    // format list, and is querying for the required size,
    // attempt to format the string into the buffer first
    // before copying in the chars.
    //
    // This handles the case where the buffer is declared like:
    //     char *buff = nullptr;
    va_list cpy;
    va_copy(cpy, list);
    i32 needed_chars = vsnprintf(NULL, 0, fmt, cpy);
    va_end(cpy);
    
    if (needed_chars < len)
    {
        needed_chars = vsnprintf(buff, len, fmt, list);
    }
    
    return needed_chars;
}

// Variadic version of the function
void Win32VLog(int level, const char *file, int line, const char *fmt, va_list args)
{
    // TID [SEVERITY] FILE:LINE:  
    const char *log_header = "%d\t[%s]\t %s:%d: ";
    
    EnterCriticalSection(&g_virtual_console.cs_lock);
    
    // Print log header
    int writ = snprintf(g_virtual_console.scratch_buffer, ARRAYCOUNT(g_virtual_console.scratch_buffer), 
                        log_header, GetCurrentThreadId(), g_log_level_strings[level], file, line);
    g_virtual_console.scratch_buffer[writ] = 0;
    
    char *message = NULL;
    int chars_read = 2 + __Win32FormatString(message, 1, fmt, args);
    message = (char*)Win32LocalAlloc(&g_virtual_console.internal_allocator, chars_read);
    __Win32FormatString(message, chars_read, fmt, args);
    message[chars_read-2] = '\n';
    
#if 0
    if (IsDebuggerPresent())
    {
        OutputDebugStringA(g_virtual_console.scratch_buffer);
        OutputDebugStringA(message);
    }
    else
#endif
    {
        Win32PrintToStream(g_virtual_console.scratch_buffer, 
                           g_virtual_console.output_stream, 
                           (EConsoleColor)g_log_level_colors[level], 
                           ConsoleColor_Black);
        
        if (level < LOG_ERROR)
        {
            Win32PrintToStream(message, 
                               g_virtual_console.output_stream, 
                               (EConsoleColor)g_log_level_colors[level], 
                               ConsoleColor_Black);
        }
        else
        {   
            Win32PrintToStream(message, 
                               g_virtual_console.error_stream, 
                               (EConsoleColor)g_log_level_colors[level], 
                               ConsoleColor_Black);
        }
    }
    
    LeaveCriticalSection(&g_virtual_console.cs_lock);
    
    if (level == LOG_FATAL)
    {
        if (PlatformShowAssertDialog(message, __FILE__, (u32)line)) DebugBreak();
    }
}

void
PlatformLog(int level, const char *file, int line, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Win32VLog(level, file, line, fmt, args);
    va_end(args);
}

bool PlatformShowAssertDialog(const char* message, const char* file, u32 line)
{
    
    
    EnterCriticalSection(&g_virtual_console.cs_lock);
    int msg_size = 1024;
    char *scratch = (char*)Win32LocalAlloc(&g_virtual_console.internal_allocator, msg_size);
	LeaveCriticalSection(&g_virtual_console.cs_lock);
    
    snprintf(scratch, msg_size,
			 "Assertion Failed!\n"
			 "    File: %s\n"
			 "    Line: %u\n"
			 "    Statement: ASSERT(%s)\n\0",
			 file, line, message);
	LogError(scratch);
    snprintf(scratch, msg_size,
			 "--File--\n"
			 "%s\n"
			 "\n"
			 "Line %u\n"
			 "\n"
			 "--Statement--\n"
			 "ASSERT(%s)\n"
			 "\n"
			 "Press Abort to stop execution, Retry to set a breakpoint (if debugging), or Ignore to continue execution.\n\0", file, line, message);
	
	int result = MessageBoxA(0, scratch, "Assertion Failed!", MB_ABORTRETRYIGNORE | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);
	if (result == IDABORT) exit(0);
	else if (result == IDRETRY) return true;
	else return false;
}

void PlatformShowErrorDialog(const char* message)
{
    MessageBoxA(0, message, "Error!", MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);
}

void Win32ShowErrorDialog(const char* message)
{
    MessageBoxA(0, message, "Error!", MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);
}

void PlatformFatalError(const char* message, ...)
{
    EnterCriticalSection(&g_virtual_console.cs_lock);
    va_list args;
    va_start (args, message);
    vsnprintf(g_virtual_console.scratch_buffer, ARRAYCOUNT(g_virtual_console.scratch_buffer), message, args);
    Win32ShowErrorDialog(g_virtual_console.scratch_buffer);
    va_end(args);
    LeaveCriticalSection(&g_virtual_console.cs_lock);
    exit(-1);
}
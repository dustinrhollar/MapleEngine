//-----------------------------------------------------------------------------------------------//
// External API

#include "TomlParser.h"

// For assertions
#ifndef TOML_ASSERT
#include <assert.h>
#define TOML_ASSERT assert
#endif

#define TOMLDATA_AS_STRING(data)  (data)->s
#define TOMLDATA_AS_INT(data)     (data)->i
#define TOMLDATA_AS_BOOL(data)    (data)->b
#define TOMLDATA_AS_FLOAT(data)   (data)->f
#define TOMLDATA_AS_TABLE(data)   (data)->t
#define TOMLDATA_AS_ARRAY(data)   (data)->a
#define TOMLDATA_STRING_LEN(data) (data)->sl
#define TOMLDATA_ARRAY_LEN(a)     (int)arrlen(a)

enum TomlLexemeType
{
    TomlLexeme_Identifier,
    TomlLexeme_Seperator,
    TomlLexeme_Operator,
    TomlLexeme_Literal,
    TomlLexeme_Comment,
    
    TomlLexeme_Count,
};

struct TomlLexeme
{
    TomlLexemeType  type;
    char           *start;
    int             len;
    int             line;
};

struct TomlScanner
{
    char   *start;   // beginning of the file
    char   *end;     // end of the file
    char   *iter;    // current location in the file
    TomlLexeme *lexemes; // list of lexemes completed after scanning th efile
};

// Fun fact, Windows in all its glory decided to enumerate "TokenType"
// As if that is not a common parser name...
enum TomlTokenType
{
    // Identifiers
    TomlToken_Identifier,
    
    // Seperators
    TomlToken_OpenBrace,
    TomlToken_CloseBrace,
    TomlToken_OpenBracket,
    TomlToken_CloseBracket,
    TomlToken_Comma,
    TomlToken_NewLine,
    
    // Operators
    TomlToken_Equals,
    
    // Literals
    TomlToken_String,
    TomlToken_Boolean,
    TomlToken_Integer,
    TomlToken_Float,
    
    TomlToken_Count,
};

struct TomlToken
{
    TomlTokenType type;
    char         *start;
    int           len;
    int           line;
    union
    { // converted literal data
        int         i; // integer literal
        bool        b; // boolean literal
        float       f; // float   literal
        char       *s; // string  literal
    };
};

struct TomlTokenizer
{
    TomlToken *tokens;
};

struct TomlBuffer
{
    char* start;
	char* current;
	int   cap;  // size of the allocation
	int   size; // current size from the start to the end of the contents
};

static int        TomlStrToInt(const char *start, const char *stop);
static float      TomlStrToFloat(const char *ptr);
static bool       TomlIsChar(char c);
static bool       TomlIsDigit(char c);
static bool       TomlIsSkippableChar(char c);
static bool       TomlLexemeIsIdentifier(const char *buffer);
static bool       TomlLexemeIsSeperator(const char *buffer);
static bool       TomlLexemeIsOperator(const char *buffer);
static bool       TomlLexemeIsLiteral(const char *buffer);
static bool       TomlLexemeIsComment(const char *buffer);
static TomlResult TomlScanFile(TomlScanner *scanner);
static TomlResult TomlTokenizeFile(TomlTokenizer *tokenizer, TomlScanner *scanner);
static TomlResult TomlParseArray(TomlData *array, TomlToken *tokens, int token_count, int *iter_advance);
static TomlResult TomlParseExpression(TomlObject *object, TomlToken *tokens, int token_count, int *iter_advance);
static TomlResult TomlParseObject(Toml *toml, TomlToken *tokens, int token_count, int *iter_advance);
static TomlResult TomlParseTitle(char **title, TomlToken *tokens, int token_count, int *iter_advance);
static TomlResult TomlParseFile(Toml *toml, TomlTokenizer *tokenizer);

static void*      TomlAlloc_Internal(uint64_t size);
static void       TomlFree_Internal(void *ptr);
static void       TomlLoadFile_Internal(const char* file_path, u8** buffer, int* size);
static void       TomlFreeFile_Internal(void *ptr);

/* Testing purposes only */
static void       TomlScannerPrintResults(TomlScanner *scanner);
static void       TomlTokenizerPrintResults(TomlTokenizer *tokenizer);

static void       TomlBufferInit(TomlBuffer *buffer, int initial_size);
static void       TomlBufferFree(TomlBuffer *buffer);
static int        __TomlFormatString(char *buf, int len, char *fmt, va_list list);
static void       TomlWrite(TomlBuffer *buffer, char *fmt, ...);
static void       TomlWriteArray(TomlBuffer *writer, TomlData *data);
static void       TomlWriteData(TomlBuffer *writer, TomlData *data, const char *name);

TomlCallbacks g_toml_internal_callbacks = {
    TomlAlloc_Internal,
    TomlFree_Internal,
    TomlLoadFile_Internal,
    TomlFreeFile_Internal,
};

static int 
TomlStrToInt(const char *start, const char *stop)
{
    int result = 0;
    for (const char *c = start; c < stop; ++c)
    {
        result = result * 10 + c[0] - '0';
    }
    return result;
}

static float 
TomlStrToFloat(const char *ptr)
{
    // TODO(Dustin): Implement a more reliable parser for floats
    float result = strtof(ptr, 0);
    return result;
}

static bool
TomlIsChar(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static bool 
TomlIsDigit(char c)
{
    return c >= '0' && c <= '9';
}

static bool
TomlIsSkippableChar(char c)
{
    bool result = false;
    switch (c)
    {
        case ' ':
        case '\0': /* Not sure if null terminator should be skippable */
        case '\t':
        case '\r': result = true;  break;
        default:   result = false; break;
    }
    return result;
}

static bool
TomlLexemeIsIdentifier(const char *buffer)
{
    return TomlIsChar(buffer[0]);
}

static bool
TomlLexemeIsSeperator(const char *buffer)
{
    bool result;
    switch (buffer[0])
    {
        case '[':
        case ']':
        case '{':
        case '}':
        case ',':
        case '\n': result = true;  break;
        default:   result = false; break;
    }
    return result;
}

static bool
TomlLexemeIsOperator(const char *buffer)
{
    bool result = false;
    if (buffer[0] == '=')
        result = true;
    return result;
}

static bool
TomlLexemeIsLiteral(const char *buffer)
{
    bool result = false;
    
    if (buffer[0] == '\"')
        result = true;
    else if (TomlIsDigit(buffer[0]))
        result = true;
    else if (buffer[0] == '.')
        result = true;
    else if ((strncmp(buffer, "true", 4) == 0) || (strncmp(buffer, "false", 5) == 0))
        result = true;
    
    return result;
}

static bool
TomlLexemeIsComment(const char *buffer)
{
    bool result = false;
    if (buffer[0] == '#') 
        result = true;
    return result;
}

static TomlResult
TomlScanFile(TomlScanner *scanner)
{
    TomlResult result = TomlResult_Success;
    char *iter = scanner->start;
    int   line = 0;
    while (iter < scanner->end)
    {
        // Skip any unnecessary characters
        while (TomlIsSkippableChar(iter[0]))
            ++iter;
        if (iter >= scanner->end) break;
        
        TomlLexeme lex = {};
        lex.type = TomlLexeme_Count;
        lex.line = line;
        
        if (TomlLexemeIsSeperator(iter))
        {
            if (iter[0] == '\n')
                ++line;
            
            lex.type  = TomlLexeme_Seperator;
            lex.start = iter;
            lex.len   = 1;
            iter += lex.len;
        }
        else if (TomlLexemeIsOperator(iter))
        {
            lex.type  = TomlLexeme_Operator;
            lex.start = iter;
            lex.len   = 1;
            iter     += lex.len;
        }
        else if (TomlLexemeIsLiteral(iter))
        {
            lex.type  = TomlLexeme_Literal;
            lex.start = iter;
            
            if (iter[0] == '\"')
            { 
                // consume until the next quotation
                // Must be handle seperately because string
                // literals allow spaces, while bool/int/float
                // literals do not allow spaces
                do { ++iter; } 
                while (iter[0] != '\"');
                ++iter; // consume the last quotation
            }
            else
            { // consume digits, characters, decimals
                do { ++iter; } 
                while (TomlIsDigit(iter[0]) || TomlIsChar(iter[0]) || iter[0] == '.');
            }
            lex.len = (int)(iter - lex.start);
        }
        else if (TomlLexemeIsIdentifier(iter))
        {
            lex.type = TomlLexeme_Identifier;
            lex.start = iter;
            { // consume the identifier
                do { ++iter; } 
                while (TomlIsDigit(iter[0]) || TomlIsChar(iter[0]) || iter[0] == '_');
            }
            lex.len = (int)(iter - lex.start);
        }
        else if (TomlLexemeIsComment(iter))
        {
            lex.type  = TomlLexeme_Comment;
            lex.start = iter;
            { // consume the identifier
                do { ++iter; } 
                while (iter[0] != '\n');
            }
            lex.len = (int)(iter - lex.start);
        }
        else
        { // Unknown lexeme
            result = TomlResult_UnknownLexeme;
            return result;
        }
        
        arrput(scanner->lexemes, lex);
    }
    return result;
}

static TomlResult
TomlTokenizeFile(TomlTokenizer *tokenizer, TomlScanner *scanner)
{
    TomlResult result = TomlResult_Success;
    for (int i = 0; i < (int)arrlen(scanner->lexemes); ++i)
    {
        TomlLexeme *lex = scanner->lexemes + i;
        
        TomlToken token = {};
        token.type  = TomlToken_Count;
        token.start = lex->start;
        token.len   = lex->len;
        token.line  = lex->line;
        
        switch (lex->type)
        {
            case TomlLexeme_Identifier:
            {
                token.type = TomlToken_Identifier;
            } break;
            
            case TomlLexeme_Seperator:
            {
                if (lex->start[0] == '[')
                    token.type = TomlToken_OpenBrace;
                else if (lex->start[0] == ']')
                    token.type = TomlToken_CloseBrace;
                else if (lex->start[0] == '{')
                    token.type = TomlToken_OpenBracket;
                else if (lex->start[0] == '}')
                    token.type = TomlToken_CloseBracket;
                else if (lex->start[0] == ',')
                    token.type = TomlToken_Comma;
                else if (lex->start[0] == '\n')
                {
                    // Strip empty lines. An empty line is a line
                    // in which the previous token is newline token
                    if (arrlen(tokenizer->tokens) == 0 || 
                        tokenizer->tokens[arrlen(tokenizer->tokens)-1].type == TomlToken_NewLine) 
                        continue;
                    token.type = TomlToken_NewLine;
                }
                else
                { // Uknown seperator type
                    result = TomlResult_InvalidSyntax;
                    return result;
                }
            } break;
            
            case TomlLexeme_Operator:
            {
                if (lex->start[0] == '=')
                    token.type = TomlToken_Equals;
                else
                { // Uknown operator type
                    result = TomlResult_InvalidSyntax;
                    return result;
                }
            } break;
            
            case TomlLexeme_Literal:
            {
                if (lex->start[0] == '\"')
                {
                    token.type = TomlToken_String;
                    token.len -= 2; // consume the quoations
                    token.s = token.start + 1;
                }
                else if (lex->start[0] == 't')
                {
                    token.type = TomlToken_Boolean;
                    token.b = true;
                }
                else if (lex->start[0] == 'f')
                {
                    token.type = TomlToken_Boolean;
                    token.b = false;
                }
                else if (TomlIsDigit(lex->start[0]) || lex->start[0] == '.')
                {
                    token.type = TomlToken_Integer;
                    // determine if token is an integer or float
                    for (int c = 0; c < lex->len; ++c)
                    {
                        if (lex->start[c] == '.')
                        {
                            token.type = TomlToken_Float;
                            break;
                        }
                    }
                    
                    if (token.type == TomlToken_Integer)
                        token.i = TomlStrToInt(lex->start, lex->start + lex->len);
                    else
                        token.f = TomlStrToFloat(lex->start);
                }
            } break;
            
            // comments are stripped
            case TomlLexeme_Comment:
            default: continue;
        }
        
        arrput(tokenizer->tokens, token);
    }
    
    return result;
}

// @returns the offset from the start of the array. For example, if 5 tokens
// were consumed to parse the object, then 5 will be returned
static TomlResult
TomlParseArray(TomlData *array, TomlToken *tokens, int token_count, int *iter_advance)
{
    TomlResult result = TomlResult_Success;
    array->a = 0;
    
    bool found_comma = true;
    int offset = 0;
    TomlToken *iter = tokens;
    TomlType array_type = Toml_Count;
    while (offset < token_count && iter->type != TomlToken_CloseBrace)
    {
        switch (iter->type)
        {
            case TomlToken_OpenBrace:
            { // Nested arrays
                if (array_type == Toml_Count) array_type = Toml_Array; // set array type
                if (array_type != Toml_Array) 
                { // Did not find an array type
                    result = TomlResult_UnexpectedDataType;
                    return result;
                }
                if (!found_comma)
                { // Did not find comma delimeter between array elements
                    result = TomlResult_InvalidSyntax;
                    return result;
                }
                else
                    found_comma = false;
                
                ++offset;
                
                TomlData data = {};
                data.type = Toml_Array;
                result = TomlParseArray(&data, tokens + offset, token_count - offset, &offset);
                if (result != TomlResult_Success) return result;
                arrput(array->a, data);
            } break;
            
            case TomlToken_Integer:
            {
                if (array_type == Toml_Count) array_type = Toml_Int; // set array type
                if (array_type != Toml_Int) // Does not match array type
                { // Did not find an int type
                    result = TomlResult_UnexpectedDataType;
                    return result;
                }
                if (!found_comma)
                { // Did not find comma delimeter between array elements
                    result = TomlResult_InvalidSyntax;
                    return result;
                }
                else
                    found_comma = false;
                
                TomlData data = {};
                data.type = Toml_Int;
                data.i = iter->i;
                arrput(array->a, data);
                ++offset;
            } break;
            
            case TomlToken_Boolean:
            {
                if (array_type == Toml_Count) array_type = Toml_Bool; // set array type
                if (array_type != Toml_Bool) // Does not match array type
                { // Did not find an boolean type
                    result = TomlResult_UnexpectedDataType;
                    return result;
                }
                if (!found_comma)
                { // Did not find comma delimeter between array elements
                    result = TomlResult_InvalidSyntax;
                    return result;
                }
                else
                    found_comma = false;
                
                TomlData data = {};
                data.type = Toml_Bool;
                data.b = iter->b;
                arrput(array->a, data);
                ++offset;
            } break;
            
            case TomlToken_Float:
            {
                if (array_type == Toml_Count) array_type = Toml_Float; // set array type
                if (array_type != Toml_Float) // Does not match array type
                { // Did not find an float type
                    result = TomlResult_UnexpectedDataType;
                    return result;
                }
                if (!found_comma)
                { // Did not find comma delimeter between array elements
                    result = TomlResult_InvalidSyntax;
                    return result;
                }
                else
                    found_comma = false;
                
                TomlData data = {};
                data.type = Toml_Float;
                data.f = iter->f;
                arrput(array->a, data);
                ++offset;
            } break;
            
            case TomlToken_String:
            {
                if (array_type == Toml_Count) array_type = Toml_String; // set array type
                if (array_type != Toml_String) // Does not match array type
                { // Did not find string type
                    result = TomlResult_UnexpectedDataType;
                    return result;
                }
                if (!found_comma)
                { // Did not find comma delimeter between array elements
                    result = TomlResult_InvalidSyntax;
                    return result;
                }
                else
                    found_comma = false;
                
                TomlData data = {};
                data.type = Toml_String;
                data.s = iter->s;
                data.sl = iter->len;
                arrput(array->a, data);
                ++offset;
            } break;
            
            case TomlToken_OpenBracket:
            { // TODO(Dustin): INLINE TABLES NOT SUPPORTED
                result = TomlResult_Count;
            } break;
            
            case TomlToken_Comma:
            { // do nothing with a comma
                found_comma = true;
                ++offset;
            } break;
            
            default:
            {
                result = TomlResult_UnknownToken;
                return result;
            } break;
        }
        
        iter = tokens + offset;
    }
    
    if (offset >= token_count || tokens[offset].type != TomlToken_CloseBrace)
    { // Missing closing brace for array
        result = TomlResult_InvalidSyntax;
        return result;
    }
    
    ++offset;
    *iter_advance += offset;
    return result;
}

// @returns the offset from the start of the array. For example, if 5 tokens
// were consumed to parse the object, then 5 will be returned
static TomlResult
TomlParseExpression(TomlObject *object, TomlToken *tokens, int token_count, int *iter_advance)
{
    TomlResult result = TomlResult_Success;
    int offset = 0;
    
    if (tokens[offset].type != TomlToken_Identifier)
    { // Expected name for expression, but was not found
        result = TomlResult_InvalidSyntax;
        return result;
    }
    
    char *expr_name = tokens[offset].start;
    uint64_t expr_name_len = tokens[offset].len;
    ++offset;
    
    if (tokens[offset].type != TomlToken_Equals)
    { // Expected equals sign, but was not found!
        result = TomlResult_InvalidSyntax;
        return result;
    }
    ++offset;
    
    TomlData data = {};
    switch (tokens[offset].type)
    {
        case TomlToken_OpenBrace:
        {
            ++offset;
            
            data.type = Toml_Array;
            result = TomlParseArray(&data, tokens + offset, token_count - offset, &offset);
            if (result != TomlResult_Success) return result;
        } break;
        
        case TomlToken_Integer:
        {
            data.type = Toml_Int;
            data.i = tokens[offset].i;
            ++offset;
        } break;
        
        case TomlToken_Boolean:
        {
            data.type = Toml_Bool;
            data.b = tokens[offset].b;
            ++offset;
        } break;
        
        case TomlToken_Float:
        {
            data.type = Toml_Float;
            data.f = tokens[offset].f;
            ++offset;
        } break;
        
        case TomlToken_String:
        {
            data.type = Toml_String;
            data.s  = tokens[offset].s;
            data.sl = tokens[offset].len;
            ++offset;
        } break;
        
        default:
        { // Unknown token found
            result = TomlResult_UnknownToken;
            return result;
        } break;
    }
    
    // Insert the toml data into the data table for the current object
    TomlKeyValue key_val = {};
    key_val.key   = expr_name;
    key_val.value = data;
    shputs_len(object->data_table, key_val, expr_name_len);
    
    *iter_advance += offset;
    return result;
}

// @returns the offset from the start of the array. For example, if 5 tokens
// were consumed to parse the object, then 5 will be returned
static TomlResult
TomlParseObject(Toml *toml, TomlToken *tokens, int token_count, int *iter_advance)
{
    TomlResult result = TomlResult_Success;
    int offset = 0;
    
    if (tokens[offset].type != TomlToken_Identifier)
    { // Expected a name for the object, but did not find one
        result = TomlResult_UnknownToken;
        return result;
    }
    
    char *expr_name = tokens[offset].start;
    uint64_t expr_name_len = tokens[offset].len;
    ++offset;
    
    if (tokens[offset].type != TomlToken_CloseBrace)
    { // Expected a close brace, but did not find one
        result = TomlResult_UnknownToken;
        return result;
    }
    ++offset;
    
    TomlObject object = {};
    object.data_table = 0;
    sh_new_strdup(object.data_table);
    
    for (; offset < token_count;)
    {
        TomlToken *tok = tokens + offset;
        if (tok->type == TomlToken_OpenBrace)
        { // At next object, go ahead and return
            break;
        }
        else if (tok->type == TomlToken_NewLine)
            ++offset;
        else
        {
            result = TomlParseExpression(&object, tokens + offset, token_count - offset, &offset);
            if (result != TomlResult_Success) return result;
        }
    }
    
    // Insert the toml data into the data table for the current object
    TomlObjectKeyValue key_val = {};
    key_val.key   = expr_name;
    key_val.value = object;
    shputs_len(toml->table, key_val, expr_name_len);
    
    *iter_advance += offset;
    return result;
}

// @returns the offset from the start of the array. For example, if 5 tokens
// were consumed to parse the object, then 5 will be returned
static TomlResult
TomlParseTitle(char **title, TomlToken *tokens, int token_count, int *iter_advance)
{
    TomlResult result = TomlResult_Success;
    int offset = 0;
    
    if (tokens[offset].type != TomlToken_Equals)
    {
        result = TomlResult_InvalidSyntax;
        return result;
    }
    ++offset;
    
    if (tokens[offset].type != TomlToken_String)
    {
        result = TomlResult_InvalidSyntax;
        return result;
    }
    
    *title = (char*)g_toml_internal_callbacks.Alloc(tokens[offset].len + 1);
    memcpy(*title, tokens[offset].s, tokens[offset].len);
    (*title)[tokens[offset].len] = 0;
    
    ++offset;
    *iter_advance += offset;
    return result;
}

static TomlResult
TomlParseFile(Toml *toml, TomlTokenizer *tokenizer)
{
    TomlResult result = TomlResult_Success;
    bool object_set = false;
    sh_new_strdup(toml->table);
    
    TomlToken *iter = tokenizer->tokens;
    int tok_count = (int)arrlen(tokenizer->tokens);
    
    int i = 0;
    while (i < tok_count)
    {
        TomlToken *tok = iter + i;
        switch (tok->type)
        {
            case TomlToken_OpenBrace:
            {
                ++i;
                result = TomlParseObject(toml, tokenizer->tokens + i, tok_count - i, &i);
                if (result != TomlResult_Success) return result;
            } break;
            
            case TomlToken_Identifier:
            {
                if (!object_set)
                {
                    if (strncmp("title", tok->start, tok->len) == 0)
                    {
                        ++i;
                        result = TomlParseTitle(&toml->title, tokenizer->tokens + i, tok_count - i, &i);
                        if (result != TomlResult_Success) return result;
                    }
                    else
                    {
                        // Identifier other than "title" was found outside of a toml object
                        result = TomlResult_UnknownToken;
                        return result;
                    }
                }
            } break;
            
            case TomlToken_NewLine:
            { // Skip new line tokens
                ++i;
            } break;
            
            default:
            {
                result = TomlResult_UnknownToken;
                return result;
            } break;
        }
    }
    
    return result;
}

#if 0
static void
TomlScannerPrintResults(TomlScanner *scanner)
{
    PrettyBuffer buffer = {};
    pb_init(&buffer, _KB(1));
    
    pb_write(&buffer, "[\n");
    
    for (int i = 0; i < (int)arrlen(scanner->lexemes); ++i)
    {
        TomlLexeme *lex = scanner->lexemes + i;
        Str str = StrInit(lex->len, lex->start);
        
        pb_write(&buffer, "\ttype: ");
        switch (lex->type)
        {
            case TomlLexeme_Identifier:
            {
                pb_write(&buffer, "IDENTIFIER, value: %s\n", StrGetString(&str));
            } break;
            
            case TomlLexeme_Seperator:
            {
                if (lex->start[0] == '\n')
                    pb_write(&buffer, "SEPERATOR, value: newline\n");
                else
                    pb_write(&buffer, "SEPERATOR, value: %s\n", StrGetString(&str));
            } break;
            
            case TomlLexeme_Operator:
            {
                pb_write(&buffer, "OPERATOR, value: %s\n", StrGetString(&str));
            } break;
            
            case TomlLexeme_Literal:
            {
                pb_write(&buffer, "LITERAL, value: %s\n", StrGetString(&str));
            } break;
            
            case TomlLexeme_Comment:
            {
                pb_write(&buffer, "COMMENT, value: %s\n", StrGetString(&str));
            } break;
            
            StrFree(&str);
        }
    }
    
    pb_write(&buffer, "]\n");
    
    buffer.current[0] = 0;
    LogInfo("Scanner Results: %s", buffer.start);
    
    pb_free(&buffer);
}

static void
TomlTokenizerPrintResults(TomlTokenizer *tokenizer)
{
    PrettyBuffer buffer = {};
    pb_init(&buffer, _KB(1));
    
    pb_write(&buffer, "[\n");
    
    for (int i = 0; i < (int)arrlen(tokenizer->tokens); ++i)
    {
        TomlToken *tok = tokenizer->tokens + i;
        
        switch (tok->type)
        {
            case TomlToken_Identifier:
            {
                Str str = StrInit(tok->len, tok->start);
                pb_write(&buffer, "\tTOKEN_IDENTIFIER, value: %s\n", StrGetString(&str));
                StrFree(&str);
            }break;
            
            case TomlToken_OpenBrace:
            {
                pb_write(&buffer, "\tTOKEN_OPENBRACE\n");
            }break;
            
            case TomlToken_CloseBrace:
            {
                pb_write(&buffer, "\tTOKEN_CLOSEBRACE\n");
            }break;
            
            case TomlToken_OpenBracket:
            {
                pb_write(&buffer, "\tTOKEN_OPENBRACKET\n");
            }break;
            
            case TomlToken_CloseBracket:
            {
                pb_write(&buffer, "\tTOKEN_CLOSEBRACKET\n");
            }break;
            
            case TomlToken_Comma:
            {
                pb_write(&buffer, "\tTOKEN_COMMA\n");
            }break;
            
            case TomlToken_NewLine:
            {
                pb_write(&buffer, "\tTOKEN_NEWLINE\n");
            }break;
            
            case TomlToken_Equals:
            {
                pb_write(&buffer, "\tTOKEN_EQUALS\n");
            }break;
            
            case TomlToken_String:
            {
                Str str = StrInit(tok->len, tok->s);
                pb_write(&buffer, "\tTOKEN_STRING, value: %s\n", StrGetString(&str));
                StrFree(&str);
            }break;
            
            case TomlToken_Boolean:
            {
                pb_write(&buffer, "\tTOKEN_BOOLEAN, value: %s\n", (tok->b) ? "true" : "false");
            }break;
            
            case TomlToken_Integer:
            {
                pb_write(&buffer, "\tTOKEN_INTEGER, value: %d\n", tok->i);
            }break;
            
            case TomlToken_Float:
            {
                pb_write(&buffer, "\tTOKEN_INTEGER, value: %lf\n", tok->f);
            }break;
        }
    }
    
    pb_write(&buffer, "]\n");
    
    buffer.current[0] = 0;
    LogInfo("TomlTokenizer Results: %s", buffer.start);
    
    pb_free(&buffer);
}
#endif

static TomlResult 
TomlLoad(Toml *toml, const char *filepath)
{
    *toml = {};
    toml->file_data = 0;
    toml->table = 0;
    
    int size;
    g_toml_internal_callbacks.LoadFile(filepath, (u8**)&toml->file_data, &size);
    
    //---------------------------------------------------------------------------------------------
    // Lexer Pass
    
    TomlScanner scanner = {};
    scanner.start   = (char*)toml->file_data;
    scanner.end     = (char*)toml->file_data + size;
    scanner.lexemes = 0;
    TomlResult result = TomlScanFile(&scanner);
    if (result != TomlResult_Success) return result;
    
    TomlTokenizer tokenizer = {};
    tokenizer.tokens = 0;
    result = TomlTokenizeFile(&tokenizer, &scanner);
    if (result != TomlResult_Success) return result;
    
#if 0 // Print results
    TomlScannerPrintResults(&scanner);
    TomlTokenizerPrintResults(&tokenizer);
#endif
    
    //---------------------------------------------------------------------------------------------
    // Parser Pass
    
    result = TomlParseFile(toml, &tokenizer);
    
    return result;
}

static void 
TomlFree(Toml *toml)
{
    // TODO(Dustin): Make it so title is not heap allocated!
    //g_toml_internal_callbacks.Free(toml->title);
    
    for (int i = 0; i < (int)shlen(toml->table); ++i)
    {
        TomlObject *obj = &toml->table[i].value;
        
        for (int j = 0; j < (int)shlen(obj->data_table); ++j)
        {
            TomlData *data = &obj->data_table[j].value;
            
            switch (data->type)
            {
                case Toml_Array:
                {
                    if (data->a[0].type == Toml_Array)
                    {
                        TomlData *queue = 0;
                        for (int k = 0; k < arrlen(data->a); ++k)
                        {
                            arrput(queue, data->a[k]);
                        }
                        
                        while (arrlen(queue) > 0)
                        {
                            TomlData elem = arrpop(queue);
                            TOML_ASSERT(elem.type == Toml_Array);
                            if (elem.a[0].type == Toml_Array)
                            {
                                for (int k = 0; k < arrlen(data->a); ++k)
                                {
                                    arrput(queue, elem.a[k]);
                                }
                            }
                            arrfree(elem.a);
                        }
                        arrfree(queue);
                    }
                    if (data->a) arrfree(data->a);
                } break;
                
                case Toml_String:
                case Toml_Int:
                case Toml_Bool:
                case Toml_Float:
                case Toml_Table: /* NOTE(Dustin): Tables are not currently supported! */
                default: break;
            }
        }
        shfree(obj->data_table);
    }
    shfree(toml->table);
    
    g_toml_internal_callbacks.FreeFile(toml->file_data);
}

static TomlObject
TomlGetObject(Toml *toml, const char *name)
{
    return shget(toml->table, name);
}

static int
TomlGetInt(TomlObject *obj, const char *name)
{
    TomlData data = shget(obj->data_table, name);
    TOML_ASSERT(data.type == Toml_Int);
    return TOMLDATA_AS_INT(&data);
}

static bool
TomlGetBool(TomlObject *obj, const char *name)
{
    TomlData data = shget(obj->data_table, name);
    TOML_ASSERT(data.type == Toml_Bool);
    return TOMLDATA_AS_BOOL(&data);
}

static float
TomlGetFloat(TomlObject *obj, const char *name)
{
    TomlData data = shget(obj->data_table, name);
    TOML_ASSERT(data.type == Toml_Float);
    return TOMLDATA_AS_FLOAT(&data);
}

static const char*
TomlGetString(TomlObject *obj, const char *name)
{
    TomlData data = shget(obj->data_table, name);
    TOML_ASSERT(data.type == Toml_String);
    return TOMLDATA_AS_STRING(&data);
}

static int
TomlGetStringLen(TomlObject *obj, const char *name)
{
    TomlData data = shget(obj->data_table, name);
    TOML_ASSERT(data.type == Toml_String);
    return TOMLDATA_STRING_LEN(&data);
}

static TomlData*
TomlGetArray(TomlObject *obj, const char *name)
{
    TomlData data = shget(obj->data_table, name);
    TOML_ASSERT(data.type == Toml_Array);
    return TOMLDATA_AS_ARRAY(&data);
}

static int
TomlGetArrayLen(TomlData *data)
{
    return TOMLDATA_ARRAY_LEN(data);
}

static int
TomlGetIntArrayElem(TomlData *data, int idx)
{
    TOML_ASSERT(arrlen(data) > idx);
    TOML_ASSERT(data[0].type == Toml_Int);
    return TOMLDATA_AS_INT(&data[idx]);
}

static bool
TomlGetBoolArrayElem(TomlData *data, int idx)
{
    TOML_ASSERT(arrlen(data) > idx);
    TOML_ASSERT(data[0].type == Toml_Bool);
    return TOMLDATA_AS_FLOAT(&data[idx]);
}

static float
TomlGetFloatArrayElem(TomlData *data, int idx)
{
    TOML_ASSERT(arrlen(data) > idx);
    TOML_ASSERT(data[0].type == Toml_Float);
    return TOMLDATA_AS_FLOAT(&data[idx]);
}

static const char*
TomlGetStringArrayElem(TomlData *data, int idx)
{
    TOML_ASSERT(arrlen(data) > idx);
    TOML_ASSERT(data[0].type == Toml_String);
    return TOMLDATA_AS_STRING(&data[idx]);
}

static int
TomlGetStringLenArrayElem(TomlData *data, int idx)
{
    TOML_ASSERT(arrlen(data) > idx);
    TOML_ASSERT(data[0].type == Toml_String);
    return TOMLDATA_STRING_LEN(&data[idx]);
}

static TomlData*
TomlGetArrayElem(TomlData *data, int idx)
{
    TOML_ASSERT(arrlen(data) > idx);
    TOML_ASSERT(data[0].type == Toml_Array);
    return TOMLDATA_AS_ARRAY(&data[idx]);
}

static void* 
TomlAlloc_Internal(uint64_t size)
{
    return malloc(size);
}

static void 
TomlFree_Internal(void *ptr)
{
    free(ptr);
}

static void 
TomlLoadFile_Internal(const char* file_path, u8** buffer, int* size)
{
    FILE *fp = fopen(file_path, "r");
    TOML_ASSERT(fp);
    
    fseek(fp, 0, SEEK_END); // seek to end of file
    uint64_t fsize = ftell(fp); // get current file pointer
    fseek(fp, 0, SEEK_SET); // seek back to beginning of file
    
    *buffer = (u8*)g_toml_internal_callbacks.Alloc(fsize+1);
    
    uint64_t read = fread(*buffer, 1, fsize, fp);
    TOML_ASSERT(read <= fsize);
    
    *size   = (int)read;
    (*buffer)[read] = 0;
}

static void  
TomlFreeFile_Internal(void *ptr)
{
    g_toml_internal_callbacks.Free(ptr);
}


static Toml        
TomlCreate()
{
    Toml toml = {};
    toml.title = 0;
    toml.file_data = 0;
    toml.table = 0;
    sh_new_strdup(toml.table);
    return toml;
}

static TomlObject  
TomlCreateObject()
{
    TomlObject obj = {};
    obj.data_table = 0;
    sh_new_strdup(obj.data_table);
    return obj;
}

static TomlData    
TomlDataInt(int val)
{
    TomlData data = {};
    data.type = Toml_Int;
    data.i = val;
    return data;
}

static TomlData    
TomlDataBool(bool val)
{
    TomlData data = {};
    data.type = Toml_Bool;
    data.b = val;
    return data;
}

static TomlData    
TomlDataFloat(float val)
{
    TomlData data = {};
    data.type = Toml_Float;
    data.f = val;
    return data;
}

static TomlData    
TomlDataString(char *str, int len)
{
    TomlData data = {};
    data.type = Toml_String;
    data.s = str;
    data.sl = len;
    return data;
}

static TomlData    
TomlDataArray()
{
    TomlData data = {};
    data.type = Toml_Array;
    data.a = 0;
    return data;
}

static void        
TomlArrayAddInt(TomlData *data, int val)
{
    TOML_ASSERT(data->type == Toml_Array);
    TomlData v = TomlDataInt(val);
    if (arrlen(data->a) > 0) TOML_ASSERT(data->a[0].type == Toml_Int);
    arrput(data->a, v);
}

static void        
TomlArrayAddBool(TomlData *data, bool val)
{
    TOML_ASSERT(data->type == Toml_Array);
    TomlData v = TomlDataBool(val);
    if (arrlen(data->a) > 0) TOML_ASSERT(data->a[0].type == Toml_Bool);
    arrput(data->a, v);
}

static void        
TomlArrayAddFloat(TomlData *data, float val)
{
    TOML_ASSERT(data->type == Toml_Array);
    TomlData v = TomlDataFloat(val);
    if (arrlen(data->a) > 0) TOML_ASSERT(data->a[0].type == Toml_Float);
    arrput(data->a, v);
}

static void        
TomlArrayAddString(TomlData *data, char *str, int len)
{
    TOML_ASSERT(data->type == Toml_Array);
    TomlData v = TomlDataString(str, len);
    if (arrlen(data->a) > 0) TOML_ASSERT(data->a[0].type == Toml_String);
    arrput(data->a, v);
}

static void        
TomlArrayAddArray(TomlData *data, TomlData *array)
{
    TOML_ASSERT(data->type == Toml_Array);
    if (arrlen(data->a) > 0) 
        TOML_ASSERT(data->a[0].type == Toml_Array);
    
    TomlData cpy = TomlDataArray();
    for (int i = 0; i < arrlen(array->a); ++i)
    {
        switch (array->a[i].type)
        {
            case Toml_String: TomlArrayAddString(&cpy, array->a[i].s, array[i].sl);  break;
            case Toml_Int:    TomlArrayAddInt(&cpy, array->a[i].i);                  break;
            case Toml_Bool:   TomlArrayAddBool(&cpy, array->a[i].b);                 break;
            case Toml_Float:  TomlArrayAddFloat(&cpy, array->a[i].f);                break;
            case Toml_Array:  TomlArrayAddArray(&cpy, array->a[i].a);                break;
            case Toml_Table: 
            default: break;
        }
    }
    
    arrput(data->a, cpy);
}

static void        
TomlArrayAddInt(TomlData *data, TomlData *i)
{
    TOML_ASSERT(data->type == Toml_Array);
    if (arrlen(data->a) > 0) TOML_ASSERT(data->a[0].type == Toml_Int);
    arrput(data->a, *i);
}

static void        
TomlArrayAddBool(TomlData *data, TomlData *b)
{
    TOML_ASSERT(data->type == Toml_Array);
    if (arrlen(data->a) > 0) TOML_ASSERT(data->a[0].type == Toml_Bool);
    arrput(data->a, *b);
}

static void        
TomlArrayAddFloat(TomlData *data, TomlData *f)
{
    TOML_ASSERT(data->type == Toml_Array);
    if (arrlen(data->a) > 0) TOML_ASSERT(data->a[0].type == Toml_Float);
    arrput(data->a, *f);
}

static void        
TomlArrayAddString(TomlData *data, TomlData *s)
{
    TOML_ASSERT(data->type == Toml_Array);
    if (arrlen(data->a) > 0) TOML_ASSERT(data->a[0].type == Toml_String);
    arrput(data->a, *s);
}

static void        
TomlObjectAddData(TomlObject *obj, TomlData *data, char *name)
{
    TomlKeyValue key_val = {};
    key_val.key   = name;
    key_val.value = *data;
    shputs_len(obj->data_table, key_val, strlen(name));
}

static void        
TomlAddObject(Toml *toml, TomlObject *obj, char *name)
{
    TomlObjectKeyValue key_val = {};
    key_val.key   = name;
    key_val.value = *obj;
    shputs_len(toml->table, key_val, strlen(name));
}

static void 
TomlBufferInit(TomlBuffer *buffer, int initial_size) 
{
	buffer->start = 0;
	if (initial_size > 0) buffer->start = (char*)g_toml_internal_callbacks.Alloc(initial_size);
	buffer->current = buffer->start;
	buffer->cap = initial_size;
}

static void 
TomlBufferFree(TomlBuffer *buffer) 
{
	if (buffer->start) g_toml_internal_callbacks.Free(buffer->start);
	buffer->start = 0;
	buffer->current = 0;
	buffer->cap = 0;
}

static int 
__TomlFormatString(char *buf, int len, char *fmt, va_list list) 
{
	va_list cpy;
	va_copy(cpy, list);
	int needed_chars = vsnprintf(NULL, 0, fmt, cpy);
	va_end(cpy);
    
	if (buf && needed_chars < len) {
		needed_chars = vsnprintf(buf, len, fmt, list);
	}
	return needed_chars;
}

static void 
TomlWrite(TomlBuffer *buffer, char *fmt, ...) 
{
	va_list args;
	va_start(args, fmt);
    
	// Check to see if we need to resize the buffer
    i64 chars_needed = __TomlFormatString(NULL, 0, fmt, args);
	if (chars_needed + (buffer->current - buffer->start) > buffer->cap) {
        i64 offset = buffer->current - buffer->start;
		i64 min_size = chars_needed + offset;
		buffer->cap = (i32)min_size * 2;
		
        void *tmp = (char*)g_toml_internal_callbacks.Alloc(buffer->cap);
		memcpy(tmp, buffer->start, buffer->cap);
        g_toml_internal_callbacks.Free(buffer->start);
        buffer->start = (char*)tmp;
        
        buffer->current = buffer->start + offset;
	}
    
    u64 leftover = buffer->cap - (buffer->current - buffer->start);
	chars_needed = __TomlFormatString(buffer->current, (i32)leftover, fmt, args);
    
	buffer->size += (i32)chars_needed;
	buffer->current += chars_needed;
    
	va_end(args);
}

static void
TomlWriteArray(TomlBuffer *writer, TomlData *data)
{
    TomlWrite(writer, "[");
    
    if (arrlen(data) > 0)
    {
        TomlWriteData(writer, &data[0], NULL);
    }
    
    for (int k = 1; k < arrlen(data); ++k)
    {
        TomlWrite(writer, ", ");
        TomlWriteData(writer, &data[k], NULL);
    }
    
    TomlWrite(writer, "]");
}

static void
TomlWriteData(TomlBuffer *writer, TomlData *data, const char *name)
{
    if (name) TomlWrite(writer, "%s = ", name);
    
    switch (data->type)
    {
        case Toml_Array:  TomlWriteArray(writer, data->a); break;
        case Toml_String: TomlWrite(writer, "\"%s\"", data->s); break;
        case Toml_Int:    TomlWrite(writer, "%d", data->i); break;
        case Toml_Bool:
        {
            if (data->b)  TomlWrite(writer, "true");
            else          TomlWrite(writer, "false");
        } break;
        case Toml_Float:  TomlWrite(writer, "%lf", data->f); break;
        case Toml_Table: /* NOTE(Dustin): Tables are not currently supported! */
        default: break;
    }
}

static void        
TomlToString(Toml *toml, char **str, int *len)
{
    TomlBuffer writer = {};
    TomlBufferInit(&writer, _KB(1));
    
    if (toml->title)
        TomlWrite(&writer, "title = \"%s\"\n\n", toml->title);
    
    for (int i = 0; i < (int)shlen(toml->table); ++i)
    {
        TomlObject *obj = &toml->table[i].value;
        TomlWrite(&writer, "[%s]\n", toml->table[i].key);
        
        for (int j = 0; j < (int)shlen(obj->data_table); ++j)
        {
            TomlData *data = &obj->data_table[j].value;
            TomlWriteData(&writer, data, obj->data_table[j].key);
            TomlWrite(&writer, "\n");
        }
        
        TomlWrite(&writer, "\n");
    }
    
    *str = writer.start;
    *len = (int)(writer.current - writer.start);
}

// Free a string allocated by the fn call to TomlToString
static void        
TomlFreeString(char **str)
{
    g_toml_internal_callbacks.Free(*str);
    *str = 0;
}

static void        
TomlSetCallbacks(TomlCallbacks *callbacks)
{
    if (callbacks->Alloc && callbacks->Free)
    {
        g_toml_internal_callbacks.Alloc = callbacks->Alloc;
        g_toml_internal_callbacks.Free  = callbacks->Free;
    }
    
    if (callbacks->LoadFile && callbacks->FreeFile)
    {
        g_toml_internal_callbacks.LoadFile = callbacks->LoadFile;
        g_toml_internal_callbacks.FreeFile = callbacks->FreeFile;
    }
}

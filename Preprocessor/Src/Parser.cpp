
    //
// TODO(Dustin):
// - (nested) (unnamed) Unions
// - Type qualifiers (const & pointers)
// 
// TODO(Dustin): (Another time)
// - Enums
// - Better error reporting
// - General C syntax so we do not have to use a unique file format

enum ParserResult
{
    ParserResult_Success,
    ParserResult_ParseError,
    ParserResult_UnknownLexeme,
    ParserResult_UnknownToken,
    ParserResult_UnexpectedDataType,
    ParserResult_InvalidSyntax,
    
    ParserResult_Count,
};

enum LexemeType
{
    Lexeme_Identifier,
    Lexeme_Seperator,
    Lexeme_Operator,
    Lexeme_Literal,
    Lexeme_Keyword,
    
    Lexeme_LineComment,
    Lexeme_MultiLineComment,
    
    Lexeme_Count,
};

struct Lexeme
{
    LexemeType      type;
    char           *start;
    int             len;
    int             line;
};

struct Scanner
{
    char   *start;   // beginning of the file
    char   *end;     // end of the file
    char   *iter;    // current location in the file
    Lexeme *lexemes; // list of lexemes completed after scanning th efile
};

// Fun fact, Windows in all its glory decided to enumerate "TokenType"
// As if that is not a common parser name...
enum ParseTokenType
{
    // Identifiers
    Token_Identifier,
    
    // Seperators
    Token_OpenBrace,
    Token_CloseBrace,
    Token_OpenBracket,
    Token_CloseBracket,
    Token_OpenParenthesis,
    Token_CloseParenthesis,
    Token_At, // @
    Token_Colon,
    Token_SemiColon,
    Token_Backslash,
    Token_Comma,
    // Stripped Sperators
    Token_NewLine,
    
    // Operators
    Token_Asterisk, // Pointer / Pointer Dereference (stripped) / Multiply (stripped)
    // Stripped Operators
    Token_Equals,
    Token_ForwardSlash,
    Token_Plus,
    Token_Minus,
    Token_Amperstand,   // Bitwise And / Logical And / C++ Reference
    Token_VerticalLine, // Bitwise Or / Logical Or
    
    // Literals
    Token_String,
    Token_Boolean,
    Token_Integer,
    Token_Float,
    
    // Keywords
    // Basic types
    Token_Char,
    Token_S8,
    Token_S16,
    Token_S32,
    Token_S64,
    Token_U8,
    Token_U16,
    Token_U32,
    Token_U64,
    Token_F32,
    Token_F64,
    Token_SizeT,
    // structure identifiers
    Token_Struct,
    Token_Enum,
    // other
    Token_Const,
    
    Token_Count,
};

struct ParseToken
{
    ParseTokenType type;
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

struct Tokenizer
{
    ParseToken *tokens;
};

struct ParserPropertyTypeInfo
{
    const char *name;     // type name
    u32         name_len; // required for custom type info
    u32         id;       // type
    u32         flags;    // types flags (i.e. const, pointer)
};

struct ParserProperty
{
    const char            *name;     // field name
    u32                    name_len; // Parsed strings are not null terminated
    size_t                 offset;   // offset into struct
    ParserPropertyTypeInfo type;     // type info
    u32                    tags;     // member tags (i.e. PARSER_PROPERTY_TAG_NO_PRINT)
};

struct ParserClass
{
    const char     *name;              // (optional, if nested) struct name
    u32             name_len;          // (optional, if nested) struct name length, or unnamed struct nested idx
    size_t          size;              // total size of the class
    u32             type;              // type of class (ex. struct, union, etc.)
    u32             tags;              // class tags (i.e. PARSER_PROPERTY_TAG_PRINT)
    ParserProperty *properties;        // stb array, member list
    ParserClass    *nested_cls = 0;    // nested classes
};

struct ParserRegistry
{
    // stb array of classes - don't need to use a hashtable
    ParserClass *classes;
    
    // TODO(Dustin): List for include directives
    struct { char *file; int len; } *includes = 0;
};

enum ParserClassType
{
    PARSER_CLASS_TYPE_STRUCT  = 0x00,
    PARSER_CLASS_TYPE_UNION,
    PARSER_CLASS_TYPE_ENUM,
};

enum ParserPropertyType
{
    PARSER_PROPERTY_TYPE_S8 = 0x00,
    PARSER_PROPERTY_TYPE_S16,
    PARSER_PROPERTY_TYPE_S32,
    PARSER_PROPERTY_TYPE_S64,
    PARSER_PROPERTY_TYPE_BOOL,
    
    PARSER_PROPERTY_TYPE_U8,
    PARSER_PROPERTY_TYPE_U16,
    PARSER_PROPERTY_TYPE_U32,
    PARSER_PROPERTY_TYPE_U64,
    
    PARSER_PROPERTY_TYPE_F32,
    PARSER_PROPERTY_TYPE_F64,
    
    PARSER_PROPERTY_TYPE_SIZE_T,
    
    PARSER_PROPERTY_TYPE_STR,
    
    // Used for nested classes
    PARSER_PROPERTY_TYPE_CLASS, 
    PARSER_PROPERTY_TYPE_CLASS_UNNAMED, 
    
    // Used for external classes
    PARSER_PROPERTY_TYPE_CUSTOM,
    
    PARSER_PROPERTY_TYPE_COUNT,
};

enum ParserFlag
{
    PARSER_PROPERTY_FLAG_NONE  = 1<<0,
    PARSER_PROPERTY_FLAG_CONST = 1<<1, // const qualifier
    PARSER_PROPERTY_FLAG_PTR   = 1<<2, // pointer type
};

enum ParserTag
{
    // Default: No tags
    PARSER_PROPERTY_TAG_NONE         = 0<<0,
    // General meta tag - will only create metadata
    PARSER_PROPERTY_TAG_META         = 1<<0,
    
    // Enable tags for structs/enums
    PARSER_PROPERTY_TAG_PRINT        = 1<<1,
    PARSER_PROPERTY_TAG_SERIALIZE    = 1<<2,
};

// Each tag has a negate variant. Prepend "No" to the front of the tag.
// For example, if you do not want to print a particular field, use the 
// tag. "NoPrint". Negate tags should not be used at the struct level,
// and will be ignored if done. The default state of a struct is 
// PARSER_PROPERTY_TAG_NONE.
static const struct { char *n; u32 t; } ParserPropertyTagNames_LUT[] = {
    { "None",        PARSER_PROPERTY_TAG_NONE         },
    { "Meta",        PARSER_PROPERTY_TAG_META         },
    { "Print",       PARSER_PROPERTY_TAG_PRINT        },
    { "Serialize",   PARSER_PROPERTY_TAG_SERIALIZE    },
};

// NOTE(Dustin): Since C++ does not allow for out of order
// initialization, the order of these elements MUST be in the
// same order as ParserPropertyType
static const size_t ParserPropertySize_LUT[PARSER_PROPERTY_TYPE_COUNT] = {
    sizeof(int8_t),
    sizeof(int16_t),
    sizeof(int32_t),
    sizeof(int64_t),
    sizeof(bool),
    sizeof(uint8_t),
    sizeof(uint16_t),
    sizeof(uint32_t),
    sizeof(uint64_t),
    sizeof(float),
    sizeof(double),
    sizeof(size_t),
    sizeof(char*),
};

static ParserRegistry ParserRegistryNew();
static void  ParserRegistryFree(ParserRegistry *registry);

static ParserPropertyTypeInfo _ParserPropertyTypeInfoDecl_Impl(const char *name, u32 id);

#define parser_to_str(T) (const char*)#T

#define parser_property_type_info_decl(T, PARSE_TYPE) \
_ParserPropertyTypeInfoDecl_Impl(parser_to_str(T), (PARSE_TYPE))

#define parser_registry_register_class(PARSER, CLS) arrput((PARSER)->classes, (CLS))

#define parser_property_type_size(PARSE_TYPE) ParserPropertySize_LUT[(PARSE_TYPE)]

#define PARSER_PROPERTY_TYPE_INFO_S8     parser_property_type_info_decl(int8_t,   PARSER_PROPERTY_TYPE_S8)
#define PARSER_PROPERTY_TYPE_INFO_S16    parser_property_type_info_decl(int16_t,  PARSER_PROPERTY_TYPE_S16)
#define PARSER_PROPERTY_TYPE_INFO_S32    parser_property_type_info_decl(int32_t,  PARSER_PROPERTY_TYPE_S32)
#define PARSER_PROPERTY_TYPE_INFO_S64    parser_property_type_info_decl(int64_t,  PARSER_PROPERTY_TYPE_S64)
#define PARSER_PROPERTY_TYPE_INFO_BOOL   parser_property_type_info_decl(bool,     PARSER_PROPERTY_TYPE_BOOL)
#define PARSER_PROPERTY_TYPE_INFO_U8     parser_property_type_info_decl(uint8_t,  PARSER_PROPERTY_TYPE_U8)
#define PARSER_PROPERTY_TYPE_INFO_U16    parser_property_type_info_decl(uint16_t, PARSER_PROPERTY_TYPE_U16)
#define PARSER_PROPERTY_TYPE_INFO_U32    parser_property_type_info_decl(uint32_t, PARSER_PROPERTY_TYPE_U32)
#define PARSER_PROPERTY_TYPE_INFO_U64    parser_property_type_info_decl(uint64_t, PARSER_PROPERTY_TYPE_U64)
#define PARSER_PROPERTY_TYPE_INFO_F32    parser_property_type_info_decl(float,    PARSER_PROPERTY_TYPE_F32)
#define PARSER_PROPERTY_TYPE_INFO_F64    parser_property_type_info_decl(double,   PARSER_PROPERTY_TYPE_F64)
#define PARSER_PROPERTY_TYPE_INFO_SIZE_T parser_property_type_info_decl(size_t,   PARSER_PROPERTY_TYPE_SIZE_T)
#define PARSER_PROPERTY_TYPE_INFO_STR    parser_property_type_info_decl(char*,    PARSER_PROPERTY_TYPE_STR)

static ParserPropertyTypeInfo 
_ParserPropertyTypeInfoDecl_Impl(const char *name, u32 id)
{
    ParserPropertyTypeInfo result = {};
    result.name = name;
    result.id = id;
    return result;
}

static ParserRegistry 
ParserRegistryNew()
{
    ParserRegistry registry = {};
    registry.classes = 0;
    return registry;
}

static void  
ParserRegistryFree(ParserRegistry *registry)
{
    arrfree(registry->classes);
}

static int 
StrToInt(const char *start, const char *stop)
{
    int result = 0;
    for (const char *c = start; c < stop; ++c)
    {
        result = result * 10 + c[0] - '0';
    }
    return result;
}

static float 
StrToFloat(const char *ptr)
{
    // TODO(Dustin): Implement a more reliable parser for floats
    float result = strtof(ptr, 0);
    return result;
}

static bool
IsChar(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static bool 
IsDigit(char c)
{
    return c >= '0' && c <= '9';
}

static bool
IsSkippableChar(char c)
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
LexemeIsIdentifier(const char *buffer)
{
    return IsChar(buffer[0]) || buffer[0] == '_';
}

static bool
LexemeIsSeperator(const char *buffer)
{
    bool result;
    switch (buffer[0])
    {
        case '<':
        case '>':
        case '@':
        case '#':
        case '[':
        case ']':
        case '{':
        case '}':
        case '(':
        case ')':
        case ',':
        case ';':
        case ':':
        case '\\':
        case '\n': result = true;  break;
        default:   result = false; break;
    }
    return result;
}

static bool
LexemeIsKeyword(const char *buffer)
{
    bool result;
    switch (buffer[0])
    {
        case 's':
        {
            if (strncmp(&buffer[1], "truct", 5) == 0) result = true;
        } break;
        
        case 'e':
        {
            if (strncmp(&buffer[1], "num", 3) == 0) result = true;
        } break;
        
        case 'i':
        {
            if      (strncmp(&buffer[1], "nt8_t", 5) == 0)  result = true;
            else if (strncmp(&buffer[1], "nt16_t", 6) == 0) result = true;
            else if (strncmp(&buffer[1], "nt32_t", 6) == 0) result = true;
            else if (strncmp(&buffer[1], "nt64_t", 6) == 0) result = true;
            else if (strncmp(&buffer[1], "nt", 2) == 0)     result = true;
            else if (strncmp(&buffer[1], "ize_t", 5) == 0)  result = true;
        } break;
        
        case 'u':
        {
            if      (strncmp(&buffer[1], "int8_t", 6) == 0)  result = true;
            else if (strncmp(&buffer[1], "int16_t", 7) == 0) result = true;
            else if (strncmp(&buffer[1], "int32_t", 7) == 0) result = true;
            else if (strncmp(&buffer[1], "int64_t", 7) == 0) result = true;
            else if (strncmp(&buffer[1], "int", 3) == 0)     result = true;
        } break;
        
        case 'f':
        {
            if (strncmp(&buffer[1], "loat", 4) == 0) result = true;
        } break;
        
        case 'd':
        {
            if (strncmp(&buffer[1], "ouble", 5) == 0) result = true;
        } break;
        
        case 'c':
        {
            if      (strncmp(&buffer[1], "onst", 4) == 0) result = true;
            else if (strncmp(&buffer[1], "har", 3) == 0)  result = true;
        } break;
        
        default:   result = false; break;
    }
    return result;
}

static bool
LexemeIsOperator(const char *buffer)
{
    bool result = false;
    if (buffer[0] == '=')
        result = true;
    else if (buffer[0] == '*')
        result = true;
    else if (buffer[0] == '/')
        result = true;
    else if (buffer[0] == '+')
        result = true;
    else if (buffer[0] == '-')
        result = true;
    else if (buffer[0] == '&')
        result = true;
    else if (buffer[0] == '|')
        result = true;
    return result;
}

static bool
LexemeIsLiteral(const char *buffer)
{
    bool result = false;
    
    if (buffer[0] == '\"')
        result = true;
    else if (IsDigit(buffer[0]))
        result = true;
    else if (buffer[0] == '.')
        result = true;
    else if ((strncmp(buffer, "true", 4) == 0) || (strncmp(buffer, "false", 5) == 0))
        result = true;
    
    return result;
}

static bool
LexemeIsSingleLineComment(const char *buffer)
{
    bool result = false;
    if (buffer[0] == '/' && buffer[1] == '/') 
        result = true;
    return result;
}

static bool
LexemeIsMultiLineComment(const char *buffer)
{
    bool result = false;
    if (buffer[0] == '/' && buffer[1] == '*') 
        result = true;
    else if (buffer[0] == '*' && buffer[1] == '/')
        result = true;
    return result;
}

static ParserResult
ScanFile(Scanner *scanner)
{
    ParserResult result = ParserResult_Success;
    char *iter = scanner->start;
    int   line = 0;
    while (iter < scanner->end)
    {
        // Skip any unnecessary characters
        while (IsSkippableChar(iter[0]))
            ++iter;
        if (iter >= scanner->end) break;
        
        Lexeme lex = {};
        lex.type = Lexeme_Count;
        lex.line = line;
        
        if (LexemeIsSingleLineComment(iter))
        {
            lex.type  = Lexeme_LineComment;
            lex.start = iter;
            { // consume the identifier
                do { ++iter; } 
                while (iter < scanner->end && iter[0] != '\n');
            }
            lex.len = (int)(iter - lex.start);
        }
        else if (LexemeIsMultiLineComment(iter))
        {
            lex.type  = Lexeme_MultiLineComment;
            lex.start = iter;
            { // consume the identifier
                ++iter;
                do { ++iter; } 
                while (!LexemeIsMultiLineComment(iter));
            }
            lex.len = (int)(iter - lex.start);
        }
        else if (LexemeIsSeperator(iter))
        {
            if (iter[0] == '\n')
                ++line;
            
            lex.type  = Lexeme_Seperator;
            lex.start = iter;
            lex.len   = 1;
            iter += lex.len;
        }
        else if (LexemeIsOperator(iter))
        {
            lex.type  = Lexeme_Operator;
            lex.start = iter;
            lex.len   = 1;
            iter     += lex.len;
        }
        else if (LexemeIsKeyword(iter))
        {
            lex.type = Lexeme_Keyword;
            lex.start = iter;
            { // consume the identifier
                do { ++iter; } 
                while (IsDigit(iter[0]) || IsChar(iter[0]) || iter[0] == '_');
            }
            lex.len = (int)(iter - lex.start);
        }
        else if (LexemeIsLiteral(iter))
        {
            lex.type  = Lexeme_Literal;
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
                while (IsDigit(iter[0]) || IsChar(iter[0]) || iter[0] == '.');
            }
            lex.len = (int)(iter - lex.start);
        }
        else if (LexemeIsIdentifier(iter))
        {
            lex.type = Lexeme_Identifier;
            lex.start = iter;
            { // consume the identifier
                while (IsDigit(iter[0]) || IsChar(iter[0]) || iter[0] == '_')
                { ++iter; } 
            }
            lex.len = (int)(iter - lex.start);
        }
        else
        { // Unknown lexeme
            result = ParserResult_UnknownLexeme;
            return result;
        }
        
        arrput(scanner->lexemes, lex);
    }
    return result;
}

//
// Tokenizer should strip:
// - Newline chars
// - Comments
// - Expressions/function definitions
// - All operators except: * (pointers), : (bitfields) (MAYBE FUTURE EDITION?)
// - Preprocessor definitions (# -> consume the entire line)
// - < / > seperators (consume until closing lexeme)
// 

static ParserResult
TokenizeSeperator(ParseToken *tok, Lexeme **lexemes, Lexeme *iter_end)
{
    ParserResult result = ParserResult_Success;
    
    Lexeme *iter = *lexemes;
    
    tok->type  = Token_Count;
    tok->start = iter->start;
    tok->len   = iter->len;
    tok->line  = iter->line;
    
    switch (tok->start[0])
    {
        case '[':  tok->type = Token_OpenBrace;        break;
        case ']':  tok->type = Token_CloseBrace;       break;
        case '{':  tok->type = Token_OpenBracket;      break;
        case '}':  tok->type = Token_CloseBracket;     break;
        case '(':  tok->type = Token_OpenParenthesis;  break;
        case ')':  tok->type = Token_CloseParenthesis; break;
        case '@':  tok->type = Token_At;               break;
        case ';':  tok->type = Token_SemiColon;        break;
        case ':':  tok->type = Token_Colon;            break;
        case '\\': tok->type = Token_Backslash;        break;
        case ',':  tok->type = Token_Comma;            break;
        
        // Stripped tokens:
        case '#':
        case '<':
        case '>':
        case '\n': break;
        default: result = ParserResult_InvalidSyntax; break;
    }
    
    ++iter;
    *lexemes += iter - *lexemes;
    
    return result;
}

static ParserResult
TokenizeOperator(ParseToken *tok, Lexeme **lexemes, Lexeme *iter_end)
{
    ParserResult result = ParserResult_Success;
    
    Lexeme *iter = *lexemes;
    
    tok->type  = Token_Count;
    tok->start = iter->start;
    tok->len   = iter->len;
    tok->line  = iter->line;
    
    switch (tok->start[0])
    {
        case '*':  tok->type = Token_Asterisk; break;
        
        // Stripped tokens:
        case '=': 
        case '/': 
        case '+': 
        case '-':  
        case '&': 
        case '|': break;
        default: result = ParserResult_InvalidSyntax; break;
    }
    
    ++iter;
    *lexemes += iter - *lexemes;
    
    return result;
}

static ParserResult
TokenizeKeyword(ParseToken *tok, Lexeme **lexemes, Lexeme *iter_end)
{
    ParserResult result = ParserResult_Success;
    
    Lexeme *iter = *lexemes;
    
    tok->type  = Token_Count;
    tok->start = iter->start;
    tok->len   = iter->len;
    tok->line  = iter->line;
    
    switch (tok->start[0])
    {
        case 's':
        {
            if (strncmp(&tok->start[1], "truct", 5) == 0) tok->type = Token_Struct; break;
        } break;
        
        case 'e':
        {
            if (strncmp(&tok->start[1], "num", 3) == 0) tok->type = Token_Enum; break;
        } break;
        
        case 'i':
        {
            if      (strncmp(&tok->start[1], "nt8_t", 5) == 0)  { tok->type = Token_S8;   }
            else if (strncmp(&tok->start[1], "nt16_t", 6) == 0) { tok->type = Token_S16;  }
            else if (strncmp(&tok->start[1], "nt32_t", 6) == 0) { tok->type = Token_S32;  } 
            else if (strncmp(&tok->start[1], "nt64_t", 6) == 0) { tok->type = Token_S32;  } 
            else if (strncmp(&tok->start[1], "nt", 2) == 0)     { tok->type = Token_S32;  }
            else if (strncmp(&tok->start[1], "ize_t", 5) == 0)  { tok->type = Token_SizeT;}
        } break;
        
        case 'u':
        {
            if      (strncmp(&tok->start[1], "int8_t", 6) == 0)  { tok->type = Token_U8;  }
            else if (strncmp(&tok->start[1], "int16_t", 7) == 0) { tok->type = Token_U16; }
            else if (strncmp(&tok->start[1], "int32_t", 7) == 0) { tok->type = Token_U32; }
            else if (strncmp(&tok->start[1], "int64_t", 7) == 0) { tok->type = Token_U64; }
            else if (strncmp(&tok->start[1], "int", 3) == 0)     { tok->type = Token_U32; }
        } break;
        
        case 'f':
        {
            if (strncmp(&tok->start[1], "loat", 4) == 0) tok->type = Token_F32; break;
        } break;
        
        case 'd':
        {
            if (strncmp(&tok->start[1], "ouble", 5) == 0) tok->type = Token_F64; break;
        } break;
        
        case 'c':
        {
            if      (strncmp(&tok->start[1], "onst", 4) == 0) { tok->type = Token_Char;  break; }
            else if (strncmp(&tok->start[1], "har", 3) == 0)  { tok->type = Token_Const; break; }
        } break;
        
        default:   result = ParserResult_InvalidSyntax; break;
    }
    
    ++iter;
    *lexemes += iter - *lexemes;
    
    return result;
}


static ParserResult
TokenizeLiteral(ParseToken *tok, Lexeme **lexemes, Lexeme *iter_end)
{
    ParserResult result = ParserResult_Success;
    
    Lexeme *iter = *lexemes;
    
    tok->type  = Token_Count;
    tok->start = iter->start;
    tok->len   = iter->len;
    tok->line  = iter->line;
    
    if (tok->start[0] == '\"')
    {
        tok->type = Token_String;
        tok->len -= 2; // consume the quoations
        tok->s = tok->start + 1;
    }
    else if (tok->start[0] == 't')
    {
        tok->type = Token_Boolean;
        tok->b = true;
    }
    else if (tok->start[0] == 'f')
    {
        tok->type = Token_Boolean;
        tok->b = false;
    }
    else if (IsDigit(tok->start[0]) || tok->start[0] == '.')
    {
        tok->type = Token_Integer;
        // determine if token is an integer or float
        for (int c = 0; c < tok->len; ++c)
        {
            if (tok->start[c] == '.')
            {
                tok->type = Token_Float;
                break;
            }
        }
        
        if (tok->type == Token_Integer)
            tok->i = StrToInt(tok->start, tok->start + tok->len);
        else
            tok->f = StrToFloat(tok->start);
    }
    else
    {
        result = ParserResult_InvalidSyntax;
    }
    
    ++iter;
    *lexemes += iter - *lexemes;
    
    return result;
}

static ParserResult
TokenizeFile(Tokenizer *tokenizer, Scanner *scanner)
{
    ParserResult result = ParserResult_Success;
    
    Lexeme *iter = scanner->lexemes;
    Lexeme *iter_end = scanner->lexemes + arrlen(scanner->lexemes);
    
    //for (int i = 0; i < (int)arrlen(scanner->lexemes); ++i)
    while (iter != iter_end)
    {
        ParseToken token = {};
        
        switch (iter->type)
        {
            case Lexeme_Identifier:
            {
                token.type  = Token_Identifier;
                token.start = iter->start;
                token.len   = iter->len;
                token.line  = iter->line;
                ++iter;
            } break;
            
            case Lexeme_Seperator:
            {
                ParserResult res = TokenizeSeperator(&token, &iter, iter_end);
                if (res != ParserResult_Success) return res;
            } break;
            
            case Lexeme_Operator:
            {
                ParserResult res = TokenizeOperator(&token, &iter, iter_end);
                if (res != ParserResult_Success) return res;
            } break;
            
            case Lexeme_Keyword:
            {
                ParserResult res = TokenizeKeyword(&token, &iter, iter_end);
                if (res != ParserResult_Success) return res;
            } break;
            
            case Lexeme_Literal:
            {
                ParserResult res = TokenizeLiteral(&token, &iter, iter_end);
                if (res != ParserResult_Success) return res;
            } break;
            
            // comments are stripped
            case Lexeme_LineComment:
            case Lexeme_MultiLineComment:
            default:  ++iter; continue;
        }
        
        if (token.type < Token_Count) arrput(tokenizer->tokens, token);
    }
    
    return result;
}

static size_t
ParserGetStructSizeByName(ParserClass *clss, const char *name, u32 name_len)
{
    size_t size = 0;
    for (u32 i = 0; i < (u32)arrlen(clss); ++i)
    {
        ParserClass *cls = clss + i;
        if (name_len == cls->name_len && strncmp(cls->name, name, name_len) == 0)
        {
            size = cls->size;
            break;
        }
    }
    return size;
}

static size_t
ParserGetStructSizeByIndex(ParserClass *clss, u32 idx)
{
    size_t size = clss[idx].size;
    return size;
}

static void
EmitErrorMessage(const char *msg)
{
    LogError("%s", msg);
}

// Consumes the token if the token type matches, otherwise returns false and emits an
// error msg.
static bool
ParserConsumeToken(ParseToken **tokens, ParseTokenType tok_type, const char *err_msg)
{
    bool result = true;
    if ((*tokens)->type == tok_type) 
    {
        (*tokens) += 1;
    }
    else
    {
        EmitErrorMessage(err_msg);
        result = false;
    }
    return false;
} 

// Consumes a token if the type matches, otherwise returns false.
static bool 
ParserMatchToken(ParseToken **tokens, ParseTokenType tok_type)
{
    bool result = true;
    if ((*tokens)->type == tok_type) 
    {
        (*tokens) += 1;
    }
    else
    {
        result = false;
    }
    return result;
}

static ParserResult
ParseTags(ParserRegistry *parser, u32 *tag_result, ParseToken **tokens, ParseToken *iter_end)
{
    // Possible Syntax:
    // @Tag0
    // @Tag1
    // @(Tag1)
    // @(Tag1, Tag2)
    // valid tokens: Identifiers, Commans, Open/Close Parenthesis
    
    ParserResult result = ParserResult_Success;
    u32 tags = *tag_result;
    ParseToken *iter = *tokens;
    
    // open paren  -> inc level
    // comma       -> inc level
    // identifier  -> dec level
    // close paren -> dec level
    u32 level = 1;
    while (iter != iter_end && level > 0)
    {
        switch (iter->type)
        {
            case Token_OpenParenthesis:  ++level; break;
            case Token_CloseParenthesis: --level; break;
            case Token_Comma:            ++level; break;
            
            case Token_Identifier:
            {
                // Two types of tag: Set and Negate flags. All Negate flags start with
                // the pair "No".
                
                char *tmp = iter->start;
                u32 len = iter->len;
                
                bool is_negate = false;
                if (strncmp("No", iter->start, 2) == 0)
                { // We are negating the flag
                    is_negate = true;
                    tmp += 2;
                    len -= 2;
                }
                
                for (u32 i = 0; i < ARRAYCOUNT(ParserPropertyTagNames_LUT); ++i)
                {
                    if (strncmp(ParserPropertyTagNames_LUT[i].n, tmp, len) == 0)
                    {
                        if (is_negate)
                            tags &= ~ParserPropertyTagNames_LUT[i].t;
                        else
                            tags |= ParserPropertyTagNames_LUT[i].t;
                        
                        --level;
                        break;
                    }
                }
            } break;
            
            default:
            {
                result = ParserResult_ParseError;
                return result;
            } break;
        }
        
        ++iter;
    }
    
    if (level != 0) result = ParserResult_ParseError;
    
    *tag_result = tags;
    *tokens = iter;
    return result;
}

static u32
ParserGetNestedClassTagByName(ParserClass *classes, u32 count, const char *name, u32 len)
{
    u32 tag = 0;
    for (u32 i = 0; i < count; ++i)
    {
        if (classes[i].name && strncmp(classes[i].name, name, len) == 0)
        {
            tag = classes[i].tags;
            break;
        }
    }
    return tag;
}

static i32
ParserGetNestedClassIdxByName(ParserClass *classes, u32 count, const char *name, u32 len)
{
    i32 idx = -1;
    for (u32 i = 0; i < count; ++i)
    {
        if (classes[i].name && strncmp(classes[i].name, name, len) == 0)
        {
            idx = i;
            break;
        }
    }
    return idx;
}

static u32
ParserGetNestedClassTagByIdx(ParserClass *classes, u32 count, u32 idx)
{
    u32 tag = 0;
    if (idx < count)
    {
        tag = classes[idx].tags;
    }
    return tag;
}

#if 1

static ParserResult
ParseStruct(ParserRegistry *parser, ParserClass *cls, ParseToken **tokens, ParseToken *iter_end)
{
    ParserResult result = ParserResult_Success;
    ParseToken *iter = *tokens;
    ParseToken *tok = iter;
    
    if (ParserMatchToken(&iter, Token_Identifier))
    {
        cls->name = tok->start;
        cls->name_len = tok->len;
    }
    
    ParserConsumeToken(&iter, Token_OpenBracket, "Expected open bracket after struct identifier");
    
    u32 tags = cls->tags;
    size_t offset = 0;
    while (iter < iter_end && iter->type != Token_CloseBracket)
    {
        // Tag Syntax: @Tag / @(Tag,Tag2) / @Tag1 @Tag2
        // Syntax:     [Type|Identifier] [Identifier] [SemiColon]
        bool parsing_intern_struct = false;
        ParserPropertyTypeInfo type_info = {};
        ParserClass intern_cls = {};
        u32 nested_tags = tags;
        
        //
        // Field Tags
        //
        
        if (ParserMatchToken(&iter, Token_At))
        {
            result = ParseTags(parser, &tags, &iter, iter_end);
            if (result != ParserResult_Success) return result;
            continue;
        }
        
        //
        // Field Type
        //
        
        tok = iter;
        if (ParserMatchToken(&iter, Token_Struct))
        {
            parsing_intern_struct = true;
            
            intern_cls.properties = 0;
            intern_cls.tags = tags;
            result = ParseStruct(parser, &intern_cls, &iter, iter_end);
            if (result != ParserResult_Success) return result;
            
            arrput(cls->nested_cls, intern_cls);
            tags = cls->tags;
            
            type_info.name_len = intern_cls.name_len;
            type_info.name = intern_cls.name;
            if (intern_cls.name)
                type_info.id   = PARSER_PROPERTY_TYPE_CLASS;
            else
            {
                type_info.id   = PARSER_PROPERTY_TYPE_CLASS_UNNAMED;
                type_info.name_len = (u32)arrlen(cls->nested_cls) - 1;
            }
            
            // Cache nested class's tags in case there is a same-line variable
            nested_tags = intern_cls.tags;
        }
        else if (ParserMatchToken(&iter, Token_Char))  type_info = PARSER_PROPERTY_TYPE_INFO_STR;
        else if (ParserMatchToken(&iter, Token_S8))    type_info = PARSER_PROPERTY_TYPE_INFO_S8;
        else if (ParserMatchToken(&iter, Token_S16))   type_info = PARSER_PROPERTY_TYPE_INFO_S16;
        else if (ParserMatchToken(&iter, Token_S32))   type_info = PARSER_PROPERTY_TYPE_INFO_S32;
        else if (ParserMatchToken(&iter, Token_S64))   type_info = PARSER_PROPERTY_TYPE_INFO_S64;
        else if (ParserMatchToken(&iter, Token_U8))    type_info = PARSER_PROPERTY_TYPE_INFO_U8;
        else if (ParserMatchToken(&iter, Token_U16))   type_info = PARSER_PROPERTY_TYPE_INFO_U16;
        else if (ParserMatchToken(&iter, Token_U32))   type_info = PARSER_PROPERTY_TYPE_INFO_U32;
        else if (ParserMatchToken(&iter, Token_U64))   type_info = PARSER_PROPERTY_TYPE_INFO_U64;
        else if (ParserMatchToken(&iter, Token_F32))   type_info = PARSER_PROPERTY_TYPE_INFO_F32;
        else if (ParserMatchToken(&iter, Token_F64))   type_info = PARSER_PROPERTY_TYPE_INFO_F64;
        else if (ParserMatchToken(&iter, Token_SizeT)) type_info = PARSER_PROPERTY_TYPE_INFO_SIZE_T;
        else if (ParserMatchToken(&iter, Token_Identifier))
        {
            type_info.name_len = tok->len;
            type_info.name = tok->start;
            type_info.id   = PARSER_PROPERTY_TYPE_CUSTOM;
        }
        else
        {
            EmitErrorMessage("Expected keyword or identfier inside of struct declaration.");
            result = ParserResult_InvalidSyntax;
            return result;
        }
        
        if (iter >= iter_end)
        {
            result = ParserResult_InvalidSyntax;
            return result;
        }
        
        //
        // Field Flags
        //
        
        // TODO(Dustin): Handle pointers AND/OR const
        tok = iter;
        if (ParserMatchToken(&iter, Token_Const))
        { // TODO(Dustin): 
        }
        
        tok = iter;
        if (ParserMatchToken(&iter, Token_Asterisk))
        { // TODO(Dustin):
        }
        
        //
        // Field Name
        //
        
        tok = iter;
        if (ParserMatchToken(&iter, Token_Identifier))
        {
            ParserProperty property = {};
            property.name     = tok->start;
            property.name_len = tok->len;
            property.offset   = offset;
            property.tags     = tags;
            tags = cls->tags;
            
            if (type_info.id == PARSER_PROPERTY_TYPE_CLASS || type_info.id == PARSER_PROPERTY_TYPE_CLASS_UNNAMED)
            {
                // The nested class might have different tags than the current set tags. An AND op
                // should get the resulting tags.
                property.tags &= nested_tags;
            }
            else if (type_info.id == PARSER_PROPERTY_TYPE_CUSTOM)
            {
                i32 idx = ParserGetNestedClassIdxByName(cls->nested_cls, (u32)arrlen(cls->nested_cls), 
                                                        type_info.name, type_info.name_len);
                
                if (idx >= 0)
                { // this is actually an internal class
                    type_info.id = PARSER_PROPERTY_TYPE_CLASS;
                }
            }
            
            property.type = type_info;
            
            if (type_info.id == PARSER_PROPERTY_TYPE_CUSTOM)
            {
                offset += ParserGetStructSizeByName(parser->classes, property.type.name, property.type.name_len);
            }
            else if (type_info.id == PARSER_PROPERTY_TYPE_CLASS)
            {
                offset += ParserGetStructSizeByName(cls->nested_cls, property.type.name, property.type.name_len);
            }
            else if (type_info.id == PARSER_PROPERTY_TYPE_CLASS_UNNAMED)
            {
                offset += ParserGetStructSizeByIndex(cls->nested_cls, property.type.name_len);
            }
            else
                offset += parser_property_type_size(type_info.id);
            
            arrput(cls->properties, property);
        }
        
        ParserConsumeToken(&iter, Token_SemiColon, "Expected semicolon after field defintion!");
    }
    
    ParserConsumeToken(&iter, Token_CloseBracket, "Expected closed bracket after struct defintion!");
    cls->size = offset;
    *tokens = iter;
    
    return result;
}

#else

static ParserResult
ParseStruct(ParserRegistry *parser, ParserClass *cls, ParseToken **tokens, ParseToken *iter_end)
{
    ParserResult result = ParserResult_Success;
    ParseToken *iter = *tokens;
    ParseToken *tok = iter;
    
    if (ParserMatchToken(&iter, Token_Identifier))
    {
        cls->name = tok->start;
        cls->name_len = tok->len;
    }
    
    ParserConsumeToken(&iter, Token_OpenBracket, "Expected open bracket after struct identifier");
    
    u32 tags = cls->tags;
    size_t offset = 0;
    while (iter < iter_end && iter->type != Token_CloseBracket)
    {
        // Tag Syntax: @Tag / @(Tag,Tag2) / @Tag1 @Tag2
        // Syntax:     [Type|Identifier] [Identifier] [SemiColon]
        bool parsing_intern_struct = false;
        ParserPropertyTypeInfo type_info = {};
        ParserProperty property = {};
        ParserClass intern_cls = {};
        
        if (ParserMatchToken(&iter, Token_At))
        {
            result = ParseTags(parser, &tags, &iter, iter_end);
            if (result != ParserResult_Success) return result;
            continue;
        }
        
        tok = iter;
        if (ParserMatchToken(&iter, Token_Struct))
        {
            parsing_intern_struct = true;
            
            intern_cls.properties = 0;
            intern_cls.tags = tags;
            result = ParseStruct(parser, &intern_cls, &iter, iter_end);
            if (result != ParserResult_Success) return result;
            
            arrput(cls->nested_cls, intern_cls);
            tags = cls->tags;
        }
        else if (ParserMatchToken(&iter, Token_Char))  type_info = PARSER_PROPERTY_TYPE_INFO_STR;
        else if (ParserMatchToken(&iter, Token_S8))    type_info = PARSER_PROPERTY_TYPE_INFO_S8;
        else if (ParserMatchToken(&iter, Token_S16))   type_info = PARSER_PROPERTY_TYPE_INFO_S16;
        else if (ParserMatchToken(&iter, Token_S32))   type_info = PARSER_PROPERTY_TYPE_INFO_S32;
        else if (ParserMatchToken(&iter, Token_S64))   type_info = PARSER_PROPERTY_TYPE_INFO_S64;
        else if (ParserMatchToken(&iter, Token_U8))    type_info = PARSER_PROPERTY_TYPE_INFO_U8;
        else if (ParserMatchToken(&iter, Token_U16))   type_info = PARSER_PROPERTY_TYPE_INFO_U16;
        else if (ParserMatchToken(&iter, Token_U32))   type_info = PARSER_PROPERTY_TYPE_INFO_U32;
        else if (ParserMatchToken(&iter, Token_U64))   type_info = PARSER_PROPERTY_TYPE_INFO_U64;
        else if (ParserMatchToken(&iter, Token_F32))   type_info = PARSER_PROPERTY_TYPE_INFO_F32;
        else if (ParserMatchToken(&iter, Token_F64))   type_info = PARSER_PROPERTY_TYPE_INFO_F64;
        else if (ParserMatchToken(&iter, Token_SizeT)) type_info = PARSER_PROPERTY_TYPE_INFO_SIZE_T;
        else if (ParserMatchToken(&iter, Token_Identifier))
        {
            type_info.name_len = tok->len;
            type_info.name = tok->start;
            type_info.id   = PARSER_PROPERTY_TYPE_CUSTOM;
        }
        else
        {
            EmitErrorMessage("Expected keyword or identfier inside of struct declaration.");
            result = ParserResult_InvalidSyntax;
            return result;
        }
        
        if (iter >= iter_end)
        {
            result = ParserResult_InvalidSyntax;
            return result;
        }
        
        // TODO(Dustin): Handle pointers AND/OR const
        
        tok = iter;
        if (ParserMatchToken(&iter, Token_Identifier))
        {
            property.name = tok->start;
            property.name_len = tok->len;
            property.offset   = offset;
            property.tags     = tags;
            tags = cls->tags;
            
            // TODO(Dustin): Use index only lookups instead of name/name_len
            if (parsing_intern_struct)
            {
                if (intern_cls.name)
                {
                    u32 parent_tags = ParserGetNestedClassTagByName(cls->nested_cls, (u32)arrlen(cls->nested_cls), 
                                                                    intern_cls.name, intern_cls.name_len);
                    
                    // The nested class might have different tags than the current set tags. An AND op
                    // should get the resulting tags.
                    property.tags &= parent_tags;
                    
                    type_info.name_len = intern_cls.name_len;
                    type_info.name     = intern_cls.name;
                }
                else
                { // use name len as the index for the unnamed type
                    u32 parent_tags = ParserGetNestedClassTagByIdx(cls->nested_cls, (u32)arrlen(cls->nested_cls), 
                                                                   (u32)arrlen(cls->nested_cls) - 1);
                    
                    // The nested class might have different tags than the current set tags. An AND op
                    // should get the resulting tags.
                    property.tags &= parent_tags;
                    
                    type_info.name_len = (u32)arrlen(cls->nested_cls) - 1;
                    type_info.name     = 0;
                }
                
                type_info.id  = PARSER_PROPERTY_TYPE_STRUCT;
                property.type = type_info;
            }
            else
                property.type = type_info;
            
            if (type_info.id == PARSER_PROPERTY_TYPE_CUSTOM)
                offset += ParserGetStructSizeByName(parser->classes, property.type.name, property.type.name_len);
            else if (type_info.id == PARSER_PROPERTY_TYPE_STRUCT)
            {
                if (property.type.name)
                    offset += ParserGetStructSizeByName(cls->nested_cls, property.type.name, property.type.name_len);
                else
                    offset += ParserGetStructSizeByIndex(cls->nested_cls, property.type.name_len);
            }
            else
                offset += parser_property_type_size(type_info.id);
            
            arrput(cls->properties, property);
        }
        else if (!parsing_intern_struct)
        {
            EmitErrorMessage("Expected field name after type!");
            result = ParserResult_InvalidSyntax;
            return result;
        }
        
        ParserConsumeToken(&iter, Token_SemiColon, "Expected semicolon after field defintion!");
    }
    
    ParserConsumeToken(&iter, Token_CloseBracket, "Expected semicolon after field defintion!");
    cls->size = offset;
    *tokens = iter;
    
    return result;
}

#endif

// TODO(Dustin): Use better name...
static ParserResult
RunParserPass(ParserRegistry *parser, Tokenizer *tokenizer)
{
    ParserResult result = ParserResult_Success;
    
    ParseToken *iter = tokenizer->tokens;
    ParseToken *iter_end = tokenizer->tokens + arrlen(tokenizer->tokens);
    
    u32 cls_tags = 0;
    while (iter != iter_end)
    {
        if (iter->type == Token_Struct)
        {
            ParserClass cls = {};
            cls.name = 0;
            cls.name_len = 0;
            cls.properties = 0;
            cls.nested_cls = 0;
            cls.tags = cls_tags;
            
            ++iter;
            result = ParseStruct(parser, &cls, &iter, iter_end);
            if (result != ParserResult_Success) return result;
            if (cls.name_len <= 0)
            {
                EmitErrorMessage("Expected name for struct, but was not found!");
                result = ParserResult_ParseError;
                return result;
            }
            
            
            ParserConsumeToken(&iter, Token_SemiColon, "Expected semicolon after struct defintion!");
            arrput(parser->classes, cls);
            cls_tags = 0;
        }
        else if (iter->type == Token_At)
        {
            ++iter;
            result = ParseTags(parser, &cls_tags, &iter, iter_end);
            if (result != ParserResult_Success) return result;
        }
        // TODO(Dustin): Enums
        else
        {
            ++iter;
        }
    }
    
    return result;
}

static void
ScannerPrintResults(Scanner *scanner)
{
    PrettyBuffer buffer = {};
    pb_init(&buffer, _KB(1));
    
    pb_write(&buffer, "[\n");
    
    for (int i = 0; i < (int)arrlen(scanner->lexemes); ++i)
    {
        Lexeme *lex = scanner->lexemes + i;
        Str str = StrInit(lex->len, lex->start);
        
        pb_write(&buffer, "\ttype: ");
        switch (lex->type)
        {
            case Lexeme_Identifier:
            {
                pb_write(&buffer, "IDENTIFIER, value: %s\n", StrGetString(&str));
            } break;
            
            case Lexeme_Seperator:
            {
                if (lex->start[0] == '\n')
                    pb_write(&buffer, "SEPERATOR, value: newline\n");
                else
                    pb_write(&buffer, "SEPERATOR, value: %s\n", StrGetString(&str));
            } break;
            
            case Lexeme_Operator:
            {
                pb_write(&buffer, "OPERATOR, value: %s\n", StrGetString(&str));
            } break;
            
            case Lexeme_Literal:
            {
                pb_write(&buffer, "LITERAL, value: %s\n", StrGetString(&str));
            } break;
            
            case Lexeme_Keyword:
            {
                pb_write(&buffer, "KEYWORD, value: %s\n", StrGetString(&str));
            } break;
            
            case Lexeme_LineComment:
            {
                pb_write(&buffer, "Single Line COMMENT, value: %s\n", StrGetString(&str));
            } break;
            
            case Lexeme_MultiLineComment:
            {
                pb_write(&buffer, "Multi Line COMMENT, value: %s\n", StrGetString(&str));
            } break;
            
            StrFree(&str);
        }
    }
    
    pb_write(&buffer, "]\n");
    
    buffer.current[0] = 0;
    LogInfo("Scanner Results:\n%s", buffer.start);
    
    pb_free(&buffer);
}

static void
TokenizerPrintResults(Tokenizer *tokenizer)
{
    PrettyBuffer buffer = {};
    pb_init(&buffer, _KB(1));
    
    pb_write(&buffer, "[\n");
    
    for (int i = 0; i < (int)arrlen(tokenizer->tokens); ++i)
    {
        ParseToken *tok = tokenizer->tokens + i;
        
        switch (tok->type)
        {
            case Token_Identifier:
            {
                Str str = StrInit(tok->len, tok->start);
                pb_write(&buffer, "\tTOKEN_IDENTIFIER, value: %s\n", StrGetString(&str));
                StrFree(&str);
            }break;
            
            case Token_OpenBrace:
            {
                pb_write(&buffer, "\tTOKEN_OPENBRACE\n");
            }break;
            
            case Token_CloseBrace:
            {
                pb_write(&buffer, "\tTOKEN_CLOSEBRACE\n");
            }break;
            
            case Token_OpenBracket:
            {
                pb_write(&buffer, "\tTOKEN_OPENBRACKET\n");
            }break;
            
            case Token_CloseBracket:
            {
                pb_write(&buffer, "\tTOKEN_CLOSEBRACKET\n");
            }break;
            
            case Token_OpenParenthesis:
            {
                pb_write(&buffer, "\tTOKEN_OPENPARENTHESIS\n");
            } break;
            
            case Token_CloseParenthesis:
            {
                pb_write(&buffer, "\tTOKEN_CLOSEPARENTHESIS\n");
            } break;
            
            case Token_At: // @
            {
                pb_write(&buffer, "\tTOKEN_AT\n");
            } break;
            
            case Token_Colon:
            {
                pb_write(&buffer, "\tTOKEN_COLON\n");
            } break;
            
            case Token_SemiColon:
            {
                pb_write(&buffer, "\tTOKEN_SEMICOLON\n");
            } break;
            
            case Token_Backslash:
            {
                pb_write(&buffer, "\tTOKEN_BACKSLASH\n");
            } break;
            
            case Token_Asterisk:
            {
                pb_write(&buffer, "\tTOKEN_ASTERISK\n");
            }break;
            
            case Token_String:
            {
                Str str = StrInit(tok->len, tok->s);
                pb_write(&buffer, "\tTOKEN_STRING, value: %s\n", StrGetString(&str));
                StrFree(&str);
            }break;
            
            case Token_Boolean:
            {
                pb_write(&buffer, "\tTOKEN_BOOLEAN, value: %s\n", (tok->b) ? "true" : "false");
            }break;
            
            case Token_Integer:
            {
                pb_write(&buffer, "\tTOKEN_INTEGER, value: %d\n", tok->i);
            }break;
            
            case Token_Float:
            {
                pb_write(&buffer, "\tTOKEN_INTEGER, value: %lf\n", tok->f);
            } break;
            
            case Token_Char:
            {
                pb_write(&buffer, "\tTOKEN_CHAR\n", tok->f);
            } break;
            
            case Token_S8:
            {
                pb_write(&buffer, "\tTOKEN_S8\n", tok->f);
            } break;
            
            case Token_S16:
            {
                pb_write(&buffer, "\tTOKEN_S16\n", tok->f);
            } break;
            
            case Token_S32:
            {
                pb_write(&buffer, "\tTOKEN_S32\n", tok->f);
            } break;
            
            case Token_S64:
            {
                pb_write(&buffer, "\tTOKEN_S64\n", tok->f);
            } break;
            
            case Token_U8:
            {
                pb_write(&buffer, "\tTOKEN_U8\n", tok->f);
            } break;
            
            case Token_U16:
            {
                pb_write(&buffer, "\tTOKEN_U16\n", tok->f);
            } break;
            
            case Token_U32:
            {
                pb_write(&buffer, "\tTOKEN_U32\n", tok->f);
            } break;
            
            case Token_U64:
            {
                pb_write(&buffer, "\tTOKEN_U64\n", tok->f);
            } break;
            
            case Token_F32:
            {
                pb_write(&buffer, "\tTOKEN_F32\n", tok->f);
            } break;
            
            case Token_F64:
            {
                pb_write(&buffer, "\tTOKEN_F64\n", tok->f);
            } break;
            
            case Token_SizeT:
            {
                pb_write(&buffer, "\tTOKEN_SIZET\n", tok->f);
            } break;
            
            case Token_Struct:
            {
                pb_write(&buffer, "\tTOKEN_STRUCT\n", tok->f);
            } break;
            
            case Token_Enum:
            {
                pb_write(&buffer, "\tTOKEN_ENUM\n", tok->f);
            } break;
            
            case Token_Const:
            {
                pb_write(&buffer, "\tTOKEN_CONST\n", tok->f);
            } break;
        }
    }
    
    pb_write(&buffer, "]\n");
    
    buffer.current[0] = 0;
    LogInfo("TomlTokenizer Results: %s", buffer.start);
    
    pb_free(&buffer);
}

static void 
LoadFile_Internal(const char* file_path, u8** buffer, int* size)
{
    FILE *fp = fopen(file_path, "r");
    assert(fp);
    
    fseek(fp, 0, SEEK_END); // seek to end of file
    uint64_t fsize = ftell(fp); // get current file pointer
    fseek(fp, 0, SEEK_SET); // seek back to beginning of file
    
    *buffer = (u8*)SysAlloc(fsize+1);
    
    uint64_t read = fread(*buffer, 1, fsize, fp);
    assert(read <= fsize);
    
    *size   = (int)read;
    (*buffer)[read] = 0;
}

static ParserResult 
ParseFile(ParserRegistry *registry, const char *filepath)
{
    int size;
    char *data;
    LoadFile_Internal(filepath, (u8**)&data, &size);
    
    //---------------------------------------------------------------------------------------------
    // Lexer Pass
    
    // Scanner Pass
    Scanner scanner = {};
    scanner.start   = (char*)data;
    scanner.end     = (char*)data + size;
    scanner.lexemes = 0;
    ParserResult result = ScanFile(&scanner);
    if (result != ParserResult_Success) return result;
    
    // Tokenizer Pass
    Tokenizer tokenizer = {};
    tokenizer.tokens = 0;
    
    result = TokenizeFile(&tokenizer, &scanner);
    if (result != ParserResult_Success) return result;
    
#if 0 // Print results
    ScannerPrintResults(&scanner);
    TokenizerPrintResults(&tokenizer);
#endif
    
    //---------------------------------------------------------------------------------------------
    // Parser Pass
    
    *registry = ParserRegistryNew();
    result = RunParserPass(registry, &tokenizer);
    if (result != ParserResult_Success) return result;
    
    LogDebug("Completed paring file %s.", filepath);
    
    return result;
}
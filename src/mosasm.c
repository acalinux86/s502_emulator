// Mos 6502 Assembler
#include <errno.h>

#include "./mos.h"

char *mos_read_file(const char *file_path, uint64_t *size)
{
    FILE *fp = fopen(file_path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "ERROR: file `%s` could not be opened because of : %s\n",
        file_path, strerror(errno));
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    uint64_t len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    *size = len;
    char *buffer = (char *)malloc(sizeof(char)*(len+1));
    if (buffer == NULL) {
        fprintf(stderr, "ERROR: Memory Allocation for buffer Failed\n");
        return NULL;
    }

    uint64_t bytes_read = fread(buffer, 1, len, fp);
    if (bytes_read != len) {
        fprintf(stderr, "ERROR: Failed to read file `%s`\n", file_path);
    return NULL;
    }

    buffer[bytes_read] = '\0';
    fclose(fp);
    return buffer;
}

char *mos_strdup(char *src, uint64_t src_len)
{
    char *dest = (char *)malloc(sizeof(char)*src_len+1);
    if (dest == NULL) {
        fprintf(stderr, "ERROR: Memory Allocation For String Duplication failed\n");
        return NULL;
    }
    strcpy(dest, src);
    dest[src_len] = '\0';
    return dest;
}

typedef enum _mos_token_type {
    MOS_TOKEN_EOF,
    MOS_TOKEN_OPERAND,
    MOS_TOKEN_IMMEDIATE,
    MOS_TOKEN_COMMENT,
    MOS_TOKEN_IDENTIFIER,
    MOS_TOKEN_LABEL,
    MOS_TOKEN_OPCODE,
    MOS_TOKEN_DIRECTIVE,
} MOS_TokenType;

typedef struct _mos_token {
    char *token;
    uint64_t token_len;
    MOS_TokenType type;
    uint64_t row, col; // for error reporting
} MOS_Token;

typedef ARRAY(MOS_Token) MOS_Tokens;

typedef struct _mos_lexer {
    const char *file_path; // path to source file
    char *content;
    uint64_t content_len;
    uint64_t lines;
    uint64_t cursor;
    uint64_t label_end; // store end of label
    MOS_Tokens tokens; // store tokens of parsed file
} MOS_Lexer;

const char *mos_token_type_as_cstr(MOS_TokenType type)
{
    switch (type) {
    case MOS_TOKEN_OPERAND:    return "MOS_TOKEN_OPERAND";
    case MOS_TOKEN_IMMEDIATE:  return "MOS_TOKEN_IMMEDIATE";
    case MOS_TOKEN_COMMENT:    return "MOS_TOKEN_COMMENT";
    case MOS_TOKEN_IDENTIFIER: return "MOS_TOKEN_IDENTIFIER";
    case MOS_TOKEN_LABEL:      return "MOS_TOKEN_LABEL";
    case MOS_TOKEN_OPCODE:     return "MOS_TOKEN_OPCODE";
    case MOS_TOKEN_DIRECTIVE:  return "MOS_TOKEN_DIRECTIVE";
    case MOS_TOKEN_EOF:        return "MOS_TOKEN_EOF";
    default:                   return NULL;
    }
}

bool mos_lexer_init(MOS_Lexer *lexer, const char *file_path)
{
    void *mem = memset(lexer, 0, sizeof(MOS_Lexer));
    if (mem == NULL) {
        fprintf(stderr, "ERROR: memset failed()\n");
        return false;
    }

    if (file_path == NULL) {
        fprintf(stderr, "ERROR: filepath `%s` is null\n", file_path);
        return false;
    }

    uint64_t size = 0;
    char *buffer = mos_read_file(file_path, &size);
    if (buffer == NULL) return false;

    lexer->file_path = file_path;
    lexer->content = buffer;
    lexer->content_len = size;
    return true;
}

bool mos_lexer_is_space(const MOS_Lexer *lexer)
{
    return lexer->content[lexer->cursor] == ' ' ||
    lexer->content[lexer->cursor] == '\t';
}

bool mos_lexer_is_comment(const MOS_Lexer *lexer)
{
    return lexer->content[lexer->cursor] == ';';
}

bool mos_lexer_is_newline(const MOS_Lexer *lexer)
{
    return lexer->content[lexer->cursor] == '\n';
}

bool mos_lexer_is_eof(const MOS_Lexer *lexer)
{
    return lexer->content[lexer->cursor] == '\0';
}

char mos_lexer_peek(const MOS_Lexer *lexer)
{
    // peek doesnt advance the cursor
    // it saves the cursor point and looks ahead
    uint64_t next = lexer->cursor;
    return lexer->content[next++];
}

bool mos_lexer_is_directive(const MOS_Lexer *lexer)
{
    return lexer->content[lexer->cursor] == '.';
}

bool mos_lexer_is_operand(const MOS_Lexer *lexer)
{
    return lexer->content[lexer->cursor] == '$';
}

bool mos_lexer_is_immediate(const MOS_Lexer *lexer)
{
    return lexer->content[lexer->cursor] == '#';
}

MOS_Token mos_create_token(char *token, uint64_t len, MOS_TokenType type, uint64_t row, uint64_t col)
{
    MOS_Token _token = {0}; // zero initialize
    _token.token = mos_strdup(token, len);
    _token.type = type;
    _token.token_len = len;
    _token.row = row;
    _token.col = col - col + len;
    return _token;
}

void mos_lexer_append_token(MOS_Lexer *lexer, MOS_Token token)
{
    assert(lexer != NULL);
    array_append(&lexer->tokens, token);
}

char mos_lexer_get_char(MOS_Lexer *lexer)
{
    // gets next char in the content && advance the cursor
    return lexer->content[lexer->cursor++];
}

#define char_to_cstr(x) ((char[2]){x, '\0'})

void mos_lexer_dump_tokens(const MOS_Lexer *lexer)
{
    for (uint32_t i = 0; i < lexer->tokens.count; ++i) {
        if (lexer->tokens.items[i].type == MOS_TOKEN_EOF) {
            fprintf(stdout, "`\\0`(%s)\n", mos_token_type_as_cstr(lexer->tokens.items[i].type));
        } else {
            fprintf(stdout, "%s(%s)\n", lexer->tokens.items[i].token, mos_token_type_as_cstr(lexer->tokens.items[i].type));
        }
    }
}

char *mos_to_upper(char *buffer)
{
    char buf[256] = {0};
    uint64_t n = strlen(buffer);
    for (uint64_t i = 0; i < n; ++i) {
        // assert the char we are receiving is lower case
        // maybe the assert is too strict
        // FIXED: resolved the assert
        if (buffer[i] >= 'a' && buffer[i] <= 'z') {
            buf[i] = buffer[i] - (97 - 65); // convert
        } else {
            buf[i] = buffer[i]; // do nothing
        }
    }
    return mos_strdup(buf, n);
}

bool mos_is_alpha(char x)
{
    return (x >= 'A' && x <= 'Z') || (x >= 'a' && x <= 'z') || (x == '_');
}

bool mos_is_opcode(char *buf)
{
    const char *buffer = (const char*)mos_to_upper(buf);
    const char *op_cstr[] = {
        "BRK","NOP","RTI","RTS","JMP","JSR","ADC","SBC","CMP","CPX",
        "CPY","LDA","LDY","LDX","STA","STY","STX","CLC","CLV","CLI",
        "CLD","SEC","SED","SEI","ORA","AND","EOR","IT","TAX","TAY",
        "TXA","TYA","TSX","TXS","PHA","PHP","PLA","PLP","INC","INX",
        "INY","DEX","DEY","DEC","BNE","BCC","BCS","BEQ","BMI","BPL",
        "BVC","BVS","ASL","LSR","ROL","ROR"
    };

    uint64_t count = MOS_ARRAY_LEN(op_cstr);
    for (uint64_t i = 0; i < count; ++i) {
        if (strcmp(op_cstr[i], buffer) == 0) {
            return true;
        }
    }
    return false;
}

bool mos_lexer_process_token_after_whitespace(MOS_Lexer *lexer, uint64_t *cursor, char *buf)
{
    if (lexer == NULL) return false;
    MOS_Token token = {0};
    buf[*cursor] = '\0';
    if (*cursor > 0) {
        //fprintf(stderr, "TODO: len: %ld, %s not catogorised yet\n",*cursor, buf);
        if (mos_is_alpha(buf[0])) {
            if (mos_is_opcode(buf)) {
                token = mos_create_token(buf, *cursor, MOS_TOKEN_OPCODE, lexer->lines, lexer->cursor);
                mos_lexer_append_token(lexer, token);
            } else {
                // TODO: if last token processed was a dot then directive
                // else label
                if (lexer->tokens.items[lexer->tokens.count - 1].type == MOS_TOKEN_DIRECTIVE) {
                    token = mos_create_token(buf, *cursor, MOS_TOKEN_DIRECTIVE, lexer->lines, lexer->cursor);
                    mos_lexer_append_token(lexer, token);
                } else {
                    token = mos_create_token(buf, *cursor, MOS_TOKEN_LABEL, lexer->lines, lexer->cursor);
                    mos_lexer_append_token(lexer, token);
                }
            }
        } else {
            if (lexer->tokens.items[lexer->tokens.count - 2].type == MOS_TOKEN_IMMEDIATE) {
                token = mos_create_token(buf, *cursor, MOS_TOKEN_IMMEDIATE, lexer->lines, lexer->cursor);
                mos_lexer_append_token(lexer, token);
            } else {
                token = mos_create_token(buf, *cursor, MOS_TOKEN_OPERAND, lexer->lines, lexer->cursor);
                mos_lexer_append_token(lexer, token);
            }
        }
    }
    *cursor = 0;
    return true;
}

MOS_Token mos_lexer_get_token(MOS_Lexer *lexer)
{
    assert(lexer->tokens.count > 0);
    MOS_Token token = lexer->tokens.items[0];
    for (uint32_t i = 1; i < lexer->tokens.count; ++i) {
        lexer->tokens.items[i - 1] = lexer->tokens.items[i]; // shift
    }
    lexer->tokens.count--;
    return token;
}

bool mos_expect_token(MOS_Token token, MOS_TokenType type)
{
    return token.type == type;
}

typedef struct _mos_symbol {
    const char    *symbol;
    const uint64_t address;
} MOS_Symbol;

typedef ARRAY(MOS_Symbol) MOS_SymbolTable;

bool mos_lexer_parse(MOS_Lexer *lexer, MOS_SymbolTable *table)
{
    if (lexer == NULL) return false;
    memset(table , 0, sizeof(MOS_SymbolTable)); // memset the table since we are filling it here

    printf("BEGIN\n");
    bool quit = false;
    while (!quit) {
        MOS_Token token = mos_lexer_get_token(lexer);
        switch (token.type) {
        case MOS_TOKEN_DIRECTIVE: {
            MOS_Token g_token = mos_lexer_get_token(lexer);
            if (mos_expect_token(g_token, MOS_TOKEN_DIRECTIVE)) {
                //table->add(token);
                printf("    DIRECTIVE");
            } else {
                fprintf(stderr, "%s:%ld:%ld: Expected %s, got %s\n", lexer->file_path, token.row+1, token.col, mos_token_type_as_cstr(token.type), mos_token_type_as_cstr(g_token.type));
                return false;
            }
        } break;
        case MOS_TOKEN_OPERAND: {
            MOS_Token g_token = mos_lexer_get_token(lexer);
            if (mos_expect_token(g_token, MOS_TOKEN_OPERAND)) {
                // add to symbol table
                printf("    OPERAND\n");
            } else {
                fprintf(stderr, "%s:%ld:%ld: Expected %s, got %s\n", lexer->file_path, token.row+1, token.col, mos_token_type_as_cstr(token.type), mos_token_type_as_cstr(g_token.type));
            }
        } break;
        case MOS_TOKEN_IMMEDIATE: {
            MOS_Token g_token = mos_lexer_get_token(lexer);
            if (mos_expect_token(g_token, MOS_TOKEN_OPERAND)) {
                g_token = mos_lexer_get_token(lexer);
                if (mos_expect_token(g_token, MOS_TOKEN_IMMEDIATE)) {
                    printf("    IMMEDIATE\n");
                } else {
                    fprintf(stderr, "%s:%ld:%ld: Expected %s, got %s\n", lexer->file_path, token.row+1, token.col, mos_token_type_as_cstr(token.type), mos_token_type_as_cstr(g_token.type));
                }
            } else {
                fprintf(stderr, "%s:%ld:%ld: Expected %s, got %s\n", lexer->file_path, token.row+1, token.col, mos_token_type_as_cstr(token.type), mos_token_type_as_cstr(g_token.type));
            }
        } break;

        case MOS_TOKEN_LABEL: {
            if (mos_expect_token(token, MOS_TOKEN_LABEL)) {
                // add to symbol table
                printf("\nLABEL:\n");
            } else {
                fprintf(stderr, "%s:%ld:%ld: Expected %s, got %s\n", lexer->file_path, token.row+1, token.col, mos_token_type_as_cstr(MOS_TOKEN_LABEL), mos_token_type_as_cstr(token.type));
            }

        } break;

        case MOS_TOKEN_OPCODE: {
            if (mos_expect_token(token, MOS_TOKEN_OPCODE)) {
                // add to symbol table
                printf("    OPCODE ");
            } else {
                fprintf(stderr, "%s:%ld:%ld: Expected %s, got %s\n", lexer->file_path, token.row+1, token.col, mos_token_type_as_cstr(MOS_TOKEN_OPCODE), mos_token_type_as_cstr(token.type));
            }
        } break;

        case MOS_TOKEN_EOF: quit = true; printf("    EOF\n"); break;
        case MOS_TOKEN_IDENTIFIER: printf("IDENTIFIER\n"); break;
        case MOS_TOKEN_COMMENT: printf("    COMMENT\n"); break;
        default:
            return false;
        }
    }
    printf("\nEND\n");
    return true;
}

const char *mos_shift(int *argc, char ***argv)
{
    assert(*argc > 0);
    const char *result = **argv;
    (*argv)++;
    (*argc)--;
    return result;
}

// Move this
#define print_tok(tok) printf("%s\n", (tok)->token)

void mos_usage(const char *program)
{
    fprintf(stderr, "MOS 6502 Assembler\n");
    fprintf(stderr, "USAGE: %s <file_path>\n", program);
}

// TODO: Provide details about the file in Usage
int main(int argc, char **argv)
{
    const char *program = mos_shift(&argc, &argv);
    if (argc == 0) {
        mos_usage(program);
        return 1;
    }

    MOS_Lexer lexer = {0};
    const char *file_path = mos_shift(&argc, &argv);
    if (!mos_lexer_init(&lexer, file_path)) return 1;

    char buf[256];
    uint64_t cursor = 0;

    MOS_Token token = {0};
    while (!mos_lexer_is_eof(&lexer)) {
        if (mos_lexer_is_newline(&lexer)) {
            lexer.cursor++; lexer.lines++;
            if (!mos_lexer_process_token_after_whitespace(&lexer, &cursor, buf)) return 1;
        } else if (mos_lexer_is_space(&lexer)) {
            lexer.cursor++;
            if (!mos_lexer_process_token_after_whitespace(&lexer, &cursor, buf)) return 1;
        } else if (mos_lexer_is_directive(&lexer)) {
            char *directive = char_to_cstr(mos_lexer_get_char(&lexer));
            token = mos_create_token(directive, 1, MOS_TOKEN_DIRECTIVE, lexer.lines, lexer.cursor);
            mos_lexer_append_token(&lexer, token);
        } else if (mos_lexer_is_immediate(&lexer)) {
            char *imme = char_to_cstr(mos_lexer_get_char(&lexer));
            token = mos_create_token(imme, 1, MOS_TOKEN_IMMEDIATE, lexer.lines, lexer.cursor);
            mos_lexer_append_token(&lexer, token);
        } else if (mos_lexer_is_operand(&lexer)) {
            char *operand = char_to_cstr(mos_lexer_get_char(&lexer));
            token = mos_create_token(operand, 1, MOS_TOKEN_OPERAND, lexer.lines, lexer.cursor);
            mos_lexer_append_token(&lexer, token);
        } else if (mos_lexer_is_comment(&lexer)) {
            while (mos_lexer_peek(&lexer) != '\n' && !mos_lexer_is_eof(&lexer)) {
                // for comment
                buf[cursor++] = mos_lexer_get_char(&lexer);
            }
            buf[cursor] = '\0';
            token = mos_create_token(buf, cursor, MOS_TOKEN_COMMENT, lexer.lines, lexer.cursor);
            mos_lexer_append_token(&lexer, token);
            cursor = 0;
        } else if (mos_lexer_peek(&lexer) == ':') {
            // end of the label
            lexer.label_end = lexer.cursor++; // store and increment
        } else {
            buf[cursor++] = mos_lexer_get_char(&lexer);
        }
    }

    // and last append the eof token
    char *operand = char_to_cstr(mos_lexer_get_char(&lexer));
    token = mos_create_token(operand, 1, MOS_TOKEN_EOF, lexer.lines, lexer.cursor);
    mos_lexer_append_token(&lexer, token);

    mos_lexer_dump_tokens(&lexer);
    
    MOS_SymbolTable table = {0};
    if (!mos_lexer_parse(&lexer, &table)) return 1;

    return 0;
}

// TODO: ERROR Reporting, locations and offsets

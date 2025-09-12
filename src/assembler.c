#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <ctype.h>

#include "./types.h"
#include "./array.h"
#include "./s502.h"
#define SV_IMPLEMENTATION
#include "./sv.h"

#define MAX_BUF_LEN  128
#define MAX_LINE_LEN 256

char *read_file(const char *file_path, u32 *size)
{
    FILE *fp = fopen(file_path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "ERROR: file `%s` could not be opened because of : %s\n",
                file_path, strerror(errno));
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    i32 len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    *size = len;
    char *buffer = (char *)malloc(sizeof(char)*(len+1));
    if (buffer == NULL) {
        fprintf(stderr, "ERROR: Memory Allocation for buffer Failed\n");
        return NULL;
    }

    i32 bytes_read = fread(buffer, 1, len, fp);
    if (bytes_read != len) {
        fprintf(stderr, "ERROR: Failed to read file `%s`\n", file_path);
        return NULL;
    }
    fseek(fp, 0, SEEK_SET);
    fclose(fp);
    return buffer;
}

typedef enum {
    TOKEN_UNKNOWN,
    TOKEN_OPCODE,
    TOKEN_IMMEDIATE,
    TOKEN_ADDRESS,
    TOKEN_DIRECTIVE,
    TOKEN_LABEL,
    TOKEN_COMMENT,
} Token_Type;

typedef struct {
    String_View sv;
    u16 address;
} Symbol;

typedef struct {
    String_View sv;
    Token_Type type;
} Token;

const char *token_type_as_cstr(Token_Type type)
{
    switch (type) {
    case TOKEN_OPCODE:    return "TOKEN_OPCODE";
    case TOKEN_IMMEDIATE: return "TOKEN_IMMEDIATE";
    case TOKEN_ADDRESS:   return "TOKEN_ADDRESS";
    case TOKEN_DIRECTIVE: return "TOKEN_DIRECTIVE";
    case TOKEN_LABEL:     return "TOKEN_LABEL";
    case TOKEN_COMMENT:   return "TOKEN_COMMENT";
    case TOKEN_UNKNOWN:   return "TOKEN_UNKNOWN";
    default:
        exit(1);
    }
}

typedef ARRAY(String_View) Lines;
typedef ARRAY(Token)       Tokens;
typedef ARRAY(Symbol)      Symbol_Table;
typedef ARRAY(u8)          Byte_Code;

typedef struct Generation {
    u16 start_address;
    Byte_Code byte_code;
} Generation;

typedef struct {
    const char *file_path;
    const char *content;
    u32 content_len;
    u32 row;
    Lines  lines;
    Tokens tokens;
    Symbol_Table table;
    u16 program_counter;
    Byte_Code byte_code;
} Lexer;

// Duplicate with Null Character
// depracated
char *string_duplicate(char *src)
{
    i32 src_len = strlen(src);
    char *dest = (char *)malloc(sizeof(char)*src_len+1);
    if (dest == NULL) {
        fprintf(stderr, "ERROR: Memory Allocation For String Duplication failed\n");
        return NULL;
    }

    strcpy(dest, src);
    dest[src_len] = '\0';
    return dest;
}

Lexer lexer_init(const char *file_path)
{
    Lexer lexer = {0};
    lexer.file_path = file_path;
    lexer.content = read_file(lexer.file_path, (u32*)&lexer.content_len);
    lexer.row = 0;
    array_new(&lexer.table);
    array_new(&lexer.tokens);
    array_new(&lexer.lines);
    return lexer;
}

bool is_opcode(const String_View *sv)
{
    const String_View opcodes_svs[] = {
        SV("BRK"),SV("NOP"),SV("RTI"),SV("RTS"),SV("JMP"),SV("JSR"),SV("ADC"),SV("SBC"),SV("CMP"),SV("CPX"),
        SV("CPY"),SV("LDA"),SV("LDY"),SV("LDX"),SV("STA"),SV("STY"),SV("STX"),SV("CLC"),SV("CLV"),SV("CLI"),
        SV("CLD"),SV("SEC"),SV("SED"),SV("SEI"),SV("ORA"),SV("AND"),SV("EOR"),SV("IT"),SV("TAX"),SV("TAY"),
        SV("TXA"),SV("TYA"),SV("TSX"),SV("TXS"),SV("PHA"),SV("PHP"),SV("PLA"),SV("PLP"),SV("INC"),SV("INX"),
        SV("INY"),SV("DEX"),SV("DEY"),SV("DEC"),SV("BNE"),SV("BCC"),SV("BCS"),SV("BEQ"),SV("BMI"),SV("BPL"),
        SV("BVC"),SV("BVS"),SV("ASL"),SV("LSR"),SV("ROL"),SV("ROR")
    };

    u32 count = ERROR_FETCH_DATA - 2;
    for (u32 i = 0; i < count; ++i) {
        if (sv_eq(*sv, opcodes_svs[i])) {
            return true;
        }
    }

    return false;
}

bool is_directive(const String_View *sv)
{
    return sv_starts_with(*sv, SV(".")) && sv->count > 1;
}

bool is_address(const String_View *sv)
{
    return sv_starts_with(*sv, SV("$")) && sv->count > 1;
}

bool is_immediate(const String_View *sv)
{
    return sv_starts_with(*sv, SV("#")) && sv->count > 1;
}

bool is_label(const String_View *sv)
{
    return sv_ends_with(*sv, SV(":")) && sv->count > 1;
}

bool append_token(String_View sv, Tokens *tokens)
{
    if (!tokens || sv.count == 0) return false;

    Token token      = {0};
    token.sv = sv_trim(sv);

    if (is_label(&sv)) {
        token.type   = TOKEN_LABEL;
    } else if (is_directive(&sv)) {
        token.type   = TOKEN_DIRECTIVE;
    } else if (is_opcode(&sv)) {
        token.type   = TOKEN_OPCODE;
    } else if (is_address(&sv)) {
        token.type   = TOKEN_ADDRESS;
    } else if (is_immediate(&sv)) {
        token.type   = TOKEN_IMMEDIATE;
    } else {
        token.type   = TOKEN_UNKNOWN;
    }

    array_append(tokens, token);
    return true;
}

bool lexer_scan_lines(Lexer *lexer)
{
    if (!lexer->content || lexer->content_len == 0) {
        fprintf(stderr, "ERROR: Invalid File: `%s`\n", lexer->file_path);
        return false;
    }

    String_View content = sv_from_parts(lexer->content, lexer->content_len);

    while (content.count > 0) {
        String_View line = sv_chop_by_delim(&content, '\n');
        line = sv_trim(line);
        lexer->row++;

        if (line.count > 0) {
            printf("Line: "SV_Fmt"\n", SV_Arg(line));
            array_append(&lexer->lines, line);
        }
    }
    return true;
}

bool tokenize_line(String_View line, Tokens *tokens)
{
    if (line.count == 0) return false;
    if (!tokens) return false;

    u32 n = 0;
    char buf[MAX_BUF_LEN] = {0};
    u32 cursor = 0;

    while (n < line.count) {
        char c = line.data[n++];
        if (c == ' ') {
            if (cursor > 0) {
                buf[cursor] = '\0';
                printf("BUF: %s|\n", buf);
                cursor = 0;
                if (append_token(sv_from_cstr(string_duplicate(buf)), tokens)) {};
            }
        } else {
            buf[cursor] = c;
            cursor++;
        }
    }

    // Remaining
    buf[cursor] = '\0';
    printf("BUF: %s|\n", buf);
    cursor = 0;
    if (append_token(sv_from_cstr(string_duplicate(buf)), tokens)) {};
    return true;
}

bool lexer_scan_line_items(Lexer *lexer)
{
    if (!lexer->content || lexer->content_len == 0) {
        fprintf(stderr, "ERROR: Invalid File: `%s`\n", lexer->file_path);
        return false;
    }

    for (u32 i = 0; i < lexer->lines.count; ++i) {
        String_View line = lexer->lines.items[i];
        if (tokenize_line(line, &lexer->tokens)) continue;
    }
    return true;
}

// Convert hexadecimal string to u16 (handles both $ prefix and without)
u16 sv_hex_to_u16(String_View sv)
{
    u16 result = 0;
    u32 start = 0;

    // Skip $ prefix if present
    if (sv.count > 0 && sv.data[0] == '$') {
        start = 1;
    }

    for (u32 i = start; i < sv.count; ++i) {
        char c = sv.data[i];
        u8 value;

        if (c >= '0' && c <= '9') {
            value = c - '0';
        } else if (c >= 'a' && c <= 'f') {
            value = 10 + (c - 'a');
        } else if (c >= 'A' && c <= 'F') {
            value = 10 + (c - 'A');
        } else {
            break; // Invalid character
        }
        result = result * 16 + value;
    }

    return result;
}

// Convert immediate value (handles # prefix)
u16 sv_immediate_to_u16(String_View sv)
{
    u32 start = 0;

    // Skip # prefix
    if (sv.count > 0 && sv.data[0] == '#') {
        start = 1;
    }

    // Check if it's hexadecimal ($ prefix)
    if (start < sv.count && sv.data[start] == '$') {
        return sv_hex_to_u16(sv_from_parts(sv.data + start, sv.count - start));
    }
    // Otherwise assume decimal
    return sv_to_u16(sv_from_parts(sv.data + start, sv.count - start));
}

bool expect_token(Token *token, Token_Type type)
{
    if (token->type != type) {
        fprintf(stderr, "Expected: `%s`, Found: `%s`\n",
                token_type_as_cstr(TOKEN_ADDRESS),
                token_type_as_cstr(token->type)
            );
        return false;
    }
    return true;
}

Symbol symbol(String_View sv, u16 address)
{
    return (Symbol){sv, address};
}

bool append_symbol(Symbol symbol, Symbol_Table *table)
{
    if (symbol.sv.count == 0 || !table) return false;
    array_append(table, symbol);
    return true;
}

bool calculate_address(Lexer *lexer)
{
    if (!lexer) return false;
    // u16 start_address = 0;
    for (u32 i = 0; i < lexer->tokens.count; ++i) {
        Token *token = &lexer->tokens.items[i];
        switch (token->type) {
        case TOKEN_DIRECTIVE: {
            if (i + 1 < lexer->tokens.count) {
                Token *next = &lexer->tokens.items[i + 1];
                if (expect_token(next, TOKEN_ADDRESS)) {
                    lexer->program_counter = sv_hex_to_u16(next->sv);
                    ++i;
                } else {
                    return false;
                }
            } else {
                fprintf(stderr, "Missing Address For Directive\n");
                return false;
            }
        } break;

        case TOKEN_LABEL: {
            if (sv_eq(token->sv, SV("START:"))) {
                Symbol symb = symbol(
                    sv_from_parts(token->sv.data, token->sv.count - 1),
                    lexer->program_counter);
                append_symbol(symb, &lexer->table);
            }
        } break;

        case TOKEN_OPCODE: {
            u8 instruction_size = 1;
            if (i + 1 < lexer->tokens.count) {
                Token *next = &lexer->tokens.items[i + 1];
                if (next->type == TOKEN_ADDRESS) {
                    ++i;
                    if (sv_hex_to_u16(next->sv) > 0xFF) {
                        instruction_size = 3;
                    } else {
                        instruction_size = 2;
                    }
                }
                if (next->type ==  TOKEN_IMMEDIATE) {
                    ++i;
                    instruction_size = 2;
                }
            }
            Symbol symb = symbol(token->sv, lexer->program_counter);
            append_symbol(symb, &lexer->table);
            lexer->program_counter+=instruction_size;
        } break;

        case TOKEN_ADDRESS: break;
        case TOKEN_IMMEDIATE: break;
        case TOKEN_COMMENT:
        case TOKEN_UNKNOWN:
        default:
            break;
        }
    }
    return true;
}

Instruction encode(char *opcode, u16 address)
{

}

Generation generation(Lexer *lexer)
{
    Generation generation = {0};
    array_new(&generation.byte_code);
    generation.start_address = lexer->table.items[0].address;

    for (u32 i = 0; i < (u32)lexer->program_counter; ++i) {
        for (u32 i = 0; i < ARRAY_LEN(opcode_matrix); ++i) {
            const char *opcode_cstr = s502_opcode_as_cstr(opcode_matrix[i].opcode);
            if (sv_eq(opcode, sv_from_cstr(opcode_cstr))) {

            }
        }
    }
}

void usage(char **argv)
{
    fprintf(stderr, "6502 Assembler\n");
    fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        usage(argv);
        return 1;
    }

    const char *file_path = argv[1];
    Lexer lexer = lexer_init(file_path);
    lexer_scan_lines(&lexer);
    lexer_scan_line_items(&lexer);
    printf("\n\nFile_path   : %s\n", lexer.file_path);
    printf("Content_len : %d\n", lexer.content_len);
    printf("Row         : %d\n", lexer.row);

    if (!calculate_address(&lexer)) return 1;
    for (u32 i = 0; i < lexer.table.count; ++i) {
        printf("Symbol: "SV_Fmt", Address: %x\n",
               SV_Arg(lexer.table.items[i].sv),
               lexer.table.items[i].address
            );
        printf("_____________________\n\n");
    }
    printf("Program Counter: 0x%x\n", lexer.program_counter);
    return 0;
}

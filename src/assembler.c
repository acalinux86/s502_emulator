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
char *read_file(const char *file_path, i32 *size)
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
    Symbol symbol;
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
typedef ARRAY(Token)       Symbol_Table;

typedef struct {
    const char *file_path;
    const char *content;
    i32 content_len;
    i32 row;
    Symbol_Table table;
    Lines  lines;
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

bool line_add(String_View *sv, Lines *lines)
{
    if (!lines) return false;
    if (!sv) return false;
    if (sv->count == 0) return false;

    String_View string = sv_trim(*sv);
    array_append(lines, string);
    return true;
}

Lexer lexer_init(const char *file_path)
{
    Lexer lexer = {0};
    lexer.file_path = file_path;
    lexer.content = read_file(lexer.file_path, &lexer.content_len);
    lexer.row = 0;
    array_new(&lexer.table);
    array_new(&lexer.lines);
    return lexer;
}

bool is_opcode(const String_View sv)
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
        if (sv_eq(sv, opcodes_svs[i])) {
            return true;
        }
    }

    return false;
}

bool is_directive(const String_View sv)
{
    return sv_starts_with(sv, SV(".")) && sv.count > 1;
}

bool is_address(const String_View sv)
{
    return sv_starts_with(sv, SV("$")) && sv.count > 1;
}

bool is_immediate(const String_View sv)
{
    return sv_starts_with(sv, SV("#")) && sv.count > 1;
}

bool is_label(const String_View sv)
{
    return sv_ends_with(sv, SV(":")) && sv.count > 1;
}

Symbol symbol(String_View sv, u16 address)
{
    return (Symbol){sv, address};
}

bool append_symbol(String_View sv, Symbol_Table *table)
{
    if (!table) return false;

    Token token      = {0};
    token.symbol = symbol(sv, 0);
    if (is_label(sv)) {
        token.type   = TOKEN_LABEL;
    } else if (is_directive(sv)) {
        token.type   = TOKEN_DIRECTIVE;
    } else if (is_opcode(sv)) {
        token.type   = TOKEN_OPCODE;
    } else if (is_address(sv)) {
        token.type   = TOKEN_ADDRESS;
    } else if (is_immediate(sv)) {
        token.type   = TOKEN_IMMEDIATE;
    } else {
        token.type   = TOKEN_UNKNOWN;
    }

    array_append(table, token);
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

bool lexer_scan_line_items(Lexer *lexer)
{
    if (!lexer->content || lexer->content_len == 0) {
        fprintf(stderr, "ERROR: Invalid File: `%s`\n", lexer->file_path);
        return false;
    }

    for (u32 i = 0; i < lexer->lines.count; ++i) {
        String_View line = lexer->lines.items[i];
        String_View remain = line;

        while (remain.count > 0) {
            remain = sv_trim_left(remain);
            if (remain.count == 0) break;

            u32 token_len = 0;
            while (token_len < remain.count &&
                   !isspace(remain.data[token_len]) &&
                   remain.data[token_len] != ','
                ) { token_len++; }

            if (token_len == 0) {
                remain = sv_chop_left(&remain, 1);
                continue;
            }

            String_View token_sv = sv_from_parts(remain.data, token_len);
            printf("Token Debug: "SV_Fmt"\n", SV_Arg(token_sv));
            append_symbol(token_sv, &lexer->table);

            sv_chop_left(&remain, token_len);
        }
    }
    return true;
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

    for (u32 i = 0; i < lexer.table.count; ++i) {
        printf("Token: "SV_Fmt", Type: %s\n",
               SV_Arg(lexer.table.items[i].symbol.sv),
               token_type_as_cstr(lexer.table.items[i].type));
    }
    return 0;
}

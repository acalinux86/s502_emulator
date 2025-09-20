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

typedef enum _token_type {
    MOS_TOKEN_OPERAND,
    MOS_TOKEN_IMMEDIATE,
    MOS_TOKEN_COMMENT,
    MOS_TOKEN_IDENTIFIER,
    MOS_TOKEN_LABEL,
    MOS_TOKEN_OPCODE,
    MOS_TOKEN_DIRECTIVE,
} MOS_TokenType;

typedef struct {
    char *token;
    uint64_t token_len;
    MOS_TokenType type;
} MOS_Token;

typedef ARRAY(MOS_Token) MOS_Tokens;

typedef struct {
    char *content;
    uint64_t content_len;
    uint64_t lines;
    uint64_t cursor;
    uint64_t label_end; // store end of label
    MOS_Tokens tokens;
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

    lexer->content = buffer;
    lexer->content_len = size;
    return true;
}

bool mos_lexer_is_whitespace(MOS_Lexer *lexer)
{
    return lexer->content[lexer->cursor] == ' ' ||
           lexer->content[lexer->cursor] == '\t';
}

bool mos_lexer_is_comment(MOS_Lexer *lexer)
{
    return lexer->content[lexer->cursor] == ';';
}

bool mos_lexer_is_newline(MOS_Lexer *lexer)
{
    return lexer->content[lexer->cursor] == '\n';
}

bool mos_lexer_is_eof(MOS_Lexer *lexer)
{
    return lexer->content[lexer->cursor] == '\0';
}

char mos_lexer_peek(MOS_Lexer *lexer)
{
    // peek doesnt advance the cursor
    // it saves the cursor point and looks ahead
    uint64_t next = lexer->cursor;
    return lexer->content[next++];
}

bool mos_lexer_is_directive(MOS_Lexer *lexer)
{
    return lexer->content[lexer->cursor] == '.';
}

bool mos_lexer_is_operand(MOS_Lexer *lexer)
{
    return lexer->content[lexer->cursor] == '$';
}

bool mos_lexer_is_immediate(MOS_Lexer *lexer)
{
    return lexer->content[lexer->cursor] == '#';
}

MOS_Token mos_create_token(char *token, uint64_t len, MOS_TokenType type)
{
    return (MOS_Token) {
        .token = mos_strdup(token, len),
        .type = type,
        .token_len = len,
    };
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

void mos_lexer_dump_tokens(MOS_Lexer *lexer)
{
    for (uint32_t i = 0; i < lexer->tokens.count; ++i) {
        fprintf(stdout, "%s(%s)\n", lexer->tokens.items[i].token, mos_token_type_as_cstr(lexer->tokens.items[i].type));
    }
}

// TODO: Provide details about the file in Usage
int main(void)
{
    const char *file_path = "Test/s502_add.asm";
    MOS_Lexer lexer;
    if (!mos_lexer_init(&lexer, file_path)) return 1;

    char buf[256];
    uint64_t cursor = 0;
    while (!mos_lexer_is_eof(&lexer)) {
        if (mos_lexer_is_newline(&lexer)) {
            lexer.cursor++; lexer.lines++;
            buf[cursor] = '\0';
            if (cursor > 0)
                fprintf(stderr, "TODO: len: %ld, %s not catogorised yet\n",cursor, buf);
            cursor = 0;
        } else if (mos_lexer_is_whitespace(&lexer)) {
            lexer.cursor++;
            buf[cursor] = '\0';
            if (cursor > 0)
                fprintf(stderr, "TODO: len: %ld, %s not catogorised yet\n",cursor, buf);
            cursor = 0;
        } else if (mos_lexer_is_directive(&lexer)) {
            char *directive = char_to_cstr(mos_lexer_get_char(&lexer));
            MOS_Token token = mos_create_token(directive, 1, MOS_TOKEN_DIRECTIVE);
            mos_lexer_append_token(&lexer, token);
        } else if (mos_lexer_is_immediate(&lexer)) {
            char *imme = char_to_cstr(mos_lexer_get_char(&lexer));
            MOS_Token token = mos_create_token(imme, 1, MOS_TOKEN_IMMEDIATE);
            mos_lexer_append_token(&lexer, token);
        } else if (mos_lexer_is_operand(&lexer)) {
            char *operand = char_to_cstr(mos_lexer_get_char(&lexer));
            MOS_Token token = mos_create_token(operand, 1, MOS_TOKEN_OPERAND);
            mos_lexer_append_token(&lexer, token);
        } else if (mos_lexer_is_comment(&lexer)) {
            while (mos_lexer_peek(&lexer) != '\n') lexer.cursor++;
        } else if (mos_lexer_peek(&lexer) == ':') {
            // end of the label
            lexer.label_end = lexer.cursor++; // store and increment
        } else {
            buf[cursor++] = mos_lexer_get_char(&lexer);
            //MOS_Token token = mos_create_token(operand, 1, MOS_TOKEN_OPERAND);
            //mos_lexer_append_token(&lexer, token);
            //continue;
        }
    }
    mos_lexer_dump_tokens(&lexer);
    return 0;
}

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <ctype.h>

#include "./types.h"
#include "./array.h"

#define MAX_BUF_LEN 256

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
    fclose(fp);
    return buffer;
}

typedef struct {
    char *buffer;
    i32   buffer_len;
} String;

typedef enum {
    TOKEN_OPCODE = 0,
    TOKEN_DATA,
    TOKEN_ADDRESS,
    TOKEN_LABEL,
    TOKEN_COMMENT,
} Token_Type;

typedef struct {
    char *string;
    Token_Type type;
} Token;

typedef ARRAY(Token) Tokens;

typedef struct {
    const char *file_path;
    const char *content;
    i32 content_len;
    i32 cursor;
    i32 row;
    Tokens tokens;
} Lexer;

// Duplicate with Null Character
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

bool token_add(char *buf, Token_Type type, Tokens *tokens)
{
    if (!tokens) return false;
    if (strlen(buf) == 0) return false;

    Token token = {0};
    token.type = type;
    token.string = string_duplicate(buf);
    array_append(tokens, token);
    return true;
}

Lexer lexer_init(const char *file_path)
{
    Lexer lexer = {0};
    lexer.file_path = file_path;
    lexer.content = read_file(lexer.file_path, &lexer.content_len);
    lexer.row = 0;
    lexer.cursor = 0;
    array_new(&lexer.tokens);
    return lexer;
}

bool lexer_end(Lexer *lexer)
{
    if (lexer->content[lexer->cursor] == '\0') {
        return true;
    } else {
        return false;
    }
}

char lexer_peek(Lexer *lexer)
{
    if (lexer_end(lexer)) return '\0';
    return lexer->content[lexer->cursor];
}

void lexer_advance_row(Lexer *lexer)
{
    if (lexer_peek(lexer) == '\n') {
        lexer->cursor++;
        lexer->row++;
    }
}

char lexer_advance(Lexer *lexer)
{
    char c = lexer->content[lexer->cursor];
    if (lexer->cursor < lexer->content_len) lexer->cursor++;
    return c;
}

bool lexer_scan(Lexer *lexer)
{
    if (!lexer->content || lexer->content_len == 0) {
        fprintf(stderr, "ERROR: Invalid File: `%s`\n", lexer->file_path);
        return false;
    }

    char buf[MAX_BUF_LEN] = {0}; // Tempory buffer to store token strings
    i32 cursor = 0;

    while (!lexer_end(lexer)) {
        char c = lexer_peek(lexer);

        if (c != ' ' || c != '\n') {
            buf[cursor] = c;
            cursor++;
            lexer->cursor++;
        }

        if (c == ' ') {
            while (lexer_peek(lexer) == ' ') lexer->cursor++;
            buf[cursor] = '\0';
            printf(" |S|Buf:%s, Len: %lu| ", buf, strlen(buf));
            cursor = 0;
            token_add(buf, TOKEN_ADDRESS, &lexer->tokens);
        }

        lexer_advance_row(lexer);
    }

    buf[cursor] = '\0';
    printf(" |S|Buf:%s, Len: %lu| ", buf, strlen(buf));
    cursor = 0;
    token_add(buf, TOKEN_ADDRESS, &lexer->tokens);

    printf("\n");
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
    Lexer lexer =lexer_init(file_path);
    printf("File_path   : %s\n", lexer.file_path);
    printf("Content_len : %d\n", lexer.content_len);
    printf("Row         : %d\n", lexer.row);
    printf("Cursor      : %d\n", lexer.cursor);
    lexer_scan(&lexer);
    printf("\n\nFile_path   : %s\n", lexer.file_path);
    printf("Content_len : %d\n", lexer.content_len);
    printf("Row         : %d\n", lexer.row);
    printf("Cursor      : %d\n", lexer.cursor);

    for (u32 i = 0; i < lexer.tokens.count; ++i) {
        printf("%s\n", lexer.tokens.items[i].string);
    }

    array_delete(&lexer.tokens);
    return 0;
}

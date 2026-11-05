#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct {
    const char* filename;
    bool isValid;
    int expectedFields;
    int lineNumber;
} FileResult;

static bool gVerbose = false;
static bool gUseMmap = true;

typedef struct {
    FileResult* result;
    bool haveExpected;
    int fields;
    uint8_t state;
} ParserContext;

static void finalizeRecord(ParserContext* ctx) {
    if (ctx->haveExpected) {
        if (ctx->fields != ctx->result->expectedFields) {
            ctx->result->isValid = false;
            if (gVerbose) {
                fprintf(stderr,
                        "%s: Line %d contains %d fields instead of the expected %d fields.\n",
                        ctx->result->filename, ctx->result->lineNumber, ctx->fields,
                        ctx->result->expectedFields);
            }
        }
    } else {
        ctx->result->expectedFields = ctx->fields;
        ctx->haveExpected = true;
    }
    ctx->fields = 0;
    ++ctx->result->lineNumber;
}

static void errorUnexpectedChar(ParserContext* ctx, unsigned char ch) {
    ctx->result->isValid = false;
    if (gVerbose) {
        fprintf(stderr, "%s: Unexpected character '%c' (decimal %d) on line %d.\n",
                ctx->result->filename, (char)ch, (int)ch, ctx->result->lineNumber);
    }
}

static void unexpectedEof(ParserContext* ctx) {
    ctx->result->isValid = false;
    if (gVerbose) {
        fprintf(stderr, "%s: Unexpected end of file while parsing line %d.\n",
                ctx->result->filename, ctx->result->lineNumber);
    }
}

%%{
machine csv_loose_c;
alphtype unsigned char;

action to_f0              { ctx->state = 0; }
action to_f2              { ctx->state = 1; }
action to_q2              { ctx->state = 2; }
action to_cr0             { ctx->state = 3; }
action to_cr1             { ctx->state = 4; }
action to_save_quote      { ctx->state = 5; }
action to_save_byte       { ctx->state = 6; }
action to_save_byte_quote { ctx->state = 7; }
action to_save_field      { ctx->state = 8; ++ctx->fields; }
action to_save_field_cr   { ctx->state = 9; ++ctx->fields; }
action to_save_record     { ctx->state = 10; finalizeRecord(ctx); }
action to_save_field_rec  { ctx->state = 11; ++ctx->fields; finalizeRecord(ctx); }
action to_error {
    ctx->state = 12;
    errorUnexpectedChar(ctx, *p);
    fbreak;
}

main := |*
    '\n'        @to_save_record     { fgoto main; };
    '\r'        @to_cr0             { fgoto cr0; };
    ','         @to_save_field      { fgoto after_field; };
    '"'         @to_f2              { fgoto quoted; };
    [^,\n\r"]   @to_save_byte       { fgoto unquoted; };
*|;

quoted := |*
    '"'         @to_q2              { fgoto after_quote; };
    [^"]        @to_save_byte_quote { fgoto quoted; };
*|;

after_quote := |*
    '\n'        @to_save_field_rec  { fgoto main; };
    '\r'        @to_cr1             { fgoto cr1; };
    ','         @to_save_field      { fgoto after_field; };
    '"'         @to_save_quote      { fgoto save_quote; };
    [^,\n\r"]   @to_save_byte       { fgoto unquoted; };
*|;

save_quote := |*
    '"'         @to_q2              { fgoto after_quote; };
    [^"]        @to_save_byte_quote { fgoto quoted; };
*|;

unquoted := |*
    '\n'        @to_save_field_rec  { fgoto main; };
    '\r'        @to_cr1             { fgoto cr1; };
    ','         @to_save_field      { fgoto after_field; };
    '"'         @to_save_byte       { fgoto unquoted; };
    [^,\n\r]    @to_save_byte       { fgoto unquoted; };
*|;

after_field := |*
    '\n'        @to_save_field_rec  { fgoto main; };
    '\r'        @to_save_field_cr   { fgoto after_field_cr; };
    ','         @to_save_field      { fgoto after_field; };
    '"'         @to_f2              { fgoto quoted; };
    [^,\n\r"]   @to_save_byte       { fgoto unquoted; };
*|;

cr0 := |*
    '\n'        @to_save_record     { fgoto main; };
    any         @to_error           { /* stay in error */ };
*|;

cr1 := |*
    '\n'        @to_save_field_rec  { fgoto main; };
    any         @to_error           { /* stay in error */ };
*|;

after_field_cr := |*
    '\n'        @to_save_record     { fgoto main; };
    any         @to_error           { /* stay in error */ };
*|;
}%%

%% write data;

%%{
machine csv_strict_c;
alphtype unsigned char;

action to_f0              { ctx->state = 0; }
action to_f2              { ctx->state = 1; }
action to_q2              { ctx->state = 2; }
action to_cr0             { ctx->state = 3; }
action to_cr1             { ctx->state = 4; }
action to_save_quote      { ctx->state = 5; }
action to_save_byte       { ctx->state = 6; }
action to_save_byte_quote { ctx->state = 7; }
action to_save_field      { ctx->state = 8; ++ctx->fields; }
action to_save_field_cr   { ctx->state = 9; ++ctx->fields; }
action to_save_record     { ctx->state = 10; finalizeRecord(ctx); }
action to_save_field_rec  { ctx->state = 11; ++ctx->fields; finalizeRecord(ctx); }
action to_error {
    ctx->state = 12;
    errorUnexpectedChar(ctx, *p);
    fbreak;
}

main := |*
    '\n'        @to_save_record     { fgoto main; };
    '\r'        @to_cr0             { fgoto cr0; };
    ','         @to_save_field      { fgoto after_field; };
    '"'         @to_f2              { fgoto quoted; };
    [^,\n\r"]   @to_save_byte       { fgoto unquoted; };
*|;

quoted := |*
    '"'         @to_q2              { fgoto after_quote; };
    [^"]        @to_save_byte_quote { fgoto quoted; };
*|;

after_quote := |*
    '\n'        @to_save_field_rec  { fgoto main; };
    '\r'        @to_cr1             { fgoto cr1; };
    ','         @to_save_field      { fgoto after_field; };
    '"'         @to_save_quote      { fgoto save_quote; };
    [^,\n\r"]   @to_error           { /* stay in error */ };
*|;

save_quote := |*
    '"'         @to_q2              { fgoto after_quote; };
    [^"]        @to_save_byte_quote { fgoto quoted; };
*|;

unquoted := |*
    '\n'        @to_save_field_rec  { fgoto main; };
    '\r'        @to_cr1             { fgoto cr1; };
    ','         @to_save_field      { fgoto after_field; };
    '"'         @to_error           { /* stay in error */ };
    [^,\n\r"]   @to_save_byte       { fgoto unquoted; };
*|;

after_field := |*
    '\n'        @to_save_field_rec  { fgoto main; };
    '\r'        @to_save_field_cr   { fgoto after_field_cr; };
    ','         @to_save_field      { fgoto after_field; };
    '"'         @to_f2              { fgoto quoted; };
    [^,\n\r"]   @to_save_byte       { fgoto unquoted; };
*|;

cr0 := |*
    '\n'        @to_save_record     { fgoto main; };
    any         @to_error           { /* stay in error */ };
*|;

cr1 := |*
    '\n'        @to_save_field_rec  { fgoto main; };
    any         @to_error           { /* stay in error */ };
*|;

after_field_cr := |*
    '\n'        @to_save_record     { fgoto main; };
    any         @to_error           { /* stay in error */ };
*|;
}%%

%% write data;

static void runLoose(const char* data, size_t len, ParserContext* ctx) {
    const unsigned char* p = (const unsigned char*)data;
    const unsigned char* pe = p + len;
    const unsigned char* eof = pe;
    const unsigned char* ts = NULL;
    const unsigned char* te = NULL;
    int act = 0;
    int cs = 0;

    %% machine csv_loose_c;
    %% write init;
    %% write exec;

    if (!ctx->result->isValid) return;
    /* End of input ends the last record, newline or not (RFC 4180 2.2). */
    switch (ctx->state) {
        case 2:   /* after a closing quote: the field is complete */
        case 6:   /* in an unquoted field */
        case 8:   /* after a comma: an empty last field */
            ++ctx->fields;
            finalizeRecord(ctx);
            break;
        case 0:   /* nothing read */
        case 10:  /* the record already ended */
        case 11:
            break;
        default:  /* 1, 5, 7: inside an open quote; 3, 4, 9: a CR with no LF */
            unexpectedEof(ctx);
            break;
    }
}

static void runStrict(const char* data, size_t len, ParserContext* ctx) {
    const unsigned char* p = (const unsigned char*)data;
    const unsigned char* pe = p + len;
    const unsigned char* eof = pe;
    const unsigned char* ts = NULL;
    const unsigned char* te = NULL;
    int act = 0;
    int cs = 0;

    %% machine csv_strict_c;
    %% write init;
    %% write exec;

    if (!ctx->result->isValid) return;
    /* End of input ends the last record, newline or not (RFC 4180 2.2). */
    switch (ctx->state) {
        case 2:   /* after a closing quote: the field is complete */
        case 6:   /* in an unquoted field */
        case 8:   /* after a comma: an empty last field */
            ++ctx->fields;
            finalizeRecord(ctx);
            break;
        case 0:   /* nothing read */
        case 10:  /* the record already ended */
        case 11:
            break;
        default:  /* 1, 5, 7: inside an open quote; 3, 4, 9: a CR with no LF */
            unexpectedEof(ctx);
            break;
    }
}

static void parseBuffer(const char* data, size_t len, bool strict, FileResult* result) {
    ParserContext ctx = {
        .result = result,
        .haveExpected = false,
        .fields = 0,
        .state = 0,
    };
    if (strict) {
        runStrict(data, len, &ctx);
    } else {
        runLoose(data, len, &ctx);
    }
}

static FileResult ValidateFile(const char* filename, bool strict) {
    FileResult result = {.filename = filename, .isValid = true, .expectedFields = 0, .lineNumber = 1};

    if (gUseMmap) {
        int fd = open(filename, O_RDONLY);
        if (fd == -1) {
            result.isValid = false;
            if (gVerbose) {
                fprintf(stderr, "Unable to open file: %s\n", filename);
            }
            return result;
        }

        struct stat sb;
        if (fstat(fd, &sb) == -1) {
            if (gVerbose) fprintf(stderr, "Unable to get file size: %s\n", filename);
            close(fd);
            result.isValid = false;
            return result;
        }

        if (sb.st_size == 0) {
            close(fd);
            return result;
        }

        char* addr = (char*)mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (addr == MAP_FAILED) {
            if (gVerbose) fprintf(stderr, "Unable to memory map file: %s\n", filename);
            close(fd);
            result.isValid = false;
            return result;
        }

        parseBuffer(addr, (size_t)sb.st_size, strict, &result);

        munmap(addr, sb.st_size);
        close(fd);
    } else {
        FILE* f = fopen(filename, "rb");
        if (!f) {
            result.isValid = false;
            if (gVerbose) fprintf(stderr, "Unable to open file: %s\n", filename);
            return result;
        }
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);
        if (size > 0) {
            char* buf = (char*)malloc((size_t)size);
            if (!buf) {
                if (gVerbose) fprintf(stderr, "Unable to allocate buffer for: %s\n", filename);
                fclose(f);
                result.isValid = false;
                return result;
            }
            size_t read = fread(buf, 1, (size_t)size, f);
            if (read == (size_t)size) {
                parseBuffer(buf, (size_t)size, strict, &result);
            } else {
                if (gVerbose) fprintf(stderr, "Unable to read file: %s\n", filename);
                result.isValid = false;
            }
            free(buf);
        }
        fclose(f);
    }

    if (gVerbose) {
        fprintf(stdout, "%s: File is %sly %s with %d columns and %d rows.\n",
                filename,
                strict ? "strict" : "loose",
                result.isValid ? "valid" : "invalid",
                result.expectedFields,
                result.lineNumber);
    }
    return result;
}

int main(int argc, char* argv[]) {
    bool strict = false;
    int maxThreads = 1; // placeholder for parity; C version processes sequentially
    (void)maxThreads;
    if (argc < 2) {
        fprintf(stdout, "Usage: validatecsv [-s|--strict] [-v|--verbose] [--stream] <filename1> [filename2] ...\n");
        return 1;
    }

    int argi = 1;
    const char* files[256];
    int fileCount = 0;
    for (; argi < argc && fileCount < 256; ++argi) {
        if (strcmp(argv[argi], "-s") == 0 || strcmp(argv[argi], "--strict") == 0) {
            strict = true;
        } else if (strcmp(argv[argi], "-v") == 0 || strcmp(argv[argi], "--verbose") == 0) {
            gVerbose = true;
        } else if (strcmp(argv[argi], "--stream") == 0) {
            gUseMmap = false;
        } else if (argv[argi][0] != '-') {
            files[fileCount++] = argv[argi];
        }
    }

    if (fileCount == 0) {
        fprintf(stdout, "Usage: validatecsv [-s|--strict] [-v|--verbose] [--stream] <filename1> [filename2] ...\n");
        return 1;
    }

    bool allValid = true;
    for (int i = 0; i < fileCount; ++i) {
        FileResult r = ValidateFile(files[i], strict);
        if (!r.isValid) {
            allValid = false;
        }
    }

    return allValid ? 0 : 1;
}

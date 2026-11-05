
#line 1 "c/validatecsv.rl"
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


#line 141 "c/validatecsv.rl"



#line 66 "c/validatecsv.c"
static const int csv_loose_c_start = 0;
static const int csv_loose_c_first_final = 0;
static const int csv_loose_c_error = -1;

static const int csv_loose_c_en_main = 0;
static const int csv_loose_c_en_quoted = 1;
static const int csv_loose_c_en_after_quote = 2;
static const int csv_loose_c_en_save_quote = 3;
static const int csv_loose_c_en_unquoted = 4;
static const int csv_loose_c_en_after_field = 5;
static const int csv_loose_c_en_cr0 = 6;
static const int csv_loose_c_en_cr1 = 7;
static const int csv_loose_c_en_after_field_cr = 8;


#line 144 "c/validatecsv.rl"


#line 223 "c/validatecsv.rl"



#line 83 "c/validatecsv.c"
static const int csv_strict_c_start = 0;
static const int csv_strict_c_first_final = 0;
static const int csv_strict_c_error = -1;

static const int csv_strict_c_en_main = 0;
static const int csv_strict_c_en_quoted = 1;
static const int csv_strict_c_en_after_quote = 2;
static const int csv_strict_c_en_save_quote = 3;
static const int csv_strict_c_en_unquoted = 4;
static const int csv_strict_c_en_after_field = 5;
static const int csv_strict_c_en_cr0 = 6;
static const int csv_strict_c_en_cr1 = 7;
static const int csv_strict_c_en_after_field_cr = 8;


#line 226 "c/validatecsv.rl"

static void runLoose(const char* data, size_t len, ParserContext* ctx) {
    const unsigned char* p = (const unsigned char*)data;
    const unsigned char* pe = p + len;
    const unsigned char* eof = pe;
    const unsigned char* ts = NULL;
    const unsigned char* te = NULL;
    int act = 0;
    int cs = 0;

    
#line 237 "c/validatecsv.rl"
    
#line 107 "c/validatecsv.c"
	{
	cs = csv_loose_c_start;
	ts = 0;
	te = 0;
	act = 0;
	}

#line 238 "c/validatecsv.rl"
    
#line 113 "c/validatecsv.c"
	{
	if ( p == pe )
		goto _test_eof;
	switch ( cs )
	{
tr0:
#line 73 "c/validatecsv.rl"
	{ ctx->state = 6; }
#line 90 "c/validatecsv.rl"
	{te = p+1;{ {goto st4;} }}
	goto st0;
tr1:
#line 77 "c/validatecsv.rl"
	{ ctx->state = 10; finalizeRecord(ctx); }
#line 86 "c/validatecsv.rl"
	{te = p+1;{ {goto st0;} }}
	goto st0;
tr2:
#line 70 "c/validatecsv.rl"
	{ ctx->state = 3; }
#line 87 "c/validatecsv.rl"
	{te = p+1;{ {goto st6;} }}
	goto st0;
tr3:
#line 68 "c/validatecsv.rl"
	{ ctx->state = 1; }
#line 89 "c/validatecsv.rl"
	{te = p+1;{ {goto st1;} }}
	goto st0;
tr4:
#line 75 "c/validatecsv.rl"
	{ ctx->state = 8; ++ctx->fields; }
#line 88 "c/validatecsv.rl"
	{te = p+1;{ {goto st5;} }}
	goto st0;
st0:
#line 1 "NONE"
	{ts = 0;}
	if ( ++p == pe )
		goto _test_eof0;
case 0:
#line 1 "NONE"
	{ts = p;}
#line 144 "c/validatecsv.c"
	switch( (*p) ) {
		case 10u: goto tr1;
		case 13u: goto tr2;
		case 34u: goto tr3;
		case 44u: goto tr4;
	}
	goto tr0;
tr5:
#line 74 "c/validatecsv.rl"
	{ ctx->state = 7; }
#line 95 "c/validatecsv.rl"
	{te = p+1;{ {goto st1;} }}
	goto st1;
tr6:
#line 69 "c/validatecsv.rl"
	{ ctx->state = 2; }
#line 94 "c/validatecsv.rl"
	{te = p+1;{ {goto st2;} }}
	goto st1;
st1:
#line 1 "NONE"
	{ts = 0;}
	if ( ++p == pe )
		goto _test_eof1;
case 1:
#line 1 "NONE"
	{ts = p;}
#line 165 "c/validatecsv.c"
	if ( (*p) == 34u )
		goto tr6;
	goto tr5;
tr7:
#line 73 "c/validatecsv.rl"
	{ ctx->state = 6; }
#line 103 "c/validatecsv.rl"
	{te = p+1;{ {goto st4;} }}
	goto st2;
tr8:
#line 78 "c/validatecsv.rl"
	{ ctx->state = 11; ++ctx->fields; finalizeRecord(ctx); }
#line 99 "c/validatecsv.rl"
	{te = p+1;{ {goto st0;} }}
	goto st2;
tr9:
#line 71 "c/validatecsv.rl"
	{ ctx->state = 4; }
#line 100 "c/validatecsv.rl"
	{te = p+1;{ {goto st7;} }}
	goto st2;
tr10:
#line 72 "c/validatecsv.rl"
	{ ctx->state = 5; }
#line 102 "c/validatecsv.rl"
	{te = p+1;{ {goto st3;} }}
	goto st2;
tr11:
#line 75 "c/validatecsv.rl"
	{ ctx->state = 8; ++ctx->fields; }
#line 101 "c/validatecsv.rl"
	{te = p+1;{ {goto st5;} }}
	goto st2;
st2:
#line 1 "NONE"
	{ts = 0;}
	if ( ++p == pe )
		goto _test_eof2;
case 2:
#line 1 "NONE"
	{ts = p;}
#line 194 "c/validatecsv.c"
	switch( (*p) ) {
		case 10u: goto tr8;
		case 13u: goto tr9;
		case 34u: goto tr10;
		case 44u: goto tr11;
	}
	goto tr7;
tr12:
#line 74 "c/validatecsv.rl"
	{ ctx->state = 7; }
#line 108 "c/validatecsv.rl"
	{te = p+1;{ {goto st1;} }}
	goto st3;
tr13:
#line 69 "c/validatecsv.rl"
	{ ctx->state = 2; }
#line 107 "c/validatecsv.rl"
	{te = p+1;{ {goto st2;} }}
	goto st3;
st3:
#line 1 "NONE"
	{ts = 0;}
	if ( ++p == pe )
		goto _test_eof3;
case 3:
#line 1 "NONE"
	{ts = p;}
#line 215 "c/validatecsv.c"
	if ( (*p) == 34u )
		goto tr13;
	goto tr12;
tr14:
#line 73 "c/validatecsv.rl"
	{ ctx->state = 6; }
#line 116 "c/validatecsv.rl"
	{te = p+1;{ {goto st4;} }}
	goto st4;
tr15:
#line 78 "c/validatecsv.rl"
	{ ctx->state = 11; ++ctx->fields; finalizeRecord(ctx); }
#line 112 "c/validatecsv.rl"
	{te = p+1;{ {goto st0;} }}
	goto st4;
tr16:
#line 71 "c/validatecsv.rl"
	{ ctx->state = 4; }
#line 113 "c/validatecsv.rl"
	{te = p+1;{ {goto st7;} }}
	goto st4;
tr17:
#line 73 "c/validatecsv.rl"
	{ ctx->state = 6; }
#line 115 "c/validatecsv.rl"
	{te = p+1;{ {goto st4;} }}
	goto st4;
tr18:
#line 75 "c/validatecsv.rl"
	{ ctx->state = 8; ++ctx->fields; }
#line 114 "c/validatecsv.rl"
	{te = p+1;{ {goto st5;} }}
	goto st4;
st4:
#line 1 "NONE"
	{ts = 0;}
	if ( ++p == pe )
		goto _test_eof4;
case 4:
#line 1 "NONE"
	{ts = p;}
#line 244 "c/validatecsv.c"
	switch( (*p) ) {
		case 10u: goto tr15;
		case 13u: goto tr16;
		case 34u: goto tr17;
		case 44u: goto tr18;
	}
	goto tr14;
tr19:
#line 73 "c/validatecsv.rl"
	{ ctx->state = 6; }
#line 124 "c/validatecsv.rl"
	{te = p+1;{ {goto st4;} }}
	goto st5;
tr20:
#line 78 "c/validatecsv.rl"
	{ ctx->state = 11; ++ctx->fields; finalizeRecord(ctx); }
#line 120 "c/validatecsv.rl"
	{te = p+1;{ {goto st0;} }}
	goto st5;
tr21:
#line 76 "c/validatecsv.rl"
	{ ctx->state = 9; ++ctx->fields; }
#line 121 "c/validatecsv.rl"
	{te = p+1;{ {goto st8;} }}
	goto st5;
tr22:
#line 68 "c/validatecsv.rl"
	{ ctx->state = 1; }
#line 123 "c/validatecsv.rl"
	{te = p+1;{ {goto st1;} }}
	goto st5;
tr23:
#line 75 "c/validatecsv.rl"
	{ ctx->state = 8; ++ctx->fields; }
#line 122 "c/validatecsv.rl"
	{te = p+1;{ {goto st5;} }}
	goto st5;
st5:
#line 1 "NONE"
	{ts = 0;}
	if ( ++p == pe )
		goto _test_eof5;
case 5:
#line 1 "NONE"
	{ts = p;}
#line 277 "c/validatecsv.c"
	switch( (*p) ) {
		case 10u: goto tr20;
		case 13u: goto tr21;
		case 34u: goto tr22;
		case 44u: goto tr23;
	}
	goto tr19;
tr24:
#line 79 "c/validatecsv.rl"
	{
    ctx->state = 12;
    errorUnexpectedChar(ctx, *p);
    {p++; cs = 6; goto _out;}
}
#line 129 "c/validatecsv.rl"
	{te = p+1;{ /* stay in error */ }}
	goto st6;
tr25:
#line 77 "c/validatecsv.rl"
	{ ctx->state = 10; finalizeRecord(ctx); }
#line 128 "c/validatecsv.rl"
	{te = p+1;{ {goto st0;} }}
#line 79 "c/validatecsv.rl"
	{
    ctx->state = 12;
    errorUnexpectedChar(ctx, *p);
    {p++; cs = 6; goto _out;}
}
	goto st6;
st6:
#line 1 "NONE"
	{ts = 0;}
	if ( ++p == pe )
		goto _test_eof6;
case 6:
#line 1 "NONE"
	{ts = p;}
#line 307 "c/validatecsv.c"
	if ( (*p) == 10u )
		goto tr25;
	goto tr24;
tr26:
#line 79 "c/validatecsv.rl"
	{
    ctx->state = 12;
    errorUnexpectedChar(ctx, *p);
    {p++; cs = 7; goto _out;}
}
#line 134 "c/validatecsv.rl"
	{te = p+1;{ /* stay in error */ }}
	goto st7;
tr27:
#line 78 "c/validatecsv.rl"
	{ ctx->state = 11; ++ctx->fields; finalizeRecord(ctx); }
#line 133 "c/validatecsv.rl"
	{te = p+1;{ {goto st0;} }}
#line 79 "c/validatecsv.rl"
	{
    ctx->state = 12;
    errorUnexpectedChar(ctx, *p);
    {p++; cs = 7; goto _out;}
}
	goto st7;
st7:
#line 1 "NONE"
	{ts = 0;}
	if ( ++p == pe )
		goto _test_eof7;
case 7:
#line 1 "NONE"
	{ts = p;}
#line 333 "c/validatecsv.c"
	if ( (*p) == 10u )
		goto tr27;
	goto tr26;
tr28:
#line 79 "c/validatecsv.rl"
	{
    ctx->state = 12;
    errorUnexpectedChar(ctx, *p);
    {p++; cs = 8; goto _out;}
}
#line 139 "c/validatecsv.rl"
	{te = p+1;{ /* stay in error */ }}
	goto st8;
tr29:
#line 77 "c/validatecsv.rl"
	{ ctx->state = 10; finalizeRecord(ctx); }
#line 138 "c/validatecsv.rl"
	{te = p+1;{ {goto st0;} }}
#line 79 "c/validatecsv.rl"
	{
    ctx->state = 12;
    errorUnexpectedChar(ctx, *p);
    {p++; cs = 8; goto _out;}
}
	goto st8;
st8:
#line 1 "NONE"
	{ts = 0;}
	if ( ++p == pe )
		goto _test_eof8;
case 8:
#line 1 "NONE"
	{ts = p;}
#line 359 "c/validatecsv.c"
	if ( (*p) == 10u )
		goto tr29;
	goto tr28;
	}
	_test_eof0: cs = 0; goto _test_eof; 
	_test_eof1: cs = 1; goto _test_eof; 
	_test_eof2: cs = 2; goto _test_eof; 
	_test_eof3: cs = 3; goto _test_eof; 
	_test_eof4: cs = 4; goto _test_eof; 
	_test_eof5: cs = 5; goto _test_eof; 
	_test_eof6: cs = 6; goto _test_eof; 
	_test_eof7: cs = 7; goto _test_eof; 
	_test_eof8: cs = 8; goto _test_eof; 

	_test_eof: {}
	_out: {}
	}

#line 239 "c/validatecsv.rl"

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

    
#line 269 "c/validatecsv.rl"
    
#line 405 "c/validatecsv.c"
	{
	cs = csv_strict_c_start;
	ts = 0;
	te = 0;
	act = 0;
	}

#line 270 "c/validatecsv.rl"
    
#line 411 "c/validatecsv.c"
	{
	if ( p == pe )
		goto _test_eof;
	switch ( cs )
	{
tr0:
#line 155 "c/validatecsv.rl"
	{ ctx->state = 6; }
#line 172 "c/validatecsv.rl"
	{te = p+1;{ {goto st4;} }}
	goto st0;
tr1:
#line 159 "c/validatecsv.rl"
	{ ctx->state = 10; finalizeRecord(ctx); }
#line 168 "c/validatecsv.rl"
	{te = p+1;{ {goto st0;} }}
	goto st0;
tr2:
#line 152 "c/validatecsv.rl"
	{ ctx->state = 3; }
#line 169 "c/validatecsv.rl"
	{te = p+1;{ {goto st6;} }}
	goto st0;
tr3:
#line 150 "c/validatecsv.rl"
	{ ctx->state = 1; }
#line 171 "c/validatecsv.rl"
	{te = p+1;{ {goto st1;} }}
	goto st0;
tr4:
#line 157 "c/validatecsv.rl"
	{ ctx->state = 8; ++ctx->fields; }
#line 170 "c/validatecsv.rl"
	{te = p+1;{ {goto st5;} }}
	goto st0;
st0:
#line 1 "NONE"
	{ts = 0;}
	if ( ++p == pe )
		goto _test_eof0;
case 0:
#line 1 "NONE"
	{ts = p;}
#line 442 "c/validatecsv.c"
	switch( (*p) ) {
		case 10u: goto tr1;
		case 13u: goto tr2;
		case 34u: goto tr3;
		case 44u: goto tr4;
	}
	goto tr0;
tr5:
#line 156 "c/validatecsv.rl"
	{ ctx->state = 7; }
#line 177 "c/validatecsv.rl"
	{te = p+1;{ {goto st1;} }}
	goto st1;
tr6:
#line 151 "c/validatecsv.rl"
	{ ctx->state = 2; }
#line 176 "c/validatecsv.rl"
	{te = p+1;{ {goto st2;} }}
	goto st1;
st1:
#line 1 "NONE"
	{ts = 0;}
	if ( ++p == pe )
		goto _test_eof1;
case 1:
#line 1 "NONE"
	{ts = p;}
#line 463 "c/validatecsv.c"
	if ( (*p) == 34u )
		goto tr6;
	goto tr5;
tr7:
#line 161 "c/validatecsv.rl"
	{
    ctx->state = 12;
    errorUnexpectedChar(ctx, *p);
    {p++; cs = 2; goto _out;}
}
#line 185 "c/validatecsv.rl"
	{te = p+1;{ /* stay in error */ }}
	goto st2;
tr8:
#line 160 "c/validatecsv.rl"
	{ ctx->state = 11; ++ctx->fields; finalizeRecord(ctx); }
#line 181 "c/validatecsv.rl"
	{te = p+1;{ {goto st0;} }}
	goto st2;
tr9:
#line 153 "c/validatecsv.rl"
	{ ctx->state = 4; }
#line 182 "c/validatecsv.rl"
	{te = p+1;{ {goto st7;} }}
	goto st2;
tr10:
#line 154 "c/validatecsv.rl"
	{ ctx->state = 5; }
#line 184 "c/validatecsv.rl"
	{te = p+1;{ {goto st3;} }}
	goto st2;
tr11:
#line 157 "c/validatecsv.rl"
	{ ctx->state = 8; ++ctx->fields; }
#line 183 "c/validatecsv.rl"
	{te = p+1;{ {goto st5;} }}
	goto st2;
st2:
#line 1 "NONE"
	{ts = 0;}
	if ( ++p == pe )
		goto _test_eof2;
case 2:
#line 1 "NONE"
	{ts = p;}
#line 496 "c/validatecsv.c"
	switch( (*p) ) {
		case 10u: goto tr8;
		case 13u: goto tr9;
		case 34u: goto tr10;
		case 44u: goto tr11;
	}
	goto tr7;
tr12:
#line 156 "c/validatecsv.rl"
	{ ctx->state = 7; }
#line 190 "c/validatecsv.rl"
	{te = p+1;{ {goto st1;} }}
	goto st3;
tr13:
#line 151 "c/validatecsv.rl"
	{ ctx->state = 2; }
#line 189 "c/validatecsv.rl"
	{te = p+1;{ {goto st2;} }}
	goto st3;
st3:
#line 1 "NONE"
	{ts = 0;}
	if ( ++p == pe )
		goto _test_eof3;
case 3:
#line 1 "NONE"
	{ts = p;}
#line 517 "c/validatecsv.c"
	if ( (*p) == 34u )
		goto tr13;
	goto tr12;
tr14:
#line 155 "c/validatecsv.rl"
	{ ctx->state = 6; }
#line 198 "c/validatecsv.rl"
	{te = p+1;{ {goto st4;} }}
	goto st4;
tr15:
#line 160 "c/validatecsv.rl"
	{ ctx->state = 11; ++ctx->fields; finalizeRecord(ctx); }
#line 194 "c/validatecsv.rl"
	{te = p+1;{ {goto st0;} }}
	goto st4;
tr16:
#line 153 "c/validatecsv.rl"
	{ ctx->state = 4; }
#line 195 "c/validatecsv.rl"
	{te = p+1;{ {goto st7;} }}
	goto st4;
tr17:
#line 161 "c/validatecsv.rl"
	{
    ctx->state = 12;
    errorUnexpectedChar(ctx, *p);
    {p++; cs = 4; goto _out;}
}
#line 197 "c/validatecsv.rl"
	{te = p+1;{ /* stay in error */ }}
	goto st4;
tr18:
#line 157 "c/validatecsv.rl"
	{ ctx->state = 8; ++ctx->fields; }
#line 196 "c/validatecsv.rl"
	{te = p+1;{ {goto st5;} }}
	goto st4;
st4:
#line 1 "NONE"
	{ts = 0;}
	if ( ++p == pe )
		goto _test_eof4;
case 4:
#line 1 "NONE"
	{ts = p;}
#line 550 "c/validatecsv.c"
	switch( (*p) ) {
		case 10u: goto tr15;
		case 13u: goto tr16;
		case 34u: goto tr17;
		case 44u: goto tr18;
	}
	goto tr14;
tr19:
#line 155 "c/validatecsv.rl"
	{ ctx->state = 6; }
#line 206 "c/validatecsv.rl"
	{te = p+1;{ {goto st4;} }}
	goto st5;
tr20:
#line 160 "c/validatecsv.rl"
	{ ctx->state = 11; ++ctx->fields; finalizeRecord(ctx); }
#line 202 "c/validatecsv.rl"
	{te = p+1;{ {goto st0;} }}
	goto st5;
tr21:
#line 158 "c/validatecsv.rl"
	{ ctx->state = 9; ++ctx->fields; }
#line 203 "c/validatecsv.rl"
	{te = p+1;{ {goto st8;} }}
	goto st5;
tr22:
#line 150 "c/validatecsv.rl"
	{ ctx->state = 1; }
#line 205 "c/validatecsv.rl"
	{te = p+1;{ {goto st1;} }}
	goto st5;
tr23:
#line 157 "c/validatecsv.rl"
	{ ctx->state = 8; ++ctx->fields; }
#line 204 "c/validatecsv.rl"
	{te = p+1;{ {goto st5;} }}
	goto st5;
st5:
#line 1 "NONE"
	{ts = 0;}
	if ( ++p == pe )
		goto _test_eof5;
case 5:
#line 1 "NONE"
	{ts = p;}
#line 583 "c/validatecsv.c"
	switch( (*p) ) {
		case 10u: goto tr20;
		case 13u: goto tr21;
		case 34u: goto tr22;
		case 44u: goto tr23;
	}
	goto tr19;
tr24:
#line 161 "c/validatecsv.rl"
	{
    ctx->state = 12;
    errorUnexpectedChar(ctx, *p);
    {p++; cs = 6; goto _out;}
}
#line 211 "c/validatecsv.rl"
	{te = p+1;{ /* stay in error */ }}
	goto st6;
tr25:
#line 159 "c/validatecsv.rl"
	{ ctx->state = 10; finalizeRecord(ctx); }
#line 210 "c/validatecsv.rl"
	{te = p+1;{ {goto st0;} }}
#line 161 "c/validatecsv.rl"
	{
    ctx->state = 12;
    errorUnexpectedChar(ctx, *p);
    {p++; cs = 6; goto _out;}
}
	goto st6;
st6:
#line 1 "NONE"
	{ts = 0;}
	if ( ++p == pe )
		goto _test_eof6;
case 6:
#line 1 "NONE"
	{ts = p;}
#line 613 "c/validatecsv.c"
	if ( (*p) == 10u )
		goto tr25;
	goto tr24;
tr26:
#line 161 "c/validatecsv.rl"
	{
    ctx->state = 12;
    errorUnexpectedChar(ctx, *p);
    {p++; cs = 7; goto _out;}
}
#line 216 "c/validatecsv.rl"
	{te = p+1;{ /* stay in error */ }}
	goto st7;
tr27:
#line 160 "c/validatecsv.rl"
	{ ctx->state = 11; ++ctx->fields; finalizeRecord(ctx); }
#line 215 "c/validatecsv.rl"
	{te = p+1;{ {goto st0;} }}
#line 161 "c/validatecsv.rl"
	{
    ctx->state = 12;
    errorUnexpectedChar(ctx, *p);
    {p++; cs = 7; goto _out;}
}
	goto st7;
st7:
#line 1 "NONE"
	{ts = 0;}
	if ( ++p == pe )
		goto _test_eof7;
case 7:
#line 1 "NONE"
	{ts = p;}
#line 639 "c/validatecsv.c"
	if ( (*p) == 10u )
		goto tr27;
	goto tr26;
tr28:
#line 161 "c/validatecsv.rl"
	{
    ctx->state = 12;
    errorUnexpectedChar(ctx, *p);
    {p++; cs = 8; goto _out;}
}
#line 221 "c/validatecsv.rl"
	{te = p+1;{ /* stay in error */ }}
	goto st8;
tr29:
#line 159 "c/validatecsv.rl"
	{ ctx->state = 10; finalizeRecord(ctx); }
#line 220 "c/validatecsv.rl"
	{te = p+1;{ {goto st0;} }}
#line 161 "c/validatecsv.rl"
	{
    ctx->state = 12;
    errorUnexpectedChar(ctx, *p);
    {p++; cs = 8; goto _out;}
}
	goto st8;
st8:
#line 1 "NONE"
	{ts = 0;}
	if ( ++p == pe )
		goto _test_eof8;
case 8:
#line 1 "NONE"
	{ts = p;}
#line 665 "c/validatecsv.c"
	if ( (*p) == 10u )
		goto tr29;
	goto tr28;
	}
	_test_eof0: cs = 0; goto _test_eof; 
	_test_eof1: cs = 1; goto _test_eof; 
	_test_eof2: cs = 2; goto _test_eof; 
	_test_eof3: cs = 3; goto _test_eof; 
	_test_eof4: cs = 4; goto _test_eof; 
	_test_eof5: cs = 5; goto _test_eof; 
	_test_eof6: cs = 6; goto _test_eof; 
	_test_eof7: cs = 7; goto _test_eof; 
	_test_eof8: cs = 8; goto _test_eof; 

	_test_eof: {}
	_out: {}
	}

#line 271 "c/validatecsv.rl"

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

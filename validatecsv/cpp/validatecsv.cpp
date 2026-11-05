
#line 1 "cpp/validatecsv.rl"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

struct FileResult {
    std::string filename;
    bool isValid;
    int nFieldsExpected;
    int nLineNumber;
    std::vector<std::string> errors;
};

namespace ragel_csv {

struct ParserContext {
    FileResult& result;
    bool haveExpected = false;
    int fields = 0;
    uint8_t state = 0;

    explicit ParserContext(FileResult& r) : result(r) {}

    void finalizeRecord() {
        if (haveExpected) {
            if (fields != result.nFieldsExpected) {
                result.isValid = false;
                result.errors.push_back("Line " + std::to_string(result.nLineNumber) +
                                        " contains " + std::to_string(fields) +
                                        " fields instead of the expected " +
                                        std::to_string(result.nFieldsExpected) + " fields.");
            }
        } else {
            result.nFieldsExpected = fields;
            haveExpected = true;
        }
        fields = 0;
        ++result.nLineNumber;
    }

    void errorUnexpectedChar(unsigned char ch) {
        result.isValid = false;
        result.errors.push_back("Unexpected character '" + std::string(1, static_cast<char>(ch)) +
                                "' (decimal " + std::to_string(static_cast<int>(ch)) +
                                ") on line " + std::to_string(result.nLineNumber) + ".");
    }

    void unexpectedEof() {
        result.isValid = false;
        result.errors.push_back("Unexpected end of file while parsing line " +
                                std::to_string(result.nLineNumber) + ".");
    }

    void finish() {
        if (!result.isValid) return;
        // End of input ends the last record, newline or not (RFC 4180 2.2).
        switch (state) {
            case 2:  // after a closing quote: the field is complete
            case 6:  // in an unquoted field
            case 8:  // after a comma: an empty last field
                ++fields;
                finalizeRecord();
                break;
            case 0:  // nothing read
            case 10: // the record already ended
            case 11:
                break;
            default: // 1, 5, 7: inside an open quote; 3, 4, 9: a CR with no LF
                unexpectedEof();
                break;
        }
    }
};


#line 162 "cpp/validatecsv.rl"



#line 87 "cpp/validatecsv.cpp"
static const int csv_loose_start = 0;
static const int csv_loose_first_final = 0;
static const int csv_loose_error = -1;

static const int csv_loose_en_main = 0;
static const int csv_loose_en_quoted = 1;
static const int csv_loose_en_after_quote = 2;
static const int csv_loose_en_save_quote = 3;
static const int csv_loose_en_unquoted = 4;
static const int csv_loose_en_after_field = 5;
static const int csv_loose_en_cr0 = 6;
static const int csv_loose_en_cr1 = 7;
static const int csv_loose_en_after_field_cr = 8;


#line 165 "cpp/validatecsv.rl"


#line 244 "cpp/validatecsv.rl"



#line 104 "cpp/validatecsv.cpp"
static const int csv_strict_start = 0;
static const int csv_strict_first_final = 0;
static const int csv_strict_error = -1;

static const int csv_strict_en_main = 0;
static const int csv_strict_en_quoted = 1;
static const int csv_strict_en_after_quote = 2;
static const int csv_strict_en_save_quote = 3;
static const int csv_strict_en_unquoted = 4;
static const int csv_strict_en_after_field = 5;
static const int csv_strict_en_cr0 = 6;
static const int csv_strict_en_cr1 = 7;
static const int csv_strict_en_after_field_cr = 8;


#line 247 "cpp/validatecsv.rl"

static void runLoose(const char* data, std::size_t len, ParserContext& ctx) {
    const unsigned char* p = reinterpret_cast<const unsigned char*>(data);
    const unsigned char* pe = p + len;
    const unsigned char* eof = pe;
    const unsigned char* ts = nullptr;
    const unsigned char* te = nullptr;
    int act = 0;
    int cs = 0;

    
#line 258 "cpp/validatecsv.rl"
    
#line 128 "cpp/validatecsv.cpp"
	{
	cs = csv_loose_start;
	ts = 0;
	te = 0;
	act = 0;
	}

#line 259 "cpp/validatecsv.rl"
    
#line 134 "cpp/validatecsv.cpp"
	{
	if ( p == pe )
		goto _test_eof;
	switch ( cs )
	{
tr0:
#line 94 "cpp/validatecsv.rl"
	{ ctx.state = 6; }
#line 111 "cpp/validatecsv.rl"
	{te = p+1;{ {goto st4;} }}
	goto st0;
tr1:
#line 98 "cpp/validatecsv.rl"
	{ ctx.state = 10; ctx.finalizeRecord(); }
#line 107 "cpp/validatecsv.rl"
	{te = p+1;{ {goto st0;} }}
	goto st0;
tr2:
#line 91 "cpp/validatecsv.rl"
	{ ctx.state = 3; }
#line 108 "cpp/validatecsv.rl"
	{te = p+1;{ {goto st6;} }}
	goto st0;
tr3:
#line 89 "cpp/validatecsv.rl"
	{ ctx.state = 1; }
#line 110 "cpp/validatecsv.rl"
	{te = p+1;{ {goto st1;} }}
	goto st0;
tr4:
#line 96 "cpp/validatecsv.rl"
	{ ctx.state = 8; ++ctx.fields; }
#line 109 "cpp/validatecsv.rl"
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
#line 165 "cpp/validatecsv.cpp"
	switch( (*p) ) {
		case 10u: goto tr1;
		case 13u: goto tr2;
		case 34u: goto tr3;
		case 44u: goto tr4;
	}
	goto tr0;
tr5:
#line 95 "cpp/validatecsv.rl"
	{ ctx.state = 7; }
#line 116 "cpp/validatecsv.rl"
	{te = p+1;{ {goto st1;} }}
	goto st1;
tr6:
#line 90 "cpp/validatecsv.rl"
	{ ctx.state = 2; }
#line 115 "cpp/validatecsv.rl"
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
#line 186 "cpp/validatecsv.cpp"
	if ( (*p) == 34u )
		goto tr6;
	goto tr5;
tr7:
#line 94 "cpp/validatecsv.rl"
	{ ctx.state = 6; }
#line 124 "cpp/validatecsv.rl"
	{te = p+1;{ {goto st4;} }}
	goto st2;
tr8:
#line 99 "cpp/validatecsv.rl"
	{ ctx.state = 11; ++ctx.fields; ctx.finalizeRecord(); }
#line 120 "cpp/validatecsv.rl"
	{te = p+1;{ {goto st0;} }}
	goto st2;
tr9:
#line 92 "cpp/validatecsv.rl"
	{ ctx.state = 4; }
#line 121 "cpp/validatecsv.rl"
	{te = p+1;{ {goto st7;} }}
	goto st2;
tr10:
#line 93 "cpp/validatecsv.rl"
	{ ctx.state = 5; }
#line 123 "cpp/validatecsv.rl"
	{te = p+1;{ {goto st3;} }}
	goto st2;
tr11:
#line 96 "cpp/validatecsv.rl"
	{ ctx.state = 8; ++ctx.fields; }
#line 122 "cpp/validatecsv.rl"
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
#line 215 "cpp/validatecsv.cpp"
	switch( (*p) ) {
		case 10u: goto tr8;
		case 13u: goto tr9;
		case 34u: goto tr10;
		case 44u: goto tr11;
	}
	goto tr7;
tr12:
#line 95 "cpp/validatecsv.rl"
	{ ctx.state = 7; }
#line 129 "cpp/validatecsv.rl"
	{te = p+1;{ {goto st1;} }}
	goto st3;
tr13:
#line 90 "cpp/validatecsv.rl"
	{ ctx.state = 2; }
#line 128 "cpp/validatecsv.rl"
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
#line 236 "cpp/validatecsv.cpp"
	if ( (*p) == 34u )
		goto tr13;
	goto tr12;
tr14:
#line 94 "cpp/validatecsv.rl"
	{ ctx.state = 6; }
#line 137 "cpp/validatecsv.rl"
	{te = p+1;{ {goto st4;} }}
	goto st4;
tr15:
#line 99 "cpp/validatecsv.rl"
	{ ctx.state = 11; ++ctx.fields; ctx.finalizeRecord(); }
#line 133 "cpp/validatecsv.rl"
	{te = p+1;{ {goto st0;} }}
	goto st4;
tr16:
#line 92 "cpp/validatecsv.rl"
	{ ctx.state = 4; }
#line 134 "cpp/validatecsv.rl"
	{te = p+1;{ {goto st7;} }}
	goto st4;
tr17:
#line 94 "cpp/validatecsv.rl"
	{ ctx.state = 6; }
#line 136 "cpp/validatecsv.rl"
	{te = p+1;{ {goto st4;} }}
	goto st4;
tr18:
#line 96 "cpp/validatecsv.rl"
	{ ctx.state = 8; ++ctx.fields; }
#line 135 "cpp/validatecsv.rl"
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
#line 265 "cpp/validatecsv.cpp"
	switch( (*p) ) {
		case 10u: goto tr15;
		case 13u: goto tr16;
		case 34u: goto tr17;
		case 44u: goto tr18;
	}
	goto tr14;
tr19:
#line 94 "cpp/validatecsv.rl"
	{ ctx.state = 6; }
#line 145 "cpp/validatecsv.rl"
	{te = p+1;{ {goto st4;} }}
	goto st5;
tr20:
#line 99 "cpp/validatecsv.rl"
	{ ctx.state = 11; ++ctx.fields; ctx.finalizeRecord(); }
#line 141 "cpp/validatecsv.rl"
	{te = p+1;{ {goto st0;} }}
	goto st5;
tr21:
#line 97 "cpp/validatecsv.rl"
	{ ctx.state = 9; ++ctx.fields; }
#line 142 "cpp/validatecsv.rl"
	{te = p+1;{ {goto st8;} }}
	goto st5;
tr22:
#line 89 "cpp/validatecsv.rl"
	{ ctx.state = 1; }
#line 144 "cpp/validatecsv.rl"
	{te = p+1;{ {goto st1;} }}
	goto st5;
tr23:
#line 96 "cpp/validatecsv.rl"
	{ ctx.state = 8; ++ctx.fields; }
#line 143 "cpp/validatecsv.rl"
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
#line 298 "cpp/validatecsv.cpp"
	switch( (*p) ) {
		case 10u: goto tr20;
		case 13u: goto tr21;
		case 34u: goto tr22;
		case 44u: goto tr23;
	}
	goto tr19;
tr24:
#line 100 "cpp/validatecsv.rl"
	{
    ctx.state = 12;
    ctx.errorUnexpectedChar(*p);
    {p++; cs = 6; goto _out;}
}
#line 150 "cpp/validatecsv.rl"
	{te = p+1;{ /* stay in error */ }}
	goto st6;
tr25:
#line 98 "cpp/validatecsv.rl"
	{ ctx.state = 10; ctx.finalizeRecord(); }
#line 149 "cpp/validatecsv.rl"
	{te = p+1;{ {goto st0;} }}
#line 100 "cpp/validatecsv.rl"
	{
    ctx.state = 12;
    ctx.errorUnexpectedChar(*p);
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
#line 328 "cpp/validatecsv.cpp"
	if ( (*p) == 10u )
		goto tr25;
	goto tr24;
tr26:
#line 100 "cpp/validatecsv.rl"
	{
    ctx.state = 12;
    ctx.errorUnexpectedChar(*p);
    {p++; cs = 7; goto _out;}
}
#line 155 "cpp/validatecsv.rl"
	{te = p+1;{ /* stay in error */ }}
	goto st7;
tr27:
#line 99 "cpp/validatecsv.rl"
	{ ctx.state = 11; ++ctx.fields; ctx.finalizeRecord(); }
#line 154 "cpp/validatecsv.rl"
	{te = p+1;{ {goto st0;} }}
#line 100 "cpp/validatecsv.rl"
	{
    ctx.state = 12;
    ctx.errorUnexpectedChar(*p);
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
#line 354 "cpp/validatecsv.cpp"
	if ( (*p) == 10u )
		goto tr27;
	goto tr26;
tr28:
#line 100 "cpp/validatecsv.rl"
	{
    ctx.state = 12;
    ctx.errorUnexpectedChar(*p);
    {p++; cs = 8; goto _out;}
}
#line 160 "cpp/validatecsv.rl"
	{te = p+1;{ /* stay in error */ }}
	goto st8;
tr29:
#line 98 "cpp/validatecsv.rl"
	{ ctx.state = 10; ctx.finalizeRecord(); }
#line 159 "cpp/validatecsv.rl"
	{te = p+1;{ {goto st0;} }}
#line 100 "cpp/validatecsv.rl"
	{
    ctx.state = 12;
    ctx.errorUnexpectedChar(*p);
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
#line 380 "cpp/validatecsv.cpp"
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

#line 260 "cpp/validatecsv.rl"

    ctx.finish();
}

static void runStrict(const char* data, std::size_t len, ParserContext& ctx) {
    const unsigned char* p = reinterpret_cast<const unsigned char*>(data);
    const unsigned char* pe = p + len;
    const unsigned char* eof = pe;
    const unsigned char* ts = nullptr;
    const unsigned char* te = nullptr;
    int act = 0;
    int cs = 0;

    
#line 274 "cpp/validatecsv.rl"
    
#line 410 "cpp/validatecsv.cpp"
	{
	cs = csv_strict_start;
	ts = 0;
	te = 0;
	act = 0;
	}

#line 275 "cpp/validatecsv.rl"
    
#line 416 "cpp/validatecsv.cpp"
	{
	if ( p == pe )
		goto _test_eof;
	switch ( cs )
	{
tr0:
#line 176 "cpp/validatecsv.rl"
	{ ctx.state = 6; }
#line 193 "cpp/validatecsv.rl"
	{te = p+1;{ {goto st4;} }}
	goto st0;
tr1:
#line 180 "cpp/validatecsv.rl"
	{ ctx.state = 10; ctx.finalizeRecord(); }
#line 189 "cpp/validatecsv.rl"
	{te = p+1;{ {goto st0;} }}
	goto st0;
tr2:
#line 173 "cpp/validatecsv.rl"
	{ ctx.state = 3; }
#line 190 "cpp/validatecsv.rl"
	{te = p+1;{ {goto st6;} }}
	goto st0;
tr3:
#line 171 "cpp/validatecsv.rl"
	{ ctx.state = 1; }
#line 192 "cpp/validatecsv.rl"
	{te = p+1;{ {goto st1;} }}
	goto st0;
tr4:
#line 178 "cpp/validatecsv.rl"
	{ ctx.state = 8; ++ctx.fields; }
#line 191 "cpp/validatecsv.rl"
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
#line 447 "cpp/validatecsv.cpp"
	switch( (*p) ) {
		case 10u: goto tr1;
		case 13u: goto tr2;
		case 34u: goto tr3;
		case 44u: goto tr4;
	}
	goto tr0;
tr5:
#line 177 "cpp/validatecsv.rl"
	{ ctx.state = 7; }
#line 198 "cpp/validatecsv.rl"
	{te = p+1;{ {goto st1;} }}
	goto st1;
tr6:
#line 172 "cpp/validatecsv.rl"
	{ ctx.state = 2; }
#line 197 "cpp/validatecsv.rl"
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
#line 468 "cpp/validatecsv.cpp"
	if ( (*p) == 34u )
		goto tr6;
	goto tr5;
tr7:
#line 182 "cpp/validatecsv.rl"
	{
    ctx.state = 12;
    ctx.errorUnexpectedChar(*p);
    {p++; cs = 2; goto _out;}
}
#line 206 "cpp/validatecsv.rl"
	{te = p+1;{ /* stay in error */ }}
	goto st2;
tr8:
#line 181 "cpp/validatecsv.rl"
	{ ctx.state = 11; ++ctx.fields; ctx.finalizeRecord(); }
#line 202 "cpp/validatecsv.rl"
	{te = p+1;{ {goto st0;} }}
	goto st2;
tr9:
#line 174 "cpp/validatecsv.rl"
	{ ctx.state = 4; }
#line 203 "cpp/validatecsv.rl"
	{te = p+1;{ {goto st7;} }}
	goto st2;
tr10:
#line 175 "cpp/validatecsv.rl"
	{ ctx.state = 5; }
#line 205 "cpp/validatecsv.rl"
	{te = p+1;{ {goto st3;} }}
	goto st2;
tr11:
#line 178 "cpp/validatecsv.rl"
	{ ctx.state = 8; ++ctx.fields; }
#line 204 "cpp/validatecsv.rl"
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
#line 501 "cpp/validatecsv.cpp"
	switch( (*p) ) {
		case 10u: goto tr8;
		case 13u: goto tr9;
		case 34u: goto tr10;
		case 44u: goto tr11;
	}
	goto tr7;
tr12:
#line 177 "cpp/validatecsv.rl"
	{ ctx.state = 7; }
#line 211 "cpp/validatecsv.rl"
	{te = p+1;{ {goto st1;} }}
	goto st3;
tr13:
#line 172 "cpp/validatecsv.rl"
	{ ctx.state = 2; }
#line 210 "cpp/validatecsv.rl"
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
#line 522 "cpp/validatecsv.cpp"
	if ( (*p) == 34u )
		goto tr13;
	goto tr12;
tr14:
#line 176 "cpp/validatecsv.rl"
	{ ctx.state = 6; }
#line 219 "cpp/validatecsv.rl"
	{te = p+1;{ {goto st4;} }}
	goto st4;
tr15:
#line 181 "cpp/validatecsv.rl"
	{ ctx.state = 11; ++ctx.fields; ctx.finalizeRecord(); }
#line 215 "cpp/validatecsv.rl"
	{te = p+1;{ {goto st0;} }}
	goto st4;
tr16:
#line 174 "cpp/validatecsv.rl"
	{ ctx.state = 4; }
#line 216 "cpp/validatecsv.rl"
	{te = p+1;{ {goto st7;} }}
	goto st4;
tr17:
#line 182 "cpp/validatecsv.rl"
	{
    ctx.state = 12;
    ctx.errorUnexpectedChar(*p);
    {p++; cs = 4; goto _out;}
}
#line 218 "cpp/validatecsv.rl"
	{te = p+1;{ /* stay in error */ }}
	goto st4;
tr18:
#line 178 "cpp/validatecsv.rl"
	{ ctx.state = 8; ++ctx.fields; }
#line 217 "cpp/validatecsv.rl"
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
#line 555 "cpp/validatecsv.cpp"
	switch( (*p) ) {
		case 10u: goto tr15;
		case 13u: goto tr16;
		case 34u: goto tr17;
		case 44u: goto tr18;
	}
	goto tr14;
tr19:
#line 176 "cpp/validatecsv.rl"
	{ ctx.state = 6; }
#line 227 "cpp/validatecsv.rl"
	{te = p+1;{ {goto st4;} }}
	goto st5;
tr20:
#line 181 "cpp/validatecsv.rl"
	{ ctx.state = 11; ++ctx.fields; ctx.finalizeRecord(); }
#line 223 "cpp/validatecsv.rl"
	{te = p+1;{ {goto st0;} }}
	goto st5;
tr21:
#line 179 "cpp/validatecsv.rl"
	{ ctx.state = 9; ++ctx.fields; }
#line 224 "cpp/validatecsv.rl"
	{te = p+1;{ {goto st8;} }}
	goto st5;
tr22:
#line 171 "cpp/validatecsv.rl"
	{ ctx.state = 1; }
#line 226 "cpp/validatecsv.rl"
	{te = p+1;{ {goto st1;} }}
	goto st5;
tr23:
#line 178 "cpp/validatecsv.rl"
	{ ctx.state = 8; ++ctx.fields; }
#line 225 "cpp/validatecsv.rl"
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
#line 588 "cpp/validatecsv.cpp"
	switch( (*p) ) {
		case 10u: goto tr20;
		case 13u: goto tr21;
		case 34u: goto tr22;
		case 44u: goto tr23;
	}
	goto tr19;
tr24:
#line 182 "cpp/validatecsv.rl"
	{
    ctx.state = 12;
    ctx.errorUnexpectedChar(*p);
    {p++; cs = 6; goto _out;}
}
#line 232 "cpp/validatecsv.rl"
	{te = p+1;{ /* stay in error */ }}
	goto st6;
tr25:
#line 180 "cpp/validatecsv.rl"
	{ ctx.state = 10; ctx.finalizeRecord(); }
#line 231 "cpp/validatecsv.rl"
	{te = p+1;{ {goto st0;} }}
#line 182 "cpp/validatecsv.rl"
	{
    ctx.state = 12;
    ctx.errorUnexpectedChar(*p);
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
#line 618 "cpp/validatecsv.cpp"
	if ( (*p) == 10u )
		goto tr25;
	goto tr24;
tr26:
#line 182 "cpp/validatecsv.rl"
	{
    ctx.state = 12;
    ctx.errorUnexpectedChar(*p);
    {p++; cs = 7; goto _out;}
}
#line 237 "cpp/validatecsv.rl"
	{te = p+1;{ /* stay in error */ }}
	goto st7;
tr27:
#line 181 "cpp/validatecsv.rl"
	{ ctx.state = 11; ++ctx.fields; ctx.finalizeRecord(); }
#line 236 "cpp/validatecsv.rl"
	{te = p+1;{ {goto st0;} }}
#line 182 "cpp/validatecsv.rl"
	{
    ctx.state = 12;
    ctx.errorUnexpectedChar(*p);
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
#line 644 "cpp/validatecsv.cpp"
	if ( (*p) == 10u )
		goto tr27;
	goto tr26;
tr28:
#line 182 "cpp/validatecsv.rl"
	{
    ctx.state = 12;
    ctx.errorUnexpectedChar(*p);
    {p++; cs = 8; goto _out;}
}
#line 242 "cpp/validatecsv.rl"
	{te = p+1;{ /* stay in error */ }}
	goto st8;
tr29:
#line 180 "cpp/validatecsv.rl"
	{ ctx.state = 10; ctx.finalizeRecord(); }
#line 241 "cpp/validatecsv.rl"
	{te = p+1;{ {goto st0;} }}
#line 182 "cpp/validatecsv.rl"
	{
    ctx.state = 12;
    ctx.errorUnexpectedChar(*p);
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
#line 670 "cpp/validatecsv.cpp"
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

#line 276 "cpp/validatecsv.rl"

    ctx.finish();
}

static void parseBuffer(const char* data, std::size_t len, bool strict, FileResult& result) {
    ParserContext ctx(result);
    if (strict) {
        runStrict(data, len, ctx);
    } else {
        runLoose(data, len, ctx);
    }
}

}  // namespace ragel_csv

class Validater {
private:
    static std::mutex cout_mutex;
    static bool verbose;
    static bool useMemoryMap;

public:
    static bool ValidateCSV(const std::vector<std::string>& args) {
        bool fStrict = false;
        verbose = false;
        useMemoryMap = true;
        unsigned int maxThreads = std::max(1u, std::thread::hardware_concurrency());
        std::vector<std::string> filenames;

        for (size_t i = 0; i < args.size(); ++i) {
            if (args[i] == "-s" || args[i] == "--strict") {
                fStrict = true;
            } else if (args[i] == "-v" || args[i] == "--verbose") {
                verbose = true;
            } else if (args[i] == "-m" || args[i] == "--memory") {
                useMemoryMap = true;
            } else if (args[i] == "--stream") {
                useMemoryMap = false;
            } else if ((args[i] == "-t" || args[i] == "--threads") && i + 1 < args.size()) {
                maxThreads = static_cast<unsigned int>(std::stoi(args[++i]));
                if (maxThreads == 0) {
                    maxThreads = 1;
                }
            } else if (!args[i].empty() && args[i][0] != '-') {
                filenames.push_back(args[i]);
            }
        }

        if (filenames.empty()) {
            std::cout << "Usage: validatecsv [-s|--strict] [-v|--verbose] [-m|--memory|--stream] "
                         "[-t|--threads N] <filename1> [filename2] ..."
                      << std::endl;
            return false;
        }

        std::vector<std::thread> threads;
        std::vector<FileResult> results(filenames.size());

        size_t batchSize = maxThreads;
        for (size_t start = 0; start < filenames.size(); start += batchSize) {
            size_t end = std::min(start + batchSize, filenames.size());

            threads.clear();
            threads.reserve(end - start);
            for (size_t i = start; i < end; ++i) {
                threads.emplace_back([&, i]() { results[i] = ValidateFile(filenames[i], fStrict); });
            }

            for (auto& thread : threads) {
                thread.join();
            }
        }

        bool isValid = true;
        for (const auto& result : results) {
            if (!result.isValid) {
                isValid = false;
            }
            if (verbose) {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << result.filename << ": File is " << (fStrict ? "strictly" : "loosely")
                          << (result.isValid ? " valid" : " invalid") << " with "
                          << result.nFieldsExpected << " columns and " << result.nLineNumber
                          << " rows." << std::endl;
                if (!result.isValid) {
                    for (const auto& error : result.errors) {
                        std::cout << "  " << error << std::endl;
                    }
                }
            }
        }
        return isValid;
    }

private:
    static FileResult ValidateFile(const std::string& filename, bool fStrict) {
        return useMemoryMap ? ValidateFileMemoryMapped(filename, fStrict)
                            : ValidateFileStream(filename, fStrict);
    }

    static FileResult ValidateFileMemoryMapped(const std::string& filename, bool fStrict) {
        FileResult result{filename, true, 0, 1, {}};

        int fd = open(filename.c_str(), O_RDONLY);
        if (fd == -1) {
            result.isValid = false;
            result.errors.push_back("Unable to open file: " + filename);
            return result;
        }

        struct stat sb {};
        if (fstat(fd, &sb) == -1) {
            close(fd);
            result.isValid = false;
            result.errors.push_back("Unable to get file size: " + filename);
            return result;
        }

        if (sb.st_size == 0) {
            close(fd);
            return result;
        }

        char* addr = static_cast<char*>(mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0));
        if (addr == MAP_FAILED) {
            close(fd);
            result.isValid = false;
            result.errors.push_back("Unable to memory map file: " + filename);
            return result;
        }

        ragel_csv::parseBuffer(addr, static_cast<std::size_t>(sb.st_size), fStrict, result);

        munmap(addr, sb.st_size);
        close(fd);

        return result;
    }

    static FileResult ValidateFileStream(const std::string& filename, bool fStrict) {
        FileResult result{filename, true, 0, 1, {}};

        FILE* file = fopen(filename.c_str(), "rb");
        if (!file) {
            result.isValid = false;
            result.errors.push_back("Unable to open file: " + filename);
            return result;
        }

        std::vector<char> buffer;
        char chunk[64 * 1024];
        size_t bytesRead = 0;
        while ((bytesRead = fread(chunk, 1, sizeof(chunk), file)) > 0) {
            buffer.insert(buffer.end(), chunk, chunk + bytesRead);
        }
        fclose(file);

        ragel_csv::parseBuffer(buffer.data(), buffer.size(), fStrict, result);
        return result;
    }
};

std::mutex Validater::cout_mutex;
bool Validater::verbose = false;
bool Validater::useMemoryMap = true;

int main(int argc, char* argv[]) {
    std::vector<std::string> args(argv + 1, argv + argc);
    return Validater::ValidateCSV(args) ? 0 : 1;
}

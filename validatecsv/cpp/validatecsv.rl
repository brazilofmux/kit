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

%%{
machine csv_loose;
alphtype unsigned char;

action to_f0              { ctx.state = 0; }
action to_f2              { ctx.state = 1; }
action to_q2              { ctx.state = 2; }
action to_cr0             { ctx.state = 3; }
action to_cr1             { ctx.state = 4; }
action to_save_quote      { ctx.state = 5; }
action to_save_byte       { ctx.state = 6; }
action to_save_byte_quote { ctx.state = 7; }
action to_save_field      { ctx.state = 8; ++ctx.fields; }
action to_save_field_cr   { ctx.state = 9; ++ctx.fields; }
action to_save_record     { ctx.state = 10; ctx.finalizeRecord(); }
action to_save_field_rec  { ctx.state = 11; ++ctx.fields; ctx.finalizeRecord(); }
action to_error {
    ctx.state = 12;
    ctx.errorUnexpectedChar(*p);
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
machine csv_strict;
alphtype unsigned char;

action to_f0              { ctx.state = 0; }
action to_f2              { ctx.state = 1; }
action to_q2              { ctx.state = 2; }
action to_cr0             { ctx.state = 3; }
action to_cr1             { ctx.state = 4; }
action to_save_quote      { ctx.state = 5; }
action to_save_byte       { ctx.state = 6; }
action to_save_byte_quote { ctx.state = 7; }
action to_save_field      { ctx.state = 8; ++ctx.fields; }
action to_save_field_cr   { ctx.state = 9; ++ctx.fields; }
action to_save_record     { ctx.state = 10; ctx.finalizeRecord(); }
action to_save_field_rec  { ctx.state = 11; ++ctx.fields; ctx.finalizeRecord(); }
action to_error {
    ctx.state = 12;
    ctx.errorUnexpectedChar(*p);
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

static void runLoose(const char* data, std::size_t len, ParserContext& ctx) {
    const unsigned char* p = reinterpret_cast<const unsigned char*>(data);
    const unsigned char* pe = p + len;
    const unsigned char* eof = pe;
    const unsigned char* ts = nullptr;
    const unsigned char* te = nullptr;
    int act = 0;
    int cs = 0;

    %% machine csv_loose;
    %% write init;
    %% write exec;

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

    %% machine csv_strict;
    %% write init;
    %% write exec;

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

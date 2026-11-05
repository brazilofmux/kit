#include <iostream>
#include <vector>
#include <iomanip>
#include <cstring>
#include <fstream>
#include "Decoder.h"

static const char* VERSION = "1.0.0";

static void printUsage(const char* progName) {
    std::cerr << "QR Tape Decoder v" << VERSION << "\n"
              << "Decodes QR code sequences back to original files.\n\n"
              << "Usage: " << progName << " [options] <output_file> <input_images...>\n\n"
              << "Options:\n"
              << "  -h, --help     Show this help message\n"
              << "  -v, --version  Show version information\n\n"
              << "Arguments:\n"
              << "  output_file    Path for reconstructed file\n"
              << "  input_images   PPM image files containing QR codes\n\n"
              << "Example:\n"
              << "  " << progName << " recovered.dat out_B*.ppm\n";
}

static void printVersion() {
    std::cout << "qrtaperead v" << VERSION << "\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    // Parse options
    int argStart = 1;
    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] != '-') break;
        if (std::strcmp(argv[i], "-h") == 0 || std::strcmp(argv[i], "--help") == 0) {
            printUsage(argv[0]);
            return 0;
        }
        if (std::strcmp(argv[i], "-v") == 0 || std::strcmp(argv[i], "--version") == 0) {
            printVersion();
            return 0;
        }
        argStart = i + 1;
    }

    if (argc - argStart < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string outputPath = argv[argStart];
    Decoder decoder;

    int imageCount = argc - argStart - 1;
    std::cout << "Scanning " << imageCount << " images..." << std::endl;

    int successCount = 0;
    int failCount = 0;
    for (int i = argStart + 1; i < argc; ++i) {
        std::string imgPath = argv[i];
        if (decoder.processImage(imgPath)) {
            std::cout << "  Processed: " << imgPath << std::endl;
            successCount++;
        } else {
            std::cerr << "  Failed or no QR found: " << imgPath << std::endl;
            failCount++;
        }
    }

    auto uuids = decoder.getDetectedUuids();
    if (uuids.empty()) {
        std::cerr << "No QR Tape fragments detected." << std::endl;
        return 1;
    }

    // For the prototype, we just take the first UUID detected
    UUID target = *uuids.begin();

    std::cout << "\n=== Decoding Summary ===" << std::endl;
    std::cout << "Images scanned:  " << imageCount << std::endl;
    std::cout << "QR codes found:  " << successCount << std::endl;
    if (failCount > 0) {
        std::cout << "Failed images:   " << failCount << std::endl;
    }

    std::cout << "Detected UUID:   ";
    for (auto b : target) std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)b;
    std::cout << std::dec << std::endl;

    if (!decoder.isComplete(target)) {
        std::cout << "Status:          Incomplete (attempting FEC recovery)" << std::endl;
    } else {
        std::cout << "Status:          Complete" << std::endl;
    }

    try {
        decoder.finalize(target, outputPath);

        // Get output file size
        std::ifstream outCheck(outputPath, std::ios::binary | std::ios::ate);
        if (outCheck) {
            size_t outputSize = outCheck.tellg();
            std::cout << "Output size:     " << outputSize << " bytes" << std::endl;
        }

        std::cout << "\nSuccessfully reconstructed file: " << outputPath << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Finalization failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
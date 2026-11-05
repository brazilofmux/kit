#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <qrencode.h>
#include "Encoder.h"

static const char* VERSION = "1.0.0";

static void printUsage(const char* progName) {
    std::cerr << "QR Tape Encoder v" << VERSION << "\n"
              << "Encodes files into QR code sequences with FEC protection.\n\n"
              << "Usage: " << progName << " [options] <input_file>\n\n"
              << "Options:\n"
              << "  -h, --help     Show this help message\n"
              << "  -v, --version  Show version information\n\n"
              << "Output:\n"
              << "  Creates PPM files named out_B{block}_F{frag}.ppm in current directory.\n\n"
              << "Example:\n"
              << "  " << progName << " myfile.dat\n";
}

static void printVersion() {
    std::cout << "qrtapemake v" << VERSION << "\n";
}

void writePPM(const std::string& filename, const QRcode* qr, int scale = 4, int margin = 4) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Could not open " << filename << " for writing." << std::endl;
        return;
    }

    int realWidth = qr->width * scale + 2 * margin * scale;
    
    // PPM Header P6
    file << "P6\n" << realWidth << " " << realWidth << "\n255\n";

    for (int y = 0; y < realWidth; ++y) {
        for (int x = 0; x < realWidth; ++x) {
            // Determine coordinate in QR module space
            int qrX = (x / scale) - margin;
            int qrY = (y / scale) - margin;

            unsigned char r = 255, g = 255, b = 255; // White background

            if (qrX >= 0 && qrX < qr->width && qrY >= 0 && qrY < qr->width) {
                if (qr->data[qrY * qr->width + qrX] & 1) {
                    r = 0; g = 0; b = 0; // Black module
                }
            }

            file.put(r).put(g).put(b);
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    // Parse options
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-h") == 0 || std::strcmp(argv[i], "--help") == 0) {
            printUsage(argv[0]);
            return 0;
        }
        if (std::strcmp(argv[i], "-v") == 0 || std::strcmp(argv[i], "--version") == 0) {
            printVersion();
            return 0;
        }
    }

    std::string inputFile = argv[1];

    // Get input file size for statistics
    std::ifstream sizeCheck(inputFile, std::ios::binary | std::ios::ate);
    if (!sizeCheck) {
        std::cerr << "Error: Cannot open input file: " << inputFile << std::endl;
        return 1;
    }
    size_t inputSize = sizeCheck.tellg();
    sizeCheck.close();

    std::cout << "Processing: " << inputFile << " (" << inputSize << " bytes)" << std::endl;

    try {
        Encoder encoder(inputFile);

        // Statistics tracking
        size_t totalBlocks = 0;
        size_t totalFragments = 0;
        size_t totalQrBytes = 0;

        encoder.process([&](const std::vector<uint8_t>& qrData, size_t blockIdx, size_t fragIdx) {
            // Encode to QR
            QRcode* qr = QRcode_encodeData(qrData.size(), qrData.data(), 0, QR_ECLEVEL_L);
            if (!qr) {
                std::cerr << "Failed to encode QR for B" << blockIdx << "F" << fragIdx << std::endl;
                return;
            }

            // Filename: out_B{0000}_F{00}.ppm
            std::ostringstream ss;
            ss << "out_B" << std::setfill('0') << std::setw(4) << blockIdx
               << "_F" << std::setw(2) << fragIdx << ".ppm";

            std::cout << "Generated " << ss.str() << " (Ver " << qr->version << ", " << qr->width << "x" << qr->width << ")" << std::endl;

            writePPM(ss.str(), qr);
            QRcode_free(qr);

            // Update statistics
            if (blockIdx + 1 > totalBlocks) totalBlocks = blockIdx + 1;
            totalFragments++;
            totalQrBytes += qrData.size();
        });

        // Print summary statistics
        std::cout << "\n=== Encoding Summary ===" << std::endl;
        std::cout << "Input size:      " << inputSize << " bytes" << std::endl;
        std::cout << "Data blocks:     " << totalBlocks << std::endl;
        std::cout << "Total fragments: " << totalFragments << " (including EOF)" << std::endl;
        std::cout << "QR payload:      " << totalQrBytes << " bytes" << std::endl;

        double overhead = ((double)totalQrBytes / inputSize - 1.0) * 100.0;
        std::cout << "Overhead:        " << std::fixed << std::setprecision(1) << overhead << "% (headers + FEC + compression)" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
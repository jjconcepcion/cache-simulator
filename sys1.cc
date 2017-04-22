#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <unistd.h>

class AccessDetail {
public:
    uint32_t instrAddress;
    uint32_t memAddress;
    uint32_t numOfBytes;
    bool memRead;
    bool memWrite;
};

void usage(char *baseName) {
    std::cerr << "Usage: " << baseName << " tracefile " << "cachesize ";
    std::cerr << "[-v ic1 ic2]" << std::endl;
}

void parseLine(const std::string &line, AccessDetail &access) {
    char accessType;

    std::stringstream sstream(line);
    sstream >> std::hex >> access.instrAddress;
    sstream.ignore(1, ':');
    sstream >> accessType;
    sstream >> std::hex >> access.memAddress;
    sstream >> std::dec >> access.numOfBytes;

    access.memRead = (accessType == 'R');
    access.memWrite = (accessType == 'W');
}

void simulate(const char *traceFilePath) {
    std::ifstream traceFile;
    AccessDetail access;
    std::string line;

    traceFile.open(traceFilePath, std::ifstream::in);
    if (traceFile.fail()) {
        std::cerr << "Error: failed to open: " << traceFilePath << std::endl;
        exit(1);
    }

    while (std::getline(traceFile, line)) {
        parseLine(line, access);
    }
}

int main(int argc, char *argv[]) {
    char *traceFilePath;
    bool vflag = false; // verbose mode flag
    int cacheSize;
    int ic1, ic2;
    int option;

    while ((option = getopt(argc, argv, "v")) != -1) {
        switch (option) {
        case 'v':
            vflag = true;
            break;
        default:
            usage(argv[0]);
            exit(1);
        }
    }

    if ((!vflag && argc != optind + 2) || (vflag && argc != optind + 4)) {
        std::cerr << "Incorrect number of arguments" << std::endl;
        usage(argv[0]);
        exit(1);
    }

    traceFilePath = argv[optind];
    cacheSize = atoi(argv[optind + 1]);

    if (vflag) {
        ic1 = atoi(argv[optind + 2]);
        ic2 = atoi(argv[optind + 3]);
    }

    simulate(traceFilePath);

    return 0;
}
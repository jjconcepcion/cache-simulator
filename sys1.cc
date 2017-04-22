#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <unistd.h>

class AccessDetail {
private:
    const static char READ = 'R';
    const static char WRITE = 'W';

    static uint32_t &accessOrder() {
        static uint32_t accessOrderCounter = 0;
        return accessOrderCounter;
    }

public:
    uint32_t order;
    uint32_t instrAddress;
    uint32_t memAddress;
    uint32_t numOfBytes;
    char accessType;
    bool memRead;
    bool memWrite;

    void parse(const std::string &line) {
        std::stringstream sstream(line);
        sstream >> std::hex >> this->instrAddress;
        sstream.ignore(1, ':');
        sstream >> this->accessType;
        sstream >> std::hex >> this->memAddress;
        sstream >> std::dec >> this->numOfBytes;

        this->memRead = (this->accessType == READ);
        this->memWrite = (this->accessType == WRITE);
        this->order = AccessDetail::accessOrder();
        AccessDetail::accessOrder()++;
    }
};

void usage(char *baseName) {
    std::cerr << "Usage: " << baseName << " tracefile " << "cachesize ";
    std::cerr << "[-v ic1 ic2]" << std::endl;
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
        access.parse(line);
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
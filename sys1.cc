#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <unistd.h>

const int DEFAULT_BLOCK_SIZE = 16;

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

class CacheSlot {
public:
    int valid;
    int dirty;
    uint32_t tag;
};

class CacheStatistics {
public:
    uint32_t reads;
    uint32_t writes;
    uint32_t readMisses;
    uint32_t writeMisses;
    uint32_t dirtyReadMisses;
    uint32_t dirtyWriteMisses;
    uint32_t bytesRead;
    uint32_t bytesWritten;
    uint32_t readAccessTime;
    uint32_t writeAccessTime;

    CacheStatistics() : reads(0),
                        writes(0),
                        readMisses(0),
                        writeMisses(0),
                        dirtyReadMisses(0),
                        dirtyWriteMisses(0),
                        bytesRead(0),
                        bytesWritten(0),
                        readAccessTime(0),
                        writeAccessTime(0) {}
};

class Cache {
private:
    CacheSlot *slots = nullptr;
    CacheStatistics stats;
    int mSize;
    int mBlockSize;
    int numBlocks;
public:
    const int MISS_PENALTY = 80;
    const int SIZE_FACTOR = 1024;

    Cache(int cacheSize, int blockSize) {
        this->mSize = cacheSize * Cache::SIZE_FACTOR;
        this->mBlockSize = blockSize;
        this->numBlocks = this->mSize / this->mBlockSize;
        this->slots = new CacheSlot[this->numBlocks];
    }

    ~Cache() {
        delete [] this->slots;
    }

    const CacheSlot &operator[](int index) {
        return this->slots[index];
    }

    void summary() {
        int totalAccesses = stats.reads + stats.writes;
        int totalMisses = stats.readMisses + stats.writeMisses;
        int readTime = 0;
        int writeTime = 0;
        int totalTime = readTime + writeTime;
        double missRate = static_cast<double>(totalMisses) / totalAccesses;

        std::cout << "loads " << stats.reads
            << " stores " << stats.writes
            << " total " << totalAccesses << "\n" 
            << "rmiss " << stats.readMisses
            << " wmiss " << stats.writeMisses
            << " total " << totalMisses << "\n"
            << "dirty rmiss " << stats.dirtyReadMisses
            << " dirty wmiss " << stats.dirtyWriteMisses << "\n"
            << "bytes read " << stats.bytesRead
            << " bytes written " << stats.bytesWritten << "\n"
            << "read time " << readTime
            << " write time " << writeTime
            << " total time " << totalTime << "\n"
            << "miss rate " << missRate << std::endl;
    }

    int size() {
        return this->mSize;
    }

    int blockSize() {
        return this->mBlockSize;
    }
};


void usage(char *baseName) {
    std::cerr << "Usage: " << baseName << " tracefile " << "cachesize ";
    std::cerr << "[-v ic1 ic2]" << std::endl;
}

void simulate(const char *traceFilePath, Cache &cache) {
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

    cache.summary();
}

int main(int argc, char *argv[]) {
    char *traceFilePath;
    bool vflag = false; // verbose mode flag
    int blockSize = DEFAULT_BLOCK_SIZE;
    int cacheSize, numBlocks;
    Cache *cache = nullptr;
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
    numBlocks = cacheSize*1024/blockSize;
    cache = new Cache(cacheSize, blockSize);

    if (vflag) {
        ic1 = atoi(argv[optind + 2]);
        ic2 = atoi(argv[optind + 3]);
    }

    simulate(traceFilePath, *cache);

    delete cache;

    return 0;
}
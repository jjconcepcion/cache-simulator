#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <cmath>
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
    uint32_t index;
    uint32_t tag;
    char accessType;
    bool memRead;
    bool memWrite;
    bool hit;
    std::string caseNum;

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

    void calculations(int cacheSize, int blockSize) {
        int offsetBits = log2(blockSize);
        int indexBits = log2(cacheSize/blockSize);
        int lowOrderBits = indexBits + offsetBits;

        this->tag = memAddress >> lowOrderBits;
        this->index = (memAddress << (32 - lowOrderBits)) >> (32 -indexBits);
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

        std::cout << "direct-mapped, writeback, size = "
            << mSize/SIZE_FACTOR << "KB" << "\n"
            << "loads " << stats.reads
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

class VerboseOption {
public:
    bool flag;
    uint32_t ic1;
    uint32_t ic2;
};

void usage(char *baseName) {
    std::cerr << "Usage: " << baseName << " tracefile " << "cachesize ";
    std::cerr << "[-v ic1 ic2]" << std::endl;
}

void printVerboseMsg(AccessDetail &access, Cache &cache) {
    std::cout << access.order << " ";
    std::cout.setf(std::ios::hex, std::ios::basefield);
    std::cout << access.index << " "
        << cache[access.index].valid << " "
        << cache[access.index].tag << " "
        << cache[access.index].dirty << " "
        << access.hit << " ";
    std::cout.unsetf(std::ios::hex);
    std::cout << access.caseNum << std::endl;
}

void simulate(const char *traceFilePath, Cache &cache, VerboseOption &verbose) {
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
        access.calculations(cache.size(), cache.blockSize());
        if (verbose.flag && access.order >= verbose.ic1 
                && access.order <= verbose.ic2) {
            printVerboseMsg(access, cache);
        }
    }

    cache.summary();
}



int main(int argc, char *argv[]) {
    char *traceFilePath;
    VerboseOption verbose = {false,0,0}; 
    int blockSize = DEFAULT_BLOCK_SIZE;
    int cacheSize;
    Cache *cache = nullptr;
    int option;

    while ((option = getopt(argc, argv, "v")) != -1) {
        switch (option) {
        case 'v':
            verbose.flag = true;
            break;
        default:
            usage(argv[0]);
            exit(1);
        }
    }

    if ((!verbose.flag && argc != optind + 2) ||
            (verbose.flag && argc != optind + 4)) {
        std::cerr << "Incorrect number of arguments" << std::endl;
        usage(argv[0]);
        exit(1);
    }

    traceFilePath = argv[optind];
    cacheSize = atoi(argv[optind + 1]);
    cache = new Cache(cacheSize, blockSize);

    if (verbose.flag) {
        verbose.ic1 = atoi(argv[optind + 2]);
        verbose.ic2 = atoi(argv[optind + 3]);
    }

    simulate(traceFilePath, *cache, verbose);

    delete cache;

    return 0;
}
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <cmath>
#include <unistd.h>

const int DEFAULT_BLOCK_SIZE = 16;

class CacheSlot {
public:
    int valid;
    int dirty;
    uint32_t tag;
};

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
    CacheSlot prevState;

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
        int indexBits = ceil(log2(cacheSize/blockSize));
        int lowOrderBits = indexBits + offsetBits;

        this->tag = memAddress >> lowOrderBits;
        this->index = (memAddress << (32 - lowOrderBits)) >> (32 -indexBits);
    }
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
    CacheSlot *slots;
    CacheStatistics stats;
    int mSize;
    int mBlockSize;
    int numBlocks;
public:
    const static int MISS_PENALTY = 80;
    const static int SIZE_FACTOR = 1024;

    Cache(int cacheSize, int blockSize) {
        this->mSize = cacheSize * Cache::SIZE_FACTOR;
        this->mBlockSize = blockSize;
        int logVal = ceil(log2(this->mSize/this->mBlockSize));
        this->numBlocks = (1 << logVal);
        this->slots = new CacheSlot[this->numBlocks];
    }

    ~Cache() {
        delete [] this->slots;
    }

    const CacheSlot &operator[](int index) {
        return this->slots[index];
    }

    int size() {
        return this->mSize;
    }

    int blockSize() {
        return this->mBlockSize;
    }

    void summary();
    void evaluate(AccessDetail &access);
};


void Cache::summary() {
    int totalAccesses = stats.reads + stats.writes;
    int totalMisses = stats.readMisses + stats.writeMisses;
    int readTime = stats.readAccessTime;
    int writeTime = stats.writeAccessTime;
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
        << " write time " << writeTime << "\n"
        << "total time " << totalTime << "\n"
        << "miss rate " << missRate << std::endl;
}

void Cache::evaluate(AccessDetail &access) {
    CacheSlot &entry = slots[access.index];
    uint32_t cycles;

    // needed for verbose messages
    access.hit = entry.valid && entry.tag == access.tag;
    access.prevState.valid = entry.valid;
    access.prevState.dirty = entry.dirty;
    access.prevState.tag = entry.tag;

    if (access.memRead) {
        stats.reads++;
    } else {
        stats.writes++;
    }
    
    // Case1: cache hit, 
    if (access.hit) {
        cycles = 1;
        if (access.memRead)  {
            stats.readAccessTime += cycles;
        } else {
            entry.dirty = 1;
            stats.writeAccessTime += cycles;
        }
        access.caseNum = "1";
    } else {
        //Case2a: clean cache miss, move block from memory into index
        if (!entry.dirty) {
            entry.valid = 1;
            entry.tag = access.tag;
            cycles = 1 + Cache::MISS_PENALTY;
            if (access.memRead) {
                entry.dirty = 0;
                stats.readMisses++;
                stats.readAccessTime += cycles;
                stats.bytesRead += Cache::mBlockSize;
            } else {
                entry.dirty = 1;
                stats.writeMisses++;
                stats.bytesRead += Cache::mBlockSize;
                stats.writeAccessTime += cycles;
            }
            access.caseNum = "2a";
        } else {
        //Case2b: dirty cache miss, write cache block to memory, move new
        //block into index
            entry.valid = 1;
            entry.tag = access.tag;
            cycles = 1 + 2*Cache::MISS_PENALTY;
            if (access.memRead) {
                entry.dirty = 0;
                stats.readMisses++;
                stats.dirtyReadMisses++;
                stats.readAccessTime += cycles;
                stats.bytesWritten += Cache::mBlockSize;
                stats.bytesRead += Cache::mBlockSize;
            } else {
                entry.dirty = 1;
                stats.writeMisses++;
                stats.dirtyWriteMisses++;
                stats.writeAccessTime += cycles;
                stats.bytesWritten += Cache::mBlockSize;
                stats.bytesRead += Cache::mBlockSize;
            }
            access.caseNum = "2b";
        }
    }
}

class VerboseOption {
public:
    bool flag;
    uint32_t ic1;
    uint32_t ic2;
};

void usage(char *baseName) {
    std::cerr << "Usage: " << baseName << " tracefile " << "cachesize ";
    std::cerr << "set associativity [-v ic1 ic2]" << std::endl;
}

void printVerboseMsg(AccessDetail &access) {
    std::cout << access.order << " ";
    std::cout.setf(std::ios::hex, std::ios::basefield);
    std::cout << access.index << " "
        << access.tag << " "
        << access.prevState.valid << " "
        << (access.prevState.valid ? access.prevState.tag : 0) << " "
        << access.prevState.dirty << " "
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
        cache.evaluate(access);

        if (verbose.flag && access.order >= verbose.ic1 
                && access.order <= verbose.ic2) {
            printVerboseMsg(access);
        }
    }

    cache.summary();
}

int main(int argc, char *argv[]) {
    char *traceFilePath;
    VerboseOption verbose = {false,0,0}; 
    int blockSize = DEFAULT_BLOCK_SIZE;
    int cacheSize;
    int option;
    int associativity;

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

    if ((!verbose.flag && argc != optind + 3) ||
            (verbose.flag && argc != optind + 5)) {
        std::cerr << "Incorrect number of arguments" << std::endl;
        usage(argv[0]);
        exit(1);
    }

    traceFilePath = argv[optind];
    cacheSize = atoi(argv[optind + 1]);
    associativity = atoi(argv[optind + 2]);

    if (verbose.flag) {
        verbose.ic1 = atoi(argv[optind + 3]);
        verbose.ic2 = atoi(argv[optind + 4]);
    }

    Cache cache(cacheSize, blockSize);
    simulate(traceFilePath, cache, verbose);


    return 0;
}

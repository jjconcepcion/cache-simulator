#include <iostream>
#include <cstdlib>
#include <unistd.h>

void usage(char *baseName) {
    std::cerr << "Usage: " << baseName << " tracefile " << "cachesize ";
    std::cerr << "[-v ic1 ic2]" << std::endl;
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

    return 0;
}
#include "convm3u.h"

int main(int argc, char** argv) {

    if (argc != 6) {
        printf("Usage %s <m3u File> <config File> <channels File> <transponder_id> <radio>\n", argv[0]);
        exit(1);
    }

    auto converter = ConvM3U(argv[1], argv[2], argv[3]);
    converter.convert(argv[4], argv[5]);
}
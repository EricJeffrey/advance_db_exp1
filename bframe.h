#if !defined(BFRAME_H)
#define BFRAME_H

#include <cstdio>

const int PAGE_SIZE = 4 * 1024;
const int FRAME_SIZE = PAGE_SIZE;
const int NUM_DATA = 50000;

struct bFrame {
    char field[FRAME_SIZE];
    int size = FRAME_SIZE;
};

// 打印出全局变量
void print_config() {
    printf("FRAME_SIZE = PAGE_SIZE: %d, NUM_DATA = %d\n", PAGE_SIZE, NUM_DATA);
    fflush(stdout);
}

#endif // BFRAME_H

#if !defined(BFRAME_H)
#define BFRAME_H

const int PAGE_SIZE = 256;
const int FRAME_SIZE = PAGE_SIZE;

struct bFrame {
    char field[FRAME_SIZE];
    int size = FRAME_SIZE;
};

#endif // BFRAME_H

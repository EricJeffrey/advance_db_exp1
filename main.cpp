/**
 * 高级数据库课程实验 - 实现一个Storage and Buffer Manager
 * EricJeffrey
 * 2019/10/2
*/

#include "buffer_mgr.h"
#include "ds_mgr.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

// 生成一个随机的 bframe，其以 id 的字符串开始
void generate_frame(bFrame &frame, int i) {
    char istr[33] = {};
    snprintf(istr, sizeof(istr), "%d:", i);
    int j;
    for (j = 0; istr[j] != '\0'; j++) 
        frame.field[j] = istr[j];
    for (; j < FRAME_SIZE; j++) {
        int x = rand() % 26;
        int y = rand() % 2;
        frame.field[j] = (y == 1 ? 'a' + x : 'A' + x);
    }
    for (int j = 0; j < 16; j++) {
        frame.field[FRAME_SIZE - j - 1] = ' ';
    }
}

// 测试 dsmgr 的健壮性
void test_ds_mgr_write_new() {
    freopen("./log.out", "w", stderr);
    DataStorageMgr dsmgr = DataStorageMgr();
    srand(time(NULL));
    // 每个记录生成一个随机序列
    const int tmp = NUM_DATA / 50;
    for (int i = 0; i < NUM_DATA; i++) {
        if (tmp > 0 && i % tmp == 0)
            printf("%d\t", (i / tmp) + 1);
        bFrame frame = bFrame();
        generate_frame(frame, i);
        dsmgr.WriteNewPage(frame);
    }
    printf("\ntest_ds_mgr done.\n");
    LOG_DEBUG("test_ds_mgr done.");
}

int main(int argc, char const *argv[]) {
    print_config();
    test_ds_mgr_write_new();
    return 0;
}

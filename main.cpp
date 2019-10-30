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

// 测试 dsmgr 的健壮性
void test_ds_mgr() {
    freopen("./log.out", "w", stderr);
    const int NUM_DATA = 1000;
    DataStorageMgr dsmgr = DataStorageMgr();
    srand(time(NULL));
    for (int i = 0; i < NUM_DATA; i++) {
        const int tmp = NUM_DATA / 10;
        if (tmp > 0 && i % tmp == 0)
            printf("%d,", i / tmp);
        bFrame frame = bFrame();
        for (int j = 0; j < FRAME_SIZE; j++) {
            int x = rand() % 26;
            int y = rand() % 2;
            // frame.field[j] = (y == 1 ? 'a' + x : 'A' + x);
            frame.field[j] = (y == 1 ? 'a' + 1 : 'A' + 1);
        }
        for (int j = 0; j < 16; j++) {
            frame.field[FRAME_SIZE - j - 1] = '$';
        }
        dsmgr.WriteNewPage(frame);
    }
    LOG_DEBUG("test_ds_mgr done.");
}

int main(int argc, char const *argv[]) {
    test_ds_mgr();
    return 0;
}

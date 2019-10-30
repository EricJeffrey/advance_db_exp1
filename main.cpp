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

// 测试 dsmgr 写入新页的正确性
void test_ds_mgr_write_new() {
    DataStorageMgr dsmgr = DataStorageMgr();
    srand(time(NULL));
    // 每个记录生成一个随机序列
    const int tmp = NUM_PAGE_TOTAL / 50;
    for (int i = 0; i < NUM_PAGE_TOTAL; i++) {
        if (tmp > 0 && i % tmp == 0)
            printf("%d\t", (i / tmp) + 1);
        bFrame frame = bFrame();
        generate_frame(frame, i);
        dsmgr.WriteNewPage(frame);
    }
    printf("\ntest_ds_mgr done.\n");
    LOG_DEBUG("test_ds_mgr done.");
}

// 测试 dsmgr 读取页的正确性
void test_ds_mgr_read() {
    // 执行300次随机测试

    bool const create_file = false;
    DataStorageMgr dsmgr(create_file);
    // 检查 page 中内容的id位(前n位)是否与 x 一致
    auto check = [](const char *str, int x) -> bool {
        char xstr[33] = {};
        snprintf(xstr, sizeof(xstr), "%d", x);
        for (int i = 0; xstr[i] != '\0'; i++)
            if (str[i] != xstr[i])
                return false;
        return true;
    };

    const int times = 300;
    srand(time(NULL));
    int ok_cnt = 0;
    for (int i = 0; i < times; i++) {
        int page_id = rand() % NUM_PAGE_TOTAL;
        printf("%d/%d - page_id: %d,\t", i, times, page_id);
        bFrame frame;
        dsmgr.ReadPage(page_id, frame);
        frame.field[10] = '\0';
        printf("data read(first 10bytes): %s\t", frame.field);
        bool res = check(frame.field, page_id);
        printf("\t\tstatus: ");
        if (res) {
            printf("1\n");
            ok_cnt += 1;
        } else {
            printf("0\n");
        }
    }
    printf("ok: %d, failed: %d\n", ok_cnt, times - ok_cnt);
}

// 测试 dsmgr 写入页的正确性，确保写入时页的 id 位不会变
void test_ds_mgr_write() {
    // todo dsmgr写入测试
    // 随机生成300个页id，前200个更新，后100个不变，记录前200个写入的数据与后100个的数据，全部更新完后读取 前200个与记录的相同，后100个不变，
    bool const create_file = false;
    DataStorageMgr dsmgr(create_file);
}

int main(int argc, char const *argv[]) {
    print_config();
    freopen("./log.out", "w", stderr);
    printf("stderr >> log.out\n");

    // test_ds_mgr_write_new();
    // test_ds_mgr_read();
    test_ds_mgr_write();

    return 0;
}

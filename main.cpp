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
#include <set>
#include <string>

using std::set;

extern const int NUM_PAGE_TOTAL;

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
    // 随机生成300个页id，前200个更新，后100个不变，记录前200个写入的数据与后100个的数据，全部更新完后读取 前200个与记录的相同，后100个不变，
    bool const create_file = false;
    DataStorageMgr dsmgr(create_file);

    const int num_test_ids = 300;
    int ids[num_test_ids] = {};

    // 生成 numtot 个不同 page_id
    auto gene_ids = [](int ids[], int num_tot, int mod_num = -1) -> void {
        if (mod_num == -1)
            mod_num = num_tot;
        srand(time(NULL));
        set<int> ids_set;
        for (int i = 0; i < num_tot; i++) {
            int tmp = rand() % mod_num;
            while (ids_set.find(tmp) != ids_set.end())
                tmp = rand() % mod_num;
            ids[i] = tmp;
            ids_set.insert(tmp);
        }
    };
    gene_ids(ids, num_test_ids);

    // 生成的新 page_id
    const int num_test_ids_use = 200;
    int new_datas[num_test_ids_use] = {};
    gene_ids(new_datas, num_test_ids_use, NUM_PAGE_TOTAL);

    // 写入新数据
    for (int i = 0; i < num_test_ids_use; i++) {
        bFrame frame = bFrame();
        char tmp_str[33] = {};
        snprintf(tmp_str, sizeof(tmp_str), "%d:", new_datas[i]);
        for (int j = 0; tmp_str[j] != '\0'; j++)
            frame.field[j] = tmp_str[j];
        dsmgr.WritePage(ids[i], frame);
    }

    // 检查 str 的id位(前n位)是否与 x 一致
    auto check = [](const char *str, int x) -> bool {
        char xstr[33] = {};
        snprintf(xstr, sizeof(xstr), "%d", x);
        for (int i = 0; xstr[i] != '\0'; i++)
            if (str[i] != xstr[i])
                return false;
        return true;
    };

    // 读取数据
    int ok_cnt = 0;
    for (int i = 0; i < num_test_ids; i++) {
        int target_id = ids[i];
        int target_res_id = (i < 200 ? new_datas[i] : ids[i]);
        printf("No.%d\t,\tid:\t%d,\tnew_id:\t%d,\t", i + 1, target_id, target_res_id);
        bFrame frame;
        dsmgr.ReadPage(target_id, frame);
        frame.field[10] = '\0';
        printf("read_page(first 10bytes):\t%s\tstatus:\t", frame.field);
        if (check(frame.field, target_res_id)) {
            printf("1\n");
            ok_cnt += 1;
        } else {
            printf("0\n");
        }
    }
    printf("%d success, %d failed.\n", ok_cnt, num_test_ids - ok_cnt);
    for (int i = 0; i < num_test_ids; i++) {
        bFrame frame = bFrame();
        char tmp_str[33] = {};
        snprintf(tmp_str, sizeof(tmp_str), "%d:", ids[i]);
        for (int j = 0; tmp_str[j] != '\0'; j++)
            frame.field[j] = tmp_str[j];
        dsmgr.WritePage(ids[i], frame);
    }
}

int main(int argc, char const *argv[]) {
    print_config();
    printf("stderr >> log.out\n");
    printf("stdout >> data.out\n");

    freopen("./log.out", "w", stderr);

    // test_ds_mgr_write_new();
    // test_ds_mgr_read();
    freopen("./data.out", "a+", stdout);
    test_ds_mgr_write();

    return 0;
}

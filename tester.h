#if !defined(TESTER_H)
#define TESTER_H

#include "buffer_mgr.h"
#include <set>
using std::set;

class Tester {
private:
    // checker
    bool (*check)(const char *str, int x) = [](const char *str, int x) -> bool {
        char xstr[33] = {};
        snprintf(xstr, sizeof(xstr), "%d", x);
        for (int i = 0; xstr[i] != '\0'; i++)
            if (str[i] != xstr[i])
                return false;
        return true;
    };
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
        printf("total io: %d\n", dsmgr.GetTotalIO());
        for (int i = 0; i < num_test_ids; i++) {
            bFrame frame = bFrame();
            char tmp_str[33] = {};
            snprintf(tmp_str, sizeof(tmp_str), "%d:", ids[i]);
            for (int j = 0; tmp_str[j] != '\0'; j++)
                frame.field[j] = tmp_str[j];
            dsmgr.WritePage(ids[i], frame);
        }
        printf("total io(after rewrite): %d\n", dsmgr.GetTotalIO());
    }

    // 测试 buffermgr 读取数据的正确性
    void test_buf_mgr_fix() {
        // 使用 buffermgr 的 fixpage 读取 page
        // 随机生成300个id，调用 fixpage 读取 frameid，检查得到的 id 正确性

        LOG_DEBUG("test_buf_mgr_read");
        const int num_ids = 300;
        int ids[num_ids] = {};
        auto gene_ids = [](int ids[], int num_ids) -> void {
            srand(time(NULL));
            for (int i = 0; i < num_ids; i++)
                ids[i] = rand() % NUM_PAGE_TOTAL;
        };
        gene_ids(ids, num_ids);

        DataStorageMgr dsmgr = DataStorageMgr(false);
        BufferMgr buffermgr = BufferMgr(&dsmgr);
        int ok_cnt = 0;
        for (int i = 0; i < num_ids; i++) {
            bFrame *res_frame = buffermgr.GetFrame(buffermgr.FixPage(ids[i], 0));
            res_frame->field[10] = '\0';
            printf("No.%d,\tpage_id: %d,\tframe_read: %s,\tstatus: ", i + 1, ids[i], res_frame->field);
            if (check(res_frame->field, ids[i])) {
                ok_cnt += 1;
                printf("1\n");
            } else {
                printf("0\n");
            }
        }
        printf("%d success, %d failed\n", ok_cnt, num_ids - ok_cnt);
    }

    // 测试 buffermgr 更新数据的正确性
    void test_buf_mgr_update() {
        // 随机生成 20 个 pageid，每个出现四次并打乱
        // 读取当前的LRU head head->next head->next->next tail tail->prev tail->prev->prev 信息
        // 依次更新 pageid 的内容，更新后输出同样的信息
        const int num_ids = 20;
        const int num_ids_rep = 4;
        using std::vector;
        vector<int> ids(num_ids_rep * num_ids, 0);
        srand(time(NULL));
        for (int i = 0; i < num_ids; i++) {
            const int tmp = rand() % NUM_PAGE_TOTAL;
            for (int j = 0; j < 4; j++)
                ids[i * num_ids_rep + j] = tmp;
        }
        random_shuffle(ids.begin(), ids.end());

        DataStorageMgr dsmgr = DataStorageMgr(false);
        BufferMgr bufmgr = BufferMgr(&dsmgr);
        for (int i = 0; i < num_ids_rep * num_ids; i++) {
            printf("No.%d,\tpage_id: %d,\tLRUInfo: %s\n", i + 1, ids[i], bufmgr.GetLRUInfo().c_str());
            bFrame frame = bFrame();
            char tmp[39] = {};
            snprintf(tmp, sizeof(tmp), "%d:~~~~~", ids[i]);
            for (int j = 0; tmp[j] != '\0'; j++)
                frame.field[j] = tmp[j];
            bufmgr.UpdatePage(ids[i], frame);
        }
    }

public:
    Tester() {}
    ~Tester() {}
    void do_test() {
        // write test here
        test_buf_mgr_update();
    }
};

#endif // TESTER_H

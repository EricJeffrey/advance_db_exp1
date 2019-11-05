/**
 * 高级数据库课程实验 - 实现一个Storage and Buffer Manager
 * EricJeffrey
 * 2019/10/2
*/

#include "buffer_mgr.h"
#include "tester.h"
#include <random>

using std::make_pair;
using std::pair;
using std::random_shuffle;
using std::set;
using std::stoi;
using std::vector;

extern const int NUM_PAGE_TOTAL;
extern const int PAGE_SIZE;
extern const int FRAME_SIZE;
extern bool LOG_DEBUG_ON;

auto ha = [](const char *str) -> void {
    printf("%s\n", str);
};

// 打印初始化配置信息，重定向IO
void init() {
    LOG_DEBUG_ON = true;
    printf("FRAME_SIZE = PAGE_SIZE: %d, NUM_DATA = %d\n", PAGE_SIZE, NUM_PAGE_TOTAL);
    printf("stderr >> log.out\n");
    printf("stdout >> data.out\n");
    freopen("./log.out", "w", stderr);
    freopen("./data.out", "w", stdout);
    fflush(stdout);
}

// 初始化所有记录
void ds_init() {
    const bool create_file = true;
    DataStorageMgr dsmgr = DataStorageMgr(create_file);
    BufferMgr buffmgr(&dsmgr);
    bFrame tmpframe;
    ha("Start Writing Data Into Pages");
    for (int i = 0; i < NUM_PAGE_TOTAL; i++) {
        memset(tmpframe.field, 0, sizeof(tmpframe.field));
        for (int j = 0; j < FRAME_SIZE; j++)
            tmpframe.field[j] = 'a' + (j % 26);
        buffmgr.FixNewPage(tmpframe);
    }
    ha("Data Written, Start Job");
}
// 执行任务
void start() {
    const bool create_file = false;
    DataStorageMgr dsmgr = DataStorageMgr(create_file);
    BufferMgr buffermgr(&dsmgr);
    ha("Start Reading Command Data");
    // 读取数据到内存
    FILE *in_fp = fopen("./data.in", "r+");
    if (in_fp == nullptr)
        FAIL;
    vector<pair<int, int>> cmds;
    while (true) {
        char line[12] = {};
        fgets(line, sizeof(line), in_fp);
        if (feof(in_fp))
            break;
        line[1] = '\0';
        auto tmp_data = make_pair(stoi(line), stoi(line + 2));
        cmds.push_back(tmp_data);
        // printf("%d,%d\n", tmp_data.first, tmp_data.second);
    }
    fclose(in_fp);
    ha("Command Data Read, Start Processing");
    // 处理读写命令，每次都输出命中率
    bFrame tmpframe;
    for (size_t i = 0; i < cmds.size(); i++) {
        memset(tmpframe.field, 0, sizeof(tmpframe.field));
        int tmpcmd = cmds[i].first;
        int tmppageid = cmds[i].second;
        // 读
        if (tmpcmd == 0)
            buffermgr.FixPage(tmppageid, 0);
        // 写
        if (tmpcmd == 1) {
            for (int j = 0; j < 5; j++)
                tmpframe.field[j] = '|';
            buffermgr.UpdatePage(tmppageid, tmpframe);
        }
        printf("%.6f,%ld\n", buffermgr.GetHitRate(), buffermgr.GetIONumTot());
    }
    buffermgr.WriteDirtys();
    ha("Processing Over, Job Done!");
}

int main(int argc, char const *argv[]) {
    init();
    bool do_test = false;
    if (do_test) {
        Tester().do_test();
    } else {
        // 写入数据
        ds_init();
        // 执行任务
        start();
    }

    return 0;
}

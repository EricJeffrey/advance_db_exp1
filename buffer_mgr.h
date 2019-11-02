#if !defined(BUFFER_MGR_H)
#define BUFFER_MGR_H

#include "ds_mgr.h"
#include <algorithm>
#include <cstring>
#include <string>
#include <time.h>
#include <unordered_map>
#include <vector>

using std::string;
using std::to_string;
using std::unordered_map;

#define BUFFER_MAX_SIZE 1024

class BufferMgr {
private:
    unordered_map<int, int> page2frame;
    int frame2page[BUFFER_MAX_SIZE];

    BCB bcbs[BUFFER_MAX_SIZE];

    bFrame *buffer;
    int head_fid, tail_fid, size;

    DataStorageMgr *ds_mgr;

    // 读/写页的操作数
    int tot_page_update_num;
    // 缓存命中次数
    int hit_num;

    // 选择一个受害者
    int SelectVictim() {
        LOG_DEBUG("BufferMgr.SelectVictim, victim", tail_fid);
        return tail_fid;
    }
    void SetDirty(int frame_id) {
        LOG_DEBUG("BufferMgr.SetDirty");
        bcbs[frame_id].dirty = true;
    }
    // 将一个dirty的frame写回磁盘
    void WriteFrame(int frame_id) {
        LOG_DEBUG("BufferMgr.WriteFrame, frame_id", frame_id);
        if (ds_mgr->WritePage(frame2page[frame_id], buffer[frame_id]) == -1)
            FAIL;
    }
    // 从磁盘读一个page到bframe
    void ReadPage(int page_id, bFrame &frame) {
        LOG_DEBUG("BufferMgr.ReadPage, page_id", page_id);
        if (ds_mgr->ReadPage(page_id, frame) == -1)
            FAIL;
    }
    // LRU -- 更新了一个缓冲块（可能淘汰了旧的）
    //对于新插入的缓冲块，使用 [LRUInsert]
    void LRUUpdate(int readed_fid) {
        LOG_DEBUG("BufferMgr.LRUUpdate");
        if (tail_fid == readed_fid) { // 选了尾巴
            // 从尾巴处删除
            tail_fid = bcbs[readed_fid].prev_frame_id;
            bcbs[tail_fid].next_frame_id = tail_fid;
        } else { // 更新了中间某个缓冲块
            int prev = bcbs[readed_fid].prev_frame_id;
            int next = bcbs[readed_fid].next_frame_id;
            // 删除它
            bcbs[prev].next_frame_id = next;
            bcbs[next].prev_frame_id = prev;
        }
        // 加到首部
        bcbs[head_fid].prev_frame_id = readed_fid;
        bcbs[readed_fid].next_frame_id = head_fid;
        bcbs[readed_fid].prev_frame_id = readed_fid;
        head_fid = readed_fid;
    }
    // LRU -- 插入了一个新缓冲块
    void LRUInsert(int inserted_fid) {
        LOG_DEBUG("BufferMgr.LRUInsert");
        if (size == 0) { // LRU 链表为空
            head_fid = tail_fid = inserted_fid;
            bcbs[head_fid].next_frame_id = bcbs[head_fid].prev_frame_id = inserted_fid;
        } else { // 链表不为空
            bcbs[head_fid].prev_frame_id = inserted_fid;
            bcbs[inserted_fid].next_frame_id = head_fid;
            bcbs[inserted_fid].prev_frame_id = inserted_fid;
            head_fid = inserted_fid;
        }
    }
    // 命中缓存，加一
    void IncHitNum() {
        hit_num += 1;
    }
    // 执行了依次读/写页
    void IncTotPageUpdateNum() {
        tot_page_update_num += 1;
    }

public:
    BufferMgr(DataStorageMgr *ds_mgr) {
        memset(frame2page, -1, sizeof(frame2page));
        memset(bcbs, -1, sizeof(bcbs));
        head_fid = tail_fid = -1;
        size = 0;
        this->ds_mgr = ds_mgr;
        buffer = new bFrame[BUFFER_MAX_SIZE];

        hit_num = tot_page_update_num = 0;
    }
    ~BufferMgr() {
        LOG_DEBUG("~BufferMgr");
        delete[] buffer;
    }
    // 读取page_id的内容，返回frame_id
    int FixPage(int page_id, int prot) {
        LOG_DEBUG("BufferMgr.FixPage, page_id", page_id);
        IncTotPageUpdateNum();
        if (page2frame.find(page_id) != page2frame.end()) { // 在缓冲区
            printf("HIT!\t");
            IncHitNum();
            return page2frame[page_id];
        }
            printf("\t\t");
        // 不在缓冲区
        int target_fid = size + 1;
        bool do_insert = false; // 确定执行更新还是插入操作

        if (size >= BUFFER_MAX_SIZE) { // 缓冲区满
            target_fid = SelectVictim();
            if (bcbs[target_fid].dirty == 1) { // dirty
                WriteFrame(target_fid);
            }
        } else { // 缓冲区未满
            target_fid = size;
            do_insert = true;
        }

        // 读数据
        bFrame tmp_frame;
        ReadPage(page_id, tmp_frame);
        // 写数据到缓冲区，更新 BCB 和 LRU
        tmp_frame.set_field(buffer[target_fid]);
        bcbs[target_fid].update(page_id, target_fid, 0, false);
        do_insert ? LRUInsert(target_fid) : LRUUpdate(target_fid);
        do_insert ? size += 1 : 0;
        // 更新哈希表
        page2frame[page_id] = target_fid;
        frame2page[target_fid] = page_id;
        return target_fid;
    }
    // 写入一个新记录到磁盘
    void FixNewPage(bFrame &tmp_frame) {
        LOG_DEBUG("BufferMgr.FixNewPage");
        if (ds_mgr->WriteNewPage(tmp_frame) == -1)
            FAIL;
    }
    // 更新某个page
    void UpdatePage(int page_id, bFrame frame) {
        LOG_DEBUG("BufferMgr.UpdatePage, page_id", page_id);
        IncTotPageUpdateNum();
        if (page2frame.find(page_id) != page2frame.end()) { // 在缓冲区
            printf("HIT!\t");
            IncHitNum();
            int tmp_frm_id = page2frame[page_id];
            buffer[tmp_frm_id] = frame;
            SetDirty(tmp_frm_id);
            LRUUpdate(tmp_frm_id);
            return;
        } else { // 不在缓冲区
            printf("\t\t");
            int target_fid = -1, ret = 0;
            bool do_insert = false;
            if (size == BUFFER_MAX_SIZE) { // 缓冲区满
                target_fid = SelectVictim();
                if (bcbs[target_fid].dirty) { // dirty
                    ret = ds_mgr->WritePage(frame2page[target_fid], buffer[target_fid]);
                    if (ret == -1)
                        FAIL;
                }
            } else { // 缓冲区未满
                target_fid = size;
                do_insert = true;
            }
            ret = ds_mgr->ReadPage(page_id, buffer[target_fid]);
            if (ret == -1)
                FAIL;
            buffer[target_fid] = frame;
            bcbs[target_fid].update(page_id, target_fid, 0, false);
            SetDirty(target_fid);
            do_insert ? LRUInsert(target_fid) : LRUUpdate(target_fid);
            do_insert ? size += 1 : 0;
            // 更新哈希表
            page2frame[page_id] = target_fid;
            frame2page[target_fid] = page_id;
        }
    }
    // 获取一个 frame，返回指针
    bFrame *GetFrame(int frame_id) {
        return buffer + frame_id;
    }
    // count减一
    void UnfixPage(int page_id) {
        LOG_DEBUG("BufferMgr.UnfixPage");
        if (page2frame.find(page_id) != page2frame.end())
            bcbs[page2frame[page_id]].count -= 1;
    }
    // 获取命中率
    double GetHitRate() {
        return tot_page_update_num == 0 ? -1 : hit_num / (double)tot_page_update_num;
    }
    // 获取IO总次数
    long long GetIONumTot() {
        return ds_mgr->GetTotalIO();
    }
    // 获取 LRU 链表中头部的三个与尾部的三个 page
    string GetLRUInfo() {
        if (size == 0)
            return string("NULL");
        using std::vector;
        int fid = head_fid;
        vector<int> ids;
        for (int i = 0; i < 3; i++) {
            ids.push_back(bcbs[fid].page_id);
            fid = bcbs[fid].next_frame_id;
        }
        fid = tail_fid;
        for (int i = 0; i < 3; i++) {
            ids.push_back(bcbs[fid].page_id);
            fid = bcbs[fid].prev_frame_id;
        }
        std::reverse(ids.begin() + 3, ids.end());
        string res;
        for (size_t i = 0; i < ids.size(); i++) {
            res += to_string(ids[i]) + "\t";
        }
        return res;
    }
    // 将所有的 dirty缓冲块写会磁盘
    void WriteDirtys() {
        LOG_DEBUG("BufferMgr.WriteDirtys");
        for (int fid = 0; fid < size; fid++) {
            if (bcbs[fid].dirty) {
                int ret = ds_mgr->WritePage(frame2page[fid], buffer[fid]);
                if (ret == -1)
                    FAIL;
                bcbs[fid].dirty = false;
            }
        }
    }
};

#endif // BUFFER_MGR_H

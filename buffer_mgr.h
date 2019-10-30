#if !defined(BUFFER_MGR_H)
#define BUFFER_MGR_H

#include "bframe.h"
#include "ds_mgr.h"
#include "logger.h"
#include <cstring>
#include <time.h>
#include <unordered_map>

using std::unordered_map;

#define BUFFER_MAX_SIZE 1024

struct BCB {
    int page_id;
    int frame_id;
    int count;
    bool dirty;
    int next_frame_id, prev_frame_id;

    BCB() { page_id = frame_id = next_frame_id = prev_frame_id - 1, count = 0, dirty = false; }
    void update(int pid, int fid, int cnt, bool dirty) {
        LOG_DEBUG("BCB.update");
        page_id = pid, frame_id = fid, count = cnt, this->dirty = dirty;
    }
};

class BufferMgr {
private:
    unordered_map<int, int> page2frame;
    int frame2page[BUFFER_MAX_SIZE];

    BCB bcbs[BUFFER_MAX_SIZE];

    bFrame buffer[BUFFER_MAX_SIZE];
    int head_fid, taile_fid, size;

    DataStorageMgr *ds_mgr;

    // Internal functions

    // 选择一个受害者
    int SelectVictim() {
        LOG_DEBUG("BufferMgr.SelectVictim");
        return taile_fid;
    }
    void SetDirty(int frame_id) {
        LOG_DEBUG("BufferMgr.SetDirty");
        bcbs[frame_id].dirty = true;
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
    void PrintFrame(int frame_id) {
        LOG_DEBUG("BufferMgr.PrintFrame");
        printf("PrintFrame-> frame_id: %d, frame: %s\n", frame_id, buffer[frame_id]);
    }
    // LRU -- 更新了一个缓冲块（可能淘汰了旧的）
    //对于新插入的缓冲块，使用 [LRUInsert]
    void LRUUpdate(int readed_fid) {
        LOG_DEBUG("BufferMgr.LRUUpdate");
        if (taile_fid == readed_fid) { // 选了受害者（尾巴）
            // 从尾巴处删除
            taile_fid = bcbs[readed_fid].prev_frame_id;
            bcbs[taile_fid].next_frame_id = taile_fid;
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
            head_fid = taile_fid = inserted_fid;
            bcbs[head_fid].next_frame_id = bcbs[head_fid].prev_frame_id = inserted_fid;
        } else { // 链表不为空
            bcbs[head_fid].prev_frame_id = inserted_fid;
            bcbs[inserted_fid].next_frame_id = head_fid;
            bcbs[inserted_fid].prev_frame_id = inserted_fid;
            head_fid = inserted_fid;
        }
    }

public:
    BufferMgr(DataStorageMgr *ds_mgr) {
        memset(frame2page, -1, sizeof(frame2page));
        memset(bcbs, -1, sizeof(bcbs));
        memset(buffer, 0, sizeof(buffer));
        head_fid = taile_fid = -1;
        size = 0;
        this->ds_mgr = ds_mgr;
    }
    ~BufferMgr() {
    }
    // 读取page_id的内容，返回frame_id
    int FixPage(int page_id, int prot) {
        LOG_DEBUG("BufferMgr.FixPage");
        if (page2frame.find(page_id) != page2frame.end()) { // 在缓冲区
            return page2frame[page_id];
        }
        // 不在缓冲区
        int target_fid = size + 1, ret = 0;
        bool do_insert = false; // 确定执行更新还是插入操作

        if (size >= BUFFER_MAX_SIZE) { // 缓冲区满
            target_fid = SelectVictim();
            if (bcbs[target_fid].dirty == 1) { // dirty
                ret = ds_mgr->WritePage(frame2page[target_fid], buffer[target_fid]);
                if (ret != FRAME_SIZE)
                    FAIL;
            }
        } else { // 缓冲区未满
            target_fid = size;
            do_insert = true;
        }

        // 读数据
        bFrame tmp_frame;
        ret = ds_mgr->ReadPage(page_id, tmp_frame);
        if (ret == -1)
            FAIL;
        // 写数据到缓冲区，更新 BCB 和 LRU
        memcpy(buffer + target_fid, tmp_frame.field, FRAME_SIZE);
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
        if (ds_mgr->WriteNewPage(tmp_frame) != FRAME_SIZE)
            FAIL;
    }

    // 更新某个page
    void UpdatePage(int page_id, bFrame frame) {
        LOG_DEBUG("BufferMgr.UpdatePage");
        if (page2frame.find(page_id) != page2frame.end()) { // 在缓冲区
            int tmp_frm_id = page2frame[page_id];
            buffer[tmp_frm_id] = frame;
            SetDirty(tmp_frm_id);
            LRUUpdate(tmp_frm_id);
            return;
        } else { // 不在缓冲区
            int target_fid = -1, ret = 0;
            bool do_insert = false;
            if (size == BUFFER_MAX_SIZE) { // 缓冲区满
                target_fid = SelectVictim();
                if (bcbs[target_fid].dirty) { // dirty
                    ret = ds_mgr->WritePage(frame2page[target_fid], buffer[target_fid]);
                    if (ret != FRAME_SIZE)
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
            SetDirty(target_fid);
            do_insert ? LRUInsert(target_fid) : LRUUpdate(target_fid);
            do_insert ? size += 1 : 0;
        }
    }

    // count减一
    void UnfixPage(int page_id) {
        LOG_DEBUG("BufferMgr.UnfixPage");
        if (page2frame.find(page_id) != page2frame.end())
            bcbs[page2frame[page_id]].count -= 1;
    }
    int NumFreeFrames() {
        LOG_DEBUG("BufferMgr.NumFreeFrames");
        return BUFFER_MAX_SIZE - size;
    }
};

#endif // BUFFER_MGR_H

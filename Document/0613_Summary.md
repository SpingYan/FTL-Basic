# FTL - Test Workflow

- Initialize FTL: FTL - Declaration and Initialize

- 寫入之前印出所有 Virtual Block 的 Status.

- 目標：寫入 1000 筆資料，每筆 1 個 Byte, 隨機資料內容 0x00 \~ 0xff

   - 根據 OP_Size，限制 Logical Page 寫入的範圍。\
      OP (Over Provisioning)

   - 寫入的 Logical Page 位置隨機產生，範圍限制如上條件。

   - 執行寫入: FTL - Write Data to Flash

   - 讀取資料: FTL - Read Data From Flash

---
tags:
  - ftl
  - write
---
# FTL - Write Data to Flash

- 條件：判斷當下要寫入的 Virtual Block 的 Free Page Count 數量，若已用完則 Action \
   → 選擇/尋找下一個 VB 寫入。

   - 條件：空的 VB 數量若 < 2，則 Action\
      → 執行 GC: FTL - Garbage Collect

   - 若尚有空的 VB 數量 >= 2，則 Action\
      → 將 FreePagesCount 重設\
      → WriteTargetVB 指定下一個 VB

```c
if (FreePagesCount == 0)
{        
    printf("[!!! Warning !!!] Free Pages Count == 0\n");
    FreeVirtualBlockCount--;
    printf("Free Virtual Block now is [%d]\n", FreeVirtualBlockCount);

    if (FreeVirtualBlockCount < 2) {
        printf("Prepare GC Flow, because Free VB < 2.\n");
        garbageCollect();
    }
    else {
        printf("Do not Prepare GC Flow, because Free VB >= 2.\n");
        printf("Initial FreePagesCount and WriteTargetVB++\n");
        FreePagesCount = TOTAL_DIES * PAGES_PER_BLOCK;
        WriteTargetVB ++;
    }
}
```

- 更新 L2P Table

   - FTL - Update L2P Table

- 更新 P2L Table

   - FTL - Update P2L Table

- 更新 Virtual Block Status Table

   - FTL - Update Virtual  Block Status

- 減少可使用的當下 VB 的 Free Page



確認產生的資料是自己預期要的

- 寫入 PBA Table，每筆寫入後 Read Data 確認資料內容。

- 每一百筆儲存一次 Table bin 來看。


# FTL - Garbage Collect

- 主要變數：

   - minValidCount: find Minimal Valid Count on VBs

   - sourceToEraseVB: 

   - targetToMoveVB

- 針對是否是 Wear-Leveling 來挑選 Source VB

   - 1: 尋找最少的 Erase Count 的 VB

   - 0: 尋找最少 Valid Count 的 VB

- 挑選 Target VB，找到 Valid Count == 0 的 VB

- 移動 Valid Data from Source VB to Target DB.

   - 如何確認是否為 Valid Data，先從 P2L Table 中撈取 physical address 對應的 logical address，並調查 logical address 中所對應的 physical address 是否相同。

   - 更新 P2L_Table 與 L2P_Table。

   - 移動 valid data，更新 VBStatus\[targetToMoveVB\].validCount++

- Erase VB

   - 更新 VBStatus\[Source\].eraseCount++

   - VBStatus\[Source\].validCount = 0

   - 對 Source VB 中所有的 Page 初始化

- Total Erase Count++

- WriteTargetVB = targetToMoveVB

- FreePageCount = 單個 VB 所有的 Page - 移動過去的 Valid Page Count.


# FTL - Enhance Plan

- 可帶入 1 個 Page 有 16K 的概念。若每次寫入的資料不足 4K 會有寫入放大的問題。

- GC 的時候，或許可以多個 VB 一起 GC 的方式。

- 可將 Pool 放大，目前 6 個 VB 可以擴展到 100 個，可以更多靈活應用。
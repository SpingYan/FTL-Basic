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



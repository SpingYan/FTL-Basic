# FTL - Test Workflow

- Initialize FTL: FTL - Declaration and Initialize

- 寫入之前印出所有 Virtual Block 的 Status.

- 目標：寫入 1000 筆資料，每筆 1 個 Byte, 隨機資料內容 0x00 \~ 0xff

   - 根據 OP_Size，限制 Logical Page 寫入的範圍。\
      OP (Over Provisioning)

   - 寫入的 Logical Page 位置隨機產生，範圍限制如上條件。

   - 執行寫入: FTL - Write Data to Flash

   - 讀取資料: FTL - Read Data From Flash







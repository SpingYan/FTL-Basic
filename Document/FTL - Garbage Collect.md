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



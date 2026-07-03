# Future World (FW) UE5 專案文件索引

## 命名規則

- 遊戲顯示名稱：`Future World`
- 遊戲縮寫：`FW`
- C++ 類別前綴、UE module、Content/Source 技術路徑與分類命名皆使用 `FW`

## 建議閱讀順序

1. `FW_Game_Design_and_Technical_Architecture_v0.1.md`
   - 遊戲設計、玩法範圍、核心系統與開發路線。

2. `FW_Project_Structure_and_Cpp_Class_Skeleton_v0.1.md`
   - UE5 專案資料夾、C++ 類別骨架、Data Asset 與 Blueprint 建立清單。

3. `FW_Architecture_Enhancement_v0.2.md`
   - 量產級架構補強：事件系統、資料管理、多人權威、存檔、Debug、狀態機、Gameplay Tags、進度、音效與效能預算。

4. `FW_Production_Readiness_v0.3.md`
   - 最後一層量產準備：測試、自動化、調參、模組邊界、Fallback、版本遷移、Telemetry、在地化與安全規則。

5. `FW_MVP_Implementation_Tasks_v0.2.md`
   - 最嚴謹標準的 MVP 實作任務清單：Phase、任務 ID、優先級、產出、驗收條件、測試方式與階段 Gate。

6. `FW_GitHub_Version_Control_Workflow_v0.1.md`
   - GitHub 與版本控制工作流程。

## 目前架構狀態

目前文件已達到可進入 UE5 原型開發與 MVP 任務拆解的程度。

目前建議下一步：

- 若 UE5 尚未安裝，先完成引擎與 C++ 工具鏈安裝。
- 若 UE5 已安裝，依照 `FW_MVP_Implementation_Tasks_v0.2.md` 從 Phase 0 開始執行。


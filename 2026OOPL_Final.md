# 2026 OOPL Final Report

## 組別資訊

組別：T45  
組員：113820032 曾靖諺  
復刻遊戲：超級炸彈人  

## 專案簡介

### 遊戲簡介
超級炸彈人 是一款參考 Super Bomberman R2 中城堡模式的遊戲，原作 Super Bomberman R2 為 3D 版本，擬定改將原作的城堡模式復刻為類似 Super Bomberman 5 的 2D 版本，雙方圍繞場地上的「鑰匙」與「寶箱」展開攻防對決。  

遊戲分為 `進攻方` 與 `防守方`。  

`進攻方` 最多可達 8 人，閃避炸彈、陷阱、源石精靈與防守方的武器攻擊，最終使用鑰匙開啟終點所有的寶箱即為勝利。  

`防守方` 只有 1 人，利用地圖陷阱、炸彈與源石精靈防禦進攻方。只要在時間內守住任一寶箱，就能獲得勝利。  

* **會死掉：** 
  - **進攻方：** 進攻方會遭受陷阱、源石精靈、炸彈爆炸波及、防守方的武器攻擊，當人物受到攻擊則死亡，死亡的進攻方將掉落身上的鑰匙，並進入重生倒數。  
  - **防守方：** 防守方會遭受陷阱、炸彈爆炸波及，當人物受到攻擊則有 3 條命，前 2 條會原地暈倒一段時間，最後 1 條用盡則死亡，並進入重生倒數。  
* **會獲勝：**
  - **進攻方獲勝：** 成功取得鑰匙並成功開啟所有寶箱
  - **防守方獲勝：** 成功拖延至倒數計時結束
* **有關卡：** 每個關卡都有不同的陷阱、寶箱、源石精靈配置

### 組別分工
單人實作，無須分工

## 遊戲介紹

### 遊戲規則

**基本操作**
- 進攻方 / 防守方以同一個鍵盤操作（方向移動 + 放置炸彈 / 使用武器），按鍵可以操作設定畫面自訂義並保存（由主選單右下方齒輪進入操作設定）
- `ESC`：於主選單或遊戲封面按 `ESC` 可以開啟結束遊戲確認框
- `ENTER`：於遊戲中按 `ENTER` 可以開啟暫停選單，可選擇繼續、作弊開關、再次開始、返回房間

**對戰流程**
1. 遊戲封面 → 主選單 → 戰鬥選項 → 進入對戰
2. 對戰中畫面上方顯示倒數計時、剩餘寶箱數，人物頭頂以皇冠顯示防守方、鑰匙顯示持有鑰匙的進攻方
3. 倒數結束（防守方勝）或所有寶箱被開啟（進攻方勝）時，進入結算畫面並依表現結算金幣，金幣會保存

**核心機制**
- **炸彈與爆炸：** 雙方皆可放置炸彈，引爆後以十字形向四方延伸火焰，遇到可破壞磚塊則炸毀、遇到不可破壞的無敵牆則停止；火焰會引爆相鄰炸彈（連鎖）
- **鑰匙與寶箱：** 進攻方需在地圖上取得鑰匙，帶到寶箱前開啟，持有鑰匙者死亡會原地掉落鑰匙
- **遊戲道具：** 磚塊被破壞後有機率掉落遊戲道具（55% 掉落空氣、15% 加速鞋 (玩家加速 5 秒 3.0f -> 5.0f)、15% 炸彈道具 (增加可放置炸彈數，上限為 10)、15% 火焰道具 (擴大火焰蔓延半徑，上限為 5)）
- **陷阱地形：** 輸送帶（強制位移）、彈跳板（彈飛角色）、砲台 (發射砲彈，在射程內的玩家砲彈路過會砸暈)
- **源石精靈：** 與防守方合作的 AI 單位，會在地圖上隨機朝四個方向走，發現進攻方則主動追擊
- **防守方武器：** 劍（將前方三格擊倒）、雷射（直線發射擊倒）、屏障（生成暫時牆阻擋），由防守方於賽前選擇，過程中需要充能完成才能使用
- **重生：** 死亡後進入重生倒數 3 秒，於原本的出生點復活

> 詳細數值定義於 `include/Core/GameConstants.hpp`

### 遊戲畫面
- **封面：**
  ![封面](docs/title.png)
- **主選單：**
  ![主選單](docs/mainmenu.png)
- **操作設定：**
  ![操作設定](docs/setting.png)

## 程式設計

### 程式架構

整體採「分層 + 單一職責」的物件導向設計，以 `App` 為頂層協調者，透過**狀態模式**切換各畫面，遊戲進行時的邏輯再委派給各 **Manager** 與 **Entity**。原始碼依職責分到 `include/` 與 `src/` 下的對應子資料夾：

```
main.cpp ── App（生命週期 / 狀態機 / 共用資源）
                │
                ├── States/        各畫面（狀態模式：IGameState 子類）
                │                  Title / MainMenu / Rules / Settings /
                │                  LevelSelect / TeamSelect / WeaponSelect /
                │                  BattleSetup / Gameplay / GamePaused / Results
                │
                └── Core/GameSession  一場對戰的容器，持有並協調：
                      ├── Managers/   LevelManager（地圖載入/碰撞）
                      │               BombManager（炸彈/爆炸/連鎖）
                      │               InteractableManager（鑰匙/寶箱/陷阱）
                      │               AIManager（進攻方 bot 決策）
                      │               CheatManager（作弊）
                      ├── Entities/   Player / Bomb / Explosion /
                      │               Interactable / Spirit / PlayerBounce
                      ├── Turret/     Turret / Projectile / TurretManager（源石砲台）
                      ├── Weapons/    IDefenderWeapon ＋ Sword/Laser/Barrier ＋ WeaponFactory
                      ├── Bot/        Pathfinder（A*）/ DangerMap / BotNavigator
                      ├── Controller/ InputController：HumanController / BotController
                      ├── UI/         UIManager / HUD / DebugConsole / DebugOverlay …
                      ├── Config/     SaveData / KeyBindings / MatchConfig（持久化設定）
                      └── Audio/      MusicPlayer
```

**體現的物件導向原則與設計模式**

- **狀態模式（State）：** `IGameState` 為純虛擬介面（`OnEnter/OnUpdate/OnExit/WantsCursor`），每個畫面是一個子類別；`App` 只持有「目前狀態」並負責轉場 `TransitionTo()`。新增畫面不需改動 `App` 主迴圈。
- **策略模式 + 工廠（Strategy + Factory）+ OCP：** 防守方武器抽象為 `IDefenderWeapon::Fire(...)`，`Sword/Laser/Barrier` 各自實作；`WeaponFactory` 依 `MatchConfig` 建立對應武器。**新增一把武器只需新增一個子類別 + 工廠加一個 case，完全不動既有武器**（開放封閉原則）。
- **依賴反轉（DIP）：** 武器不直接依賴具體場景，而是透過 `IWeaponEffects`（特效接收端）與 `IWorldContext`（世界查詢）等介面與外界互動，由 `GameSession` 提供實作。角色 `Player::Update(const IWorldContext&)` 同理。
- **單一職責（SRP）：** 原本龐大的 gameplay 邏輯被拆成多個 Manager；AI 也再細分為 `Pathfinder`（通用 A*）、`DangerMap`（哪些格會被火焰掃到、BFS 找安全格）、`BotNavigator`（單一 bot 對地圖的視角與成本函式）。
- **封裝（Encapsulation）：** 各類別成員一律 private，對外只開放必要的 getter / 行為方法（如 `Player` 的座標、生命、暈眩狀態查詢）。
- **繼承與多型（Inheritance / Polymorphism）：** `Player`、`Spirit`、`Turret` 等實體皆繼承框架的 `Util::GameObject`；`Spirit` 內部以列舉 `State{PATROL,CHASE,DEAD}` 實作有限狀態機。
- **控制器抽象：** `InputController` 介面讓「人類鍵盤輸入」`HumanController` 與「AI 決策」`BotController` 可互換地驅動同一個 `Player`。

### 程式技術

- **遊戲框架 / 繪圖：** 以課程框架 PTSD（封裝 SDL2 + OpenGL）為底；圖片與文字皆經由 OpenGL shader（`Resources/shaders/Base.vert`、`Base.frag`）繪製。
- **AI 尋路：** 進攻方 bot 採 **A\*** 演算法（`Pathfinder`，`f = g + h`，以 `priority_queue` 做 min-heap，走法成本以 `costFunc` 注入），搭配 **DangerMap**（以 BFS 評估火焰致命格與最近安全格），讓 bot 會「閃炸彈、繞障礙、追目標」。
- **地圖資料驅動：** 關卡以文字檔 `Resources/Map/level_*.txt` 定義，`LevelManager` 載入後生成磚塊 / 出生點 / 互動物件；不同關卡套用不同 `TileSet`。
- **設定持久化：** 以 nlohmann/json 將金幣 / 音量（`SaveData`）與自訂按鍵（`KeyBindings`）寫入 `Resources/` 下的 json，關閉重開仍保留。
- **Debug Mode：** 內建 ImGui debug 主控台（`DebugConsole`）與資訊疊層（`DebugOverlay`），可即時改金幣、即殺指定玩家、顯示 AI / 碰撞資訊等，方便測試與展示。
- **音訊：** `MusicPlayer` 封裝背景音樂播放，切換畫面時換曲且同曲不重播。
- **記憶體管理：** 全程以 `shared_ptr` / `unique_ptr` 管理物件；Debug 建置開啟 CRT 記憶體洩漏偵測（`main.cpp`），正常退出為零洩漏。
- **跨工具鏈建置與打包：** 專案可同時以 **MSVC（Visual Studio）** 與 **MinGW（CLion）** 建置；`scripts/package.ps1` 會把 exe + `Resources/` + 設定 + 必要 runtime 打包成可直接發佈的 zip。

### 使用到 AI/AI Agent 的部分

開發過程中使用 **Claude Code（AI coding agent）** 協助以下工作（皆由本人主導、審閱後採用）：

- **發佈問題除錯：** 診斷出「打包後的 Release 在別台電腦圖片 / 文字全部不顯示」的根因——框架把 shader 路徑寫死成開發機的絕對路徑，打包未帶 shader；以將 shader 納入 `Resources/shaders/` 並改相對路徑解決。
- **跨工具鏈相容：** 排除 CLion（CMake 4.2）因舊版 SDL2 的最低版本宣告而無法 configure 的問題，並補上 MinGW 的 GUI 子系統與靜態連結設定，使 exe 自帶 runtime。
- **Bug 修正：** 修正「玩家改過 Windows 滑鼠游標時，遊戲游標顯示為黑方塊」（改以 `SDL_CreateSystemCursor` 明確指定標準箭頭游標）；以及「開關 debug 主控台後人物卡住」（移動改讀 SDL 即時鍵盤狀態，避開框架在 ImGui 抓滑鼠時略過鍵盤事件造成的卡鍵）。
- **重構建議：** 協助檢視 OOP 分層（如 AI 邏輯抽出 Pathfinder / DangerMap / BotNavigator）。

> 註：遊戲「內部」的 AI（進攻方 bot、源石精靈）為自行設計的 A\* + 狀態機，非外部 AI 服務。

## 結語

### 問題與解決方法

| 問題 | 解決方法 |
|:----|:----|
| 打包後的 Release 換台電腦圖片 / 文字全不顯示 | 根因為框架 shader 路徑寫死為絕對路徑、打包未帶 shader。改將 shader 納入 `Resources/shaders/` 並覆寫為相對路徑，打包腳本一併複製。 |
| CLion（CMake 4.2）無法編譯 | 新版 CMake 移除對 `cmake_minimum_required(<3.5)` 的相容，舊版 SDL2 觸發。於專案設定 `CMAKE_POLICY_VERSION_MINIMUM=3.5`，並補 MinGW 的 `-mwindows -static` 設定。 |
| 玩家改過滑鼠游標時出現黑方塊 | SDL 還原預設游標的已知問題；改以 `SDL_CreateSystemCursor(ARROW)` + `SDL_SetCursor` 明確指定游標。 |
| 開關 debug 主控台後人物卡住、要再按方向鍵才恢復 | 框架在 ImGui 抓滑鼠時會連鍵盤事件一起略過，導致方向鍵 KEYUP 遺失、按住狀態卡死。移動改讀 SDL 即時鍵盤狀態 `SDL_GetKeyboardState`，不受此影響。 |
| 進攻方在輸送帶上走不上去 | 修正輸送帶位移與玩家移動的判定順序。 |
| 音效重複播放 / UI 互相遮蔽 | 調整音效觸發時機與 UI 繪製層級（z 值）。 |
| 玩家可多放一顆炸彈（計數變負） | 死亡重生會把炸彈計數歸零，殘留炸彈爆炸時又遞減導致負值；`DecBombCount()` 加下限保護。 |
| AI bot 行為（卡牆 / 不閃炸彈） | 將散落的成本函式收斂為 `BotNavigator`，並以 `DangerMap` 評估安全格，修正追擊與閃避邏輯。 |

### 自評

| 項次 |          項目          | 完成 |
|:----:|:---------------------|:-----:|
| 1    | 這是範例 |  V  |
| 2    | 完成專案權限改為 public |  V  |
| 3    | 具有 debug mode 的功能  |  V  |
| 4    | 解決專案上所有 Memory Leak 的問題  |  V  |
| 5    | 報告中沒有任何錯字，以及沒有任何一項遺漏  |  V  |
| 6    | 報告至少保持基本的美感，人類可讀  |  V  |

### 心得

> （請以本人視角填寫，以下為可參考的草稿，請依實際感受改寫）

這次以單人完成《超級炸彈人》的城堡模式復刻，最大的收穫是把上課學到的物件導向原則真正落實到一個會動的遊戲上。一開始所有邏輯都擠在少數幾個大類別裡，越寫越難維護；後來逐步用狀態模式拆分畫面、用策略模式與工廠拆分武器、把 AI 拆成 Pathfinder / DangerMap / BotNavigator，程式才變得好讀又好改——新增一把武器或一個畫面都只要加檔案、幾乎不動舊碼，深刻體會到「開放封閉」的價值。

另外，發佈與跨平台（MSVC / MinGW、打包到別台電腦）遇到的問題，也讓我學到「能在自己電腦跑」和「能交到別人手上跑」是兩回事，資源路徑、shader、runtime 這些細節都得處理好。

### 貢獻比例
|      組員      | 貢獻 |
|:--------------:|:----:|
| 113820032 曾靖諺| 100% |

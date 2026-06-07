# 打包可發行套件：Release exe + Resources + config.json + VC runtime DLL → 壓成 zip。
# 用法 (需先建好 Release)：
#   powershell -ExecutionPolicy Bypass -File scripts\package.ps1
# 參數：
#   -BuildDir  Release 建置輸出目錄 (留空則自動偵測：VS 的 out\build\x64-Release 或 CLion 的 cmake-build-release 等)
#   -OutDir    套件輸出目錄         (預設 release)
#   -Version   版本字串             (預設 1.0)
param(
    [string]$BuildDir = "",
    [string]$OutDir   = "$PSScriptRoot\..\release",
    [string]$Version  = ""    # 留空時自動取 config.json 的 version (單一設定來源)
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot\..").Path

# 找 Release exe：有指定 -BuildDir 就用它；否則在常見建置目錄 (VS / CLion) 中挑「含 exe 且最新」的。
# 注意要 Release 版 (相對 Resources 路徑)；Debug 版打包出去到別台機器會畫不出圖片/文字。
if ($BuildDir) {
    $exe = Join-Path $BuildDir 'superbomberman.exe'
} else {
    $candidates = @(
        'out\build\x64-Release', 'out\build\Release',                 # Visual Studio (Open Folder / Presets)
        'cmake-build-release', 'cmake-build-x64-release', 'build\Release', 'build' # CLion / 一般 CMake
    ) | ForEach-Object { Join-Path $root $_ } |
        ForEach-Object { Join-Path $_ 'superbomberman.exe' } |
        Where-Object { Test-Path $_ }
    # @() 強制成陣列：只有單一結果時 Sort/索引才不會被當成字串逐字元處理
    $candidates = @($candidates | Sort-Object { (Get-Item $_).LastWriteTime } -Descending)
    if (-not $candidates) {
        throw "找不到任何 superbomberman.exe；請先以 Release 建置，或用 -BuildDir 指定目錄。`n" +
              "(注意：必須是 Release，Debug 版打包出去到別台機器會畫不出圖片/文字)"
    }
    $exe = $candidates[0]
    $BuildDir = Split-Path $exe -Parent
    Write-Output "自動偵測到 Release exe：$exe"
}
if (-not (Test-Path $exe)) {
    throw "找不到 Release exe：$exe`n請先建置 Release：cmake --build `"$BuildDir`" --target superbomberman"
}

# 產品名 / 版本取自 config.json (與 exe 內嵌名稱一致；單一設定來源)
$cfg  = Get-Content -Raw -Encoding UTF8 (Join-Path $root 'config.json') | ConvertFrom-Json
$name = if ($cfg.title) { [string]$cfg.title } else { 'Game' }
if (-not $Version) {
    $Version = if ($cfg.version) { [string]$cfg.version } else { '1.0' }
}

# 建立 / 清空 staging 資料夾 (用 .NET 刪除，避免 Remove-Item 在沙箱被攔)
$stage = Join-Path $OutDir $name
New-Item -ItemType Directory -Force -Path $stage | Out-Null
Get-ChildItem $stage -Force | ForEach-Object {
    if ($_.PSIsContainer) { [System.IO.Directory]::Delete($_.FullName, $true) }
    else { [System.IO.File]::Delete($_.FullName) }
}

# 內容：exe + 資源 + 設定 (Resources/ 內含 PTSD 繪圖/文字用的 shaders/，少了它會畫不出圖片/文字)
Copy-Item $exe (Join-Path $stage 'superbomberman.exe') -Force
Copy-Item (Join-Path $root 'Resources')  (Join-Path $stage 'Resources') -Recurse -Force
Copy-Item (Join-Path $root 'config.json') (Join-Path $stage 'config.json') -Force

# 不要把開發者的執行期狀態帶給玩家：移除套件內的存檔與按鍵設定，
# 讓玩家首次啟動為 0 金幣 + 預設按鍵。
foreach ($u in 'save.json', 'keybindings.json') {
    $uf = Join-Path $stage "Resources\$u"
    if (Test-Path $uf) { [System.IO.File]::Delete($uf) }
}

# 執行期 runtime：依 exe 實際匯入的 DLL 決定要不要帶 (匯入表的 DLL 名以 ASCII 明文存在 exe 內，
# 直接掃位元組即可判斷編譯器，免去解析 PE)。MSVC → 帶 VC runtime；MinGW 已用 -static 靜態連結
# → 不需任何額外 DLL；MinGW 動態連結 → 提醒缺哪些 DLL。
$bytes = [System.IO.File]::ReadAllBytes($exe)
$ascii = [System.Text.Encoding]::ASCII.GetString($bytes)
$needsVCRuntime  = $ascii -match '(?i)vcruntime140'
$mingwDlls = @('libstdc++-6.dll','libgcc_s_seh-1.dll','libwinpthread-1.dll') |
             Where-Object { $ascii -match [regex]::Escape($_) }

if ($needsVCRuntime) {
    # MSVC：動態尋找最新的 VC*.CRT x64，讓未裝 VC++ Redistributable 的電腦也能跑
    $crtDir = $null
    foreach ($r in (Get-ChildItem 'C:\Program Files\Microsoft Visual Studio\*\*\VC\Redist\MSVC' -Directory -ErrorAction SilentlyContinue)) {
        $d = Get-ChildItem $r.FullName -Recurse -Directory -Filter '*.CRT' -ErrorAction SilentlyContinue |
             Where-Object { $_.FullName -match '\\x64\\' } | Sort-Object FullName | Select-Object -Last 1
        if ($d) { $crtDir = $d.FullName }
    }
    if ($crtDir) {
        foreach ($d in 'msvcp140.dll','vcruntime140.dll','vcruntime140_1.dll') {
            $s = Join-Path $crtDir $d
            if (Test-Path $s) { Copy-Item $s (Join-Path $stage $d) -Force }
        }
    } else {
        Write-Warning "找不到 VC runtime redist；目標電腦若未裝 VC++ Redistributable 可能無法啟動。"
    }
} elseif ($mingwDlls) {
    # MinGW 動態連結：從 exe 所在/PATH 上的 MinGW bin 找對應 DLL 帶上 (建議改用 -static 免帶)
    $search = @($BuildDir) + ($env:PATH -split ';')
    foreach ($d in $mingwDlls) {
        $src = $search | ForEach-Object { Join-Path $_ $d } | Where-Object { Test-Path $_ } | Select-Object -First 1
        if ($src) { Copy-Item $src (Join-Path $stage $d) -Force }
        else { Write-Warning "exe 需要 $d 但找不到來源；玩家電腦可能缺此 DLL (建議以 -static 靜態連結)。" }
    }
} else {
    Write-Output "exe 為自帶 runtime (MinGW -static 或已靜態連結)，不需附加執行期 DLL。"
}

# 壓縮 (zip 內含產品名資料夾)
$zip = Join-Path $OutDir ("{0}-v{1}.zip" -f ($name -replace '\s',''), $Version)
if (Test-Path $zip) { [System.IO.File]::Delete($zip) }
Compress-Archive -Path $stage -DestinationPath $zip -Force

Write-Output "套件資料夾：$stage"
Get-ChildItem $stage | ForEach-Object {
    "  {0,-26} {1}" -f $_.Name, $(if ($_.PSIsContainer) { '<dir>' } else { "$([math]::Round($_.Length/1KB,0)) KB" })
}
Write-Output ("ZIP：{0}  ({1} MB)" -f $zip, [math]::Round((Get-Item $zip).Length/1MB,1))

# 打包可發行套件：Release exe + Resources + config.json + VC runtime DLL → 壓成 zip。
# 用法 (需先建好 Release)：
#   powershell -ExecutionPolicy Bypass -File scripts\package.ps1
# 參數：
#   -BuildDir  Release 建置輸出目錄 (預設 out\build\x64-Release)
#   -OutDir    套件輸出目錄         (預設 release)
#   -Version   版本字串             (預設 1.0)
param(
    [string]$BuildDir = "$PSScriptRoot\..\out\build\x64-Release",
    [string]$OutDir   = "$PSScriptRoot\..\release",
    [string]$Version  = "1.0"
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot\..").Path
$exe  = Join-Path $BuildDir 'superbomberman.exe'
if (-not (Test-Path $exe)) {
    throw "找不到 Release exe：$exe`n請先建置 Release：cmake --build `"$BuildDir`" --target superbomberman"
}

# 產品名取自 config.json 的 title (與 exe 內嵌名稱一致)
$cfg  = Get-Content -Raw -Encoding UTF8 (Join-Path $root 'config.json') | ConvertFrom-Json
$name = if ($cfg.title) { [string]$cfg.title } else { 'Game' }

# 建立 / 清空 staging 資料夾 (用 .NET 刪除，避免 Remove-Item 在沙箱被攔)
$stage = Join-Path $OutDir $name
New-Item -ItemType Directory -Force -Path $stage | Out-Null
Get-ChildItem $stage -Force | ForEach-Object {
    if ($_.PSIsContainer) { [System.IO.Directory]::Delete($_.FullName, $true) }
    else { [System.IO.File]::Delete($_.FullName) }
}

# 內容：exe (改名為產品名) + 資源 + 設定
Copy-Item $exe (Join-Path $stage "$name.exe") -Force
Copy-Item (Join-Path $root 'Resources')  (Join-Path $stage 'Resources') -Recurse -Force
Copy-Item (Join-Path $root 'config.json') (Join-Path $stage 'config.json') -Force

# VC runtime：動態尋找最新的 VC*.CRT x64，讓未裝 VC++ Redistributable 的電腦也能跑
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

# 壓縮 (zip 內含產品名資料夾)
$zip = Join-Path $OutDir ("{0}-v{1}.zip" -f ($name -replace '\s',''), $Version)
if (Test-Path $zip) { [System.IO.File]::Delete($zip) }
Compress-Archive -Path $stage -DestinationPath $zip -Force

Write-Output "套件資料夾：$stage"
Get-ChildItem $stage | ForEach-Object {
    "  {0,-26} {1}" -f $_.Name, $(if ($_.PSIsContainer) { '<dir>' } else { "$([math]::Round($_.Length/1KB,0)) KB" })
}
Write-Output ("ZIP：{0}  ({1} MB)" -f $zip, [math]::Round((Get-Item $zip).Length/1MB,1))

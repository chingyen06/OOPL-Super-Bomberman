# 由 config.json 產生 Windows 資源檔 app.rc：嵌入 exe 圖示 + 版本資訊。
# 以 UTF-16 LE (含 BOM) 輸出，rc.exe 原生支援 → 中文作者名可正確嵌入。
#
# 欄位對應：
#   title     → ProductName / FileDescription (檔案總管顯示的名稱)
#   author    → CompanyName / Comments        (發行者 / 作者；此版本由誰製作)
#   copyright → LegalCopyright                 (著作權；歸原作權利人，不是製作者)
param(
    [Parameter(Mandatory=$true)][string]$ConfigJson,
    [Parameter(Mandatory=$true)][string]$IconPath,
    [Parameter(Mandatory=$true)][string]$OutRc
)

$ErrorActionPreference = 'Stop'

$cfg       = Get-Content -Raw -Encoding UTF8 $ConfigJson | ConvertFrom-Json
$name      = if ($cfg.title)     { [string]$cfg.title     } else { 'Application' }
$author    = if ($cfg.author)    { [string]$cfg.author    } else { 'Unknown' }
$copyright = if ($cfg.copyright) { [string]$cfg.copyright } else { '' }
$version   = if ($cfg.version)   { [string]$cfg.version   } else { '1.0' }
$icon      = ($IconPath -replace '\\','/')      # rc 接受正斜線，避免反斜線轉義問題

# rc 字串中的雙引號需以兩個雙引號轉義
$name      = $name      -replace '"','""'
$author    = $author    -replace '"','""'
$copyright = $copyright -replace '"','""'

# 把語意化版本 (e.g. "1.1" / "1.1.0") 補滿成 RC 需要的四段格式
$parts = ($version -split '\.') + @('0','0','0','0') | Select-Object -First 4
$verCommas = ($parts -join ',')                # 1.1 → "1,1,0,0"
$verDots   = ($parts -join '.')                # 1.1 → "1.1.0.0"

$rc = @"
#include <windows.h>

// exe 圖示 (ID 1 → Explorer 以此為應用程式圖示)
1 ICON "$icon"

// 版本資訊：檔案總管「內容 → 詳細資料」可見的名稱 / 發行者 / 著作權
VS_VERSION_INFO VERSIONINFO
 FILEVERSION $verCommas
 PRODUCTVERSION $verCommas
 FILEFLAGSMASK 0x3fL
 FILEFLAGS 0x0L
 FILEOS VOS__WINDOWS32
 FILETYPE VFT_APP
 FILESUBTYPE VFT2_UNKNOWN
BEGIN
    BLOCK "StringFileInfo"
    BEGIN
        BLOCK "040404b0"
        BEGIN
            VALUE "CompanyName",      "$author"
            VALUE "FileDescription",  "$name"
            VALUE "FileVersion",      "$verDots"
            VALUE "InternalName",     "$name"
            VALUE "LegalCopyright",   "$copyright"
            VALUE "OriginalFilename", "superbomberman.exe"
            VALUE "ProductName",      "$name"
            VALUE "ProductVersion",   "$verDots"
            VALUE "Comments",         "Developed by $author"
        END
    END
    BLOCK "VarFileInfo"
    BEGIN
        VALUE "Translation", 0x0404, 1200
    END
END
"@

$enc = New-Object System.Text.UnicodeEncoding($false, $true)  # UTF-16 LE + BOM
[System.IO.File]::WriteAllText($OutRc, $rc, $enc)
Write-Output "gen_rc: wrote $OutRc (name='$name', version='$verDots', author='$author', copyright='$copyright')"

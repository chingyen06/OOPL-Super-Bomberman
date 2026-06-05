cmake_minimum_required(VERSION 3.16)
project(superbomberman)

if(WIN32 AND MSVC)
    enable_language(RC)  # 編譯 app.rc (exe 圖示 + 版本資訊)
endif()

set(CMAKE_CXX_STANDARD 17)

cmake_policy(SET CMP0135 NEW)

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

FetchContent_Declare(
    ptsd

    URL         https://github.com/ntut-open-source-club/practical-tools-for-simple-design/archive/refs/tags/v0.3-rc1.zip

    # Or alternatively if you want to use a specific git commit instead of a release:
    # GIT_REPOSITORY  https://github.com/ntut-open-source-club/practical-tools-for-simple-design.git
    # GIT_TAG         <git_tag>

    # SOURCE_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/PTSD
)

FetchContent_MakeAvailable(ptsd)

# ---- 讓 PTSD 的 shader 在 Release / 封裝後仍可解析 ----
# PTSD 把繪製圖片/文字用的 shader 路徑 (PTSD_ASSETS_DIR) 寫死成「編譯當下的絕對路徑」
# (其建置資料夾 _deps/ptsd-src/assets)。在開發機從建置資料夾跑沒問題，但一旦把套件
# 複製到別台電腦 (或刪掉建置資料夾)，該絕對路徑就不存在 → Base.vert/frag 載不到 →
# shader program 連結失敗 → 所有圖片與文字都畫不出來 (只 log error 不崩潰)。
# 修法：非 Debug 時把 PTSD_ASSETS_DIR 改成相對路徑 "Resources" (與遊戲自己的 RESOURCE_DIR
# 同一個資料夾；執行期 cwd 已被 main.cpp chdir 到 exe 旁)，shader 解析為 Resources/shaders/Base.*。
# Resources/shaders/ 已隨 repo 一起版控，POST_BUILD 與 package.ps1 既有的 Resources 複製即可帶上。
# (用 FILTER 先移除 PTSD 自己加的絕對路徑定義，避免 MSVC C4005 巨集重定義警告。)
if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    get_target_property(_ptsd_defs PTSD COMPILE_DEFINITIONS)
    if(_ptsd_defs)
        list(FILTER _ptsd_defs EXCLUDE REGEX "^PTSD_ASSETS_DIR=")
    else()
        set(_ptsd_defs "")
    endif()
    list(APPEND _ptsd_defs "PTSD_ASSETS_DIR=\"Resources\"")
    set_target_properties(PTSD PROPERTIES COMPILE_DEFINITIONS "${_ptsd_defs}")
endif()

include(files.cmake)
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
list(TRANSFORM SRC_FILES PREPEND ${SRC_DIR}/)
list(TRANSFORM INCLUDE_FILES PREPEND ${INCLUDE_DIR}/)

# file(GLOB_RECURSE SRC_FILES src/*)
# file(GLOB_RECURSE HEADER_FILES include/*)

# ---- Windows：exe 圖示與版本資訊 (名稱 / 作者) 由 config.json 產生 app.rc ----
# config.json 是單一設定來源；gen_rc.ps1 讀其 title/author 並指向 Resources/app.ico。
set(APP_RC "")
if(WIN32 AND MSVC)
    set(APP_RC "${CMAKE_BINARY_DIR}/app.rc")
    execute_process(
        COMMAND powershell -NoProfile -ExecutionPolicy Bypass
                -File "${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen_rc.ps1"
                -ConfigJson "${CMAKE_CURRENT_SOURCE_DIR}/config.json"
                -IconPath  "${CMAKE_CURRENT_SOURCE_DIR}/Resources/app.ico"
                -OutRc     "${APP_RC}"
        RESULT_VARIABLE APP_RC_RESULT)
    if(NOT APP_RC_RESULT EQUAL 0)
        message(WARNING "gen_rc.ps1 failed (code ${APP_RC_RESULT}); building without icon/version resource")
        set(APP_RC "")
    endif()
endif()

add_executable(${PROJECT_NAME}
    ${SRC_DIR}/main.cpp
    ${SRC_FILES}
    ${HEADER_FILES}
    ${APP_RC}
)

if(MSVC)
    # /utf-8: 來源與執行字元集都用 UTF-8，確保中文字串字面值 (暫停選單標籤) 以 UTF-8
    # 進入二進位檔，供 TTF_RenderUTF8 正確繪製；同時消除 C4819 (cp950) 警告。
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /utf-8)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Resources")
else()
    # Release / 封裝：資源以相對路徑置於 exe 旁 (打包時一併複製 Resources 與 config.json，
    # 雙擊 exe 時工作目錄即為 exe 所在資料夾，"Resources/..." 與 config.json 皆能解析)。
    target_compile_definitions(${PROJECT_NAME} PRIVATE RESOURCE_DIR="Resources")
endif()

target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${DEPENDENCY_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/PTSD/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 依職責把標頭分到 include/ 的各子資料夾後，仍允許直接 #include 檔名 —
# 把 include/ 下每個子資料夾都加入 include 路徑 (新增資料夾自動納入)。
file(GLOB INCLUDE_SUBDIRS LIST_DIRECTORIES true ${CMAKE_CURRENT_SOURCE_DIR}/include/*)
foreach(dir ${INCLUDE_SUBDIRS})
    if(IS_DIRECTORY ${dir})
        target_include_directories(${PROJECT_NAME} PRIVATE ${dir})
    endif()
endforeach()

target_link_libraries(${PROJECT_NAME}
    SDL2::SDL2main
    PTSD
)

# ---- Release / 封裝：以 Windows GUI 子系統連結，避免雙擊 exe 時彈出黑色 console 視窗 ----
# 仍用標準 main() 進入點 (mainCRTStartup)，故不需改寫成 WinMain。Debug 保留 console 方便看 log。
if(MSVC AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_options(${PROJECT_NAME} PRIVATE /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup)
endif()

# ---- 建置後：把執行所需檔案放到 exe 旁 (main.cpp 啟動時會 chdir 到此目錄) ----
# config.json 一律複製 (框架讀視窗標題 / 尺寸；亦供 gen_rc 之外的執行期使用)。
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/config.json"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/config.json"
    VERBATIM)

# 非 Debug (Release / 封裝)：RESOURCE_DIR 為相對 "Resources"，把整個 Resources 複製到 exe 旁，
# 使 exe 可獨立執行 / 直接打包。Debug 用絕對路徑，不需複製。
if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/Resources"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/Resources"
        VERBATIM)
endif()
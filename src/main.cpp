#include "App.hpp"

#include <imgui.h>
#include <imgui_impl_opengl3.h>

#include "Core/Context.hpp"
#include "config.hpp"

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif

#ifdef _WIN32
// windows.h 必須在所有專案標頭「之後」才 include：它的 DELETE / IN / OUT 等巨集會與
// Util::Keycode 的列舉名衝突，放到最後可避免污染上方標頭的解析。
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

// 把工作目錄切到 exe 所在資料夾：讓相對路徑的 Resources/ 與 config.json 一律可解析
// (打包後雙擊、或從任意目錄啟動皆可)。用寬字元版本以支援含非 ASCII 字元的路徑。
static void SetCwdToExeDir() {
    wchar_t path[MAX_PATH];
    DWORD n = GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (n > 0 && n < MAX_PATH) {
        for (DWORD i = n; i-- > 0; ) {
            if (path[i] == L'\\' || path[i] == L'/') { path[i] = L'\0'; break; }
        }
        SetCurrentDirectoryW(path);
    }
}
#endif

int main(int, char**) {
#ifdef _WIN32
    SetCwdToExeDir();
#endif
#if defined(_MSC_VER) && defined(_DEBUG)
    // Debug 建置：程式正常結束時，把仍未釋放的 CRT 堆積配置 (記憶體洩漏) 傾印到 stdout。
    // 本專案所有遊戲物件皆以 shared_ptr / unique_ptr 管理，App 解構時連同 Renderer 一併釋放，
    // 因此「玩完一場後正常退出」應為零洩漏；此旗標讓這點可在執行期被直接驗證。
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    auto context = Core::Context::GetInstance();
    App app;

    while (!context->GetExit()) {
        context->Setup();  // ImGui NewFrame — debug 主控台在 app.Update() 期間提交視窗

        switch (app.GetCurrentState()) {
            case App::State::START:
                app.Start();
                break;

            case App::State::UPDATE:
                app.Update();
                break;

            case App::State::END:
                app.End();
                context->SetExit(true);
                break;
        }

        // ImGui 疊在遊戲場景之上繪製，再交給 Context::Update() 進行 SwapWindow
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        context->Update();
    }
    return 0;
}

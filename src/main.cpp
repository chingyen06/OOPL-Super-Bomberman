#include "App.hpp"

#include <imgui.h>
#include <imgui_impl_opengl3.h>

#include "Core/Context.hpp"
#include "config.hpp"

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif

int main(int, char**) {
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

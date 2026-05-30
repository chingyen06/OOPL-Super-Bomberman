#include "App.hpp"

#include <imgui.h>
#include <imgui_impl_opengl3.h>

#include "Core/Context.hpp"
#include "config.hpp"

int main(int, char**) {
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

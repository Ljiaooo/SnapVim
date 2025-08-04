#include "snapvim.h"
#include "third_party/imgui/imgui.h"

namespace SnapVim {

void renderSnapVimEditor(char* textBuffer, int winWidth, int winHeight, ImGuiWindowFlags window_flags)
{
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

    if (ImGui::Begin("InvisibleWindow", nullptr, window_flags)) {
       ImGui::InputTextMultiline("###SnapVim", textBuffer, 10000, ImVec2(winWidth - 15, winHeight - 15));
    }
    ImGui::End();
}
}
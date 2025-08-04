#include "snapvim.h"
#include "imgui.h"
#include "third_party/imgui/imgui.h"

namespace SnapVim {

void renderSnapVimEditor(char* textBuffer, int winWidth, int winHeight)
{
    ImGuiWindowFlags window_flags = 
    ImGuiWindowFlags_NoDecoration |      
    ImGuiWindowFlags_NoBackground |      
    ImGuiWindowFlags_NoMove |            
    ImGuiWindowFlags_NoResize |          
    ImGuiWindowFlags_NoSavedSettings |   
    ImGuiWindowFlags_NoFocusOnAppearing |
    ImGuiWindowFlags_NoBringToFrontOnFocus |
    ImGuiWindowFlags_NoBackground;

    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f); // Set frame background color to transparent

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

    if (ImGui::Begin("InvisibleWindow", nullptr, window_flags)) {
       ImGui::InputTextMultiline("###SnapVim", textBuffer, 10000, ImVec2(winWidth - 15, winHeight - 15));
    }
    ImGui::End();
}
}
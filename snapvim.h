#pragma once
#include "imgui.h"
#include "svimconfig.h"

#define HISTORY_BUFFER_SIZE 2

namespace SnapVim 
{

struct SnapVimState
{
    ImVec2 Scroll;
    bool Edited;
    bool CursorFollow;
    float CursorAnim;
    int BufCapacity;

    SnapVimState() : Edited(false), CursorFollow(true), CursorAnim(0.0f), BufCapacity(0) {}

    void CursorAnimReset() { CursorAnim = -0.3f; };
};
void initSnapVim();
void renderSnapVimEditor(char* textBuffer, int winWidth, int winHeight, ImGuiWindowFlags window_flags);
bool snapVimEditor(char* buf, int buf_size, const ImVec2& size_arg);

}
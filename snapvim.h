#pragma once
#include "external/libvim/src/vim.h"
#include "imgui.h"
#include "imgui_internal.h"

#define HISTORY_BUFFER_SIZE 2

namespace SnapVim 
{

struct SnapVimState
{
    ImVec2 Scroll;
    bool Edited;
    bool CursorFollow;
    bool HighlightLine;
    float CursorAnim;
    int BufCapacity;

    SnapVimState() : Scroll(0.0f, 0.0f), Edited(false), CursorFollow(true), HighlightLine(true), CursorAnim(0.0f), BufCapacity(0) {}

    void CursorAnimReset() { CursorAnim = -0.3f; };
};
void InitSnapVim();
void OnKeyPressed(unsigned int key);
void RenderSnapVimEditor(char* textBuffer, int winWidth, int winHeight, ImGuiWindowFlags window_flags);
bool SnapVimEditor(char* buf, const ImVec2& size_arg);
ImVec2 CalCursorXAndWidth(ImGuiContext* ctx, char_u* line, int col, int mode);

}
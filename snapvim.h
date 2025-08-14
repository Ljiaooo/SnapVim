/**
 * Not Implemented Features:
 * - vim search highlight
 * - visual mode multi-cursor
 * - runtime configuration
 */

#pragma once
#include "vim.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <windef.h>

#define HISTORY_BUFFER_SIZE 2

extern HWND g_previousFocus; // Global variable to store the last focused window

namespace SnapVim 
{

struct SnapVimState
{
    ImVec2 Scroll;
    bool CursorFollow;
    bool HighlightLine;
    float CursorAnim;
    int BufCapacity;
    char* PasteBuffer;
    HWND hwnd;
    bool BufferNeedReset;
    bool BufferNeedSwitch;
    bool CursorCountNeedReset;
    int CursorCount;
    pos_T Cursors[1024]; // Maximum number of cursors

    SnapVimState() 
    : Scroll(0.0f, 0.0f),
      CursorFollow(true),
      HighlightLine(true),
      CursorAnim(0.0f),
      BufCapacity(0),
      PasteBuffer(nullptr),
      hwnd(nullptr),
      BufferNeedReset(false),
      BufferNeedSwitch(false),
      CursorCountNeedReset(false),
      CursorCount(0) {}

    void CursorAnimReset() { CursorAnim = -0.3f; };
};
void InitSnapVim(HWND hwnd);
void DestroySnapVim();
void OnKeyPressed(unsigned int key);
void RenderSnapVimEditor(char* textBuffer, int winWidth, int winHeight, ImGuiWindowFlags window_flags);
bool SnapVimEditor(char* buf, const ImVec2& size_arg);
ImVec2 CalCursorXAndWidth(ImGuiContext* ctx, char_u* line, int col, int mode);
void CopyToPasteBuffer();
void PasteTextToPreviousFocus(const char* text);
void DisableIME(bool disable);
void ResetCurrentBuffer();
void OnWriteCallback();
void OnBufferPreviousCallback();
void OnCursorAdd(pos_T cursor);

}
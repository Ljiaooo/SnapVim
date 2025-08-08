#pragma once
#include "imgui.h"
#include "svimconfig.h"

#define HISTORY_BUFFER_SIZE 2

namespace SnapVim 
{

void initVim();
void renderSnapVimEditor(char* textBuffer, int winWidth, int winHeight, ImGuiWindowFlags window_flags);
//bool snapVimEditor(char* buf, int buf_size, const ImVec2& size_arg);

}
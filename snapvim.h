#pragma once
#include "imgui.h"
#include "svimconfig.h"

#define HISTORY_BUFFER_SIZE 2

namespace SnapVim 
{

void initVim();
void renderSnapVimEditor(char* textBuffer, int winWidth, int winHeight, ImGuiWindowFlags window_flags);
//bool snapVimEditor(const char* label, const char* hint, char* buf, int buf_size, const ImVec2& size_arg, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* callback_user_data);

}
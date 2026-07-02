#pragma once
#include "imgui.h"
#include "svimconfig.h"
#include <string>

struct RuntimeConfig
{
    ImVec4 BackgroundColor = BACKGROUND_COLOR;
    ImVec4 CursorColor = ImColor(80, 80, 120);
    ImVec4 HighlightLineColor = ImColor(60, 60, 60);
    ImVec4 SelectionColor = ImColor(120, 120, 120, 180);
    ImVec4 ScrollbarBgColor = ImColor(0, 0, 0, 0);
    ImVec4 FontColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

    std::string EnglishFont = ENGLISH_FONT;
    std::string ChineseFont = CHINESE_FONT;
    float FontSize = FONT_SIZE;

    float Padding = PADDING;

    int TransparentValue = TRANSPARENT_VALUE;
    int WindowWidth = WINDOW_WIDTH;
    int WindowHeight = WINDOW_HEIGHT;

    bool NeedsFontRebuild = false;
};

extern RuntimeConfig g_config;

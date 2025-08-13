/**
 * Not Implemented Features:
 * - vim search highlight
 * - visual mode multi-cursor
 * - custom commands for view history / input(:w)
 */

#pragma once
#define FONT_SIZE 25.0f
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 400
#define BUFFER_SIZE 10000
#define TRANSPARENT_VALUE 240 
#define ENGLISH_FONT "JetBrainsMono-Medium.ttf"
#define CHINESE_FONT "NotoSansSC-Medium.ttf"

#define BACKGROUND_COLOR ImVec4(0.157, 0.157, 0.157, 1.0)
#define CURSOR_COLOR ImColor(80, 80, 120)
#define HIGHLIGHT_LINE_COLOR ImColor(60, 60, 60)
#define SCROLLBAR_BG_COLOR ImColor(0, 0, 0, 0)
#define PADDING 10.0f
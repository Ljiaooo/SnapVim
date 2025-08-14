#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "svimconfig.h"
#include "snapvim.h"

#ifdef __cplusplus
extern "C" {
#include <libvim.h>
}
#endif

namespace SnapVim {

buf_T* buffer0 = nullptr;
buf_T* buffer1 = nullptr;
buf_T* currentBuffer = nullptr;
SnapVimState* state = nullptr;

void InitSnapVim(HWND hwnd)
{
    // init vim backend
    vimInit(0, nullptr);
    vimOptionSetInsertSpaces(TRUE);
    vimOptionSetTabSize(2);
    vimSetWriteRedirectCallback(OnWriteCallback);
    vimSetBufferPreviousCallback(OnBufferPreviousCallback);
    vimSetCursorAddCallback(OnCursorAdd);

    // initially create two buffers, one for the current text and one for the history
    buffer1 = vimBufferOpen((char_u*)"dummy_buffer1", 1, 0);
    buffer0 = vimBufferOpen((char_u*)"dummy_buffer0", 1, 0);
    currentBuffer = buffer0;

    state = new SnapVimState();
    state->PasteBuffer = (char*)ImGui::MemAlloc(BUFFER_SIZE * sizeof(char));
    state->hwnd = hwnd;

    // global settings
    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;
    io.ConfigNavEscapeClearFocusItem = false;
    io.ConfigNavEscapeClearFocusWindow = false;
    ImGuiStyle& style = g.Style;
    style.Colors[ImGuiCol_FrameBg] = BACKGROUND_COLOR;
    style.Colors[ImGuiCol_InputTextCursor] = CURSOR_COLOR;
    style.Colors[ImGuiCol_ScrollbarBg] = SCROLLBAR_BG_COLOR;
    style.FramePadding = ImVec2(0, 0);
    style.WindowPadding = ImVec2(0, 10);
    style.ScrollbarSize = 12;

    DisableIME(true);
}

void DestroySnapVim()
{
    delete state;
}

void OnKeyPressed(unsigned int key)
{
    // handle input
    char utf[5];
    ImTextCharToUtf8(utf, key);
    int urf_len = strlen(utf);
    if ((vimGetMode() & INSERT) == INSERT)
    {
        vimInput((char_u*)utf);
        pos_T current_cursor = vimCursorGetPosition();
        for (int cursorIndex = 0; cursorIndex < state->CursorCount; ++cursorIndex)
        {
            pos_T &cursor = state->Cursors[cursorIndex];
            vimCursorSetPosition(cursor);
            vimInput((char_u*)utf);
            cursor.col += urf_len;
        }
        vimCursorSetPosition(current_cursor);
    }
    else vimInput((char_u*)utf);

    state->CursorFollow = true;
    state->CursorAnimReset();
}

void RenderSnapVimEditor(char* textBuffer, int winWidth, int winHeight, ImGuiWindowFlags window_flags)
{
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

    if (ImGui::Begin("InvisibleWindow", nullptr, window_flags)) {
       SnapVim::SnapVimEditor(textBuffer, ImVec2(winWidth, winHeight));
    }
    ImGui::End();
}

bool SnapVimEditor(char* buf, const ImVec2& size_arg)
{
    using namespace ImGui;
    ImGuiWindow* window = GetCurrentWindow();

    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;
    const ImGuiStyle& style = g.Style;

    const char* label = "###SnapVim";
    int flags = 0;

    if (state->BufferNeedSwitch)
    {
        currentBuffer = (currentBuffer == buffer0) ? buffer1 : buffer0;
        vimBufferSetCurrent(currentBuffer);
        state->BufferNeedSwitch = false;
    }
    if (state->BufferNeedReset)
    {
        ResetCurrentBuffer();
        state->BufferNeedReset = false;
    }
    if (state->CursorCountNeedReset && (vimGetMode() & NORMAL) == NORMAL)
    {
        state->CursorCount = 0;
        state->CursorCountNeedReset = false;
    }

    // Disable IME when not in insert mode
    if ((vimGetMode() & INSERT) != INSERT && ImmGetOpenStatus(ImmGetContext(state->hwnd)))
        DisableIME(true);

    BeginGroup();
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImVec2 frame_size = CalcItemSize(ImVec2(size_arg.x - style.WindowPadding.x * 2, size_arg.y - style.WindowPadding.y), CalcItemWidth(), ( g.FontSize * 8.0f) + style.FramePadding.y * 2.0f); // Arbitrary default of 8 lines high for multi-line
    const ImVec2 total_size = ImVec2(frame_size.x + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), frame_size.y);

    const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + frame_size);
    const ImRect total_bb(frame_bb.Min, frame_bb.Min + total_size);

    ImGuiWindow* draw_window = window;
    ImVec2 inner_size = frame_size;
    ImGuiLastItemData item_data_backup;
    ImVec2 backup_pos = window->DC.CursorPos;
    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, id, &frame_bb, ImGuiItemFlags_Inputable))
    {
        EndGroup();
        return false;
    }
    item_data_backup = g.LastItemData;
    window->DC.CursorPos = backup_pos;

    // Prevent NavActivation from Tabbing when our widget accepts Tab inputs: this allows cycling through widgets without stopping.
    if (g.NavActivateId == id && (g.NavActivateFlags & ImGuiActivateFlags_FromTabbing))
        g.NavActivateId = 0;

    PushStyleColor(ImGuiCol_ChildBg, style.Colors[ImGuiCol_FrameBg]);
    PushStyleVar(ImGuiStyleVar_ChildRounding, style.FrameRounding);
    PushStyleVar(ImGuiStyleVar_ChildBorderSize, style.FrameBorderSize);
    PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); // Ensure no clip rect so mouse hover can reach FramePadding edges
    bool child_visible = BeginChildEx(label, id, frame_bb.GetSize(), ImGuiChildFlags_Borders, ImGuiWindowFlags_NoMove);
    PopStyleVar(3);
    PopStyleColor();
    if (!child_visible)
    {
        EndChild();
        EndGroup();
        return false;
    }
    draw_window = g.CurrentWindow; // Child window
    draw_window->DC.NavLayersActiveMaskNext |= (1 << draw_window->DC.NavLayerCurrent); // This is to ensure that EndChild() will display a navigation highlight so we can "enter" into it.
    draw_window->DC.CursorPos += style.FramePadding;

    // Ensure mouse cursor is set even after switching to keyboard/gamepad mode. May generalize further? (#6417)
    bool hovered = ItemHoverable(frame_bb, id, g.LastItemData.ItemFlags | ImGuiItemFlags_NoNavDisableMouseHover);
    if (hovered)
        SetMouseCursor(ImGuiMouseCursor_TextInput);
    if (hovered && g.NavHighlightItemUnderNav)
        hovered = false;



    const bool user_clicked = hovered && io.MouseClicked[0];
    const bool user_scroll_finish = g.ActiveId == 0 && g.ActiveIdPreviousFrame == GetWindowScrollbarID(draw_window, ImGuiAxis_Y);
    const bool user_scroll_active = g.ActiveId == GetWindowScrollbarID(draw_window, ImGuiAxis_Y);
    bool clear_active_id = false;
    bool select_all = false;

    float scroll_y = draw_window->Scroll.y;
    buf_T *vimBuf = currentBuffer;

    const bool is_osx = io.ConfigMacOSXBehaviors;
    if (g.ActiveId != id)
    {
        SetActiveID(id, window);
        SetFocusID(id, window);
        FocusWindow(window);
    }
    if (g.ActiveId == id)
    {
        // Declare some inputs, the other are registered and polled via Shortcut() routing system.
        // FIXME: The reason we don't use Shortcut() is we would need a routing flag to specify multiple mods, or to all mods combination into individual shortcuts.
        const ImGuiKey always_owned_keys[] = { ImGuiKey_LeftArrow, ImGuiKey_RightArrow, ImGuiKey_Enter, ImGuiKey_KeypadEnter, ImGuiKey_Delete, ImGuiKey_Backspace, ImGuiKey_Home, ImGuiKey_End };
        for (ImGuiKey key : always_owned_keys)
            SetKeyOwner(key, id);
        if (user_clicked)
            SetKeyOwner(ImGuiKey_MouseLeft, id);
        g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
        SetKeyOwner(ImGuiKey_PageUp, id);
        SetKeyOwner(ImGuiKey_PageDown, id);
        // FIXME: May be a problem to always steal Alt on OSX, would ideally still allow an uninterrupted Alt down-up to toggle menu
        if (is_osx)
            SetKeyOwner(ImGuiMod_Alt, id);

        // Expose scroll in a manner that is agnostic to us using a child window
        if (state != NULL)
            state->Scroll.y = draw_window->Scroll.y;

    }

    // Release focus when we click outside
    if (g.ActiveId == id && io.MouseClicked[0]) //-V560
        clear_active_id = true;

    // Lock the decision of whether we are going to take the path displaying the cursor or selection
    bool value_changed = false;
    bool validated = false;

    // Process mouse inputs and character inputs
    if (g.ActiveId == id)
    {
        // Process regular text input (before we check for Return because using some IME will effectively send a Return?)
        // We ignore CTRL inputs, but need to allow ALT+CTRL as some keyboards (e.g. German) use AltGR (which _is_ Alt+Ctrl) to input certain characters.
        if (Shortcut(ImGuiKey_Escape, 0, id)) vimKey((char_u*)"<Esc>");
        if (Shortcut(ImGuiMod_Ctrl | ImGuiKey_V, 0, id)) vimKey((char_u*)"<c-v>");
        const bool ignore_char_inputs = (io.KeyCtrl && !io.KeyAlt) || (is_osx && io.KeyCtrl);
        if (io.InputQueueCharacters.Size > 0)
        {
            if (!ignore_char_inputs)
                for (int n = 0; n < io.InputQueueCharacters.Size; n++)
                {
                    unsigned int c = (unsigned int)io.InputQueueCharacters[n];
                    OnKeyPressed(c);
                }

            // Consume characters
            io.InputQueueCharacters.resize(0);
        }
    }

    // Release active ID at the end of the function (so e.g. pressing Return still does a final application of the value)
    // Otherwise request text input ahead for next frame.
    if (g.ActiveId == id && clear_active_id)
        ClearActiveID();

    const ImVec4 clip_rect(frame_bb.Min.x, frame_bb.Min.y, frame_bb.Min.x + inner_size.x, frame_bb.Min.y + inner_size.y); // Not using frame_bb.Max because we have adjusted size
    ImVec2 draw_pos = draw_window->DC.CursorPos;
    ImVec2 text_size(0.0f, 0.0f);

    // Set upper limit of single-line InputTextEx() at 2 million characters strings. The current pathological worst case is a long line
    // without any carriage return, which would makes ImFont::RenderText() reserve too many vertices and probably crash. Avoid it altogether.
    // Note that we only use this limit on single-line InputText(), so a pathologically large line on a InputTextMultiline() would still crash.
    const int buf_display_max_length = 2 * 1024 * 1024;


    ImVec2 cursor_offset, select_start_offset;

    // Find lines numbers straddling cursor and selection min position
    int cursor_line_no = vimCursorGetLine();
    int cursor_col_no = vimCursorGetColumn();
    int selmin_line_no =  -1;

    // Count lines and find line number for cursor and selection ends
    size_t line_count = vimBufferGetLineCount(vimBuf);
    if (cursor_line_no == -1)
        cursor_line_no = line_count;
    if (selmin_line_no == -1)
        selmin_line_no = line_count;

    // Calculate 2d position by finding the beginning of the line and measuring distance
    char_u* line = vimBufferGetLine(vimBuf, cursor_line_no);
    ImVec2 cursor_x_and_width = CalCursorXAndWidth(&g, line, cursor_col_no, vimGetMode());
    cursor_offset.x = cursor_x_and_width.x;
    cursor_offset.y = cursor_line_no * g.FontSize;
    if (selmin_line_no >= 0)
    {
        select_start_offset.x = 0;
        select_start_offset.y = selmin_line_no * g.FontSize;
    }

    // Store text height (note that we haven't calculated text width at all, see GitHub issues #383, #1224)
    text_size = ImVec2(inner_size.x, line_count * g.FontSize);

    if (state->CursorFollow)
    {
        // Horizontal scroll, move 1 / 10 of the inner size
        const float scroll_increment_x = inner_size.x * 0.1f;
        const float visible_width = inner_size.x - style.FramePadding.x - 2 * PADDING;
        if (cursor_offset.x < state->Scroll.x)
            state->Scroll.x = IM_TRUNC(ImMax(0.0f, cursor_offset.x - scroll_increment_x));
        else if (cursor_offset.x - visible_width >= state->Scroll.x)
            state->Scroll.x = IM_TRUNC(cursor_offset.x - visible_width + scroll_increment_x);

        // Vertical scroll
        if (cursor_offset.y - g.FontSize < scroll_y)
            scroll_y = ImMax(0.0f, cursor_offset.y - g.FontSize);
        else if (cursor_offset.y - (inner_size.y - style.FramePadding.y * 2.0f) >= scroll_y)
            scroll_y = cursor_offset.y - inner_size.y + style.FramePadding.y * 2.0f;
        const float scroll_max_y = ImMax((text_size.y + style.FramePadding.y * 2.0f) - inner_size.y, 0.0f);
        scroll_y = ImClamp(scroll_y, 0.0f, scroll_max_y);
        draw_pos.y += (draw_window->Scroll.y - scroll_y);   // Manipulate cursor pos immediately avoid a frame of lag
        draw_window->Scroll.y = scroll_y;

        state->CursorFollow = false;
    }

    // Draw hightlight line
    if (state->HighlightLine)
    {
        ImVec2 highlight_line_pos = ImTrunc(draw_pos + ImVec2(PADDING, (cursor_line_no - 1) * g.FontSize));
        ImRect highlight_line_rect(highlight_line_pos.x, highlight_line_pos.y, highlight_line_pos.x + inner_size.x - 2 * PADDING, highlight_line_pos.y + g.FontSize - 1.5f);
        if (highlight_line_rect.Overlaps(clip_rect))
            draw_window->DrawList->AddRectFilled(highlight_line_rect.Min, highlight_line_rect.Max, HIGHLIGHT_LINE_COLOR);
    }

    // Draw selection
    const ImVec2 draw_scroll = ImVec2(state->Scroll.x - PADDING, 0.0f);

    // Draw blinking cursor
    state->CursorAnim += io.DeltaTime;
    bool cursor_is_visible = (!g.IO.ConfigInputTextCursorBlink) || (state->CursorAnim <= 0.0f) || ImFmod(state->CursorAnim, 1.20f) <= 0.80f;
    ImVec2 cursor_screen_pos = ImTrunc(draw_pos + cursor_offset - draw_scroll);
    ImRect cursor_screen_rect(cursor_screen_pos.x, cursor_screen_pos.y - g.FontSize + 0.5f, cursor_screen_pos.x + 1.0f, cursor_screen_pos.y - 1.5f);
    if (cursor_is_visible && cursor_screen_rect.Overlaps(clip_rect))
        draw_window->DrawList->AddRectFilled(cursor_screen_rect.Min, ImVec2(cursor_screen_pos.x + cursor_x_and_width.y, cursor_screen_rect.GetBL().y), GetColorU32(ImGuiCol_InputTextCursor));

    if ((vimGetMode() & INSERT) == INSERT)
    {
        for (int cursorIndex = 0; cursorIndex < state->CursorCount; ++cursorIndex)
        {
            pos_T cursor = state->Cursors[cursorIndex];
            int cursor_line_no = cursor.lnum;
            int cursor_col_no = cursor.col;
            char_u* line = vimBufferGetLine(vimBuf, cursor_line_no);
            ImVec2 x_and_width = CalCursorXAndWidth(&g, line, cursor.col, vimGetMode());
            ImVec2 offset;
            offset.x = x_and_width.x;
            offset.y = cursor_line_no * g.FontSize;
            ImVec2 screen_pos = ImTrunc(draw_pos + offset - draw_scroll);
            ImRect screen_rect(screen_pos.x, screen_pos.y - g.FontSize + 0.5f, screen_pos.x + 1.0f, screen_pos.y - 1.5f);
            if (cursor_is_visible && screen_rect.Overlaps(clip_rect))
                draw_window->DrawList->AddRectFilled(screen_rect.Min, ImVec2(screen_pos.x + x_and_width.y, screen_rect.GetBL().y), GetColorU32(ImGuiCol_InputTextCursor));
        }
    }


    // Notify OS of text input position for advanced IME (-1 x offset so that Windows IME can cover our cursor. Bit of an extra nicety.)
    // This is required for some backends (SDL3) to start emitting character/text inputs.
    // As per #6341, make sure we don't set that on the deactivating frame.
    if (g.ActiveId == id)
    {
        ImGuiPlatformImeData* ime_data = &g.PlatformImeData; // (this is a public struct, passed to io.Platform_SetImeDataFn() handler)
        ime_data->WantVisible = true;
        ime_data->WantTextInput = true;
        ime_data->InputPos = ImVec2(cursor_screen_pos.x - 1.0f, cursor_screen_pos.y - g.FontSize);
        ime_data->InputLineHeight = g.FontSize;
        ime_data->ViewportId = window->Viewport->ID;
    }

    // We test for 'buf_display_max_length' as a way to avoid some pathological cases (e.g. single-line 1 MB string) which would make ImDrawList crash.
    // FIXME-OPT: Multiline could submit a smaller amount of contents to AddText() since we already iterated through it.
    ImU32 col = GetColorU32(ImGuiCol_Text);
    ImVec2 offset = ImVec2(0.0f, 0.0f);
    for (size_t line_no = 1; line_no <= line_count; line_no++)
    {
        // Get the line text
        char_u* line = vimBufferGetLine(vimBuf, (linenr_T)line_no);
        if (line == NULL)
            continue;
        ImVec2 line_pos = ImTrunc(draw_pos + ImVec2(0.0f, line_no * g.FontSize) - draw_scroll);

        // Render the text
        draw_window->DrawList->AddText(g.Font, g.FontSize, draw_pos - draw_scroll + offset, col, (const char*)line, (const char*)line + ImStrlen((char*)line), 0.0f, NULL);
        offset.y += g.FontSize;
    }

    // Draw Padding Rectangle
    draw_window->DrawList->AddRectFilled(frame_bb.Min, ImVec2(PADDING, frame_bb.Max.y), GetColorU32(ImGuiCol_FrameBg));
    draw_window->DrawList->AddRectFilled(ImVec2(frame_bb.Max.x - PADDING, frame_bb.Min.y), ImVec2(frame_bb.Max.x, frame_bb.Max.y), GetColorU32(ImGuiCol_FrameBg));

    // Draw Command Line
    if ((vimGetMode() & CMDLINE) == CMDLINE)
    {
        char_u* cmdline = vimCommandLineGetText();
        char_u cmdetype = vimCommandLineGetType();
        char_u* combined = (char_u*)alloca(ImStrlen((const char*)cmdline) + 2);
        combined[0] = cmdetype;
        ImStrncpy((char*)combined + 1, (const char*)cmdline, ImStrlen((const char*)cmdline) + 1);
        draw_window->DrawList->AddRectFilled(ImVec2(frame_bb.Min.x, frame_bb.Max.y - g.FontSize + 5), frame_bb.Max, GetColorU32(ImVec4(0.25f, 0.25f, 0.25f, 1.0f)));
        draw_window->DrawList->AddText(g.Font, g.FontSize - 5, ImVec2(frame_bb.Min.x + PADDING, frame_bb.Max.y - g.FontSize + 5 + style.FramePadding.y), GetColorU32(ImVec4(0.7f, 0.7f, 0.7f, 1.0f)), (const char*)combined, (const char*)combined + ImStrlen((char*)combined), 0.0f, NULL);
    }

    // For focus requests to work on our multiline we need to ensure our child ItemAdd() call specifies the ImGuiItemFlags_Inputable (see #4761, #7870)...
    Dummy(ImVec2(text_size.x, text_size.y + style.FramePadding.y));
    g.NextItemData.ItemFlags |= (ImGuiItemFlags)ImGuiItemFlags_Inputable | ImGuiItemFlags_NoTabStop;
    EndChild();
    item_data_backup.StatusFlags |= (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_HoveredWindow);

    // ...and then we need to undo the group overriding last item data, which gets a bit messy as EndGroup() tries to forward scrollbar being active...
    // FIXME: This quite messy/tricky, should attempt to get rid of the child window.
    EndGroup();
    if (g.LastItemData.ID == 0 || g.LastItemData.ID != GetWindowScrollbarID(draw_window, ImGuiAxis_Y))
    {
        g.LastItemData.ID = id;
        g.LastItemData.ItemFlags = item_data_backup.ItemFlags;
        g.LastItemData.StatusFlags = item_data_backup.StatusFlags;
    }

    if (label_size.x > 0)
        RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

    if (value_changed)
        MarkItemEdited(id);

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Inputable);
    if ((flags & ImGuiInputTextFlags_EnterReturnsTrue) != 0)
        return validated;
    else
        return value_changed;
}

ImVec2 CalCursorXAndWidth(ImGuiContext*ctx, char_u* line, int col, int mode)
{
    ImGuiContext& g = *ctx;
    ImFontBaked* baked = g.FontBaked;
    const float line_height = g.FontSize;
    const float scale = line_height / baked->Size;

    float cursor_x = 0;
    float cursor_width = 1.0f;
    float line_width = 0.0f;
    const char* text_end = (const char*)line + ImStrlen((char*)line);
    int current_col = 0;

    const char* s = (const char*)line;
    const char* cur_pos = s;
    unsigned int c = 97; // 'a' init value for empty text
    if ((mode & INSERT) == INSERT) col -= 1;
    while (s < text_end && current_col <= col)
    {
        c = (unsigned int)*s;
        if (c < 0x80)
            s += 1;
        else
            s += ImTextCharFromUtf8(&c, s, text_end);

        current_col += (s - cur_pos);
        cur_pos = s;
        line_width += baked->GetCharAdvance((ImWchar)c) * scale;
    }

    if (cursor_x < line_width)
        cursor_x = line_width;

    if ((mode & INSERT) == INSERT)
        cursor_width = 2.0f;
    else
    {
        cursor_width = baked->GetCharAdvance((ImWchar)c) * scale;
        cursor_x = cursor_x > cursor_width ? cursor_x - cursor_width : 0.0f;
    }
    return ImVec2(cursor_x, cursor_width);
}

void CopyToPasteBuffer()
{
    int line_count = vimBufferGetLineCount(currentBuffer);
    if (line_count > 0)
    {
        int buf_size = 0;
        for (int line_no = 1; line_no <= line_count; ++line_no)
        {
            char_u* line = vimBufferGetLine(currentBuffer, (linenr_T)line_no);
            if (line != NULL)
            {
                int line_length = ImStrlen((const char*)line);
                if (buf_size + line_length + 2 >= BUFFER_SIZE) // +2 for '\n' and '\0'
                    break;
                memcpy(state->PasteBuffer + buf_size, (const char*)line, line_length);
                buf_size += line_length;
                if (line_no < line_count) state->PasteBuffer[buf_size++] = '\n';
            }
        }
        state->PasteBuffer[buf_size] = '\0';
        state->PasteBuffer[BUFFER_SIZE - 1] = '\0';
    }
}

void PasteTextToPreviousFocus(const char* text) {
    if (!IsWindow(g_previousFocus))
        return;

    // activate target window
    SetForegroundWindow(g_previousFocus);

    // Set clipboard content
    ImGui::SetClipboardText(text);

    // Simulate Ctrl+V paste
    INPUT inputs[4] = {};

    // Ctrl down
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_CONTROL;

    // V down
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = 'V';

    // V up
    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.wVk = 'V';
    inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;

    // Ctrl up
    inputs[3].type = INPUT_KEYBOARD;
    inputs[3].ki.wVk = VK_CONTROL;
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(4, inputs, sizeof(INPUT));
    ShowWindow(state->hwnd, SW_HIDE);
}

void DisableIME(bool disable)
{
    HIMC hIMC = ImmGetContext(state->hwnd);
    if (hIMC) {
        ImmSetOpenStatus(hIMC, !disable);
        ImmReleaseContext(state->hwnd, hIMC);
    }
}

void ResetCurrentBuffer()
{
    vimKey((char_u*)"<Esc>");
    vimKey((char_u*)"<Esc>");
    vimInput((char_u*)"g");
    vimInput((char_u*)"g");
    vimInput((char_u*)"d");
    vimInput((char_u*)"G");
}

void OnWriteCallback()
{
    CopyToPasteBuffer();
    PasteTextToPreviousFocus(state->PasteBuffer);
    state->BufferNeedSwitch = true;
    state->BufferNeedReset = true;
}

void OnBufferPreviousCallback()
{
    state->BufferNeedSwitch = true;
}

void OnCursorAdd(pos_T cursor)
{
    if (state->CursorCount < 1024) {
        state->Cursors[state->CursorCount++] = cursor;
        state->CursorCountNeedReset = true;
    }
}

}
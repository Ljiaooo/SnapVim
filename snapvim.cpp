#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include <vim.h>
#endif

#include <windows.h>
#include <lm.h>
#include "snapvim.h"
#include "imgui_internal.h"

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

void initSnapVim()
{
    vimInit(0, nullptr);
    vimOptionSetInsertSpaces(TRUE);
    vimOptionSetTabSize(2);

    // initially create two buffers, one for the current text and one for the history
    buffer1 = vimBufferOpen((char_u*)"", 1, 0);
    buffer0 = vimBufferOpen((char_u*)"", 1, 0);
    currentBuffer = buffer0;

    state = new SnapVimState();
}

void renderSnapVimEditor(char* textBuffer, int winWidth, int winHeight, ImGuiWindowFlags window_flags)
{
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

    if (ImGui::Begin("InvisibleWindow", nullptr, window_flags)) {
       //ImGui::InputTextMultiline("###SnapVim", textBuffer, BUFFER_SIZE, ImVec2(winWidth, winHeight), ImGuiInputTextFlags_AllowTabInput);
       SnapVim::snapVimEditor(textBuffer, BUFFER_SIZE, ImVec2(winWidth, winHeight));
    }
    ImGui::End();
}

bool snapVimEditor(char* buf, int buf_size, const ImVec2& size_arg)
{
    using namespace ImGui;
    ImGuiWindow* window = GetCurrentWindow();

    IM_ASSERT(buf != NULL && buf_size >= 0);

    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;
    io.ConfigNavEscapeClearFocusItem = false;
    io.ConfigNavEscapeClearFocusWindow = false;
    const ImGuiStyle& style = g.Style;

    const bool RENDER_SELECTION_WHEN_INACTIVE = false;
    const char* label = "###SnapVim";
    int flags = 0;

    BeginGroup();
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImVec2 frame_size = CalcItemSize(size_arg, CalcItemWidth(), ( g.FontSize * 8.0f) + style.FramePadding.y * 2.0f); // Arbitrary default of 8 lines high for multi-line
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

    // Prevent NavActivate reactivating in BeginChild() when we are already active.
    const ImGuiID backup_activate_id = g.NavActivateId;
    if (g.ActiveId == id) // Prevent reactivation
        g.NavActivateId = 0;

    // We reproduce the contents of BeginChildFrame() in order to provide 'label' so our window internal data are easier to read/debug.
    PushStyleColor(ImGuiCol_ChildBg, style.Colors[ImGuiCol_FrameBg]);
    PushStyleVar(ImGuiStyleVar_ChildRounding, style.FrameRounding);
    PushStyleVar(ImGuiStyleVar_ChildBorderSize, style.FrameBorderSize);
    PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); // Ensure no clip rect so mouse hover can reach FramePadding edges
    bool child_visible = BeginChildEx(label, id, frame_bb.GetSize(), ImGuiChildFlags_Borders, ImGuiWindowFlags_NoMove);
    g.NavActivateId = backup_activate_id;
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
    inner_size.x -= draw_window->ScrollbarSizes.x;

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

    //const bool init_reload_from_user_buf = (state != NULL && state->WantReloadUserBuf);
    const bool init_changed_specs = (state != NULL );// state != NULL means its our state.

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
    bool render_cursor = (g.ActiveId == id) || (state && user_scroll_active);
    //bool render_selection = state && (state->HasSelection() || select_all) && (RENDER_SELECTION_WHEN_INACTIVE || render_cursor);
    bool value_changed = false;
    bool validated = false;

    // Select the buffer to render.
    //const bool buf_display_from_state = (render_cursor || render_selection || g.ActiveId == id) && state;

    // Process mouse inputs and character inputs
    if (g.ActiveId == id)
    {
        // Process regular text input (before we check for Return because using some IME will effectively send a Return?)
        // We ignore CTRL inputs, but need to allow ALT+CTRL as some keyboards (e.g. German) use AltGR (which _is_ Alt+Ctrl) to input certain characters.
        if (Shortcut(ImGuiKey_Escape, 0, id)) vimKey((char_u*)"<Esc>");
        const bool ignore_char_inputs = (io.KeyCtrl && !io.KeyAlt) || (is_osx && io.KeyCtrl);
        if (io.InputQueueCharacters.Size > 0)
        {
            if (!ignore_char_inputs)
                for (int n = 0; n < io.InputQueueCharacters.Size; n++)
                {
                    unsigned int c = (unsigned int)io.InputQueueCharacters[n];
                    char utf[5];
                    ImTextCharToUtf8(utf, c);
                    vimInput((char_u*)utf);
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
    const char* buf_display = (char*)vimBufferGetLine(vimBuf, 0); //-V595
    const char* buf_display_end = NULL; // We have specialized paths below for setting the length

    // Render text. We currently only render selection when the widget is active or while scrolling.
    // FIXME: We could remove the '&& render_cursor' to keep rendering selection when inactive.

    // TODO, do not delete
    //if (true)
    {
        IM_ASSERT(state != NULL);
        buf_display_end = buf_display + ImStrlen((char*)buf_display);

        // Render text (with cursor and selection)
        // This is going to be messy. We need to:
        // - Display the text (this alone can be more easily clipped)
        // - Handle scrolling, highlight selection, display cursor (those all requires some form of 1d->2d cursor position calculation)
        // - Measure text height (for scrollbar)
        // We are attempting to do most of that in **one main pass** to minimize the computation cost (non-negligible for large amount of text) + 2nd pass for selection rendering (we could merge them by an extra refactoring effort)
        // FIXME: This should occur on buf_display but we'd need to maintain cursor/select_start/select_end for UTF-8.
        const char* text_begin = buf_display;
        const char* text_end = buf_display_end;
        ImVec2 cursor_offset, select_start_offset;

        {
            // Find lines numbers straddling cursor and selection min position
            int cursor_line_no = -1;
            int selmin_line_no =  -1;
            const char* cursor_ptr = text_begin;
            const char* selmin_ptr = text_begin;

            // Count lines and find line number for cursor and selection ends
            int line_count = 1;
            for (const char* s = text_begin; (s = (const char*)ImMemchr(s, '\n', (size_t)(text_end - s))) != NULL; s++)
            {
                if (cursor_line_no == -1 && s >= cursor_ptr) { cursor_line_no = line_count; }
                if (selmin_line_no == -1 && s >= selmin_ptr) { selmin_line_no = line_count; }
                line_count++;
            }
            if (cursor_line_no == -1)
                cursor_line_no = line_count;
            if (selmin_line_no == -1)
                selmin_line_no = line_count;

            // Calculate 2d position by finding the beginning of the line and measuring distance
            //cursor_offset.x = InputTextCalcTextSize(&g, ImStrbol(cursor_ptr, text_begin), cursor_ptr).x;
            cursor_offset.x = 0;
            cursor_offset.y = cursor_line_no * g.FontSize;
            if (selmin_line_no >= 0)
            {
                //select_start_offset.x = InputTextCalcTextSize(&g, ImStrbol(selmin_ptr, text_begin), selmin_ptr).x;
                select_start_offset.x = 0;
                select_start_offset.y = selmin_line_no * g.FontSize;
            }

            // Store text height (note that we haven't calculated text width at all, see GitHub issues #383, #1224)
            text_size = ImVec2(inner_size.x, line_count * g.FontSize);
        }

        // Scroll
        if (render_cursor && state->CursorFollow)
        {
            // Horizontal scroll in chunks of quarter width
            if (!(flags & ImGuiInputTextFlags_NoHorizontalScroll))
            {
                const float scroll_increment_x = inner_size.x * 0.25f;
                const float visible_width = inner_size.x - style.FramePadding.x;
                if (cursor_offset.x < state->Scroll.x)
                    state->Scroll.x = IM_TRUNC(ImMax(0.0f, cursor_offset.x - scroll_increment_x));
                else if (cursor_offset.x - visible_width >= state->Scroll.x)
                    state->Scroll.x = IM_TRUNC(cursor_offset.x - visible_width + scroll_increment_x);
            }
            else
            {
                state->Scroll.y = 0.0f;
            }

            // Vertical scroll
            // Test if cursor is vertically visible
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

        // Draw selection
        const ImVec2 draw_scroll = ImVec2(state->Scroll.x, 0.0f);

        // We test for 'buf_display_max_length' as a way to avoid some pathological cases (e.g. single-line 1 MB string) which would make ImDrawList crash.
        // FIXME-OPT: Multiline could submit a smaller amount of contents to AddText() since we already iterated through it.
        ImU32 col = GetColorU32(ImGuiCol_Text);
        draw_window->DrawList->AddText(g.Font, g.FontSize, draw_pos - draw_scroll, col, buf_display, buf_display_end, 0.0f, NULL);

        // Draw blinking cursor
        if (render_cursor)
        {
            state->CursorAnim += io.DeltaTime;
            bool cursor_is_visible = (!g.IO.ConfigInputTextCursorBlink) || (state->CursorAnim <= 0.0f) || ImFmod(state->CursorAnim, 1.20f) <= 0.80f;
            ImVec2 cursor_screen_pos = ImTrunc(draw_pos + cursor_offset - draw_scroll);
            ImRect cursor_screen_rect(cursor_screen_pos.x, cursor_screen_pos.y - g.FontSize + 0.5f, cursor_screen_pos.x + 1.0f, cursor_screen_pos.y - 1.5f);
            if (cursor_is_visible && cursor_screen_rect.Overlaps(clip_rect))
                draw_window->DrawList->AddLine(cursor_screen_rect.Min, cursor_screen_rect.GetBL(), GetColorU32(ImGuiCol_InputTextCursor), 1.0f); // FIXME-DPI: Cursor thickness (#7031)

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
        }
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

    // Log as text
    if (g.LogEnabled)
    {
        LogSetNextTextDecoration("{", "}");
        LogRenderedText(&draw_pos, buf_display, buf_display_end);
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

}
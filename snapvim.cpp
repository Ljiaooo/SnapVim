#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS

#include "svimconfig.h"
#endif

#include "snapvim.h"
#include "imgui_internal.h"


namespace SnapVim {

void initVim()
{

}

void renderSnapVimEditor(char* textBuffer, int winWidth, int winHeight, ImGuiWindowFlags window_flags)
{
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

    if (ImGui::Begin("InvisibleWindow", nullptr, window_flags)) {
       ImGui::InputTextMultiline("###SnapVim", textBuffer, BUFFER_SIZE, ImVec2(winWidth, winHeight), ImGuiInputTextFlags_AllowTabInput);
    }
    ImGui::End();
}

//bool snapVimEditor(const char* label, char* buf, int buf_size, const ImVec2& size_arg, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* callback_user_data)
//{
//    using namespace ImGui;
//    ImGuiWindow* window = GetCurrentWindow();
//
//    IM_ASSERT(buf != NULL && buf_size >= 0);
//    IM_ASSERT(!((flags & ImGuiInputTextFlags_CallbackHistory) && (flags & ImGuiInputTextFlags_Multiline)));        // Can't use both together (they both use up/down keys)
//    IM_ASSERT(!((flags & ImGuiInputTextFlags_CallbackCompletion) && (flags & ImGuiInputTextFlags_AllowTabInput))); // Can't use both together (they both use tab key)
//    IM_ASSERT(!((flags & ImGuiInputTextFlags_ElideLeft) && (flags & ImGuiInputTextFlags_Multiline)));               // Multiline will not work with left-trimming
//
//    ImGuiContext& g = *GImGui;
//    ImGuiIO& io = g.IO;
//    const ImGuiStyle& style = g.Style;
//
//    const bool RENDER_SELECTION_WHEN_INACTIVE = false;
//    const bool is_multiline = (flags & ImGuiInputTextFlags_Multiline) != 0;
//
//    if (is_multiline) // Open group before calling GetID() because groups tracks id created within their scope (including the scrollbar)
//        BeginGroup();
//    const ImGuiID id = window->GetID(label);
//    const ImVec2 label_size = CalcTextSize(label, NULL, true);
//    const ImVec2 frame_size = CalcItemSize(size_arg, CalcItemWidth(), (is_multiline ? g.FontSize * 8.0f : label_size.y) + style.FramePadding.y * 2.0f); // Arbitrary default of 8 lines high for multi-line
//    const ImVec2 total_size = ImVec2(frame_size.x + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), frame_size.y);
//
//    const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + frame_size);
//    const ImRect total_bb(frame_bb.Min, frame_bb.Min + total_size);
//
//    ImGuiWindow* draw_window = window;
//    ImVec2 inner_size = frame_size;
//    ImGuiLastItemData item_data_backup;
//    if (is_multiline)
//    {
//        ImVec2 backup_pos = window->DC.CursorPos;
//        ItemSize(total_bb, style.FramePadding.y);
//        if (!ItemAdd(total_bb, id, &frame_bb, ImGuiItemFlags_Inputable))
//        {
//            EndGroup();
//            return false;
//        }
//        item_data_backup = g.LastItemData;
//        window->DC.CursorPos = backup_pos;
//
//        // Prevent NavActivation from Tabbing when our widget accepts Tab inputs: this allows cycling through widgets without stopping.
//        if (g.NavActivateId == id && (g.NavActivateFlags & ImGuiActivateFlags_FromTabbing) && (flags & ImGuiInputTextFlags_AllowTabInput))
//            g.NavActivateId = 0;
//
//        // Prevent NavActivate reactivating in BeginChild() when we are already active.
//        const ImGuiID backup_activate_id = g.NavActivateId;
//        if (g.ActiveId == id) // Prevent reactivation
//            g.NavActivateId = 0;
//
//        // We reproduce the contents of BeginChildFrame() in order to provide 'label' so our window internal data are easier to read/debug.
//        PushStyleColor(ImGuiCol_ChildBg, style.Colors[ImGuiCol_FrameBg]);
//        PushStyleVar(ImGuiStyleVar_ChildRounding, style.FrameRounding);
//        PushStyleVar(ImGuiStyleVar_ChildBorderSize, style.FrameBorderSize);
//        PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); // Ensure no clip rect so mouse hover can reach FramePadding edges
//        bool child_visible = BeginChildEx(label, id, frame_bb.GetSize(), ImGuiChildFlags_Borders, ImGuiWindowFlags_NoMove);
//        g.NavActivateId = backup_activate_id;
//        PopStyleVar(3);
//        PopStyleColor();
//        if (!child_visible)
//        {
//            EndChild();
//            EndGroup();
//            return false;
//        }
//        draw_window = g.CurrentWindow; // Child window
//        draw_window->DC.NavLayersActiveMaskNext |= (1 << draw_window->DC.NavLayerCurrent); // This is to ensure that EndChild() will display a navigation highlight so we can "enter" into it.
//        draw_window->DC.CursorPos += style.FramePadding;
//        inner_size.x -= draw_window->ScrollbarSizes.x;
//    }
//    else
//    {
//        // Support for internal ImGuiInputTextFlags_MergedItem flag, which could be redesigned as an ItemFlags if needed (with test performed in ItemAdd)
//        ItemSize(total_bb, style.FramePadding.y);
//        if (!(flags & ImGuiInputTextFlags_MergedItem))
//            if (!ItemAdd(total_bb, id, &frame_bb, ImGuiItemFlags_Inputable))
//                return false;
//    }
//
//    // Ensure mouse cursor is set even after switching to keyboard/gamepad mode. May generalize further? (#6417)
//    bool hovered = ItemHoverable(frame_bb, id, g.LastItemData.ItemFlags | ImGuiItemFlags_NoNavDisableMouseHover);
//    if (hovered)
//        SetMouseCursor(ImGuiMouseCursor_TextInput);
//    if (hovered && g.NavHighlightItemUnderNav)
//        hovered = false;
//
//    // We are only allowed to access the state if we are already the active widget.
//    ImGuiInputTextState* state = GetInputTextState(id);
//
//    if (g.LastItemData.ItemFlags & ImGuiItemFlags_ReadOnly)
//        flags |= ImGuiInputTextFlags_ReadOnly;
//    const bool is_readonly = (flags & ImGuiInputTextFlags_ReadOnly) != 0;
//    const bool is_password = (flags & ImGuiInputTextFlags_Password) != 0;
//    const bool is_undoable = (flags & ImGuiInputTextFlags_NoUndoRedo) == 0;
//    const bool is_resizable = (flags & ImGuiInputTextFlags_CallbackResize) != 0;
//    if (is_resizable)
//        IM_ASSERT(callback != NULL); // Must provide a callback if you set the ImGuiInputTextFlags_CallbackResize flag!
//
//    const bool input_requested_by_nav = (g.ActiveId != id) && ((g.NavActivateId == id) && ((g.NavActivateFlags & ImGuiActivateFlags_PreferInput) || (g.NavInputSource == ImGuiInputSource_Keyboard)));
//
//    const bool user_clicked = hovered && io.MouseClicked[0];
//    const bool user_scroll_finish = is_multiline && state != NULL && g.ActiveId == 0 && g.ActiveIdPreviousFrame == GetWindowScrollbarID(draw_window, ImGuiAxis_Y);
//    const bool user_scroll_active = is_multiline && state != NULL && g.ActiveId == GetWindowScrollbarID(draw_window, ImGuiAxis_Y);
//    bool clear_active_id = false;
//    bool select_all = false;
//
//    float scroll_y = is_multiline ? draw_window->Scroll.y : FLT_MAX;
//
//    const bool init_reload_from_user_buf = (state != NULL && state->WantReloadUserBuf);
//    const bool init_changed_specs = (state != NULL && state->Stb->single_line != !is_multiline); // state != NULL means its our state.
//    const bool init_make_active = (user_clicked || user_scroll_finish || input_requested_by_nav);
//    const bool init_state = (init_make_active || user_scroll_active);
//    if (init_reload_from_user_buf)
//    {
//        int new_len = (int)ImStrlen(buf);
//        IM_ASSERT(new_len + 1 <= buf_size && "Is your input buffer properly zero-terminated?");
//        state->WantReloadUserBuf = false;
//        InputTextReconcileUndoState(state, state->TextA.Data, state->TextLen, buf, new_len);
//        state->TextA.resize(buf_size + 1); // we use +1 to make sure that .Data is always pointing to at least an empty string.
//        state->TextLen = new_len;
//        memcpy(state->TextA.Data, buf, state->TextLen + 1);
//        state->Stb->select_start = state->ReloadSelectionStart;
//        state->Stb->cursor = state->Stb->select_end = state->ReloadSelectionEnd;
//        state->CursorClamp();
//    }
//    else if ((init_state && g.ActiveId != id) || init_changed_specs)
//    {
//        // Access state even if we don't own it yet.
//        state = &g.InputTextState;
//        state->CursorAnimReset();
//
//        // Backup state of deactivating item so they'll have a chance to do a write to output buffer on the same frame they report IsItemDeactivatedAfterEdit (#4714)
//        InputTextDeactivateHook(state->ID);
//
//        // Take a copy of the initial buffer value.
//        // From the moment we focused we are normally ignoring the content of 'buf' (unless we are in read-only mode)
//        const int buf_len = (int)ImStrlen(buf);
//        IM_ASSERT(buf_len + 1 <= buf_size && "Is your input buffer properly zero-terminated?");
//        state->TextToRevertTo.resize(buf_len + 1);    // UTF-8. we use +1 to make sure that .Data is always pointing to at least an empty string.
//        memcpy(state->TextToRevertTo.Data, buf, buf_len + 1);
//
//        // Preserve cursor position and undo/redo stack if we come back to same widget
//        // FIXME: Since we reworked this on 2022/06, may want to differentiate recycle_cursor vs recycle_undostate?
//        bool recycle_state = (state->ID == id && !init_changed_specs);
//        if (recycle_state && (state->TextLen != buf_len || (state->TextA.Data == NULL || strncmp(state->TextA.Data, buf, buf_len) != 0)))
//            recycle_state = false;
//
//        // Start edition
//        state->ID = id;
//        state->TextLen = buf_len;
//        if (!is_readonly)
//        {
//            state->TextA.resize(buf_size + 1); // we use +1 to make sure that .Data is always pointing to at least an empty string.
//            memcpy(state->TextA.Data, buf, state->TextLen + 1);
//        }
//
//        // Find initial scroll position for right alignment
//        state->Scroll = ImVec2(0.0f, 0.0f);
//        if (flags & ImGuiInputTextFlags_ElideLeft)
//            state->Scroll.x += ImMax(0.0f, CalcTextSize(buf).x - frame_size.x + style.FramePadding.x * 2.0f);
//
//        // Recycle existing cursor/selection/undo stack but clamp position
//        // Note a single mouse click will override the cursor/position immediately by calling stb_textedit_click handler.
//        if (recycle_state)
//            state->CursorClamp();
//        else
//            stb_textedit_initialize_state(state->Stb, !is_multiline);
//
//        if (!is_multiline)
//        {
//            if (flags & ImGuiInputTextFlags_AutoSelectAll)
//                select_all = true;
//            if (input_requested_by_nav && (!recycle_state || !(g.NavActivateFlags & ImGuiActivateFlags_TryToPreserveState)))
//                select_all = true;
//            if (user_clicked && io.KeyCtrl)
//                select_all = true;
//        }
//
//        if (flags & ImGuiInputTextFlags_AlwaysOverwrite)
//            state->Stb->insert_mode = 1; // stb field name is indeed incorrect (see #2863)
//    }
//
//    const bool is_osx = io.ConfigMacOSXBehaviors;
//    if (g.ActiveId != id && init_make_active)
//    {
//        IM_ASSERT(state && state->ID == id);
//        SetActiveID(id, window);
//        SetFocusID(id, window);
//        FocusWindow(window);
//    }
//    if (g.ActiveId == id)
//    {
//        // Declare some inputs, the other are registered and polled via Shortcut() routing system.
//        // FIXME: The reason we don't use Shortcut() is we would need a routing flag to specify multiple mods, or to all mods combination into individual shortcuts.
//        const ImGuiKey always_owned_keys[] = { ImGuiKey_LeftArrow, ImGuiKey_RightArrow, ImGuiKey_Enter, ImGuiKey_KeypadEnter, ImGuiKey_Delete, ImGuiKey_Backspace, ImGuiKey_Home, ImGuiKey_End };
//        for (ImGuiKey key : always_owned_keys)
//            SetKeyOwner(key, id);
//        if (user_clicked)
//            SetKeyOwner(ImGuiKey_MouseLeft, id);
//        g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
//        if (is_multiline || (flags & ImGuiInputTextFlags_CallbackHistory))
//        {
//            g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Up) | (1 << ImGuiDir_Down);
//            SetKeyOwner(ImGuiKey_UpArrow, id);
//            SetKeyOwner(ImGuiKey_DownArrow, id);
//        }
//        if (is_multiline)
//        {
//            SetKeyOwner(ImGuiKey_PageUp, id);
//            SetKeyOwner(ImGuiKey_PageDown, id);
//        }
//        // FIXME: May be a problem to always steal Alt on OSX, would ideally still allow an uninterrupted Alt down-up to toggle menu
//        if (is_osx)
//            SetKeyOwner(ImGuiMod_Alt, id);
//
//        // Expose scroll in a manner that is agnostic to us using a child window
//        if (is_multiline && state != NULL)
//            state->Scroll.y = draw_window->Scroll.y;
//
//        // Read-only mode always ever read from source buffer. Refresh TextLen when active.
//        if (is_readonly && state != NULL)
//            state->TextLen = (int)ImStrlen(buf);
//        //if (is_readonly && state != NULL)
//        //    state->TextA.clear(); // Uncomment to facilitate debugging, but we otherwise prefer to keep/amortize th allocation.
//    }
//    if (state != NULL)
//        state->TextSrc = is_readonly ? buf : state->TextA.Data;
//
//    // We have an edge case if ActiveId was set through another widget (e.g. widget being swapped), clear id immediately (don't wait until the end of the function)
//    if (g.ActiveId == id && state == NULL)
//        ClearActiveID();
//
//    // Release focus when we click outside
//    if (g.ActiveId == id && io.MouseClicked[0] && !init_state && !init_make_active) //-V560
//        clear_active_id = true;
//
//    // Lock the decision of whether we are going to take the path displaying the cursor or selection
//    bool render_cursor = (g.ActiveId == id) || (state && user_scroll_active);
//    bool render_selection = state && (state->HasSelection() || select_all) && (RENDER_SELECTION_WHEN_INACTIVE || render_cursor);
//    bool value_changed = false;
//    bool validated = false;
//
//    // Select the buffer to render.
//    const bool buf_display_from_state = (render_cursor || render_selection || g.ActiveId == id) && !is_readonly && state;
//
//    // Process mouse inputs and character inputs
//    if (g.ActiveId == id)
//    {
//        IM_ASSERT(state != NULL);
//        state->Edited = false;
//        state->BufCapacity = buf_size;
//        state->Flags = flags;
//
//        // Although we are active we don't prevent mouse from hovering other elements unless we are interacting right now with the widget.
//        // Down the line we should have a cleaner library-wide concept of Selected vs Active.
//        g.ActiveIdAllowOverlap = !io.MouseDown[0];
//
//        // Edit in progress
//        const float mouse_x = (io.MousePos.x - frame_bb.Min.x - style.FramePadding.x) + state->Scroll.x;
//        const float mouse_y = (is_multiline ? (io.MousePos.y - draw_window->DC.CursorPos.y) : (g.FontSize * 0.5f));
//
//        if (select_all)
//        {
//            state->SelectAll();
//            state->SelectedAllMouseLock = true;
//        }
//        else if (hovered && io.MouseClickedCount[0] >= 2 && !io.KeyShift)
//        {
//            stb_textedit_click(state, state->Stb, mouse_x, mouse_y);
//            const int multiclick_count = (io.MouseClickedCount[0] - 2);
//            if ((multiclick_count % 2) == 0)
//            {
//                // Double-click: Select word
//                // We always use the "Mac" word advance for double-click select vs CTRL+Right which use the platform dependent variant:
//                // FIXME: There are likely many ways to improve this behavior, but there's no "right" behavior (depends on use-case, software, OS)
//                const bool is_bol = (state->Stb->cursor == 0) || ImStb::STB_TEXTEDIT_GETCHAR(state, state->Stb->cursor - 1) == '\n';
//                if (STB_TEXT_HAS_SELECTION(state->Stb) || !is_bol)
//                    state->OnKeyPressed(STB_TEXTEDIT_K_WORDLEFT);
//                //state->OnKeyPressed(STB_TEXTEDIT_K_WORDRIGHT | STB_TEXTEDIT_K_SHIFT);
//                if (!STB_TEXT_HAS_SELECTION(state->Stb))
//                    ImStb::stb_textedit_prep_selection_at_cursor(state->Stb);
//                state->Stb->cursor = ImStb::STB_TEXTEDIT_MOVEWORDRIGHT_MAC(state, state->Stb->cursor);
//                state->Stb->select_end = state->Stb->cursor;
//                ImStb::stb_textedit_clamp(state, state->Stb);
//            }
//            else
//            {
//                // Triple-click: Select line
//                const bool is_eol = ImStb::STB_TEXTEDIT_GETCHAR(state, state->Stb->cursor) == '\n';
//                state->OnKeyPressed(STB_TEXTEDIT_K_LINESTART);
//                state->OnKeyPressed(STB_TEXTEDIT_K_LINEEND | STB_TEXTEDIT_K_SHIFT);
//                state->OnKeyPressed(STB_TEXTEDIT_K_RIGHT | STB_TEXTEDIT_K_SHIFT);
//                if (!is_eol && is_multiline)
//                {
//                    ImSwap(state->Stb->select_start, state->Stb->select_end);
//                    state->Stb->cursor = state->Stb->select_end;
//                }
//                state->CursorFollow = false;
//            }
//            state->CursorAnimReset();
//        }
//        else if (io.MouseClicked[0] && !state->SelectedAllMouseLock)
//        {
//            if (hovered)
//            {
//                if (io.KeyShift)
//                    stb_textedit_drag(state, state->Stb, mouse_x, mouse_y);
//                else
//                    stb_textedit_click(state, state->Stb, mouse_x, mouse_y);
//                state->CursorAnimReset();
//            }
//        }
//        else if (io.MouseDown[0] && !state->SelectedAllMouseLock && (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f))
//        {
//            stb_textedit_drag(state, state->Stb, mouse_x, mouse_y);
//            state->CursorAnimReset();
//            state->CursorFollow = true;
//        }
//        if (state->SelectedAllMouseLock && !io.MouseDown[0])
//            state->SelectedAllMouseLock = false;
//
//        // We expect backends to emit a Tab key but some also emit a Tab character which we ignore (#2467, #1336)
//        // (For Tab and Enter: Win32/SFML/Allegro are sending both keys and chars, GLFW and SDL are only sending keys. For Space they all send all threes)
//        if ((flags & ImGuiInputTextFlags_AllowTabInput) && !is_readonly)
//        {
//            if (Shortcut(ImGuiKey_Tab, ImGuiInputFlags_Repeat, id))
//            {
//                unsigned int c = '\t'; // Insert TAB
//                if (InputTextFilterCharacter(&g, &c, flags, callback, callback_user_data))
//                    state->OnCharPressed(c);
//            }
//            // FIXME: Implement Shift+Tab
//            /*
//            if (Shortcut(ImGuiKey_Tab | ImGuiMod_Shift, ImGuiInputFlags_Repeat, id))
//            {
//            }
//            */
//        }
//
//        // Process regular text input (before we check for Return because using some IME will effectively send a Return?)
//        // We ignore CTRL inputs, but need to allow ALT+CTRL as some keyboards (e.g. German) use AltGR (which _is_ Alt+Ctrl) to input certain characters.
//        const bool ignore_char_inputs = (io.KeyCtrl && !io.KeyAlt) || (is_osx && io.KeyCtrl);
//        if (io.InputQueueCharacters.Size > 0)
//        {
//            if (!ignore_char_inputs && !is_readonly && !input_requested_by_nav)
//                for (int n = 0; n < io.InputQueueCharacters.Size; n++)
//                {
//                    // Insert character if they pass filtering
//                    unsigned int c = (unsigned int)io.InputQueueCharacters[n];
//                    if (c == '\t') // Skip Tab, see above.
//                        continue;
//                    if (InputTextFilterCharacter(&g, &c, flags, callback, callback_user_data))
//                        state->OnCharPressed(c);
//                }
//
//            // Consume characters
//            io.InputQueueCharacters.resize(0);
//        }
//    }
//
//    // Process other shortcuts/key-presses
//    bool revert_edit = false;
//    if (g.ActiveId == id && !g.ActiveIdIsJustActivated && !clear_active_id)
//    {
//        IM_ASSERT(state != NULL);
//
//        const int row_count_per_page = ImMax((int)((inner_size.y - style.FramePadding.y) / g.FontSize), 1);
//        state->Stb->row_count_per_page = row_count_per_page;
//
//        const int k_mask = (io.KeyShift ? STB_TEXTEDIT_K_SHIFT : 0);
//        const bool is_wordmove_key_down = is_osx ? io.KeyAlt : io.KeyCtrl;                     // OS X style: Text editing cursor movement using Alt instead of Ctrl
//        const bool is_startend_key_down = is_osx && io.KeyCtrl && !io.KeySuper && !io.KeyAlt;  // OS X style: Line/Text Start and End using Cmd+Arrows instead of Home/End
//
//        // Using Shortcut() with ImGuiInputFlags_RouteFocused (default policy) to allow routing operations for other code (e.g. calling window trying to use CTRL+A and CTRL+B: former would be handled by InputText)
//        // Otherwise we could simply assume that we own the keys as we are active.
//        const ImGuiInputFlags f_repeat = ImGuiInputFlags_Repeat;
//        const bool is_cut   = (Shortcut(ImGuiMod_Ctrl | ImGuiKey_X, f_repeat, id) || Shortcut(ImGuiMod_Shift | ImGuiKey_Delete, f_repeat, id)) && !is_readonly && !is_password && (!is_multiline || state->HasSelection());
//        const bool is_copy  = (Shortcut(ImGuiMod_Ctrl | ImGuiKey_C, 0,        id) || Shortcut(ImGuiMod_Ctrl  | ImGuiKey_Insert, 0,        id)) && !is_password && (!is_multiline || state->HasSelection());
//        const bool is_paste = (Shortcut(ImGuiMod_Ctrl | ImGuiKey_V, f_repeat, id) || Shortcut(ImGuiMod_Shift | ImGuiKey_Insert, f_repeat, id)) && !is_readonly;
//        const bool is_undo  = (Shortcut(ImGuiMod_Ctrl | ImGuiKey_Z, f_repeat, id)) && !is_readonly && is_undoable;
//        const bool is_redo =  (Shortcut(ImGuiMod_Ctrl | ImGuiKey_Y, f_repeat, id) || Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z, f_repeat, id)) && !is_readonly && is_undoable;
//        const bool is_select_all = Shortcut(ImGuiMod_Ctrl | ImGuiKey_A, 0, id);
//
//        // We allow validate/cancel with Nav source (gamepad) to makes it easier to undo an accidental NavInput press with no keyboard wired, but otherwise it isn't very useful.
//        const bool nav_gamepad_active = (io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) != 0 && (io.BackendFlags & ImGuiBackendFlags_HasGamepad) != 0;
//        const bool is_enter_pressed = IsKeyPressed(ImGuiKey_Enter, true) || IsKeyPressed(ImGuiKey_KeypadEnter, true);
//        const bool is_gamepad_validate = nav_gamepad_active && (IsKeyPressed(ImGuiKey_NavGamepadActivate, false) || IsKeyPressed(ImGuiKey_NavGamepadInput, false));
//        const bool is_cancel = Shortcut(ImGuiKey_Escape, f_repeat, id) || (nav_gamepad_active && Shortcut(ImGuiKey_NavGamepadCancel, f_repeat, id));
//
//        // FIXME: Should use more Shortcut() and reduce IsKeyPressed()+SetKeyOwner(), but requires modifiers combination to be taken account of.
//        // FIXME-OSX: Missing support for Alt(option)+Right/Left = go to end of line, or next line if already in end of line.
//        if (IsKeyPressed(ImGuiKey_LeftArrow))                        { state->OnKeyPressed((is_startend_key_down ? STB_TEXTEDIT_K_LINESTART : is_wordmove_key_down ? STB_TEXTEDIT_K_WORDLEFT : STB_TEXTEDIT_K_LEFT) | k_mask); }
//        else if (IsKeyPressed(ImGuiKey_RightArrow))                  { state->OnKeyPressed((is_startend_key_down ? STB_TEXTEDIT_K_LINEEND : is_wordmove_key_down ? STB_TEXTEDIT_K_WORDRIGHT : STB_TEXTEDIT_K_RIGHT) | k_mask); }
//        else if (IsKeyPressed(ImGuiKey_UpArrow) && is_multiline)     { if (io.KeyCtrl) SetScrollY(draw_window, ImMax(draw_window->Scroll.y - g.FontSize, 0.0f)); else state->OnKeyPressed((is_startend_key_down ? STB_TEXTEDIT_K_TEXTSTART : STB_TEXTEDIT_K_UP) | k_mask); }
//        else if (IsKeyPressed(ImGuiKey_DownArrow) && is_multiline)   { if (io.KeyCtrl) SetScrollY(draw_window, ImMin(draw_window->Scroll.y + g.FontSize, GetScrollMaxY())); else state->OnKeyPressed((is_startend_key_down ? STB_TEXTEDIT_K_TEXTEND : STB_TEXTEDIT_K_DOWN) | k_mask); }
//        else if (IsKeyPressed(ImGuiKey_PageUp) && is_multiline)      { state->OnKeyPressed(STB_TEXTEDIT_K_PGUP | k_mask); scroll_y -= row_count_per_page * g.FontSize; }
//        else if (IsKeyPressed(ImGuiKey_PageDown) && is_multiline)    { state->OnKeyPressed(STB_TEXTEDIT_K_PGDOWN | k_mask); scroll_y += row_count_per_page * g.FontSize; }
//        else if (IsKeyPressed(ImGuiKey_Home))                        { state->OnKeyPressed(io.KeyCtrl ? STB_TEXTEDIT_K_TEXTSTART | k_mask : STB_TEXTEDIT_K_LINESTART | k_mask); }
//        else if (IsKeyPressed(ImGuiKey_End))                         { state->OnKeyPressed(io.KeyCtrl ? STB_TEXTEDIT_K_TEXTEND | k_mask : STB_TEXTEDIT_K_LINEEND | k_mask); }
//        else if (IsKeyPressed(ImGuiKey_Delete) && !is_readonly && !is_cut)
//        {
//            if (!state->HasSelection())
//            {
//                // OSX doesn't seem to have Super+Delete to delete until end-of-line, so we don't emulate that (as opposed to Super+Backspace)
//                if (is_wordmove_key_down)
//                    state->OnKeyPressed(STB_TEXTEDIT_K_WORDRIGHT | STB_TEXTEDIT_K_SHIFT);
//            }
//            state->OnKeyPressed(STB_TEXTEDIT_K_DELETE | k_mask);
//        }
//        else if (IsKeyPressed(ImGuiKey_Backspace) && !is_readonly)
//        {
//            if (!state->HasSelection())
//            {
//                if (is_wordmove_key_down)
//                    state->OnKeyPressed(STB_TEXTEDIT_K_WORDLEFT | STB_TEXTEDIT_K_SHIFT);
//                else if (is_osx && io.KeyCtrl && !io.KeyAlt && !io.KeySuper)
//                    state->OnKeyPressed(STB_TEXTEDIT_K_LINESTART | STB_TEXTEDIT_K_SHIFT);
//            }
//            state->OnKeyPressed(STB_TEXTEDIT_K_BACKSPACE | k_mask);
//        }
//        else if (is_enter_pressed || is_gamepad_validate)
//        {
//            // Determine if we turn Enter into a \n character
//            bool ctrl_enter_for_new_line = (flags & ImGuiInputTextFlags_CtrlEnterForNewLine) != 0;
//            if (!is_multiline || is_gamepad_validate || (ctrl_enter_for_new_line && !io.KeyCtrl) || (!ctrl_enter_for_new_line && io.KeyCtrl))
//            {
//                validated = true;
//                if (io.ConfigInputTextEnterKeepActive && !is_multiline)
//                    state->SelectAll(); // No need to scroll
//                else
//                    clear_active_id = true;
//            }
//            else if (!is_readonly)
//            {
//                unsigned int c = '\n'; // Insert new line
//                if (InputTextFilterCharacter(&g, &c, flags, callback, callback_user_data))
//                    state->OnCharPressed(c);
//            }
//        }
//        else if (is_cancel)
//        {
//            if (flags & ImGuiInputTextFlags_EscapeClearsAll)
//            {
//                if (buf[0] != 0)
//                {
//                    revert_edit = true;
//                }
//                else
//                {
//                    render_cursor = render_selection = false;
//                    clear_active_id = true;
//                }
//            }
//            else
//            {
//                clear_active_id = revert_edit = true;
//                render_cursor = render_selection = false;
//            }
//        }
//        else if (is_undo || is_redo)
//        {
//            state->OnKeyPressed(is_undo ? STB_TEXTEDIT_K_UNDO : STB_TEXTEDIT_K_REDO);
//            state->ClearSelection();
//        }
//        else if (is_select_all)
//        {
//            state->SelectAll();
//            state->CursorFollow = true;
//        }
//        else if (is_cut || is_copy)
//        {
//            // Cut, Copy
//            if (g.PlatformIO.Platform_SetClipboardTextFn != NULL)
//            {
//                // SetClipboardText() only takes null terminated strings + state->TextSrc may point to read-only user buffer, so we need to make a copy.
//                const int ib = state->HasSelection() ? ImMin(state->Stb->select_start, state->Stb->select_end) : 0;
//                const int ie = state->HasSelection() ? ImMax(state->Stb->select_start, state->Stb->select_end) : state->TextLen;
//                g.TempBuffer.reserve(ie - ib + 1);
//                memcpy(g.TempBuffer.Data, state->TextSrc + ib, ie - ib);
//                g.TempBuffer.Data[ie - ib] = 0;
//                SetClipboardText(g.TempBuffer.Data);
//            }
//            if (is_cut)
//            {
//                if (!state->HasSelection())
//                    state->SelectAll();
//                state->CursorFollow = true;
//                stb_textedit_cut(state, state->Stb);
//            }
//        }
//        else if (is_paste)
//        {
//            if (const char* clipboard = GetClipboardText())
//            {
//                // Filter pasted buffer
//                const int clipboard_len = (int)ImStrlen(clipboard);
//                ImVector<char> clipboard_filtered;
//                clipboard_filtered.reserve(clipboard_len + 1);
//                for (const char* s = clipboard; *s != 0; )
//                {
//                    unsigned int c;
//                    int in_len = ImTextCharFromUtf8(&c, s, NULL);
//                    s += in_len;
//                    if (!InputTextFilterCharacter(&g, &c, flags, callback, callback_user_data, true))
//                        continue;
//                    char c_utf8[5];
//                    ImTextCharToUtf8(c_utf8, c);
//                    int out_len = (int)ImStrlen(c_utf8);
//                    clipboard_filtered.resize(clipboard_filtered.Size + out_len);
//                    memcpy(clipboard_filtered.Data + clipboard_filtered.Size - out_len, c_utf8, out_len);
//                }
//                if (clipboard_filtered.Size > 0) // If everything was filtered, ignore the pasting operation
//                {
//                    clipboard_filtered.push_back(0);
//                    stb_textedit_paste(state, state->Stb, clipboard_filtered.Data, clipboard_filtered.Size - 1);
//                    state->CursorFollow = true;
//                }
//            }
//        }
//
//        // Update render selection flag after events have been handled, so selection highlight can be displayed during the same frame.
//        render_selection |= state->HasSelection() && (RENDER_SELECTION_WHEN_INACTIVE || render_cursor);
//    }
//
//    // Process callbacks and apply result back to user's buffer.
//    const char* apply_new_text = NULL;
//    int apply_new_text_length = 0;
//    if (g.ActiveId == id)
//    {
//        IM_ASSERT(state != NULL);
//        if (revert_edit && !is_readonly)
//        {
//            if (flags & ImGuiInputTextFlags_EscapeClearsAll)
//            {
//                // Clear input
//                IM_ASSERT(buf[0] != 0);
//                apply_new_text = "";
//                apply_new_text_length = 0;
//                value_changed = true;
//                IMSTB_TEXTEDIT_CHARTYPE empty_string;
//                stb_textedit_replace(state, state->Stb, &empty_string, 0);
//            }
//            else if (strcmp(buf, state->TextToRevertTo.Data) != 0)
//            {
//                apply_new_text = state->TextToRevertTo.Data;
//                apply_new_text_length = state->TextToRevertTo.Size - 1;
//
//                // Restore initial value. Only return true if restoring to the initial value changes the current buffer contents.
//                // Push records into the undo stack so we can CTRL+Z the revert operation itself
//                value_changed = true;
//                stb_textedit_replace(state, state->Stb, state->TextToRevertTo.Data, state->TextToRevertTo.Size - 1);
//            }
//        }
//
//        // FIXME-OPT: We always reapply the live buffer back to the input buffer before clearing ActiveId,
//        // even though strictly speaking it wasn't modified on this frame. Should mark dirty state from the stb_textedit callbacks.
//        // If we do that, need to ensure that as special case, 'validated == true' also writes back.
//        // This also allows the user to use InputText() without maintaining any user-side storage.
//        // (please note that if you use this property along ImGuiInputTextFlags_CallbackResize you can end up with your temporary string object
//        // unnecessarily allocating once a frame, either store your string data, either if you don't then don't use ImGuiInputTextFlags_CallbackResize).
//        const bool apply_edit_back_to_user_buffer = true;// !revert_edit || (validated && (flags & ImGuiInputTextFlags_EnterReturnsTrue) != 0);
//        if (apply_edit_back_to_user_buffer)
//        {
//            // Apply current edited text immediately.
//            // Note that as soon as the input box is active, the in-widget value gets priority over any underlying modification of the input buffer
//
//            // User callback
//            if ((flags & (ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackEdit | ImGuiInputTextFlags_CallbackAlways)) != 0)
//            {
//                IM_ASSERT(callback != NULL);
//
//                // The reason we specify the usage semantic (Completion/History) is that Completion needs to disable keyboard TABBING at the moment.
//                ImGuiInputTextFlags event_flag = 0;
//                ImGuiKey event_key = ImGuiKey_None;
//                if ((flags & ImGuiInputTextFlags_CallbackCompletion) != 0 && Shortcut(ImGuiKey_Tab, 0, id))
//                {
//                    event_flag = ImGuiInputTextFlags_CallbackCompletion;
//                    event_key = ImGuiKey_Tab;
//                }
//                else if ((flags & ImGuiInputTextFlags_CallbackHistory) != 0 && IsKeyPressed(ImGuiKey_UpArrow))
//                {
//                    event_flag = ImGuiInputTextFlags_CallbackHistory;
//                    event_key = ImGuiKey_UpArrow;
//                }
//                else if ((flags & ImGuiInputTextFlags_CallbackHistory) != 0 && IsKeyPressed(ImGuiKey_DownArrow))
//                {
//                    event_flag = ImGuiInputTextFlags_CallbackHistory;
//                    event_key = ImGuiKey_DownArrow;
//                }
//                else if ((flags & ImGuiInputTextFlags_CallbackEdit) && state->Edited)
//                {
//                    event_flag = ImGuiInputTextFlags_CallbackEdit;
//                }
//                else if (flags & ImGuiInputTextFlags_CallbackAlways)
//                {
//                    event_flag = ImGuiInputTextFlags_CallbackAlways;
//                }
//
//                if (event_flag)
//                {
//                    ImGuiInputTextCallbackData callback_data;
//                    callback_data.Ctx = &g;
//                    callback_data.EventFlag = event_flag;
//                    callback_data.Flags = flags;
//                    callback_data.UserData = callback_user_data;
//
//                    // FIXME-OPT: Undo stack reconcile needs a backup of the data until we rework API, see #7925
//                    char* callback_buf = is_readonly ? buf : state->TextA.Data;
//                    IM_ASSERT(callback_buf == state->TextSrc);
//                    state->CallbackTextBackup.resize(state->TextLen + 1);
//                    memcpy(state->CallbackTextBackup.Data, callback_buf, state->TextLen + 1);
//
//                    callback_data.EventKey = event_key;
//                    callback_data.Buf = callback_buf;
//                    callback_data.BufTextLen = state->TextLen;
//                    callback_data.BufSize = state->BufCapacity;
//                    callback_data.BufDirty = false;
//
//                    const int utf8_cursor_pos = callback_data.CursorPos = state->Stb->cursor;
//                    const int utf8_selection_start = callback_data.SelectionStart = state->Stb->select_start;
//                    const int utf8_selection_end = callback_data.SelectionEnd = state->Stb->select_end;
//
//                    // Call user code
//                    callback(&callback_data);
//
//                    // Read back what user may have modified
//                    callback_buf = is_readonly ? buf : state->TextA.Data; // Pointer may have been invalidated by a resize callback
//                    IM_ASSERT(callback_data.Buf == callback_buf);         // Invalid to modify those fields
//                    IM_ASSERT(callback_data.BufSize == state->BufCapacity);
//                    IM_ASSERT(callback_data.Flags == flags);
//                    const bool buf_dirty = callback_data.BufDirty;
//                    if (callback_data.CursorPos != utf8_cursor_pos || buf_dirty)            { state->Stb->cursor = callback_data.CursorPos; state->CursorFollow = true; }
//                    if (callback_data.SelectionStart != utf8_selection_start || buf_dirty)  { state->Stb->select_start = (callback_data.SelectionStart == callback_data.CursorPos) ? state->Stb->cursor : callback_data.SelectionStart; }
//                    if (callback_data.SelectionEnd != utf8_selection_end || buf_dirty)      { state->Stb->select_end = (callback_data.SelectionEnd == callback_data.SelectionStart) ? state->Stb->select_start : callback_data.SelectionEnd; }
//                    if (buf_dirty)
//                    {
//                        // Callback may update buffer and thus set buf_dirty even in read-only mode.
//                        IM_ASSERT(callback_data.BufTextLen == (int)ImStrlen(callback_data.Buf)); // You need to maintain BufTextLen if you change the text!
//                        InputTextReconcileUndoState(state, state->CallbackTextBackup.Data, state->CallbackTextBackup.Size - 1, callback_data.Buf, callback_data.BufTextLen);
//                        state->TextLen = callback_data.BufTextLen;  // Assume correct length and valid UTF-8 from user, saves us an extra strlen()
//                        state->CursorAnimReset();
//                    }
//                }
//            }
//
//            // Will copy result string if modified
//            if (!is_readonly && strcmp(state->TextSrc, buf) != 0)
//            {
//                apply_new_text = state->TextSrc;
//                apply_new_text_length = state->TextLen;
//                value_changed = true;
//            }
//        }
//    }
//
//    // Handle reapplying final data on deactivation (see InputTextDeactivateHook() for details)
//    if (g.InputTextDeactivatedState.ID == id)
//    {
//        if (g.ActiveId != id && IsItemDeactivatedAfterEdit() && !is_readonly && strcmp(g.InputTextDeactivatedState.TextA.Data, buf) != 0)
//        {
//            apply_new_text = g.InputTextDeactivatedState.TextA.Data;
//            apply_new_text_length = g.InputTextDeactivatedState.TextA.Size - 1;
//            value_changed = true;
//            //IMGUI_DEBUG_LOG("InputText(): apply Deactivated data for 0x%08X: \"%.*s\".\n", id, apply_new_text_length, apply_new_text);
//        }
//        g.InputTextDeactivatedState.ID = 0;
//    }
//
//    // Copy result to user buffer. This can currently only happen when (g.ActiveId == id)
//    if (apply_new_text != NULL)
//    {
//        //// We cannot test for 'backup_current_text_length != apply_new_text_length' here because we have no guarantee that the size
//        //// of our owned buffer matches the size of the string object held by the user, and by design we allow InputText() to be used
//        //// without any storage on user's side.
//        IM_ASSERT(apply_new_text_length >= 0);
//        if (is_resizable)
//        {
//            ImGuiInputTextCallbackData callback_data;
//            callback_data.Ctx = &g;
//            callback_data.EventFlag = ImGuiInputTextFlags_CallbackResize;
//            callback_data.Flags = flags;
//            callback_data.Buf = buf;
//            callback_data.BufTextLen = apply_new_text_length;
//            callback_data.BufSize = ImMax(buf_size, apply_new_text_length + 1);
//            callback_data.UserData = callback_user_data;
//            callback(&callback_data);
//            buf = callback_data.Buf;
//            buf_size = callback_data.BufSize;
//            apply_new_text_length = ImMin(callback_data.BufTextLen, buf_size - 1);
//            IM_ASSERT(apply_new_text_length <= buf_size);
//        }
//        //IMGUI_DEBUG_PRINT("InputText(\"%s\"): apply_new_text length %d\n", label, apply_new_text_length);
//
//        // If the underlying buffer resize was denied or not carried to the next frame, apply_new_text_length+1 may be >= buf_size.
//        ImStrncpy(buf, apply_new_text, ImMin(apply_new_text_length + 1, buf_size));
//    }
//
//    // Release active ID at the end of the function (so e.g. pressing Return still does a final application of the value)
//    // Otherwise request text input ahead for next frame.
//    if (g.ActiveId == id && clear_active_id)
//        ClearActiveID();
//
//    // Render frame
//    if (!is_multiline)
//    {
//        RenderNavCursor(frame_bb, id);
//        RenderFrame(frame_bb.Min, frame_bb.Max, GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);
//    }
//
//    const ImVec4 clip_rect(frame_bb.Min.x, frame_bb.Min.y, frame_bb.Min.x + inner_size.x, frame_bb.Min.y + inner_size.y); // Not using frame_bb.Max because we have adjusted size
//    ImVec2 draw_pos = is_multiline ? draw_window->DC.CursorPos : frame_bb.Min + style.FramePadding;
//    ImVec2 text_size(0.0f, 0.0f);
//
//    // Set upper limit of single-line InputTextEx() at 2 million characters strings. The current pathological worst case is a long line
//    // without any carriage return, which would makes ImFont::RenderText() reserve too many vertices and probably crash. Avoid it altogether.
//    // Note that we only use this limit on single-line InputText(), so a pathologically large line on a InputTextMultiline() would still crash.
//    const int buf_display_max_length = 2 * 1024 * 1024;
//    const char* buf_display = buf_display_from_state ? state->TextA.Data : buf; //-V595
//    const char* buf_display_end = NULL; // We have specialized paths below for setting the length
//
//    // Render text. We currently only render selection when the widget is active or while scrolling.
//    // FIXME: We could remove the '&& render_cursor' to keep rendering selection when inactive.
//    if (render_cursor || render_selection)
//    {
//        IM_ASSERT(state != NULL);
//        buf_display_end = buf_display + state->TextLen;
//
//        // Render text (with cursor and selection)
//        // This is going to be messy. We need to:
//        // - Display the text (this alone can be more easily clipped)
//        // - Handle scrolling, highlight selection, display cursor (those all requires some form of 1d->2d cursor position calculation)
//        // - Measure text height (for scrollbar)
//        // We are attempting to do most of that in **one main pass** to minimize the computation cost (non-negligible for large amount of text) + 2nd pass for selection rendering (we could merge them by an extra refactoring effort)
//        // FIXME: This should occur on buf_display but we'd need to maintain cursor/select_start/select_end for UTF-8.
//        const char* text_begin = buf_display;
//        const char* text_end = text_begin + state->TextLen;
//        ImVec2 cursor_offset, select_start_offset;
//
//        {
//            // Find lines numbers straddling cursor and selection min position
//            int cursor_line_no = render_cursor ? -1 : -1000;
//            int selmin_line_no = render_selection ? -1 : -1000;
//            const char* cursor_ptr = render_cursor ? text_begin + state->Stb->cursor : NULL;
//            const char* selmin_ptr = render_selection ? text_begin + ImMin(state->Stb->select_start, state->Stb->select_end) : NULL;
//
//            // Count lines and find line number for cursor and selection ends
//            int line_count = 1;
//            if (is_multiline)
//            {
//                for (const char* s = text_begin; (s = (const char*)ImMemchr(s, '\n', (size_t)(text_end - s))) != NULL; s++)
//                {
//                    if (cursor_line_no == -1 && s >= cursor_ptr) { cursor_line_no = line_count; }
//                    if (selmin_line_no == -1 && s >= selmin_ptr) { selmin_line_no = line_count; }
//                    line_count++;
//                }
//            }
//            if (cursor_line_no == -1)
//                cursor_line_no = line_count;
//            if (selmin_line_no == -1)
//                selmin_line_no = line_count;
//
//            // Calculate 2d position by finding the beginning of the line and measuring distance
//            cursor_offset.x = InputTextCalcTextSize(&g, ImStrbol(cursor_ptr, text_begin), cursor_ptr).x;
//            cursor_offset.y = cursor_line_no * g.FontSize;
//            if (selmin_line_no >= 0)
//            {
//                select_start_offset.x = InputTextCalcTextSize(&g, ImStrbol(selmin_ptr, text_begin), selmin_ptr).x;
//                select_start_offset.y = selmin_line_no * g.FontSize;
//            }
//
//            // Store text height (note that we haven't calculated text width at all, see GitHub issues #383, #1224)
//            if (is_multiline)
//                text_size = ImVec2(inner_size.x, line_count * g.FontSize);
//        }
//
//        // Scroll
//        if (render_cursor && state->CursorFollow)
//        {
//            // Horizontal scroll in chunks of quarter width
//            if (!(flags & ImGuiInputTextFlags_NoHorizontalScroll))
//            {
//                const float scroll_increment_x = inner_size.x * 0.25f;
//                const float visible_width = inner_size.x - style.FramePadding.x;
//                if (cursor_offset.x < state->Scroll.x)
//                    state->Scroll.x = IM_TRUNC(ImMax(0.0f, cursor_offset.x - scroll_increment_x));
//                else if (cursor_offset.x - visible_width >= state->Scroll.x)
//                    state->Scroll.x = IM_TRUNC(cursor_offset.x - visible_width + scroll_increment_x);
//            }
//            else
//            {
//                state->Scroll.y = 0.0f;
//            }
//
//            // Vertical scroll
//            if (is_multiline)
//            {
//                // Test if cursor is vertically visible
//                if (cursor_offset.y - g.FontSize < scroll_y)
//                    scroll_y = ImMax(0.0f, cursor_offset.y - g.FontSize);
//                else if (cursor_offset.y - (inner_size.y - style.FramePadding.y * 2.0f) >= scroll_y)
//                    scroll_y = cursor_offset.y - inner_size.y + style.FramePadding.y * 2.0f;
//                const float scroll_max_y = ImMax((text_size.y + style.FramePadding.y * 2.0f) - inner_size.y, 0.0f);
//                scroll_y = ImClamp(scroll_y, 0.0f, scroll_max_y);
//                draw_pos.y += (draw_window->Scroll.y - scroll_y);   // Manipulate cursor pos immediately avoid a frame of lag
//                draw_window->Scroll.y = scroll_y;
//            }
//
//            state->CursorFollow = false;
//        }
//
//        // Draw selection
//        const ImVec2 draw_scroll = ImVec2(state->Scroll.x, 0.0f);
//        if (render_selection)
//        {
//            const char* text_selected_begin = text_begin + ImMin(state->Stb->select_start, state->Stb->select_end);
//            const char* text_selected_end = text_begin + ImMax(state->Stb->select_start, state->Stb->select_end);
//
//            ImU32 bg_color = GetColorU32(ImGuiCol_TextSelectedBg, render_cursor ? 1.0f : 0.6f); // FIXME: current code flow mandate that render_cursor is always true here, we are leaving the transparent one for tests.
//            float bg_offy_up = is_multiline ? 0.0f : -1.0f;    // FIXME: those offsets should be part of the style? they don't play so well with multi-line selection.
//            float bg_offy_dn = is_multiline ? 0.0f : 2.0f;
//            ImVec2 rect_pos = draw_pos + select_start_offset - draw_scroll;
//            for (const char* p = text_selected_begin; p < text_selected_end; )
//            {
//                if (rect_pos.y > clip_rect.w + g.FontSize)
//                    break;
//                if (rect_pos.y < clip_rect.y)
//                {
//                    p = (const char*)ImMemchr((void*)p, '\n', text_selected_end - p);
//                    p = p ? p + 1 : text_selected_end;
//                }
//                else
//                {
//                    ImVec2 rect_size = InputTextCalcTextSize(&g, p, text_selected_end, &p, NULL, true);
//                    if (rect_size.x <= 0.0f) rect_size.x = IM_TRUNC(g.FontBaked->GetCharAdvance((ImWchar)' ') * 0.50f); // So we can see selected empty lines
//                    ImRect rect(rect_pos + ImVec2(0.0f, bg_offy_up - g.FontSize), rect_pos + ImVec2(rect_size.x, bg_offy_dn));
//                    rect.ClipWith(clip_rect);
//                    if (rect.Overlaps(clip_rect))
//                        draw_window->DrawList->AddRectFilled(rect.Min, rect.Max, bg_color);
//                    rect_pos.x = draw_pos.x - draw_scroll.x;
//                }
//                rect_pos.y += g.FontSize;
//            }
//        }
//
//        // We test for 'buf_display_max_length' as a way to avoid some pathological cases (e.g. single-line 1 MB string) which would make ImDrawList crash.
//        // FIXME-OPT: Multiline could submit a smaller amount of contents to AddText() since we already iterated through it.
//        if (is_multiline || (buf_display_end - buf_display) < buf_display_max_length)
//        {
//            ImU32 col = GetColorU32(ImGuiCol_Text);
//            draw_window->DrawList->AddText(g.Font, g.FontSize, draw_pos - draw_scroll, col, buf_display, buf_display_end, 0.0f, is_multiline ? NULL : &clip_rect);
//        }
//
//        // Draw blinking cursor
//        if (render_cursor)
//        {
//            state->CursorAnim += io.DeltaTime;
//            bool cursor_is_visible = (!g.IO.ConfigInputTextCursorBlink) || (state->CursorAnim <= 0.0f) || ImFmod(state->CursorAnim, 1.20f) <= 0.80f;
//            ImVec2 cursor_screen_pos = ImTrunc(draw_pos + cursor_offset - draw_scroll);
//            ImRect cursor_screen_rect(cursor_screen_pos.x, cursor_screen_pos.y - g.FontSize + 0.5f, cursor_screen_pos.x + 1.0f, cursor_screen_pos.y - 1.5f);
//            if (cursor_is_visible && cursor_screen_rect.Overlaps(clip_rect))
//                draw_window->DrawList->AddLine(cursor_screen_rect.Min, cursor_screen_rect.GetBL(), GetColorU32(ImGuiCol_InputTextCursor), 1.0f); // FIXME-DPI: Cursor thickness (#7031)
//
//            // Notify OS of text input position for advanced IME (-1 x offset so that Windows IME can cover our cursor. Bit of an extra nicety.)
//            // This is required for some backends (SDL3) to start emitting character/text inputs.
//            // As per #6341, make sure we don't set that on the deactivating frame.
//            if (!is_readonly && g.ActiveId == id)
//            {
//                ImGuiPlatformImeData* ime_data = &g.PlatformImeData; // (this is a public struct, passed to io.Platform_SetImeDataFn() handler)
//                ime_data->WantVisible = true;
//                ime_data->WantTextInput = true;
//                ime_data->InputPos = ImVec2(cursor_screen_pos.x - 1.0f, cursor_screen_pos.y - g.FontSize);
//                ime_data->InputLineHeight = g.FontSize;
//                ime_data->ViewportId = window->Viewport->ID;
//            }
//        }
//    }
//    else
//    {
//        // Render text only (no selection, no cursor)
//        if (is_multiline)
//            text_size = ImVec2(inner_size.x, InputTextCalcTextLenAndLineCount(buf_display, &buf_display_end) * g.FontSize); // We don't need width
//        else if (g.ActiveId == id)
//            buf_display_end = buf_display + state->TextLen;
//        else
//            buf_display_end = buf_display + ImStrlen(buf_display);
//
//        if (is_multiline || (buf_display_end - buf_display) < buf_display_max_length)
//        {
//            // Find render position for right alignment
//            if (flags & ImGuiInputTextFlags_ElideLeft)
//                draw_pos.x = ImMin(draw_pos.x, frame_bb.Max.x - CalcTextSize(buf_display, NULL).x - style.FramePadding.x);
//
//            const ImVec2 draw_scroll = /*state ? ImVec2(state->Scroll.x, 0.0f) :*/ ImVec2(0.0f, 0.0f); // Preserve scroll when inactive?
//            ImU32 col = GetColorU32(ImGuiCol_Text);
//            draw_window->DrawList->AddText(g.Font, g.FontSize, draw_pos - draw_scroll, col, buf_display, buf_display_end, 0.0f, is_multiline ? NULL : &clip_rect);
//        }
//    }
//
//    if (is_multiline)
//    {
//        // For focus requests to work on our multiline we need to ensure our child ItemAdd() call specifies the ImGuiItemFlags_Inputable (see #4761, #7870)...
//        Dummy(ImVec2(text_size.x, text_size.y + style.FramePadding.y));
//        g.NextItemData.ItemFlags |= (ImGuiItemFlags)ImGuiItemFlags_Inputable | ImGuiItemFlags_NoTabStop;
//        EndChild();
//        item_data_backup.StatusFlags |= (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_HoveredWindow);
//
//        // ...and then we need to undo the group overriding last item data, which gets a bit messy as EndGroup() tries to forward scrollbar being active...
//        // FIXME: This quite messy/tricky, should attempt to get rid of the child window.
//        EndGroup();
//        if (g.LastItemData.ID == 0 || g.LastItemData.ID != GetWindowScrollbarID(draw_window, ImGuiAxis_Y))
//        {
//            g.LastItemData.ID = id;
//            g.LastItemData.ItemFlags = item_data_backup.ItemFlags;
//            g.LastItemData.StatusFlags = item_data_backup.StatusFlags;
//        }
//    }
//    if (state)
//        state->TextSrc = NULL;
//
//    // Log as text
//    if (g.LogEnabled && !is_password)
//    {
//        LogSetNextTextDecoration("{", "}");
//        LogRenderedText(&draw_pos, buf_display, buf_display_end);
//    }
//
//    if (label_size.x > 0)
//        RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);
//
//    if (value_changed)
//        MarkItemEdited(id);
//
//    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Inputable);
//    if ((flags & ImGuiInputTextFlags_EnterReturnsTrue) != 0)
//        return validated;
//    else
//        return value_changed;
//}

}
#define UNICODE
#define _UNICODE

#include <d3d12.h>
#include <dxgi1_4.h>
#include <tchar.h>
#include <shellapi.h>
#include <dwmapi.h>
#include <commctrl.h>
#include <commdlg.h>
#include <string>
#include <vector>
#include <map>
#include "snapvim.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
#include "res/resources.h"
#include "svimconfig.h"
#include "svimconfig_ini.h"
#include "runtime_config.h"

#pragma comment(lib, "comctl32.lib")

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_EXIT 1001
#define ID_TRAY_SETTINGS 1002

#ifdef _DEBUG
#define DX12_ENABLE_DEBUG_LAYER
#endif

#ifdef DX12_ENABLE_DEBUG_LAYER
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

NOTIFYICONDATA nid = {};

// Config for snapvim
static const int APP_NUM_FRAMES_IN_FLIGHT = 2;
static const int APP_NUM_BACK_BUFFERS = 2;
static const int APP_SRV_HEAP_SIZE = 64;

// Global values
HWND g_previousFocus = nullptr;
static char* g_textBuffer = nullptr;

RuntimeConfig g_config;

static HWND g_hwnd = nullptr;
static float g_mainScale = 1.0f;

// Feature 1: text grab from active window
static std::string g_grabbedText;
static bool g_hasGrabbedText = false;

// Feature 2: settings dialog
static HWND g_settingsHwnd = nullptr;
static RuntimeConfig g_settingsBackup;

// Settings control IDs
enum {
    IDC_BTN_BG_COLOR = 200,
    IDC_BTN_FONT_COLOR,
    IDC_BTN_CURSOR_COLOR,
    IDC_BTN_HIGHLIGHT_COLOR,
    IDC_BTN_SELECTION_COLOR,
    IDC_EDIT_ENG_FONT,
    IDC_EDIT_CH_FONT,
    IDC_TRACK_FONT_SIZE,
    IDC_TRACK_PADDING,
    IDC_TRACK_TRANSPARENCY,
    IDC_BTN_APPLY,
    IDC_BTN_SAVE,
    IDC_BTN_CANCEL,
    IDC_LBL_FONT_SIZE,
    IDC_LBL_PADDING,
    IDC_LBL_TRANSPARENCY,
    IDC_LBL_BG,
    IDC_LBL_FONT_CLR,
    IDC_LBL_CURSOR_CLR,
    IDC_LBL_HIGHLIGHT_CLR,
    IDC_LBL_SELECTION_CLR,
};

static COLORREF g_settingsColors[5] = {};

struct FrameContext
{
    ID3D12CommandAllocator*     CommandAllocator;
    UINT64                      FenceValue;
};

struct ExampleDescriptorHeapAllocator
{
    ID3D12DescriptorHeap*       Heap = nullptr;
    D3D12_DESCRIPTOR_HEAP_TYPE  HeapType = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
    D3D12_CPU_DESCRIPTOR_HANDLE HeapStartCpu;
    D3D12_GPU_DESCRIPTOR_HANDLE HeapStartGpu;
    UINT                        HeapHandleIncrement;
    ImVector<int>               FreeIndices;

    void Create(ID3D12Device* device, ID3D12DescriptorHeap* heap)
    {
        IM_ASSERT(Heap == nullptr && FreeIndices.empty());
        Heap = heap;
        D3D12_DESCRIPTOR_HEAP_DESC desc = heap->GetDesc();
        HeapType = desc.Type;
        HeapStartCpu = Heap->GetCPUDescriptorHandleForHeapStart();
        HeapStartGpu = Heap->GetGPUDescriptorHandleForHeapStart();
        HeapHandleIncrement = device->GetDescriptorHandleIncrementSize(HeapType);
        FreeIndices.reserve((int)desc.NumDescriptors);
        for (int n = desc.NumDescriptors; n > 0; n--)
            FreeIndices.push_back(n - 1);
    }
    void Destroy() { Heap = nullptr; FreeIndices.clear(); }
    void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu)
    {
        IM_ASSERT(FreeIndices.Size > 0);
        int idx = FreeIndices.back(); FreeIndices.pop_back();
        out_cpu->ptr = HeapStartCpu.ptr + (idx * HeapHandleIncrement);
        out_gpu->ptr = HeapStartGpu.ptr + (idx * HeapHandleIncrement);
    }
    void Free(D3D12_CPU_DESCRIPTOR_HANDLE out_cpu, D3D12_GPU_DESCRIPTOR_HANDLE out_gpu)
    {
        int cpu_idx = (int)((out_cpu.ptr - HeapStartCpu.ptr) / HeapHandleIncrement);
        int gpu_idx = (int)((out_gpu.ptr - HeapStartGpu.ptr) / HeapHandleIncrement);
        IM_ASSERT(cpu_idx == gpu_idx);
        FreeIndices.push_back(cpu_idx);
    }
};

static FrameContext                 g_frameContext[APP_NUM_FRAMES_IN_FLIGHT] = {};
static UINT                         g_frameIndex = 0;
static ID3D12Device*                g_pd3dDevice = nullptr;
static ID3D12DescriptorHeap*        g_pd3dRtvDescHeap = nullptr;
static ID3D12DescriptorHeap*        g_pd3dSrvDescHeap = nullptr;
static ExampleDescriptorHeapAllocator g_pd3dSrvDescHeapAlloc;
static ID3D12CommandQueue*          g_pd3dCommandQueue = nullptr;
static ID3D12GraphicsCommandList*   g_pd3dCommandList = nullptr;
static ID3D12Fence*                 g_fence = nullptr;
static HANDLE                       g_fenceEvent = nullptr;
static UINT64                       g_fenceLastSignaledValue = 0;
static IDXGISwapChain3*             g_pSwapChain = nullptr;
static bool                         g_SwapChainOccluded = false;
static HANDLE                       g_hSwapChainWaitableObject = nullptr;
static ID3D12Resource*              g_mainRenderTargetResource[APP_NUM_BACK_BUFFERS] = {};
static D3D12_CPU_DESCRIPTOR_HANDLE  g_mainRenderTargetDescriptor[APP_NUM_BACK_BUFFERS] = {};

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
void WaitForLastSubmittedFrame();
FrameContext* WaitForNextFrameResources();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI SettingsWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static void HideWindowDecorations(HWND hwnd) {
    LONG style = GetWindowLong(hwnd, GWL_STYLE);
    style &= ~(WS_CAPTION | WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
    SetWindowLong(hwnd, GWL_STYLE, style);
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    exStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
    SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
}

void StartWindowDrag(HWND hwnd) {
    ReleaseCapture();
    SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
}

void EnableWindowRoundedCorners(HWND hwnd)
{
    const DWORD DWMWA_WINDOW_CORNER_PREFERENCE = 33;
    enum DWM_WINDOW_CORNER_PREFERENCE { DWMWCP_DEFAULT = 0, DWMWCP_DONOTROUND = 1, DWMWCP_ROUND = 2, DWMWCP_ROUNDSMALL = 3 };
    DWM_WINDOW_CORNER_PREFERENCE preference = DWMWCP_ROUND;
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &preference, sizeof(preference));
}

// --- Color conversion helpers ---

static COLORREF ImVec4ToColorRef(ImVec4 c)
{
    return RGB((BYTE)(c.x * 255), (BYTE)(c.y * 255), (BYTE)(c.z * 255));
}

static ImVec4 ColorRefToImVec4(COLORREF cr)
{
    return ImVec4(GetRValue(cr) / 255.0f, GetGValue(cr) / 255.0f, GetBValue(cr) / 255.0f, 1.0f);
}

// --- Config persistence ---

static ImVec4 ParseColor(const std::string& s, ImVec4 def)
{
    ImVec4 c = def;
    sscanf_s(s.c_str(), "%f,%f,%f,%f", &c.x, &c.y, &c.z, &c.w);
    return c;
}

static std::string ColorToString(ImVec4 c)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%.3f,%.3f,%.3f,%.3f", c.x, c.y, c.z, c.w);
    return buf;
}

static void LoadConfig()
{
    SnapVimIni::EnsureConfigDir();
    auto kv = SnapVimIni::ParseIni(SnapVimIni::GetConfigPath().c_str());
    if (kv.empty()) return;

    auto getFloat = [&](const std::string& key, float def) -> float {
        auto it = kv.find(key);
        if (it == kv.end()) return def;
        try { return std::stof(it->second); } catch (...) { return def; }
    };
    auto getInt = [&](const std::string& key, int def) -> int {
        auto it = kv.find(key);
        if (it == kv.end()) return def;
        try { return std::stoi(it->second); } catch (...) { return def; }
    };
    auto getStr = [&](const std::string& key, const std::string& def) -> std::string {
        auto it = kv.find(key);
        return (it != kv.end()) ? it->second : def;
    };

    g_config.BackgroundColor    = ParseColor(kv.count("background_color") ? kv["background_color"] : "", g_config.BackgroundColor);
    g_config.CursorColor        = ParseColor(kv.count("cursor_color") ? kv["cursor_color"] : "", g_config.CursorColor);
    g_config.HighlightLineColor = ParseColor(kv.count("highlight_line_color") ? kv["highlight_line_color"] : "", g_config.HighlightLineColor);
    g_config.SelectionColor     = ParseColor(kv.count("selection_color") ? kv["selection_color"] : "", g_config.SelectionColor);
    g_config.FontColor          = ParseColor(kv.count("font_color") ? kv["font_color"] : "", g_config.FontColor);
    g_config.EnglishFont        = getStr("english_font", g_config.EnglishFont);
    g_config.ChineseFont        = getStr("chinese_font", g_config.ChineseFont);
    g_config.FontSize           = getFloat("font_size", g_config.FontSize);
    g_config.Padding            = getFloat("padding", g_config.Padding);
    g_config.TransparentValue   = getInt("transparent_value", g_config.TransparentValue);
}

static void SaveConfig()
{
    std::map<std::string, std::string> kv;
    kv["background_color"]      = ColorToString(g_config.BackgroundColor);
    kv["cursor_color"]          = ColorToString(g_config.CursorColor);
    kv["highlight_line_color"]  = ColorToString(g_config.HighlightLineColor);
    kv["selection_color"]       = ColorToString(g_config.SelectionColor);
    kv["font_color"]            = ColorToString(g_config.FontColor);
    kv["english_font"]          = g_config.EnglishFont;
    kv["chinese_font"]          = g_config.ChineseFont;
    kv["font_size"]             = std::to_string(g_config.FontSize);
    kv["padding"]               = std::to_string(g_config.Padding);
    kv["transparent_value"]     = std::to_string(g_config.TransparentValue);

    SnapVimIni::EnsureConfigDir();
    SnapVimIni::WriteIni(SnapVimIni::GetConfigPath().c_str(), kv);
}

static void ApplyConfig()
{
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_FrameBg]          = g_config.BackgroundColor;
    style.Colors[ImGuiCol_InputTextCursor]  = g_config.CursorColor;
    style.Colors[ImGuiCol_ScrollbarBg]      = g_config.ScrollbarBgColor;
    style.Colors[ImGuiCol_Text]             = g_config.FontColor;

    if (g_hwnd)
        SetLayeredWindowAttributes(g_hwnd, 0, (BYTE)g_config.TransparentValue, LWA_ALPHA);

    if (g_config.NeedsFontRebuild)
    {
        WaitForLastSubmittedFrame();

        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->Clear();

        float fontSize = g_config.FontSize * g_mainScale;

        ImFontConfig config_en;
        config_en.MergeMode = false;
        char* localAppData = nullptr;
        _dupenv_s(&localAppData, nullptr, "LOCALAPPDATA");

        char userFontPath[MAX_PATH];
        snprintf(userFontPath, MAX_PATH, "%s\\Microsoft\\Windows\\Fonts\\%s", localAppData, g_config.EnglishFont.c_str());
        ImFont* font = io.Fonts->AddFontFromFileTTF(userFontPath, fontSize, &config_en, io.Fonts->GetGlyphRangesDefault());

        ImFontConfig config_zh;
        config_zh.MergeMode = true;
        snprintf(userFontPath, MAX_PATH, "%s\\Microsoft\\Windows\\Fonts\\%s", localAppData, g_config.ChineseFont.c_str());
        io.Fonts->AddFontFromFileTTF(userFontPath, fontSize, &config_zh, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());

        if (!font)
            font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\consola.ttf", fontSize, &config_en, io.Fonts->GetGlyphRangesDefault());

        free(localAppData);

        ImGui_ImplDX12_InvalidateDeviceObjects();
        ImGui_ImplDX12_CreateDeviceObjects();

        g_config.NeedsFontRebuild = false;
    }
}

// --- Feature 1: clipboard-based text grab ---

static void SendKeyCombo(WORD vk, bool ctrl)
{
    int n = ctrl ? 4 : 2;
    INPUT inputs[4] = {};
    int i = 0;
    if (ctrl) { inputs[i].type = INPUT_KEYBOARD; inputs[i].ki.wVk = VK_CONTROL; i++; }
    inputs[i].type = INPUT_KEYBOARD; inputs[i].ki.wVk = vk; i++;
    inputs[i].type = INPUT_KEYBOARD; inputs[i].ki.wVk = vk; inputs[i].ki.dwFlags = KEYEVENTF_KEYUP; i++;
    if (ctrl) { inputs[i].type = INPUT_KEYBOARD; inputs[i].ki.wVk = VK_CONTROL; inputs[i].ki.dwFlags = KEYEVENTF_KEYUP; }
    SendInput(n, inputs, sizeof(INPUT));
}

static std::string ReadClipboardText()
{
    if (!OpenClipboard(nullptr)) return "";
    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (!hData) { CloseClipboard(); return ""; }

    wchar_t* wstr = (wchar_t*)GlobalLock(hData);
    if (!wstr) { CloseClipboard(); return ""; }

    int wlen = (int)wcslen(wstr);
    int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wstr, wlen, nullptr, 0, nullptr, nullptr);
    std::string result;
    if (utf8Len > 0)
    {
        result.resize(utf8Len);
        WideCharToMultiByte(CP_UTF8, 0, wstr, wlen, &result[0], utf8Len, nullptr, nullptr);
    }
    GlobalUnlock(hData);
    CloseClipboard();
    return result;
}

static std::string GrabTextFromActiveWindow(HWND foregroundWnd)
{
    if (!foregroundWnd) return "";

    SendKeyCombo('A', true);  // Ctrl+A
    Sleep(50);
    SendKeyCombo('C', true);  // Ctrl+C
    Sleep(50);

    return ReadClipboardText();
}

static void ClearActiveWindowContent(HWND foregroundWnd)
{
    if (!foregroundWnd || GetForegroundWindow() != foregroundWnd)
        return;
    SendKeyCombo(VK_DELETE, false);  // Delete
}

// --- Feature 2: native Win32 settings dialog ---

static void ReadSettingsFromDialog(HWND dlg)
{
    g_settingsColors[0] = (COLORREF)GetWindowLongPtr(GetDlgItem(dlg, IDC_BTN_BG_COLOR), GWLP_USERDATA);
    g_settingsColors[1] = (COLORREF)GetWindowLongPtr(GetDlgItem(dlg, IDC_BTN_FONT_COLOR), GWLP_USERDATA);
    g_settingsColors[2] = (COLORREF)GetWindowLongPtr(GetDlgItem(dlg, IDC_BTN_CURSOR_COLOR), GWLP_USERDATA);
    g_settingsColors[3] = (COLORREF)GetWindowLongPtr(GetDlgItem(dlg, IDC_BTN_HIGHLIGHT_COLOR), GWLP_USERDATA);
    g_settingsColors[4] = (COLORREF)GetWindowLongPtr(GetDlgItem(dlg, IDC_BTN_SELECTION_COLOR), GWLP_USERDATA);

    g_config.BackgroundColor    = ColorRefToImVec4(g_settingsColors[0]);
    g_config.FontColor          = ColorRefToImVec4(g_settingsColors[1]);
    g_config.CursorColor        = ColorRefToImVec4(g_settingsColors[2]);
    g_config.HighlightLineColor = ColorRefToImVec4(g_settingsColors[3]);
    g_config.SelectionColor     = ColorRefToImVec4(g_settingsColors[4]);

    wchar_t wbuf[256];
    GetDlgItemText(dlg, IDC_EDIT_ENG_FONT, wbuf, 256);
    char utf8[512];
    WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, utf8, sizeof(utf8), nullptr, nullptr);
    g_config.EnglishFont = utf8;

    GetDlgItemText(dlg, IDC_EDIT_CH_FONT, wbuf, 256);
    WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, utf8, sizeof(utf8), nullptr, nullptr);
    g_config.ChineseFont = utf8;

    g_config.FontSize         = (float)SendDlgItemMessage(dlg, IDC_TRACK_FONT_SIZE, TBM_GETPOS, 0, 0);
    g_config.Padding          = (float)SendDlgItemMessage(dlg, IDC_TRACK_PADDING, TBM_GETPOS, 0, 0);
    g_config.TransparentValue = (int)SendDlgItemMessage(dlg, IDC_TRACK_TRANSPARENCY, TBM_GETPOS, 0, 0);
    g_config.NeedsFontRebuild = true;
}

static void CreateColorButton(HWND parent, int id, int x, int y, int w, int h, COLORREF color, const wchar_t* label)
{
    HWND btn = CreateWindowEx(0, L"BUTTON", label,
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        x, y, w, h, parent, (HMENU)(INT_PTR)id, GetModuleHandle(nullptr), nullptr);
    SetWindowLongPtr(btn, GWLP_USERDATA, (LONG_PTR)color);
}

static void ShowSettingsDialog()
{
    if (g_settingsHwnd && IsWindow(g_settingsHwnd))
    {
        ShowWindow(g_settingsHwnd, SW_SHOW);
        SetForegroundWindow(g_settingsHwnd);
        return;
    }

    g_settingsBackup = g_config;

    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_BAR_CLASSES };
    InitCommonControlsEx(&icc);

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = SettingsWndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"SnapVimSettings";
    RegisterClassExW(&wc);

    int dlgW = 380, dlgH = 480;
    RECT rc;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);
    int posX = rc.right - dlgW - 50;
    int posY = rc.top + 50;

    g_settingsHwnd = CreateWindowEx(WS_EX_TOOLWINDOW, L"SnapVimSettings", L"SnapVim Settings",
        (WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX) | WS_VSCROLL,
        posX, posY, dlgW, dlgH, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);

    ShowWindow(g_settingsHwnd, SW_SHOW);
    UpdateWindow(g_settingsHwnd);
}

static void PickColor(HWND buttonHwnd)
{
    COLORREF current = (COLORREF)GetWindowLongPtr(buttonHwnd, GWLP_USERDATA);
    static COLORREF customColors[16] = {};

    CHOOSECOLOR cc = {};
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = buttonHwnd;
    cc.rgbResult = current;
    cc.lpCustColors = customColors;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;

    if (ChooseColor(&cc))
    {
        SetWindowLongPtr(buttonHwnd, GWLP_USERDATA, (LONG_PTR)cc.rgbResult);
        InvalidateRect(buttonHwnd, nullptr, TRUE);
    }
}

static int s_settingsScrollY = 0;
static int s_settingsContentHeight = 0;
static HBRUSH s_settingsBgBrush = nullptr;

LRESULT WINAPI SettingsWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        s_settingsScrollY = 0;
        if (s_settingsBgBrush) DeleteObject(s_settingsBgBrush);
        s_settingsBgBrush = CreateSolidBrush(RGB(40, 40, 40));
        int y = 15, labelW = 110, btnW = 160, h = 24, gap = 32;

        // Colors section
        CreateWindowEx(0, L"STATIC", L"Colors", WS_CHILD | WS_VISIBLE, 15, y, 300, 20, hWnd, nullptr, nullptr, nullptr);
        y += 24;

        const wchar_t* colorLabels[] = { L"Background:", L"Font Color:", L"Cursor:", L"Highlight Line:", L"Selection:" };
        int colorIds[] = { IDC_BTN_BG_COLOR, IDC_BTN_FONT_COLOR, IDC_BTN_CURSOR_COLOR, IDC_BTN_HIGHLIGHT_COLOR, IDC_BTN_SELECTION_COLOR };
        int labelIds[] = { IDC_LBL_BG, IDC_LBL_FONT_CLR, IDC_LBL_CURSOR_CLR, IDC_LBL_HIGHLIGHT_CLR, IDC_LBL_SELECTION_CLR };
        COLORREF initColors[] = {
            ImVec4ToColorRef(g_config.BackgroundColor),
            ImVec4ToColorRef(g_config.FontColor),
            ImVec4ToColorRef(g_config.CursorColor),
            ImVec4ToColorRef(g_config.HighlightLineColor),
            ImVec4ToColorRef(g_config.SelectionColor),
        };

        for (int i = 0; i < 5; i++)
        {
            CreateWindowEx(0, L"STATIC", colorLabels[i], WS_CHILD | WS_VISIBLE, 15, y + 3, labelW, 20, hWnd, (HMENU)(INT_PTR)labelIds[i], nullptr, nullptr);
            CreateColorButton(hWnd, colorIds[i], 130, y, btnW, h, initColors[i], L"");
            y += gap;
        }

        // Font section
        y += 8;
        CreateWindowEx(0, L"STATIC", L"Font", WS_CHILD | WS_VISIBLE, 15, y, 300, 20, hWnd, nullptr, nullptr, nullptr);
        y += 24;

        CreateWindowEx(0, L"STATIC", L"English Font:", WS_CHILD | WS_VISIBLE, 15, y + 3, labelW, 20, hWnd, nullptr, nullptr, nullptr);
        {
            wchar_t wbuf[256];
            MultiByteToWideChar(CP_UTF8, 0, g_config.EnglishFont.c_str(), -1, wbuf, 256);
            CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", wbuf, WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 130, y, btnW, h, hWnd, (HMENU)(INT_PTR)IDC_EDIT_ENG_FONT, nullptr, nullptr);
        }
        y += gap;

        CreateWindowEx(0, L"STATIC", L"Chinese Font:", WS_CHILD | WS_VISIBLE, 15, y + 3, labelW, 20, hWnd, nullptr, nullptr, nullptr);
        {
            wchar_t wbuf[256];
            MultiByteToWideChar(CP_UTF8, 0, g_config.ChineseFont.c_str(), -1, wbuf, 256);
            CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", wbuf, WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 130, y, btnW, h, hWnd, (HMENU)(INT_PTR)IDC_EDIT_CH_FONT, nullptr, nullptr);
        }
        y += gap;

        // Font size trackbar
        wchar_t lbl[64];
        swprintf_s(lbl, L"Font Size: %.0f", g_config.FontSize);
        CreateWindowEx(0, L"STATIC", lbl, WS_CHILD | WS_VISIBLE, 15, y + 3, labelW, 20, hWnd, (HMENU)(INT_PTR)IDC_LBL_FONT_SIZE, nullptr, nullptr);
        CreateWindowEx(0, TRACKBAR_CLASS, L"", WS_CHILD | WS_VISIBLE | TBS_NOTICKS,
            130, y, btnW, h, hWnd, (HMENU)(INT_PTR)IDC_TRACK_FONT_SIZE, nullptr, nullptr);
        SendDlgItemMessage(hWnd, IDC_TRACK_FONT_SIZE, TBM_SETRANGE, TRUE, MAKELONG(8, 72));
        SendDlgItemMessage(hWnd, IDC_TRACK_FONT_SIZE, TBM_SETPOS, TRUE, (LPARAM)g_config.FontSize);
        y += gap;

        // Layout section
        y += 8;
        CreateWindowEx(0, L"STATIC", L"Layout", WS_CHILD | WS_VISIBLE, 15, y, 300, 20, hWnd, nullptr, nullptr, nullptr);
        y += 24;

        swprintf_s(lbl, L"Padding: %.0f", g_config.Padding);
        CreateWindowEx(0, L"STATIC", lbl, WS_CHILD | WS_VISIBLE, 15, y + 3, labelW, 20, hWnd, (HMENU)(INT_PTR)IDC_LBL_PADDING, nullptr, nullptr);
        CreateWindowEx(0, TRACKBAR_CLASS, L"", WS_CHILD | WS_VISIBLE | TBS_NOTICKS,
            130, y, btnW, h, hWnd, (HMENU)(INT_PTR)IDC_TRACK_PADDING, nullptr, nullptr);
        SendDlgItemMessage(hWnd, IDC_TRACK_PADDING, TBM_SETRANGE, TRUE, MAKELONG(0, 50));
        SendDlgItemMessage(hWnd, IDC_TRACK_PADDING, TBM_SETPOS, TRUE, (LPARAM)g_config.Padding);
        y += gap;

        // Window section
        y += 8;
        CreateWindowEx(0, L"STATIC", L"Window", WS_CHILD | WS_VISIBLE, 15, y, 300, 20, hWnd, nullptr, nullptr, nullptr);
        y += 24;

        swprintf_s(lbl, L"Transparency: %d", g_config.TransparentValue);
        CreateWindowEx(0, L"STATIC", lbl, WS_CHILD | WS_VISIBLE, 15, y + 3, labelW, 20, hWnd, (HMENU)(INT_PTR)IDC_LBL_TRANSPARENCY, nullptr, nullptr);
        CreateWindowEx(0, TRACKBAR_CLASS, L"", WS_CHILD | WS_VISIBLE | TBS_NOTICKS,
            130, y, btnW, h, hWnd, (HMENU)(INT_PTR)IDC_TRACK_TRANSPARENCY, nullptr, nullptr);
        SendDlgItemMessage(hWnd, IDC_TRACK_TRANSPARENCY, TBM_SETRANGE, TRUE, MAKELONG(50, 255));
        SendDlgItemMessage(hWnd, IDC_TRACK_TRANSPARENCY, TBM_SETPOS, TRUE, (LPARAM)g_config.TransparentValue);
        y += gap + 10;

        // Buttons
        int btnX = 30;
        CreateWindowEx(0, L"BUTTON", L"Apply", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            btnX, y, 90, 30, hWnd, (HMENU)(INT_PTR)IDC_BTN_APPLY, nullptr, nullptr);
        CreateWindowEx(0, L"BUTTON", L"Save", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            btnX + 100, y, 90, 30, hWnd, (HMENU)(INT_PTR)IDC_BTN_SAVE, nullptr, nullptr);
        CreateWindowEx(0, L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            btnX + 200, y, 90, 30, hWnd, (HMENU)(INT_PTR)IDC_BTN_CANCEL, nullptr, nullptr);
        y += 40;

        s_settingsContentHeight = y;

        RECT clientRc;
        GetClientRect(hWnd, &clientRc);
        int clientH = clientRc.bottom;

        SCROLLINFO si = {};
        si.cbSize = sizeof(si);
        si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
        si.nMin = 0;
        si.nMax = s_settingsContentHeight;
        si.nPage = clientH;
        si.nPos = 0;
        SetScrollInfo(hWnd, SB_VERT, &si, TRUE);

        return 0;
    }

    case WM_SIZE:
    {
        if (s_settingsContentHeight > 0)
        {
            RECT clientRc;
            GetClientRect(hWnd, &clientRc);
            int clientH = clientRc.bottom;

            SCROLLINFO si = {};
            si.cbSize = sizeof(si);
            si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
            si.nMin = 0;
            si.nMax = s_settingsContentHeight;
            si.nPage = clientH;
            si.nPos = s_settingsScrollY;
            SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
        }
        break;
    }

    case WM_VSCROLL:
    {
        SCROLLINFO si = {};
        si.cbSize = sizeof(si);
        si.fMask = SIF_ALL;
        GetScrollInfo(hWnd, SB_VERT, &si);

        int pos = si.nPos;
        switch (LOWORD(wParam))
        {
        case SB_LINEUP:    pos -= 20; break;
        case SB_LINEDOWN:  pos += 20; break;
        case SB_PAGEUP:    pos -= si.nPage; break;
        case SB_PAGEDOWN:  pos += si.nPage; break;
        case SB_THUMBTRACK: pos = si.nTrackPos; break;
        }

        int maxPos = si.nMax - (int)si.nPage;
        if (maxPos < 0) maxPos = 0;
        if (pos < 0) pos = 0;
        if (pos > maxPos) pos = maxPos;

        int delta = pos - s_settingsScrollY;
        if (delta != 0)
        {
            ScrollWindowEx(hWnd, 0, -delta, nullptr, nullptr, nullptr, nullptr, SW_SCROLLCHILDREN | SW_INVALIDATE);
            s_settingsScrollY = pos;
            si.fMask = SIF_POS;
            si.nPos = pos;
            SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
            InvalidateRect(hWnd, nullptr, TRUE);
            UpdateWindow(hWnd);
        }
        break;
    }

    case WM_MOUSEWHEEL:
    {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        int scrollAmount = -(delta / WHEEL_DELTA) * 40;

        SCROLLINFO si = {};
        si.cbSize = sizeof(si);
        si.fMask = SIF_ALL;
        GetScrollInfo(hWnd, SB_VERT, &si);

        int pos = s_settingsScrollY + scrollAmount;
        int maxPos = si.nMax - (int)si.nPage;
        if (maxPos < 0) maxPos = 0;
        if (pos < 0) pos = 0;
        if (pos > maxPos) pos = maxPos;

        int actualDelta = pos - s_settingsScrollY;
        if (actualDelta != 0)
        {
            ScrollWindowEx(hWnd, 0, -actualDelta, nullptr, nullptr, nullptr, nullptr, SW_SCROLLCHILDREN | SW_INVALIDATE);
            s_settingsScrollY = pos;
            si.fMask = SIF_POS;
            si.nPos = pos;
            SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
            InvalidateRect(hWnd, nullptr, TRUE);
            UpdateWindow(hWnd);
        }
        break;
    }

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, RGB(220, 220, 220));
        SetBkColor(hdc, RGB(40, 40, 40));
        SetBkMode(hdc, TRANSPARENT);
        if (msg == WM_CTLCOLOREDIT)
        {
            SetBkColor(hdc, RGB(55, 55, 55));
            SetBkMode(hdc, OPAQUE);
            static HBRUSH editBrush = CreateSolidBrush(RGB(55, 55, 55));
            return (LRESULT)editBrush;
        }
        return (LRESULT)s_settingsBgBrush;
    }

    case WM_CTLCOLORBTN:
        return (LRESULT)s_settingsBgBrush;

    case WM_ERASEBKGND:
    {
        RECT rc;
        GetClientRect(hWnd, &rc);
        FillRect((HDC)wParam, &rc, s_settingsBgBrush);
        return 1;
    }

    case WM_DRAWITEM:
    {
        DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lParam;
        COLORREF color = (COLORREF)GetWindowLongPtr(dis->hwndItem, GWLP_USERDATA);
        HBRUSH brush = CreateSolidBrush(color);
        FillRect(dis->hDC, &dis->rcItem, brush);
        DeleteObject(brush);
        FrameRect(dis->hDC, &dis->rcItem, (HBRUSH)GetStockObject(BLACK_BRUSH));
        return TRUE;
    }

    case WM_COMMAND:
    {
        int id = LOWORD(wParam);
        if (id == IDC_BTN_BG_COLOR || id == IDC_BTN_FONT_COLOR || id == IDC_BTN_CURSOR_COLOR ||
            id == IDC_BTN_HIGHLIGHT_COLOR || id == IDC_BTN_SELECTION_COLOR)
        {
            PickColor((HWND)lParam);
        }
        else if (id == IDC_BTN_APPLY)
        {
            ReadSettingsFromDialog(hWnd);
            ApplyConfig();
        }
        else if (id == IDC_BTN_SAVE)
        {
            ReadSettingsFromDialog(hWnd);
            ApplyConfig();
            SaveConfig();
        }
        else if (id == IDC_BTN_CANCEL)
        {
            g_config = g_settingsBackup;
            ApplyConfig();
            ShowWindow(hWnd, SW_HIDE);
        }
        break;
    }

    case WM_HSCROLL:
    {
        HWND trackbar = (HWND)lParam;
        int id = GetDlgCtrlID(trackbar);
        int pos = (int)SendMessage(trackbar, TBM_GETPOS, 0, 0);
        wchar_t lbl[64];
        if (id == IDC_TRACK_FONT_SIZE)
        {
            swprintf_s(lbl, L"Font Size: %d", pos);
            SetDlgItemText(hWnd, IDC_LBL_FONT_SIZE, lbl);
        }
        else if (id == IDC_TRACK_PADDING)
        {
            swprintf_s(lbl, L"Padding: %d", pos);
            SetDlgItemText(hWnd, IDC_LBL_PADDING, lbl);
        }
        else if (id == IDC_TRACK_TRANSPARENCY)
        {
            swprintf_s(lbl, L"Transparency: %d", pos);
            SetDlgItemText(hWnd, IDC_LBL_TRANSPARENCY, lbl);
            if (g_hwnd)
                SetLayeredWindowAttributes(g_hwnd, 0, (BYTE)pos, LWA_ALPHA);
        }
        break;
    }

    case WM_CLOSE:
        ShowWindow(hWnd, SW_HIDE);
        return 0;

    case WM_DESTROY:
        if (s_settingsBgBrush) { DeleteObject(s_settingsBgBrush); s_settingsBgBrush = nullptr; }
        g_settingsHwnd = nullptr;
        break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// Main code
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    ImGui_ImplWin32_EnableDpiAwareness();
    g_mainScale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    LoadConfig();

    RECT rc;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);

    int winWidth = (int)(g_config.WindowWidth * g_mainScale);
    int winHeight = (int)(g_config.WindowHeight * g_mainScale);
    int posX = rc.left + (rc.right - rc.left - winWidth) / 2;
    int posY = rc.top + (rc.bottom - rc.top - winHeight) * 2 / 5;

    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"Snap Vim", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_LAYERED, wc.lpszClassName, L"Snap Vim", WS_OVERLAPPEDWINDOW, posX, posY, winWidth, winHeight, nullptr, nullptr, wc.hInstance, nullptr);
    g_hwnd = hwnd;
    HideWindowDecorations(hwnd);
    EnableWindowRoundedCorners(hwnd);
    SetLayeredWindowAttributes(hwnd, 0, (BYTE)g_config.TransparentValue, LWA_ALPHA);

    if (!RegisterHotKey(hwnd, 1, MOD_CONTROL, VK_SPACE)) {
        MessageBox(hwnd, L"[SnapVim]: RegisterHotKey failed. Maybe the hotkey [Ctrl+Space] is occupied.", L"Error", MB_ICONERROR);
    }

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(g_mainScale);
    style.FontScaleDpi = g_mainScale;

    ImGui_ImplWin32_Init(hwnd);

    ImGui_ImplDX12_InitInfo init_info = {};
    init_info.Device = g_pd3dDevice;
    init_info.CommandQueue = g_pd3dCommandQueue;
    init_info.NumFramesInFlight = APP_NUM_FRAMES_IN_FLIGHT;
    init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;
    init_info.SrvDescriptorHeap = g_pd3dSrvDescHeap;
    init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu) { return g_pd3dSrvDescHeapAlloc.Alloc(out_cpu, out_gpu); };
    init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE gpu) { return g_pd3dSrvDescHeapAlloc.Free(cpu, gpu); };
    ImGui_ImplDX12_Init(&init_info);

    ImFont* font = nullptr;
    float fontSize = g_config.FontSize * g_mainScale;

    ImFontConfig config_en;
    config_en.MergeMode = false;
    char* localAppData = nullptr;
    _dupenv_s(&localAppData, nullptr, "LOCALAPPDATA");
    char userFontPath[MAX_PATH];
    snprintf(userFontPath, MAX_PATH, "%s\\Microsoft\\Windows\\Fonts\\%s", localAppData, g_config.EnglishFont.c_str());
    font = io.Fonts->AddFontFromFileTTF(userFontPath, fontSize, &config_en, io.Fonts->GetGlyphRangesDefault());

    ImFontConfig config_zh;
    config_zh.MergeMode = true;
    snprintf(userFontPath, MAX_PATH, "%s\\Microsoft\\Windows\\Fonts\\%s", localAppData, g_config.ChineseFont.c_str());
    io.Fonts->AddFontFromFileTTF(userFontPath, fontSize, &config_zh, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());

    if (!font) font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\consola.ttf", fontSize, &config_en, io.Fonts->GetGlyphRangesDefault());
    free(localAppData);

    ApplyConfig();

    ImGuiWindowFlags window_flags =
    ImGuiWindowFlags_NoDecoration |
    ImGuiWindowFlags_NoBackground |
    ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoResize |
    ImGuiWindowFlags_NoSavedSettings |
    ImGuiWindowFlags_NoFocusOnAppearing |
    ImGuiWindowFlags_NoBringToFrontOnFocus |
    ImGuiWindowFlags_NoBackground |
    ImGuiWindowFlags_NoNav;

    SnapVim::InitSnapVim(hwnd);

    bool done = false;
    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done) break;

        if ((g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) || ::IsIconic(hwnd))
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        if (g_config.NeedsFontRebuild)
        {
            WaitForLastSubmittedFrame();

            ImGuiIO& rebuildIO = ImGui::GetIO();
            rebuildIO.Fonts->Clear();

            float fs = g_config.FontSize * g_mainScale;
            ImFontConfig cfg_en;
            cfg_en.MergeMode = false;
            char* lad = nullptr;
            _dupenv_s(&lad, nullptr, "LOCALAPPDATA");
            char fp[MAX_PATH];
            snprintf(fp, MAX_PATH, "%s\\Microsoft\\Windows\\Fonts\\%s", lad, g_config.EnglishFont.c_str());
            ImFont* f = rebuildIO.Fonts->AddFontFromFileTTF(fp, fs, &cfg_en, rebuildIO.Fonts->GetGlyphRangesDefault());

            ImFontConfig cfg_zh;
            cfg_zh.MergeMode = true;
            snprintf(fp, MAX_PATH, "%s\\Microsoft\\Windows\\Fonts\\%s", lad, g_config.ChineseFont.c_str());
            rebuildIO.Fonts->AddFontFromFileTTF(fp, fs, &cfg_zh, rebuildIO.Fonts->GetGlyphRangesChineseSimplifiedCommon());

            if (!f) f = rebuildIO.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\consola.ttf", fs, &cfg_en, rebuildIO.Fonts->GetGlyphRangesDefault());
            free(lad);

            ImGui_ImplDX12_InvalidateDeviceObjects();
            ImGui_ImplDX12_CreateDeviceObjects();
            g_config.NeedsFontRebuild = false;
        }

        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (g_hasGrabbedText)
        {
            SnapVim::LoadTextIntoBuffer(g_grabbedText.c_str());
            g_hasGrabbedText = false;
            g_grabbedText.clear();
        }

        SnapVim::RenderSnapVimEditor(g_textBuffer, winWidth, winHeight, window_flags);

        ImGui::Render();

        FrameContext* frameCtx = WaitForNextFrameResources();
        UINT backBufferIdx = g_pSwapChain->GetCurrentBackBufferIndex();
        frameCtx->CommandAllocator->Reset();

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource   = g_mainRenderTargetResource[backBufferIdx];
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
        g_pd3dCommandList->Reset(frameCtx->CommandAllocator, nullptr);
        g_pd3dCommandList->ResourceBarrier(1, &barrier);

        const float clear_color_with_alpha[4] = {
            g_config.BackgroundColor.x * g_config.BackgroundColor.w,
            g_config.BackgroundColor.y * g_config.BackgroundColor.w,
            g_config.BackgroundColor.z * g_config.BackgroundColor.w,
            g_config.BackgroundColor.w
        };
        g_pd3dCommandList->ClearRenderTargetView(g_mainRenderTargetDescriptor[backBufferIdx], clear_color_with_alpha, 0, nullptr);
        g_pd3dCommandList->OMSetRenderTargets(1, &g_mainRenderTargetDescriptor[backBufferIdx], FALSE, nullptr);
        g_pd3dCommandList->SetDescriptorHeaps(1, &g_pd3dSrvDescHeap);
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), g_pd3dCommandList);
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
        g_pd3dCommandList->ResourceBarrier(1, &barrier);
        g_pd3dCommandList->Close();

        g_pd3dCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&g_pd3dCommandList);

        HRESULT hr = g_pSwapChain->Present(1, 0);
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);

        UINT64 fenceValue = g_fenceLastSignaledValue + 1;
        g_pd3dCommandQueue->Signal(g_fence, fenceValue);
        g_fenceLastSignaledValue = fenceValue;
        frameCtx->FenceValue = fenceValue;
    }

    WaitForLastSubmittedFrame();

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    SnapVim::DestroySnapVim();
    UnregisterHotKey(hwnd, 1);
    return 0;
}

bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC1 sd;
    {
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = APP_NUM_BACK_BUFFERS;
        sd.Width = 0;
        sd.Height = 0;
        sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        sd.Scaling = DXGI_SCALING_STRETCH;
        sd.Stereo = FALSE;
    }

#ifdef DX12_ENABLE_DEBUG_LAYER
    ID3D12Debug* pdx12Debug = nullptr;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pdx12Debug))))
        pdx12Debug->EnableDebugLayer();
#endif

    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    if (D3D12CreateDevice(nullptr, featureLevel, IID_PPV_ARGS(&g_pd3dDevice)) != S_OK)
        return false;

#ifdef DX12_ENABLE_DEBUG_LAYER
    if (pdx12Debug != nullptr)
    {
        ID3D12InfoQueue* pInfoQueue = nullptr;
        g_pd3dDevice->QueryInterface(IID_PPV_ARGS(&pInfoQueue));
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
        pInfoQueue->Release();
        pdx12Debug->Release();
    }
#endif

    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        desc.NumDescriptors = APP_NUM_BACK_BUFFERS;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NodeMask = 1;
        if (g_pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dRtvDescHeap)) != S_OK)
            return false;
        SIZE_T rtvDescriptorSize = g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = g_pd3dRtvDescHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < APP_NUM_BACK_BUFFERS; i++)
        {
            g_mainRenderTargetDescriptor[i] = rtvHandle;
            rtvHandle.ptr += rtvDescriptorSize;
        }
    }

    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = APP_SRV_HEAP_SIZE;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if (g_pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dSrvDescHeap)) != S_OK)
            return false;
        g_pd3dSrvDescHeapAlloc.Create(g_pd3dDevice, g_pd3dSrvDescHeap);
    }

    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask = 1;
        if (g_pd3dDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&g_pd3dCommandQueue)) != S_OK)
            return false;
    }

    for (UINT i = 0; i < APP_NUM_FRAMES_IN_FLIGHT; i++)
        if (g_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_frameContext[i].CommandAllocator)) != S_OK)
            return false;

    if (g_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_frameContext[0].CommandAllocator, nullptr, IID_PPV_ARGS(&g_pd3dCommandList)) != S_OK ||
        g_pd3dCommandList->Close() != S_OK)
        return false;

    if (g_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence)) != S_OK)
        return false;

    g_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (g_fenceEvent == nullptr) return false;

    {
        IDXGIFactory4* dxgiFactory = nullptr;
        IDXGISwapChain1* swapChain1 = nullptr;
        if (CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)) != S_OK) return false;
        if (dxgiFactory->CreateSwapChainForHwnd(g_pd3dCommandQueue, hWnd, &sd, nullptr, nullptr, &swapChain1) != S_OK) return false;
        if (swapChain1->QueryInterface(IID_PPV_ARGS(&g_pSwapChain)) != S_OK) return false;
        swapChain1->Release();
        dxgiFactory->Release();
        g_pSwapChain->SetMaximumFrameLatency(APP_NUM_BACK_BUFFERS);
        g_hSwapChainWaitableObject = g_pSwapChain->GetFrameLatencyWaitableObject();
    }

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->SetFullscreenState(false, nullptr); g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_hSwapChainWaitableObject != nullptr) { CloseHandle(g_hSwapChainWaitableObject); }
    for (UINT i = 0; i < APP_NUM_FRAMES_IN_FLIGHT; i++)
        if (g_frameContext[i].CommandAllocator) { g_frameContext[i].CommandAllocator->Release(); g_frameContext[i].CommandAllocator = nullptr; }
    if (g_pd3dCommandQueue) { g_pd3dCommandQueue->Release(); g_pd3dCommandQueue = nullptr; }
    if (g_pd3dCommandList) { g_pd3dCommandList->Release(); g_pd3dCommandList = nullptr; }
    if (g_pd3dRtvDescHeap) { g_pd3dRtvDescHeap->Release(); g_pd3dRtvDescHeap = nullptr; }
    if (g_pd3dSrvDescHeap) { g_pd3dSrvDescHeap->Release(); g_pd3dSrvDescHeap = nullptr; }
    if (g_fence) { g_fence->Release(); g_fence = nullptr; }
    if (g_fenceEvent) { CloseHandle(g_fenceEvent); g_fenceEvent = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }

#ifdef DX12_ENABLE_DEBUG_LAYER
    IDXGIDebug1* pDebug = nullptr;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
    {
        pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
        pDebug->Release();
    }
#endif
}

void CreateRenderTarget()
{
    for (UINT i = 0; i < APP_NUM_BACK_BUFFERS; i++)
    {
        ID3D12Resource* pBackBuffer = nullptr;
        g_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, g_mainRenderTargetDescriptor[i]);
        g_mainRenderTargetResource[i] = pBackBuffer;
    }
}

void CleanupRenderTarget()
{
    WaitForLastSubmittedFrame();
    for (UINT i = 0; i < APP_NUM_BACK_BUFFERS; i++)
        if (g_mainRenderTargetResource[i]) { g_mainRenderTargetResource[i]->Release(); g_mainRenderTargetResource[i] = nullptr; }
}

void WaitForLastSubmittedFrame()
{
    FrameContext* frameCtx = &g_frameContext[g_frameIndex % APP_NUM_FRAMES_IN_FLIGHT];
    UINT64 fenceValue = frameCtx->FenceValue;
    if (fenceValue == 0) return;
    frameCtx->FenceValue = 0;
    if (g_fence->GetCompletedValue() >= fenceValue) return;
    g_fence->SetEventOnCompletion(fenceValue, g_fenceEvent);
    WaitForSingleObject(g_fenceEvent, INFINITE);
}

FrameContext* WaitForNextFrameResources()
{
    UINT nextFrameIndex = g_frameIndex + 1;
    g_frameIndex = nextFrameIndex;
    HANDLE waitableObjects[] = { g_hSwapChainWaitableObject, nullptr };
    DWORD numWaitableObjects = 1;
    FrameContext* frameCtx = &g_frameContext[nextFrameIndex % APP_NUM_FRAMES_IN_FLIGHT];
    UINT64 fenceValue = frameCtx->FenceValue;
    if (fenceValue != 0)
    {
        frameCtx->FenceValue = 0;
        g_fence->SetEventOnCompletion(fenceValue, g_fenceEvent);
        waitableObjects[1] = g_fenceEvent;
        numWaitableObjects = 2;
    }
    WaitForMultipleObjects(numWaitableObjects, waitableObjects, TRUE, INFINITE);
    return frameCtx;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_CREATE:
        nid.cbSize = sizeof(nid);
        nid.hWnd = hWnd;
        nid.uID = 1;
        nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
        nid.uCallbackMessage = WM_TRAYICON;
        nid.hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_TRAYICON));
        wcscpy_s(nid.szTip, sizeof(nid.szTip) / sizeof(nid.szTip[0]), L"Snap Vim");
        Shell_NotifyIcon(NIM_ADD, &nid);
        break;

    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP)
        {
            HMENU hMenu = CreatePopupMenu();
            AppendMenu(hMenu, MF_STRING, ID_TRAY_SETTINGS, L"Settings");
            AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenu(hMenu, MF_STRING, ID_TRAY_EXIT, L"Quit");
            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(hWnd);
            TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);
            DestroyMenu(hMenu);
        }
        else if (lParam == WM_LBUTTONDBLCLK)
        {
            ShowWindow(hWnd, SW_SHOW);
            SetForegroundWindow(hWnd);
        }
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_TRAY_EXIT)
        {
            Shell_NotifyIcon(NIM_DELETE, &nid);
            PostQuitMessage(0);
        }
        else if (LOWORD(wParam) == ID_TRAY_SETTINGS)
        {
            ShowSettingsDialog();
        }
        break;

    case WM_CLOSE:
        ShowWindow(hWnd, SW_HIDE);
        return 0;

    case WM_KILLFOCUS:
        ShowWindow(hWnd, SW_HIDE);
        break;

    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
        {
            WaitForLastSubmittedFrame();
            CleanupRenderTarget();
            HRESULT result = g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
            assert(SUCCEEDED(result) && "Failed to resize swapchain.");
            CreateRenderTarget();
        }
        return 0;

    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;

    case WM_DESTROY:
        Shell_NotifyIcon(NIM_DELETE, &nid);
        ::PostQuitMessage(0);
        return 0;

    case WM_LBUTTONDOWN:
        ReleaseCapture();
        SendMessage(hWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        return 0;

    case WM_HOTKEY:
        if (wParam == 1)
        {
            if (IsWindowVisible(hWnd))
            {
                ShowWindow(hWnd, SW_HIDE);
            }
            else {
                g_previousFocus = GetForegroundWindow();

                g_grabbedText = GrabTextFromActiveWindow(g_previousFocus);
                g_hasGrabbedText = !g_grabbedText.empty();
                if (g_hasGrabbedText)
                    ClearActiveWindowContent(g_previousFocus);

                ShowWindow(hWnd, SW_SHOW);
                SetForegroundWindow(hWnd);
                SetFocus(hWnd);
            }
        }
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

# SnapVim Rust 迁移设计文档

> 日期: 2026-07-01
> 状态: 设计中

## 1. 概述

### 1.1 项目目标

将 SnapVim 从 C++ (libvim + Dear ImGui + DirectX 12) 迁移到新技术栈，实现：

- **跨平台支持**: Windows + macOS
- **完整 Vim 功能**: 查找替换、宏、寄存器、书签等（通过 Neovim 引擎）
- **系统剪贴板集成**: 复制粘贴 + "保存即粘贴回原窗口"
- **可配置 UI**: 通过托盘菜单配置尺寸、透明度、字体、颜色、行高等
- **默认暗色主题**: 开箱即用的 SnapDark 主题
- **语法高亮**: 通过 Neovim TreeSitter 自然支持多语言

### 1.2 技术架构

```
Tauri v2 (Rust shell)
  ├── 窗口管理: 无边框 + 透明 + 圆角 + 全局热键 + 系统托盘
  ├── 进程管理: spawn nvim --headless --embed
  └── TypeScript 前端 (React + Vite)
       ├── Canvas 2D 渲染: 文字、光标、选区、状态栏
       ├── 键盘输入 → nvim_input()
       └── 接收 ui_attach 事件流 → 渲染更新

Neovim 后端 (用户自行安装)
  ├── 完整 Vim 引擎
  ├── 查找替换、宏、:s、寄存器等
  ├── TreeSitter 语法高亮
  └── msgpack-rpc over stdio
```

### 1.3 为什么选择这个架构

**为什么不用纯 Rust (egui/wgpu)?**
- egui 的 IME 支持有已知问题
- 自定义渲染管线工作量大
- Web 技术栈开发速度更快，UI 灵活性更高

**为什么不用 Neovide (fork)?**
- Neovide 定位为全功能编辑器 GUI，SnapVim 是弹出式快捷编辑器
- 目标定位完全不同，fork 后需要大量删除和重构
- 对抗其"完整编辑器窗口"的设计假设

**为什么需要 Tauri 而不是纯 Web 前端?**
SnapVim 需要浏览器沙箱无法提供的原生能力：
- 全局热键 (Ctrl+Space)
- 系统托盘图标
- 无边框半透明窗口 + 置顶
- spawn nvim 子进程
- 写入系统剪贴板 + 粘贴回其他窗口
- 检测前台窗口 + 焦点丢失隐藏

**为什么用 TypeScript 前端而不是 Rust + Skia?**
- Canvas 2D 足够渲染等宽文本网格
- React 组件化开发设置 UI 更高效
- Web 生态丰富，开发速度快

## 2. 架构详细设计

### 2.1 整体数据流

```
用户按键
  → TS 前端捕获 keydown
  → 转换为 Neovim 按键格式 (如 "<Esc>", "<C-w>")
  → Tauri IPC: invoke("nvim_input", keys)
  → Rust 层调用 nvim_input(keys)
  → Neovim 处理并推送 ui_attach 事件
  → Rust 层转发事件到前端 (Tauri event system)
  → TS 前端更新本地状态 → Canvas 重绘

用户 :w / :wq
  → Neovim 触发 save 事件
  → Rust 层拦截 (通过 nvim rpcnotify)
  → 读取缓冲区内容
  → 写入系统剪贴板
  → 聚焦回之前记录的前台窗口
  → SendInput: Ctrl+V (Windows) / Cmd+V (macOS)
  → 隐藏 SnapVim 窗口
```

### 2.2 进程生命周期

**App 启动（开机自启或手动启动）:**
```
→ spawn nvim --headless --embed
→ 系统托盘常驻，窗口默认隐藏
→ 等待 Ctrl+Space 触发
```

**窗口内操作（不退出进程）:**
- `:w` / `:write` → 拷贝缓冲区内容 → 粘贴回原窗口 → 隐藏窗口
- `:wq` / `:x` → 同上
- `:q` / `:q!` → 隐藏窗口（不保存）

**App 关闭（通过托盘菜单）:**
```
→ 发送 nvim_command("qa!")
→ 等待子进程退出（超时强杀）
→ 退出应用
```

**Neovim 按键重映射（Rust 层启动后注入）:**
```lua
vim.keymap.set('n', ':w<CR>',  function() vim.rpcnotify(0, 'snapvim:save') end)
vim.keymap.set('n', ':wq<CR>', function() vim.rpcnotify(0, 'snapvim:save_quit') end)
vim.keymap.set('n', ':q<CR>',  function() vim.rpcnotify(0, 'snapvim:hide') end)
```

Rust 监听这些 notification，执行对应的窗口操作。

## 3. Neovim 进程管理与通信层

### 3.1 Neovim 查找策略（仅支持用户安装）

启动时查找顺序：
1. 用户配置的自定义路径（config.json 中）
2. 系统 PATH 中的 `nvim` / `nvim.exe`
3. 常见安装路径:
   - Windows: `C:\Program Files\Neovim\bin\nvim.exe`, `%LOCALAPPDATA%\nvim-win64\bin\nvim.exe`
   - macOS: `/opt/homebrew/bin/nvim`, `/usr/local/bin/nvim`
4. 均未找到 → 弹出提示窗口 + 提供下载链接

### 3.2 进程生命周期

```
App 启动
  → 查找 nvim 路径
  → spawn: nvim --headless --embed --clean
  → 等待 nvim 就绪（channel_id 返回）
  → 调用 nvim_ui_attach(width, height, options)
  → 注入按键重映射和 colorscheme
  → 进入就绪状态

App 关闭
  → 发送 nvim_command("qa!")
  → 等待子进程退出（超时强杀）
```

### 3.3 通信架构

```
TypeScript 前端                    Rust 后端                     Neovim
    │                                │                             │
    │── invoke("nvim_input", keys) ──│                             │
    │                                │── nvim_input(keys) ─────────│
    │                                │                             │
    │                                │◄── grid_line event ─────────│
    │◄── event("nvim-ui", data) ─────│                             │
    │                                │                             │
    │── invoke("nvim_command", cmd) ─│                             │
    │                                │── nvim_command(cmd) ────────│
```

**Rust 层职责:**
- 管理 nvim 子进程的 spawn/kill
- 运行 msgpack-rpc 编解码（使用 `rmpv` + `tokio`）
- 转发 `ui_attach` 事件到前端（通过 Tauri event system）
- 暴露 `invoke` 命令给前端调用（`nvim_input`, `nvim_command`, `nvim_buf_get_lines` 等）
- 拦截 `:w` / `:wq` 写入事件，重定向为"读取缓冲区 → 粘贴回原窗口"

**前端职责:**
- 键盘事件捕获 → 转换为 Neovim 按键格式 → invoke 调用
- 接收 ui 事件 → 更新渲染状态（grid、cursor、mode、cmdline）
- 维护本地状态缓存（避免每帧都查询 Neovim）

## 4. 前端渲染层

### 4.1 渲染方案

**Canvas 2D**（非 WebGL/WebGPU）

理由：SnapVim 渲染的是等宽字体的文本网格，Canvas 2D 足够且实现简单，无需引入 GPU 管线。

### 4.2 渲染模型 — Grid-based

```
Neovim 维护一个字符网格 (rows × cols)
每个 cell = { char, hl_id }
hl_id 映射到一组样式 { fg, bg, bold, italic, underline }

前端维护一个相同大小的二维数组作为本地缓存
收到 grid_line 事件 → 更新本地缓存 → 标记脏行 → 重绘
收到 flush 事件 → 一次性提交所有脏行绘制
```

### 4.3 Canvas 绘制层级

```
Layer 1: 背景色填充（按 cell 的 bg 色）
Layer 2: 当前行高亮
Layer 3: 选区高亮（Visual mode）
Layer 4: 文字渲染（measureText + fillText，逐 cell）
Layer 5: 光标（块/线/I-beam，根据模式切换）+ 闪烁动画
Layer 6: 命令行栏（CMDLINE 模式时在底部显示）
```

### 4.4 字体处理

**等宽字体 + 中文 fallback 方案:**
- 英文等宽字体（JetBrains Mono）作为主要字体，1 cell 宽
- 中文字体（Noto Sans SC）作为 fallback，2 cell 宽（全角）
- 使用 CSS `@font-face` 加载用户配置的字体
- 通过 `measureText` 获取精确 cell 宽度
- 通过 Unicode 范围判断是否为 CJK 字符，渲染宽度 = `cell_width × 2`

这是编辑器界的通用方案（VS Code、Neovide 均采用）。

### 4.5 键盘输入处理

```
keydown 事件
  → 转换为 Neovim 按键字符串（如 "i", "<Esc>", "<C-w>", "<S-Tab>"）
  → 特殊键映射表（Enter→<CR>, Backspace→<BS>, Arrow→<Up>等）
  → invoke("nvim_input", key_string)
```

### 4.6 窗口大小同步

```
窗口 resize
  → 计算新的 grid 尺寸 (cols = width/cell_width, rows = height/cell_height)
  → invoke("nvim_ui_try_resize", cols, rows)
  → Neovim 自动重排并推送新的 grid 事件
```

### 4.7 多光标渲染

- 多光标功能通过 Neovim 插件实现（如 `vim-visual-multi`），由用户自行安装配置
- SnapVim 不捆绑任何 Neovim 插件，仅提供渲染支持
- 前端按 grid_line 事件自然渲染，无需额外逻辑
- 如果用户未安装多光标插件，该功能不可用（不影响核心编辑）

## 5. 窗口与 OS 集成层

### 5.1 窗口属性（Tauri v2 配置）

```rust
decorations: false          // 无边框
transparent: true           // 透明背景
always_on_top: true         // 置顶
resizable: true             // 可调大小
skip_taskbar: true          // 不在任务栏显示
```

圆角通过平台 API 实现：
- Windows: `DwmSetWindowAttribute(DWMWA_WINDOW_CORNER_PREFERENCE)`
- macOS: NSWindow 原生圆角

### 5.2 全局热键

```
默认: Ctrl+Space
自定义: 设置页面修改，持久化到配置文件

触发流程:
  1. 记录当前前台窗口句柄（HWND / NSRunningApplication）
  2. 检测前台窗口是否包含可编辑文本控件
     → Windows: 使用 UI Automation API 查询焦点元素是否为文本输入控件
     → macOS: 使用 Accessibility API (AXUIElement) 查询焦点元素的 AXRole 是否为文本字段
     → 是: SendInput Ctrl+A, Ctrl+C → 读取剪贴板 → 检查大小上限（默认 64KB）
     → 否: 跳过
  3. 如果 SnapVim 窗口已显示 → 隐藏
  4. 如果已隐藏 → 定位到鼠标附近/屏幕中央 → 显示并聚焦
  5. 如果步骤 2 获取了文本 → 清空缓冲区 → 写入内容
```

### 5.3 焦点丢失处理

```
窗口 blur 事件
  → invoke("hide_window")
  → Rust 层隐藏窗口（不退出 nvim）
```

### 5.4 系统托盘

| 菜单项 | 行为 |
|---|---|
| 显示编辑器 | 显示窗口 |
| 设置 | 打开设置窗口（独立小窗口） |
| 退出 | 终止 nvim → 退出应用 |

托盘图标：
- Windows: 复用现有 `res/snapvim_32x32.ico`
- macOS: 使用 Template Image（自动适配深色模式）

### 5.5 "粘贴回原窗口"流程（`:w` 触发）

```
Rust 层拦截 save notification
  → nvim_buf_get_lines(全量缓冲区)
  → 写入系统剪贴板
  → 聚焦回之前记录的前台窗口
  → SendInput: Ctrl+V（Windows）/ Cmd+V（macOS）
  → 隐藏 SnapVim 窗口
```

### 5.6 平台差异封装

```rust
trait PlatformBridge {
    fn get_foreground_window() -> WindowHandle;
    fn focus_window(handle: WindowHandle);
    fn send_paste(handle: WindowHandle);        // Ctrl+V / Cmd+V
    fn send_select_all_and_copy(handle: WindowHandle); // Ctrl+A+C
    fn set_window_rounded_corners(hwnd: ...);
}

// Windows 实现: Win32 API (SendInput, SetForegroundWindow, ...)
// macOS 实现:   Accessibility API (AXUIElement) + CGEvent
```

### 5.7 DPI 感知

- Tauri v2 内置 DPI 支持，`window.scale_factor()` 获取缩放比
- 字体大小和窗口尺寸均乘以 scale_factor
- 窗口弹出位置根据当前显示器 DPI 计算

## 6. 配置系统

### 6.1 设置窗口形态

**独立小窗口**（非主编辑器窗口），从托盘菜单 "设置" 打开。

- 不触发"焦点丢失隐藏"
- 不置顶
- 标准窗口样式（有标题栏、可关闭）

原因：主窗口有焦点丢失自动隐藏的行为，如果设置 UI 嵌在主窗口里，用户切到其他地方查看效果时窗口会消失。

### 6.2 配置项

| 分类 | 配置项 | 类型 | 默认值 |
|---|---|---|---|
| **窗口** | 宽度 | number (px) | 640 |
| | 高度 | number (px) | 400 |
| | 透明度 | slider 0-100% | 94% (alpha 240) |
| | 圆角 | toggle | 开 |
| **字体** | 英文字体 | text | JetBrains Mono |
| | 中文字体 | text | Noto Sans SC |
| | 字号 | number (px) | 16 |
| | 行高 | slider 1.0-2.0 | 1.5 |
| **主题** | 预设主题 | select | SnapDark |
| | 背景色 | color picker | #282828 |
| | 前景色 | color picker | #ebdbb2 |
| | 光标颜色 | color picker | #505078 |
| | 当前行高亮色 | color picker | #3c3c3c |
| | 选区颜色 | color picker | #787878B4 |
| **快捷键** | 唤起热键 | keybind | Ctrl+Space |
| **行为** | 内容复制上限 | number (KB) | 64 |
| **Neovim** | 自定义路径 | text | (自动检测) |

### 6.3 存储

配置文件路径：
```
Windows: %APPDATA%/snapvim/config.json
macOS:   ~/Library/Application Support/snapvim/config.json
```

JSON 格式，仅保存用户修改过的项（与默认值合并）。

### 6.4 配置生效机制

```
用户在设置页修改值
  → 写入 config.json
  → Tauri event 通知主窗口
  → 窗口属性变更（尺寸/透明度/圆角）→ Rust 层立即应用
  → 渲染属性变更（字体/颜色/行高）→ TS 前端立即重绘
  → 热键变更 → Rust 层注销旧热键 + 注册新热键
```

### 6.5 预设主题（SnapDark）

基于 Gruvbox Dark 调色板，开箱即用：

```json
{
  "name": "SnapDark",
  "background": "#282828",
  "foreground": "#ebdbb2",
  "cursor": "#505078",
  "current_line": "#3c3c3c",
  "selection": "#787878B4",
  "comment": "#928374",
  "keyword": "#fb4934",
  "string": "#b8bb26",
  "number": "#d3869b",
  "function": "#fabd2f",
  "type": "#83a598",
  "operator": "#fe8019"
}
```

**Neovim colorscheme 注入机制：**
- colorscheme 文件位于 `nvim-config/colors/snapdark.lua`
- Rust 层启动 nvim 时通过 `--cmd "set rtp+=<snapvim_resource_path>"` 注入运行时路径
- 启动后执行 `vim.cmd("colorscheme snapdark")` 加载主题
- 前端只需渲染 Neovim 传来的 hl 属性，主题预设同时包含一份 Neovim colorscheme 文件，确保编辑器内颜色和设置面板预览一致

## 7. 项目结构与构建

### 7.1 项目目录结构

```
SnapVim/
├── src-tauri/                    # Rust 后端
│   ├── Cargo.toml
│   ├── tauri.conf.json           # Tauri 配置（窗口、托盘、权限）
│   ├── src/
│   │   ├── main.rs               # 入口
│   │   ├── lib.rs                # Tauri 命令注册
│   │   ├── nvim/
│   │   │   ├── mod.rs
│   │   │   ├── process.rs        # nvim 子进程 spawn/kill/查找
│   │   │   ├── rpc.rs            # msgpack-rpc 编解码
│   │   │   └── bridge.rs         # ui_attach 事件转发 + :w 拦截
│   │   ├── window/
│   │   │   ├── mod.rs
│   │   │   └── manager.rs        # 窗口显示/隐藏/定位
│   │   ├── platform/
│   │   │   ├── mod.rs
│   │   │   ├── windows.rs        # Win32: SendInput, HWND, 热键
│   │   │   └── macos.rs          # macOS: AXUIElement, CGEvent
│   │   ├── tray/
│   │   │   └── mod.rs            # 托盘菜单与图标
│   │   ├── config/
│   │   │   └── mod.rs            # 配置读写与热重载
│   │   └── hotkey/
│   │       └── mod.rs            # 全局热键注册/注销
│   ├── resources/
│   │   ├── snapvim_32x32.ico
│   │   └── snapvim_icon.png      # macOS 托盘用
│   └── capabilities/             # Tauri v2 权限声明
│
├── src/                          # TypeScript 前端
│   ├── main.ts                   # 入口
│   ├── App.tsx                   # 根组件
│   ├── nvim/
│   │   ├── client.ts             # Tauri IPC → nvim 调用封装
│   │   ├── ui-events.ts          # ui_attach 事件类型定义与处理
│   │   └── keymap.ts             # 键盘事件 → Neovim 按键转换
│   ├── renderer/
│   │   ├── grid.ts               # 字符网格状态管理
│   │   ├── canvas.ts             # Canvas 2D 绘制逻辑
│   │   ├── cursor.ts             # 光标渲染与闪烁
│   │   ├── cmdline.ts            # 命令行栏渲染
│   │   └── highlight.ts          # hl_id → CSS 颜色映射
│   ├── settings/
│   │   ├── SettingsApp.tsx       # 设置窗口根组件
│   │   ├── WindowSettings.tsx    # 窗口设置页
│   │   ├── FontSettings.tsx      # 字体设置页
│   │   ├── ThemeSettings.tsx     # 主题设置页
│   │   └── HotkeySettings.tsx    # 快捷键设置页
│   ├── theme/
│   │   ├── snap-dark.ts          # 默认 SnapDark 主题定义
│   │   └── types.ts              # 主题类型
│   └── styles/
│       └── global.css            # 全局样式
│
├── nvim-config/                  # 注入给 Neovim 的 init 配置
│   ├── init.lua                  # :w 重映射、选项设置等
│   └── colors/
│       └── snapdark.lua          # SnapDark 主题 colorscheme
│
├── package.json
├── tsconfig.json
├── vite.config.ts                # 前端构建（Vite）
├── .gitignore
├── README.md
└── LICENSE
```

### 7.2 技术栈

| 层 | 选型 |
|---|---|
| Rust 后端框架 | Tauri v2 |
| 前端框架 | React + TypeScript |
| 前端构建 | Vite |
| Rust msgpack | `rmpv`（轻量 msgpack 序列化） |
| Rust RPC 通信 | `tokio`（async runtime）+ 手写 msgpack-rpc 协议 |
| 设置 UI | React 组件（独立窗口入口） |

### 7.3 Neovim 分发策略

**仅支持用户自行安装**，不提供预安装。

设置页面提供：
- "Neovim 路径" 输入框 + 文件选择器
- "检测" 按钮（验证路径是否有效 + 显示版本号）
- 未检测到时的引导文案 + 官方下载链接

安装包体积：**~5MB**（仅 Tauri + 前端）。

### 7.4 跨平台构建

| 平台 | 构建环境 | 安装包格式 |
|---|---|---|
| Windows x64 | GitHub Actions (windows-latest) | NSIS (.exe 安装器) + 便携 .zip |
| macOS arm64 | GitHub Actions (macos-latest) | DMG |
| macOS x64 | GitHub Actions (macos-latest) | DMG |

**CI 流水线:**
```
push tag → GitHub Actions
  → 并行构建 Windows + macOS
  → tauri build
  → 上传到 GitHub Releases
```

## 8. 关键实现细节

### 8.1 内容复制上限

默认 64KB（约几万行纯文本），可在设置中调整。超出时静默截断而非弹窗打断用户。

### 8.2 多光标支持

通过 Neovim 插件（如 `vim-visual-multi`）实现，前端无需额外逻辑，按 grid_line 事件自然渲染。

### 8.3 LSP 支持

**当前版本不包含 LSP 功能**，语法高亮通过 Neovim TreeSitter 自然支持。LSP 诊断和补全可作为未来扩展。

### 8.4 多语言支持

Neovim 通过 TreeSitter 支持 300+ 种语言的语法高亮，前端无需关心具体语言。

## 9. 迁移路径

### 阶段 1: 基础架构搭建
- Tauri v2 项目初始化
- Neovim 进程管理与 RPC 通信
- 基础窗口显示与隐藏

### 阶段 2: 核心编辑功能
- Canvas 2D 渲染实现
- 键盘输入处理
- ui_attach 事件处理
- 光标与选区渲染

### 阶段 3: OS 集成
- 全局热键
- 系统托盘
- 焦点丢失隐藏
- "粘贴回原窗口"

### 阶段 4: 配置系统
- 设置窗口 UI
- 配置持久化
- 主题系统
- SnapDark 默认主题

### 阶段 5: 跨平台适配
- Windows 平台实现
- macOS 平台实现
- DPI 感知
- 跨平台测试

### 阶段 6: 打磨与发布
- 性能优化
- 错误处理与日志
- CI/CD 流水线
- 文档与用户引导

## 10. 风险与缓解

| 风险 | 影响 | 缓解措施 |
|---|---|---|
| Neovim UI 协议兼容性 | 某些 Vim 操作可能渲染异常 | 充分测试常用操作，维护事件处理覆盖 |
| macOS Accessibility API 权限 | 粘贴回原窗口可能失败 | 首次启动引导用户授权，提供手动粘贴备选 |
| Canvas 2D 渲染性能 | 大量文本时可能卡顿 | 脏行标记 + 局部重绘，必要时升级到 WebGL |
| 中文字体宽度不一致 | 网格对齐错乱 | 严格使用等宽英文字体 + 2x cell 中文 fallback |
| Tauri v2 跨平台差异 | 窗口行为不一致 | 抽象 PlatformBridge trait，平台特定实现 |

## 11. 总结

本设计将 SnapVim 从 Windows 专属的 C++ 项目迁移为跨平台的现代化应用：

- **技术栈**: Tauri v2 (Rust) + TypeScript (React) + Neovim
- **核心优势**: 完整 Vim 功能 + 轻量安装包 (~5MB) + 跨平台
- **开发效率**: Web 前端 + 成熟生态
- **用户体验**: 保留全局热键、托盘常驻、保存即粘贴的核心体验

架构清晰、模块化、可扩展，为未来功能（LSP、插件系统等）预留空间。

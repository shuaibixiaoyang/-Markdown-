# CuteMarkEd — Qt Markdown 编辑器

一款基于 Qt 6 的免费开源 Markdown 编辑器，支持实时 HTML 预览、数学公式、代码语法高亮、Mermaid 图表、拼写检查、学术引用管理、AI 辅助写作等功能。

---

## 功能概览

| 分类 | 功能 |
|------|------|
| **编辑** | Markdown 语法高亮、行号显示、代码片段补全、单词自动补全、查找/替换、YAML 头信息 |
| **预览** | 实时 HTML 预览、编辑器-预览同步滚动、Reveal.js 演示模式、自定义 CSS 主题 |
| **导出** | HTML 导出、PDF 导出 |
| **学术** | BibTeX 引用管理（APA / MLA / Chicago / IEEE / GB/T 7714）、数学公式 |
| **图表** | Mermaid 流程图、时序图、甘特图 |
| **语言** | 拼写检查（Hunspell）、多语言界面（中/英/德/法/俄等）、多引擎翻译（百度/有道/DeepL/Google） |
| **AI** | AI 辅助写作、OCR 文字识别 |
| **协作** | 版本管理、云同步、文档分享 |
| **其他** | Font Awesome 图标引擎、模板系统、文件浏览器、数据可视化 |

---

## 项目结构

```
CuteMarkEd/
├── CuteMarkEd.pro          # 主项目文件（subdirs 模板）
│
├── app/                     # 主应用程序（GUI 层）
│   ├── main.cpp             # 程序入口
│   ├── mainwindow.cpp/h     # 主窗口（菜单、工具栏、信号槽）
│   ├── options.cpp/h        # 应用设置
│   ├── markdownhighlighter  # Markdown 语法高亮器
│   ├── htmlhighlighter      # HTML 语法高亮器
│   ├── htmlpreviewgenerator # HTML 预览生成器
│   ├── markdownmanipulator  # 文本操作工具
│   ├── snippetcompleter     # 代码片段补全
│   ├── controls/            # 自定义控件（文件浏览器、查找替换、行号区域）
│   ├── icons/               # 应用图标
│   ├── styles/              # CSS 样式表
│   ├── themes/              # 编辑器主题
│   ├── translations/        # 国际化翻译文件
│   └── hunspell/            # 拼写检查词典
│
├── app-static/              # 核心引擎（静态库，非 GUI）
│   ├── htmlpreviewcontroller    # HTML 预览控制器
│   ├── htmlviewsynchronizer     # 编辑器-预览同步
│   ├── revealviewsynchronizer   # 演示文稿同步
│   ├── yamlheaderchecker        # YAML 头信息检查
│   ├── datalocation             # 数据路径管理
│   ├── completionlistmodel      # 补全建议模型
│   │
│   ├── academic/            # 学术功能（引用管理 CitationManager）
│   ├── ai/                  # AI 集成
│   ├── annotation/          # 文档注释
│   ├── collaboration/       # 多人协作
│   ├── converter/           # Markdown 转换器（Discount / Hoedown）
│   ├── datavisualization/   # 数据可视化
│   ├── editor/              # 编辑器核心
│   ├── export/              # 导出格式处理
│   ├── extension/           # 插件/扩展系统
│   ├── library/             # 文档库管理
│   ├── ocr/                 # OCR 文字识别
│   ├── preview/             # 预览渲染
│   ├── rendering/           # 渲染引擎
│   ├── revision/            # 版本追踪
│   ├── search/              # 搜索功能
│   ├── security/            # 安全功能
│   ├── sharing/             # 文件分享
│   ├── snippets/            # 代码片段
│   ├── spellchecker/        # 拼写检查
│   ├── sync/                # 云同步
│   ├── template/            # 模板系统
│   ├── themes/              # 主题管理
│   ├── translation/         # 翻译引擎（百度/有道/DeepL/Google）
│   ├── vcs/                 # 版本控制集成
│   └── writing/             # 写作辅助工具
│
├── libs/                    # 自定义库
│   ├── jsonconfig/          # JSON 配置文件读写
│   └── peg-markdown-highlight/ # PEG Markdown 解析器封装
│
├── 3rdparty/                # 第三方依赖
│   ├── discount/            # Discount Markdown 解析库 (BSD)
│   ├── hoedown/             # Hoedown Markdown 解析库
│   ├── hunspell/            # Hunspell 拼写检查引擎 (LGPL v2.1)
│   └── peg-markdown-highlight/ # PEG Markdown 语法高亮 (MIT)
│
├── fontawesomeicon/         # Font Awesome 图标引擎插件
│   ├── fontawesomeiconengine    # 图标渲染引擎
│   ├── fontawesome.ttf          # 字体文件
│   └── fontawesomeicon.json     # 图标定义
│
└── test/                    # 测试
    ├── unit/                # 单元测试
    └── integration/         # 集成测试
```

---

## 技术架构

### 构建系统

使用 qmake subdirs 模板，构建顺序：

```
3rdparty → libs → app-static → app / test
```

### 依赖项

| 依赖 | 版本 | 用途 | 许可证 |
|------|------|------|--------|
| Qt | 6.10.1 | GUI 框架 | LGPL v3 |
| Discount | 2.1.7+ | Markdown → HTML 转换 | BSD 3-Clause |
| Hoedown | — | 备选 Markdown 解析器 | ISC |
| PEG Markdown Highlight | — | 语法高亮 | MIT |
| Hunspell | 1.3.2+ | 拼写检查 | LGPL v2.1 |

> **说明：** 以上依赖的源码已包含在项目 `3rdparty/` 目录中，随项目一起编译，**无需单独安装**。

### 条件编译宏

在 MinGW / 无 WebEngine 环境下，通过以下宏裁剪功能：

| 宏 | 说明 |
|----|------|
| `NO_WEBENGINE` | 禁用 Qt WebEngine（改用 QTextBrowser 预览） |
| `NO_PDFWIDGETS` | 禁用 PDF 导出 |
| `NO_WEBSOCKETS` | 禁用 WebSocket 协作功能 |
| `NO_OPENSSL` | 禁用 OpenSSL 相关功能 |

---

## 环境要求

### macOS

| 项目 | 要求 |
|------|------|
| 操作系统 | macOS 12+ (Apple Silicon / Intel) |
| 编译器 | Apple Clang 14+（Xcode 自带） |
| Qt | Qt 6.10.1（已实测可编译运行，优先推荐） |
| 构建工具 | qmake6 |
| OpenSSL | OpenSSL 3.x（当前 macOS 构建链路需要） |

> `Windows MinGW` 在定义 `NO_OPENSSL` 的裁剪构建下可不依赖 OpenSSL。

### Windows

| 项目 | 要求 |
|------|------|
| 操作系统 | Windows 10/11 64-bit |
| 编译器 | 推荐 MSVC 2019/2022 64-bit；MinGW 13.1.0 64-bit 仅建议用于裁剪构建 |
| Qt | 推荐 Qt 6.10.1 |
| 构建工具 | qmake |

> **Windows 构建建议：**
> 推荐使用 `Qt 6.10.1 + MSVC 2019/2022 64-bit` 进行完整功能构建。
> `MinGW` 仅建议用于定义 `NO_WEBENGINE` 等宏后的裁剪版构建。

---

## 编译与运行

### macOS

```bash
# 1. 安装已验证版本依赖（推荐）
brew install qt@6 openssl@3

# 2. 确认 qmake6 指向 Qt 6.10.1
qmake6 --version

# 3. 克隆项目
git clone <仓库地址>
cd CuteMarkEd

# 4. 构建
qmake6 CuteMarkEd.pro
make -j$(sysctl -n hw.ncpu)

# 5. 运行
open app/cutemarked.app
# 或
./app/cutemarked.app/Contents/MacOS/cutemarked
```

> **注意：** macOS 上请使用 `qmake6` 而非 `qmake`，后者可能指向系统自带的 Qt 5。
>
> **建议：** 如果 `qmake6 --version` 显示的不是 `Qt version 6.10.1`，请先切换到对应 Qt 环境后再编译。

### Windows (MinGW)

```batch
REM 1. 安装 Qt 6（使用 Qt 在线安装器，勾选 MinGW 13.1.0 64-bit）

REM 2. 打开 Qt MinGW 终端（开始菜单 → Qt 6.x.x → MinGW 命令行）

REM 3. 进入项目目录
cd CuteMarkEd

REM 4. 构建
qmake CuteMarkEd.pro
mingw32-make -j%NUMBER_OF_PROCESSORS%

REM 5. 运行
app\release\cutemarked.exe
```

### Windows (MSVC)

```batch
REM 1. 安装 Visual Studio 2019/2022 + Qt 6（勾选 MSVC 组件）

REM 2. 打开 "Qt MSVC 命令行" 或 "x64 Native Tools Command Prompt"

REM 3. 构建
qmake CuteMarkEd.pro
nmake
```

---

## IDE 配置

### Qt Creator

1. 打开 Qt Creator → **文件** → **打开文件或项目**
2. 选择项目根目录的 `CuteMarkEd.pro`
3. 在 **项目** 面板选择 Qt 6 Kit：
   - macOS：**Qt 6.x.x clang 64bit**
   - Windows：**Qt 6.x.x MinGW 64-bit** 或 **Qt 6.x.x MSVC2019 64bit**
4. 点击 **Configure Project**
5. 点击左下角 **运行**（或 `Ctrl+R` / `Cmd+R`）

### VS Code

安装 **C/C++**（Microsoft）扩展，然后在终端中手动执行 `qmake6 && make` 编译即可。

---

## 跨平台兼容说明

本项目同时支持 macOS 和 Windows，代码中通过条件编译处理平台差异：

- **macOS (Clang + Qt 6)**：完整功能，使用 Qt WebEngine 预览。
- **Windows (MinGW + Qt 6)**：MinGW 不支持 Qt WebEngine，需定义 `NO_WEBENGINE` 等宏，使用 QTextBrowser 作为预览替代方案。PDF 导出等依赖 WebEngine 的功能同时被禁用。
- **Windows (MSVC + Qt 6)**：完整功能支持。

macOS 下动态库使用 `@rpath` 加载路径，在 `.pro` 文件中通过 `QMAKE_SONAME_PREFIX` 配置。

---

## 许可证

本项目使用 GPL v3 许可证。第三方依赖各有独立许可证，详见 [LICENSE.md](LICENSE.md)。

# PDB Insight 核心文档

## 项目概述

PDB Insight 是一款功能强大、轻量级的 PDB（Program Database）文件查看工具。该工具基于 Windows Win32 API 和 DIA SDK（Debug Interface Access）开发，提供了完整的 PDB 文件解析、查看和导出功能。

### 技术栈

- **开发语言**: C++17
- **GUI框架**: Win32 API
- **开发环境**: Visual Studio 2022 (v143 工具集)
- **核心依赖**: DIA SDK (msdia140.dll)
- **控件**: TreeView、ListView、TabCtrl、RichEdit
- **字符编码**: Unicode

### 系统要求

- Windows 7 或更高版本
- Visual Studio 2022（用于编译）
- msdia140.dll（DIA SDK，随 Visual Studio 安装）

---

## 项目结构

```
PDBViewer/
├── src/
│   ├── Core/                    # 核心窗口和应用程序逻辑
│   │   ├── MainWindow.cpp/h     # 主窗口实现
│   │   └── PDBViewer.cpp/h      # 应用程序入口点
│   ├── Managers/                # 各种管理器模块
│   │   ├── ConfigManager.cpp/h  # 配置管理
│   │   ├── ControlsManager.cpp/h # 控件管理
│   │   ├── DragDropManager.cpp/h # 拖放功能管理
│   │   ├── DPIManager.cpp/h     # DPI缩放管理
│   │   ├── ExportManager.cpp/h  # 导出功能管理
│   │   ├── ExportEnhancer.cpp/h # 导出增强功能
│   │   ├── FontManager.cpp/h    # 字体管理
│   │   ├── FunctionInfoDisplayManager.cpp/h # 函数信息显示管理
│   │   ├── GlobalVarAddressManager.cpp/h # 全局变量地址管理
│   │   ├── HeaderViewManager.cpp/h # 头文件视图管理
│   │   ├── LanguageManager.cpp/h # 多语言支持管理
│   │   ├── ListViewManager.cpp/h # 列表视图管理
│   │   ├── MenuManager.cpp/h    # 菜单管理
│   │   ├── PDBViewerGlobals.cpp/h # 全局变量和常量
│   │   ├── SearchManager.cpp/h  # 搜索功能管理
│   │   ├── SearchOptimizer.cpp/h # 搜索优化
│   │   ├── SettingsManager.cpp/h # 设置窗口管理
│   │   └── TreeViewManager.cpp/h # 树形视图管理
│   └── PDB/                     # PDB解析和导出功能
│       ├── PDBData.h            # 数据结构定义
│       ├── PDBParser.cpp/h      # PDB解析器
│       ├── PDBExporter.cpp/h    # 导出功能
│       ├── PDBHeaderGenerator.cpp/h # 头文件生成器
│       ├── dia2.h               # DIA SDK接口定义
│       ├── diaCreate.cpp/h      # DIA创建辅助函数
│       └── cvConst.h            # CodeView常量定义
├── docs/                        # 文档目录
├── zh-CN.json                   # 中文语言文件
├── en-US.json                   # 英文语言文件
├── PDBViewer.sln                # Visual Studio解决方案
├── PDBViewer.vcxproj            # 项目文件
├── PDBViewer.rc                 # 资源文件
├── Resource.h                   # 资源ID定义
├── framework.h                  # 框架头文件
└── targetver.h                  # 目标版本定义
```

---

## 核心模块详解

### 1. PDB解析模块 (src/PDB/)

#### PDBData.h - 数据结构定义

定义了所有用于存储PDB信息的数据结构：

| 结构体 | 描述 |
|--------|------|
| `FunctionInfo` | 函数信息（名称、返回类型、调用约定、参数、RVA等） |
| `ClassInfo` | 类/结构体/联合体信息（名称、大小、成员、基类、虚函数等） |
| `MemberVariableInfo` | 成员变量信息（名称、类型、偏移、访问权限、位域信息） |
| `BaseClassInfo` | 基类信息（名称、继承类型、偏移、访问权限） |
| `VirtualFunctionInfo` | 虚函数信息（名称、返回类型、参数、vtable索引） |
| `EnumInfo` | 枚举信息（名称、底层类型、枚举值列表） |
| `GlobalVariableInfo` | 全局变量信息（名称、类型、RVA、大小） |
| `ModuleInfo` | 模块信息（包含所有上述信息的集合） |

#### 枚举类型

```cpp
enum NumberDisplayMode {
    NUMBER_HEX,     // 十六进制显示
    NUMBER_DEC,     // 十进制显示
    NUMBER_BOTH     // 两者都显示
};

enum AccessType {
    PDB_ACCESS_PUBLIC,
    PDB_ACCESS_PROTECTED,
    PDB_ACCESS_PRIVATE,
    PDB_ACCESS_UNKNOWN
};

enum CallingConvention {
    PDB_CALL_CDECL,
    PDB_CALL_STDCALL,
    PDB_CALL_FASTCALL,
    PDB_CALL_VECTORCALL,
    PDB_CALL_THISCALL,
    PDB_CALL_UNKNOWN
};

enum InheritanceType {
    PDB_INHERITANCE_NORMAL,
    PDB_INHERITANCE_VIRTUAL,
    PDB_INHERITANCE_UNKNOWN
};
```

#### PDBParser - PDB解析器

核心解析类，使用DIA SDK解析PDB文件：

**主要方法**:
- `LoadPDB(pdbPath)` - 加载PDB文件
- `ParseModule()` - 解析整个模块，返回ModuleInfo
- `Unload()` - 卸载当前PDB

**解析流程**:
1. 初始化COM
2. 创建DIA数据源（msdia140.dll）
3. 加载PDB文件
4. 打开DIA会话
5. 获取全局作用域符号
6. 解析各类符号（函数、类、结构体、联合体、枚举、全局变量）

#### PDBExporter - 导出功能

提供多种导出格式：

- `ExportToCSV()` - 导出为CSV格式
- `ExportToXML()` - 导出为XML格式
- `ExportFunctionsToCSV()` - 仅导出函数
- `ExportClassesToCSV()` - 仅导出类
- `ExportGlobalVariablesToCSV()` - 导出全局变量

#### PDBHeaderGenerator - 头文件生成器

自动生成C++头文件声明：

- `GenerateClassHeader()` - 生成类头文件
- `GenerateStructDeclaration()` - 生成结构体声明
- `GenerateUnionDeclaration()` - 生成联合体声明
- `GenerateEnumDeclaration()` - 生成枚举声明
- `GenerateAllHeaders()` - 批量生成所有头文件
- `GeneratePureCStructDeclaration()` - 生成IDA兼容的纯C结构体

---

### 2. 管理器模块 (src/Managers/)

#### ConfigManager - 配置管理

单例模式实现，管理应用程序配置：

- 语言设置
- 数值显示模式（十六进制/十进制/两者）
- 是否展开基类成员
- 导出设置（ExportSettings）
- 搜索历史记录

配置存储在用户AppData目录下的JSON文件中。

#### LanguageManager - 多语言支持

单例模式，支持中英文切换：

- 从JSON文件加载语言字符串
- 自动检测系统语言
- 提供便捷宏 `LANG_STR(key)` 获取本地化字符串

#### TreeViewManager - 树形视图管理

管理左侧树形视图：

- `PopulateTreeView()` - 填充树形视图
- `AddTreeItem()` - 添加树节点
- `OpenPDBFile()` - 打开PDB文件对话框
- `ClosePDBFile()` - 关闭当前PDB

树形结构：
```
模块名称
├── 函数
├── 类
├── 结构体
├── 联合体
├── 枚举
└── 全局变量
```

#### ListViewManager - 列表视图管理

管理右侧"详细信息"标签页的列表视图：

- `PopulateListView()` - 根据选中的树节点填充列表
- `AddListViewColumn()` - 添加列

显示内容包括：
- 函数：名称、返回类型、调用约定、RVA、大小、类名
- 类/结构体：名称、大小、成员数量、基类数量、虚函数数量
- 成员变量：名称、类型、偏移、访问权限、位域信息
- 枚举：名称、值

#### HeaderViewManager - 头文件视图管理

管理右侧"头文件视图"标签页的RichEdit控件：

- `ShowHeaderView()` - 显示自动生成的头文件
- `ApplySyntaxHighlighting()` - 应用C++语法高亮

语法高亮颜色：
- 关键字：蓝色
- 类型名：深绿色
- 注释：灰色
- 数字：红色
- 字符串：深红色

#### SearchManager - 搜索功能

提供符号搜索功能：

- `SearchItems()` - 搜索树节点
- 支持搜索历史记录
- 支持搜索历史下拉菜单

#### ExportManager - 导出管理

协调导出功能：

- `ExportToCSV()` - 导出全部为CSV
- `ExportToXML()` - 导出为XML
- `ExportFunctionsToCSV()` - 导出函数
- `ExportClassesToCSV()` - 导出类
- `ExportHeader()` - 导出当前类头文件
- `ExportAllHeaders()` - 导出所有头文件
- `ExportEnumsHeader()` - 导出枚举头文件

#### ControlsManager - 控件管理

创建和管理所有UI控件：

- 搜索框和按钮
- 树形视图
- 标签页控件
- 列表视图
- RichEdit控件
- 状态栏
- 分隔器

#### MenuManager - 菜单管理

管理菜单栏和右键菜单：

- 动态重建菜单（支持多语言）
- 偏移显示模式右键菜单
- 搜索历史菜单
- PDB文件关联功能

#### DragDropManager - 拖放管理

支持拖放打开PDB文件：

- `Initialize()` - 初始化拖放支持
- `HandleDropFiles()` - 处理拖放文件

#### DPIManager - DPI缩放管理

处理高DPI显示：

- `ScaleX/ScaleY()` - 缩放坐标
- `SetProcessDPIAware()` - 设置DPI感知

#### FontManager - 字体管理

管理应用程序字体：

- 头文件视图专用字体（等宽字体）
- 默认UI字体

#### SettingsManager - 设置窗口管理

管理导出设置对话框：

- 扁平化命名空间
- 消除void参数
- IDA兼容格式
- Enums.h包含枚举

---

### 3. 核心模块 (src/Core/)

#### PDBViewer.cpp - 应用程序入口

WinMain入口点：

1. 设置DPI感知
2. 初始化通用控件
3. 加载配置
4. 初始化语言管理器
5. 注册窗口类
6. 创建主窗口
7. 初始化拖放支持
8. 处理命令行参数（自动打开PDB）
9. 运行消息循环

#### MainWindow.cpp - 主窗口

主窗口实现：

- `MyRegisterClass()` - 注册窗口类
- `InitInstance()` - 初始化实例
- `WndProc()` - 窗口消息处理
- `About()` - 关于对话框

消息处理：
- `WM_COMMAND` - 菜单命令
- `WM_NOTIFY` - 控件通知
- `WM_SIZE` - 窗口大小调整
- `WM_DROPFILES` - 拖放文件
- `WM_CONTEXTMENU` - 右键菜单
- `WM_DESTROY` - 窀口销毁

---

## 功能特性

### 1. PDB文件查看

- **树形视图浏览**: 浏览所有类型、函数、类、结构体、联合体、枚举和全局变量
- **详细信息视图**: 查看选中项的详细属性
- **头文件视图**: 自动生成C++头文件声明

### 2. 语法高亮

头文件视图支持C++语法高亮：
- 关键字（class, struct, public, private等）
- 类型名（int, void, char等）
- 注释
- 数字
- 字符串

### 3. 搜索功能

- 在树形视图中搜索符号
- 支持搜索历史记录
- 搜索历史下拉菜单

### 4. 导出功能

支持多种导出格式：
- CSV格式（全部、函数、类）
- XML格式
- C++头文件（单个类、全部类、枚举）
- IDA兼容的纯C结构体

### 5. 多语言支持

- 中文（简体）
- 英文
- 自动检测系统语言
- 动态切换语言

### 6. 用户界面

- 可调整大小的分隔器
- 状态栏显示加载信息
- 键盘快捷键（Ctrl+O打开、Ctrl+C复制、Ctrl+F搜索）
- 右键菜单
- 拖放打开文件

### 7. 数值显示模式

- 十六进制
- 十进制
- 两者都显示

### 8. PDB文件关联

- 关联PDB文件到应用程序
- 双击PDB文件自动打开
- 取消关联

---

## 数据流程

### PDB加载流程

```
用户操作 → 打开PDB文件
    ↓
PDBParser::LoadPDB()
    ↓
创建DIA数据源 → 加载PDB → 打开会话 → 获取全局符号
    ↓
PDBParser::ParseModule()
    ↓
解析函数 → 解析类 → 解析结构体 → 解析联合体 → 解析枚举 → 解析全局变量
    ↓
PopulateTreeView()
    ↓
显示在树形视图
```

### 选择项显示流程

```
用户选择树节点 → TVN_SELCHANGED通知
    ↓
判断当前标签页
    ↓
[详细信息标签] → PopulateListView() → 显示属性列表
[头文件标签] → ShowHeaderView() → 生成并显示头文件
```

---

## 构建说明

### 使用Visual Studio 2022构建

1. 打开 `PDBViewer.sln` 解决方案文件
2. 选择配置（Debug或Release）和平台（x64）
3. 点击"构建" → "构建解决方案"或按F7

### 使用命令行构建

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" PDBViewer.sln /p:Configuration=Release /p:Platform=x64
```

---

## 使用说明

### 基本操作

1. 启动 `PDB Insight.exe`
2. 点击"文件" → "打开"或按Ctrl+O选择PDB文件
3. 在左侧树形视图中浏览和选择项目
4. 在右侧"详细信息"标签页查看详细属性
5. 在右侧"头文件视图"标签页查看自动生成的头文件
6. 使用顶部搜索框搜索符号
7. 右键点击树形视图、详细表格或头文件视图访问附加功能

### 命令行参数

PDB Insight 支持以下命令行参数：

```
PDBInsight.exe [选项] [文件]
```

**选项**:
| 参数 | 说明 |
|------|------|
| `-h, --help, /?` | 显示帮助信息 |
| `-q, --quiet` | 安静模式（无UI，用于批处理） |
| `-d, --download-pdb` | 自动下载DLL/EXE对应的PDB文件 |
| `-e, --export <格式>` | 导出格式：csv, xml, header, allheaders |
| `-o, --output <路径>` | 输出文件/目录路径 |
| `-c, --class <名称>` | 导出指定类/结构体 |
| `-a, --all` | 导出所有类/结构体 |
| `-l, --list` | 列出PDB中所有类 |

**文件**:
| 类型 | 说明 |
|------|------|
| `<pdb文件>` | 直接打开PDB文件 |
| `<dll/exe文件>` | 打开DLL/EXE文件，自动下载对应PDB |

**示例**:
```bash
# 直接打开PDB文件
PDBInsight.exe mylib.pdb

# 打开DLL并自动下载PDB
PDBInsight.exe -d kernel32.dll

# 导出为CSV格式
PDBInsight.exe -e csv -o output.csv mylib.pdb

# 导出指定类的头文件
PDBInsight.exe -e header -c MyClass -o MyClass.h mylib.pdb

# 导出所有类头文件到目录
PDBInsight.exe -e allheaders -o ./headers mylib.pdb

# 列出PDB中所有类
PDBInsight.exe -l mylib.pdb

# 显示帮助
PDBInsight.exe --help
```

### DLL/EXE自动下载PDB功能

PDB Insight 支持直接打开Windows系统DLL或EXE文件，自动从微软符号服务器下载对应的PDB文件：

1. **工作原理**:
   - 读取DLL/EXE文件的PE调试信息
   - 获取PDB文件名、GUID和Age
   - 构建微软符号服务器URL
   - 下载PDB文件到DLL所在目录
   - 自动打开下载的PDB文件

2. **使用方式**:
   - 命令行：`PDBInsight.exe -d kernel32.dll`
   - 直接拖放DLL文件到程序窗口
   - 菜单：文件 → 打开DLL/EXE文件

3. **支持的文件类型**:
   - Windows系统DLL（如kernel32.dll, ntdll.dll）
   - Windows系统EXE文件
   - 第三方DLL/EXE（需要有调试信息）

4. **注意事项**:
   - 需要网络连接访问微软符号服务器
   - PDB文件下载到DLL/EXE所在目录
   - 文件必须包含调试信息（Debug Directory）

### 键盘快捷键

| 快捷键 | 功能 |
|--------|------|
| Ctrl+O | 打开PDB文件 |
| Ctrl+C | 复制选中内容 |
| Ctrl+F | 搜索 |

### 右键菜单功能

#### 树形视图
- 切换偏移显示格式（十六进制/十进制/两者）

#### 详细表格
- 复制 - 复制选中单元格内容
- 搜索 - 使用选中单元格内容搜索

#### 头文件视图
- 复制 - 复制选中文本
- 搜索 - 使用选中文本搜索

---

## 扩展与定制

### 添加新的语言支持

1. 创建新的JSON语言文件（如 `ja-JP.json`）
2. 复制 `en-US.json` 的结构
3. 翻译所有字符串值
4. 在 `MenuManager.cpp` 中添加语言菜单项
5. 在 `LanguageManager.cpp` 中添加语言代码映射

### 添加新的导出格式

1. 在 `PDBExporter.h/cpp` 中添加新的导出方法
2. 在 `ExportManager.h/cpp` 中添加对应的导出管理函数
3. 在 `MainWindow.cpp` 的 `WM_COMMAND` 处理中添加菜单命令处理
4. 在资源文件中添加菜单项ID

### 自定义头文件生成格式

修改 `PDBHeaderGenerator.cpp` 中的生成逻辑：
- 修改缩进风格
- 修改注释格式
- 修改类型处理逻辑


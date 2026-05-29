# PDB Insight 核心文档

## 项目概述

PDB Insight 是一款轻量级的 PDB（Program Database）文件查看工具，基于 Win32 API 和 DIA SDK 开发。

### 技术栈

- **开发语言**: C++17
- **GUI框架**: Win32 API
- **开发环境**: Visual Studio 2022
- **核心依赖**: DIA SDK (msdia140.dll)
- **字符编码**: Unicode

### 系统要求

- Windows 7 或更高版本
- Visual Studio 2022（用于编译）

---

## 项目结构

```
PDBViewer/
├── src/
│   ├── Core/                    # 核心窗口和应用程序逻辑
│   │   ├── MainWindow.cpp/h     # 主窗口实现
│   │   └── PDBViewer.cpp/h      # 应用程序入口点
│   ├── Managers/                # 各种管理器模块
│   │   ├── ConfigManager.cpp/h      # 配置管理
│   │   ├── ControlsManager.cpp/h    # 控件管理
│   │   ├── CommandLineManager.cpp/h # 命令行参数管理
│   │   ├── DragDropManager.cpp/h    # 拖放功能管理
│   │   ├── DPIManager.cpp/h         # DPI缩放管理
│   │   ├── ExportManager.cpp/h      # 导出功能管理
│   │   ├── ExportEnhancer.cpp/h     # 导出增强功能
│   │   ├── FontManager.cpp/h        # 字体管理
│   │   ├── HeaderViewManager.cpp/h  # 头文件视图管理
│   │   ├── LanguageManager.cpp/h    # 多语言支持管理
│   │   ├── ListViewManager.cpp/h    # 列表视图管理
│   │   ├── MenuManager.cpp/h        # 菜单管理
│   │   ├── PDBDownloader.cpp/h      # PDB下载管理
│   │   ├── PDBViewerGlobals.cpp/h   # 全局变量和常量
│   │   ├── SearchManager.cpp/h      # 搜索功能管理
│   │   ├── SettingsManager.cpp/h    # 设置窗口管理
│   │   └── TreeViewManager.cpp/h    # 树形视图管理
│   └── PDB/                     # PDB解析和导出功能
│       ├── PDBData.h            # 数据结构定义
│       ├── PDBParser.cpp/h      # PDB解析器
│       ├── PDBExporter.cpp/h    # 导出功能
│       ├── PDBHeaderGenerator.cpp/h # 头文件生成器
│       ├── dia2.h               # DIA SDK接口定义
│       ├── diaCreate.cpp/h      # DIA创建辅助函数
│       └── cvConst.h            # CodeView常量定义
├── i18n/                        # 国际化语言文件
│   ├── zh-CN.json               # 中文语言文件
│   └ en-US.json                 # 英文语言文件
├── docs/                        # 文档目录
├── PDBViewer.sln                # Visual Studio解决方案
├── PDBViewer.rc                 # 资源文件
└── Resource.h                   # 资源ID定义
```

---

## 核心模块

### 1. PDB解析模块 (src/PDB/)

#### 数据结构 (PDBData.h)

| 结构体 | 描述 |
|--------|------|
| `FunctionInfo` | 函数信息 |
| `ClassInfo` | 类/结构体/联合体信息 |
| `MemberVariableInfo` | 成员变量信息 |
| `BaseClassInfo` | 基类信息 |
| `VirtualFunctionInfo` | 虚函数信息 |
| `EnumInfo` | 枚举信息 |
| `GlobalVariableInfo` | 全局变量信息 |
| `ModuleInfo` | 模块信息集合 |

#### PDBParser

使用DIA SDK解析PDB文件：
- `LoadPDB()` - 加载PDB文件
- `ParseModule()` - 解析整个模块
- 支持进度回调

#### PDBHeaderGenerator

自动生成C++头文件声明：
- `GenerateClassHeader()` - 生成类头文件
- `GenerateAllHeaders()` - 批量生成
- 支持IDA兼容格式

### 2. 管理器模块 (src/Managers/)

#### ConfigManager

单例模式，管理配置：
- 语言、数值显示模式
- 导出设置、搜索历史
- 镜像源设置

#### LanguageManager

多语言支持：
- JSON语言文件嵌入exe资源
- 自动检测系统语言
- 支持中英文切换

#### TreeViewManager

树形视图管理：
- 显示函数、类、结构体、联合体、枚举、全局变量
- 支持打开PDB和DLL/EXE文件

#### ListViewManager / HeaderViewManager

详细信息视图和头文件视图管理。

#### PDBDownloader

PDB下载功能：
- 从微软符号服务器或镜像源下载
- 支持自定义镜像源URL
- 支持HTTP和HTTPS协议

#### CommandLineManager

命令行参数处理：
- 支持导出、列表、下载等功能
- 安静模式支持批处理

---

## 核心功能

### 1. PDB文件查看

- 树形视图浏览所有符号
- 详细信息视图显示属性
- 头文件视图自动生成声明
- 语法高亮显示

### 2. DLL/EXE自动下载PDB

- 读取PE调试信息
- 从符号服务器下载PDB
- 支持自定义镜像源加速

### 3. 导出功能

- CSV、XML格式导出
- C++头文件生成
- IDA兼容格式

### 4. 多语言支持

- 中英文界面
- 语言文件嵌入exe

### 5. 命令行支持

- 批处理导出
- 安静模式
- 类列表查询

---

## 命令行参数

```
PDBInsight.exe [选项] [文件]

选项:
  -h, --help        显示帮助
  -q, --quiet       安静模式
  -d, --download    自动下载PDB
  -e, --export      导出格式(csv/xml/header/allheaders)
  -o, --output      输出路径
  -c, --class       导出指定类
  -a, --all         导出所有类
  -l, --list        列出所有类
```

---

## 构建说明

```powershell
# 使用MSBuild编译
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" PDBViewer.sln /p:Configuration=Release /p:Platform=x64
```

输出目录: `x64\Release\PDB Insight.exe`
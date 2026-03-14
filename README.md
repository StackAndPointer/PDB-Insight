# PDB Insight

一款新的、强大且小巧的 PDB 查看工具。

## 功能特点

- **树状视图** - 浏览 PDB 文件中的所有类型、函数、类、结构体、联合体、枚举和全局变量
- **详细信息** - 查看选中项的详细属性
- **头文件视图** - 自动生成类/结构体的 C++ 头文件声明
- **语法高亮** - 头文件视图支持 C++ 语法高亮
- **搜索功能** - 在树状视图中搜索符号
- **导出功能** - 支持导出为 CSV、XML 和头文件
- **多语言** - 支持中文和英文界面
- **可调整分隔条** - 灵活调整各视图宽度
- **快捷键支持** - Ctrl+C 复制，Ctrl+F 搜索

## 系统要求

- Windows 7 或更高版本

## 构建说明

### 使用 Visual Studio 2022 构建

1. 打开 `PDBViewer.sln` 解决方案文件
2. 选择配置（Debug 或 Release）和平台（x64）
3. 点击「生成」→「生成解决方案」或按 F7

### 使用命令行构建

```powershell
# 使用 MSBuild 构建 Release 版本
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" PDBViewer.sln /p:Configuration=Release /p:Platform=x64
```

## 项目结构

```
PDBViewer/
├── src/
│   ├── Core/           # 核心窗口和应用程序逻辑
│   ├── Managers/       # 各个管理器模块
│   └── PDB/            # PDB 解析和导出功能
├── PDBViewer.sln       # Visual Studio 解决方案
├── PDBViewer.vcxproj   # 项目文件
└── PDBViewer.rc        # 资源文件
```

## 使用方法

1. 启动 `PDB Insight.exe`
2. 点击「文件」→「打开」或按 Ctrl+O 选择 PDB 文件
3. 在左侧树状视图中浏览和选择项目
4. 在右侧「详细信息」标签页查看详细属性
5. 在右侧「头文件视图」标签页查看自动生成的头文件
6. 使用顶部搜索框搜索符号
7. 右键点击树状视图、详细信息表格或头文件视图可以访问额外功能

## 快捷键

- **Ctrl+O** - 打开 PDB 文件
- **Ctrl+C** - 复制选中内容（详细信息和头文件视图）
- **Ctrl+F** - 搜索（详细信息和头文件视图）

## 右键菜单功能

### 树状视图
- 切换偏移值显示格式（十六进制/十进制/两者都显示）

### 详细信息表格
- 复制 - 复制选中单元格的内容
- 搜索 - 使用选中单元格的内容进行搜索

### 头文件视图
- 复制 - 复制选中文本
- 搜索 - 使用选中文本进行搜索

## 版本历史

### 1.0.0.0 (2026)
- 初始版本发布
- 完整的 PDB 查看功能
- 树状视图、详细信息、头文件视图
- 导出功能（CSV、XML、头文件）
- 多语言支持
- 分隔条调整
- 快捷键和右键菜单

## 许可证

本项目为开源项目。

## 技术栈

- C++/Win32 API
- Visual Studio 2022
- DIA SDK (Debug Interface Access)
- RichEdit 控件

## 联系方式



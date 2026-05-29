# PDB Insight Core Documentation

## Project Overview

PDB Insight is a lightweight PDB (Program Database) file viewer based on Win32 API and DIA SDK.

### Technology Stack

- **Language**: C++17
- **GUI Framework**: Win32 API
- **Development Environment**: Visual Studio 2022
- **Core Dependency**: DIA SDK (msdia140.dll)
- **Encoding**: Unicode

### System Requirements

- Windows 7 or later
- Visual Studio 2022 (for compilation)

---

## Project Structure

```
PDBViewer/
├── src/
│   ├── Core/                    # Core window and application logic
│   │   ├── MainWindow.cpp/h     # Main window implementation
│   │   └── PDBViewer.cpp/h      # Application entry point
│   ├── Managers/                # Manager modules
│   │   ├── ConfigManager.cpp/h      # Configuration management
│   │   ├── ControlsManager.cpp/h    # Controls management
│   │   ├── CommandLineManager.cpp/h # Command line management
│   │   ├── DragDropManager.cpp/h    # Drag-drop management
│   │   ├── DPIManager.cpp/h         # DPI scaling management
│   │   ├── ExportManager.cpp/h      # Export management
│   │   ├── ExportEnhancer.cpp/h     # Export enhancement
│   │   ├── FontManager.cpp/h        # Font management
│   │   ├── HeaderViewManager.cpp/h  # Header view management
│   │   ├── LanguageManager.cpp/h    # Language management
│   │   ├── ListViewManager.cpp/h    # List view management
│   │   ├── MenuManager.cpp/h        # Menu management
│   │   ├── PDBDownloader.cpp/h      # PDB download management
│   │   ├── PDBViewerGlobals.cpp/h   # Global variables
│   │   ├── SearchManager.cpp/h      # Search management
│   │   ├── SettingsManager.cpp/h    # Settings window
│   │   └── TreeViewManager.cpp/h    # Tree view management
│   └── PDB/                     # PDB parsing and export
│       ├── PDBData.h            # Data structure definitions
│       ├── PDBParser.cpp/h      # PDB parser
│       ├── PDBExporter.cpp/h    # Export functionality
│       ├── PDBHeaderGenerator.cpp/h # Header generator
│       ├── dia2.h               # DIA SDK interface
│       ├── diaCreate.cpp/h      # DIA creation helpers
│       └── cvConst.h            # CodeView constants
├── i18n/                        # Internationalization
│   ├── zh-CN.json               # Chinese language file
│   └ en-US.json                 # English language file
├── docs/                        # Documentation
├── PDBViewer.sln                # Visual Studio solution
├── PDBViewer.rc                 # Resource file
└── Resource.h                   # Resource ID definitions
```

---

## Core Modules

### 1. PDB Parsing Module (src/PDB/)

#### Data Structures (PDBData.h)

| Structure | Description |
|-----------|-------------|
| `FunctionInfo` | Function information |
| `ClassInfo` | Class/struct/union information |
| `MemberVariableInfo` | Member variable information |
| `BaseClassInfo` | Base class information |
| `VirtualFunctionInfo` | Virtual function information |
| `EnumInfo` | Enum information |
| `GlobalVariableInfo` | Global variable information |
| `ModuleInfo` | Module information collection |

#### PDBParser

Uses DIA SDK to parse PDB files:
- `LoadPDB()` - Load PDB file
- `ParseModule()` - Parse entire module
- Progress callback support

#### PDBHeaderGenerator

Auto-generate C++ header declarations:
- `GenerateClassHeader()` - Generate class header
- `GenerateAllHeaders()` - Batch generation
- IDA compatible format support

### 2. Manager Modules (src/Managers/)

#### ConfigManager

Singleton pattern, manages configuration:
- Language, number display mode
- Export settings, search history
- Mirror source settings

#### LanguageManager

Multi-language support:
- JSON language files embedded in exe
- Auto-detect system language
- Chinese/English switching

#### TreeViewManager

Tree view management:
- Display functions, classes, structs, unions, enums, global variables
- Support opening PDB and DLL/EXE files

#### ListViewManager / HeaderViewManager

Details view and header view management.

#### PDBDownloader

PDB download functionality:
- Download from Microsoft symbol server or mirror
- Custom mirror URL support
- HTTP and HTTPS protocol support

#### CommandLineManager

Command line parameter handling:
- Export, list, download functions
- Quiet mode for batch processing

---

## Core Features

### 1. PDB File Viewing

- Tree view browsing all symbols
- Details view showing properties
- Header view auto-generating declarations
- Syntax highlighting

### 2. DLL/EXE Auto PDB Download

- Read PE debug information
- Download PDB from symbol server
- Custom mirror source acceleration

### 3. Export Functionality

- CSV, XML format export
- C++ header generation
- IDA compatible format

### 4. Multi-language Support

- Chinese/English interface
- Language files embedded in exe

### 5. Command Line Support

- Batch export
- Quiet mode
- Class list query

---

## Command Line Parameters

```
PDBInsight.exe [options] [file]

Options:
  -h, --help        Show help
  -q, --quiet       Quiet mode
  -d, --download    Auto download PDB
  -e, --export      Export format(csv/xml/header/allheaders)
  -o, --output      Output path
  -c, --class       Export specific class
  -a, --all         Export all classes
  -l, --list        List all classes
```

---

## Build Instructions

```powershell
# Build with MSBuild
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" PDBViewer.sln /p:Configuration=Release /p:Platform=x64
```

Output: `x64\Release\PDB Insight.exe`
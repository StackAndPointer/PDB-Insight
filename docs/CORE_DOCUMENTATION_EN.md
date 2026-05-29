# PDB Insight Core Documentation

## Project Overview

PDB Insight is a powerful, lightweight PDB (Program Database) file viewer tool. Built on Windows Win32 API and DIA SDK (Debug Interface Access), it provides complete PDB file parsing, viewing, and exporting capabilities.

### Technology Stack

- **Language**: C++17
- **GUI Framework**: Win32 API
- **Development Environment**: Visual Studio 2022 (v143 toolset)
- **Core Dependency**: DIA SDK (msdia140.dll)
- **Controls**: TreeView, ListView, TabCtrl, RichEdit
- **Character Encoding**: Unicode

### System Requirements

- Windows 7 or later
- Visual Studio 2022 (for compilation)
- msdia140.dll (DIA SDK, installed with Visual Studio)

---

## Project Structure

```
PDBViewer/
├── src/
│   ├── Core/                    # Core window and application logic
│   │   ├── MainWindow.cpp/h     # Main window implementation
│   │   └── PDBViewer.cpp/h      # Application entry point
│   ├── Managers/                # Various manager modules
│   │   ├── ConfigManager.cpp/h  # Configuration management
│   │   ├── ControlsManager.cpp/h # UI controls management
│   │   ├── DragDropManager.cpp/h # Drag-and-drop functionality
│   │   ├── DPIManager.cpp/h     # DPI scaling management
│   │   ├── ExportManager.cpp/h  # Export functionality management
│   │   ├── ExportEnhancer.cpp/h # Export enhancement features
│   │   ├── FontManager.cpp/h    # Font management
│   │   ├── FunctionInfoDisplayManager.cpp/h # Function info display
│   │   ├── GlobalVarAddressManager.cpp/h # Global variable address management
│   │   ├── HeaderViewManager.cpp/h # Header view management
│   │   ├── LanguageManager.cpp/h # Multi-language support
│   │   ├── ListViewManager.cpp/h # List view management
│   │   ├── MenuManager.cpp/h    # Menu management
│   │   ├── PDBViewerGlobals.cpp/h # Global variables and constants
│   │   ├── SearchManager.cpp/h  # Search functionality
│   │   ├── SearchOptimizer.cpp/h # Search optimization
│   │   ├── SettingsManager.cpp/h # Settings window management
│   │   └── TreeViewManager.cpp/h # Tree view management
│   └── PDB/                     # PDB parsing and export functionality
│       ├── PDBData.h            # Data structure definitions
│       ├── PDBParser.cpp/h      # PDB parser
│       ├── PDBExporter.cpp/h    # Export functionality
│       ├── PDBHeaderGenerator.cpp/h # Header file generator
│       ├── dia2.h               # DIA SDK interface definitions
│       ├── diaCreate.cpp/h      # DIA creation helper functions
│       └── cvConst.h            # CodeView constant definitions
├── docs/                        # Documentation directory
├── zh-CN.json                   # Chinese language file
├── en-US.json                   # English language file
├── PDBViewer.sln                # Visual Studio solution
├── PDBViewer.vcxproj            # Project file
├── PDBViewer.rc                 # Resource file
├── Resource.h                   # Resource ID definitions
├── framework.h                  # Framework header
└── targetver.h                  # Target version definition
```

---

## Core Module Details

### 1. PDB Parsing Module (src/PDB/)

#### PDBData.h - Data Structure Definitions

Defines all data structures for storing PDB information:

| Structure | Description |
|-----------|-------------|
| `FunctionInfo` | Function information (name, return type, calling convention, parameters, RVA, etc.) |
| `ClassInfo` | Class/struct/union information (name, size, members, base classes, virtual functions, etc.) |
| `MemberVariableInfo` | Member variable information (name, type, offset, access, bit-field info) |
| `BaseClassInfo` | Base class information (name, inheritance type, offset, access) |
| `VirtualFunctionInfo` | Virtual function information (name, return type, parameters, vtable index) |
| `EnumInfo` | Enum information (name, underlying type, value list) |
| `GlobalVariableInfo` | Global variable information (name, type, RVA, size) |
| `ModuleInfo` | Module information (collection of all above information) |

#### Enum Types

```cpp
enum NumberDisplayMode {
    NUMBER_HEX,     // Hexadecimal display
    NUMBER_DEC,     // Decimal display
    NUMBER_BOTH     // Show both
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

#### PDBParser - PDB Parser

Core parsing class using DIA SDK to parse PDB files:

**Main Methods**:
- `LoadPDB(pdbPath)` - Load PDB file
- `ParseModule()` - Parse entire module, returns ModuleInfo
- `Unload()` - Unload current PDB

**Parsing Flow**:
1. Initialize COM
2. Create DIA data source (msdia140.dll)
3. Load PDB file
4. Open DIA session
5. Get global scope symbol
6. Parse various symbols (functions, classes, structs, unions, enums, global variables)

#### PDBExporter - Export Functionality

Provides multiple export formats:

- `ExportToCSV()` - Export to CSV format
- `ExportToXML()` - Export to XML format
- `ExportFunctionsToCSV()` - Export only functions
- `ExportClassesToCSV()` - Export only classes
- `ExportGlobalVariablesToCSV()` - Export global variables

#### PDBHeaderGenerator - Header File Generator

Automatically generates C++ header declarations:

- `GenerateClassHeader()` - Generate class header file
- `GenerateStructDeclaration()` - Generate struct declaration
- `GenerateUnionDeclaration()` - Generate union declaration
- `GenerateEnumDeclaration()` - Generate enum declaration
- `GenerateAllHeaders()` - Batch generate all header files
- `GeneratePureCStructDeclaration()` - Generate IDA-compatible pure C struct

---

### 2. Manager Modules (src/Managers/)

#### ConfigManager - Configuration Management

Singleton pattern implementation, manages application configuration:

- Language settings
- Number display mode (hex/decimal/both)
- Expand base class members option
- Export settings (ExportSettings)
- Search history

Configuration is stored in a JSON file in the user's AppData directory.

#### LanguageManager - Multi-language Support

Singleton pattern, supports Chinese/English switching:

- Load language strings from JSON files
- Auto-detect system language
- Provides convenient macro `LANG_STR(key)` for localized strings

#### TreeViewManager - Tree View Management

Manages the left-side tree view:

- `PopulateTreeView()` - Populate tree view
- `AddTreeItem()` - Add tree node
- `OpenPDBFile()` - Open PDB file dialog
- `ClosePDBFile()` - Close current PDB

Tree structure:
```
Module Name
├── Functions
├── Classes
├── Structs
├── Unions
├── Enums
└── Global Variables
```

#### ListViewManager - List View Management

Manages the right-side "Details" tab list view:

- `PopulateListView()` - Populate list based on selected tree node
- `AddListViewColumn()` - Add column

Display content includes:
- Functions: name, return type, calling convention, RVA, size, class name
- Classes/structs: name, size, member count, base class count, virtual function count
- Member variables: name, type, offset, access, bit-field info
- Enums: name, value

#### HeaderViewManager - Header View Management

Manages the right-side "Header View" tab RichEdit control:

- `ShowHeaderView()` - Display auto-generated header file
- `ApplySyntaxHighlighting()` - Apply C++ syntax highlighting

Syntax highlighting colors:
- Keywords: blue
- Type names: dark green
- Comments: gray
- Numbers: red
- Strings: dark red

#### SearchManager - Search Functionality

Provides symbol search functionality:

- `SearchItems()` - Search tree nodes
- Supports search history
- Supports search history dropdown menu

#### ExportManager - Export Management

Coordinates export functionality:

- `ExportToCSV()` - Export all to CSV
- `ExportToXML()` - Export to XML
- `ExportFunctionsToCSV()` - Export functions
- `ExportClassesToCSV()` - Export classes
- `ExportHeader()` - Export current class header
- `ExportAllHeaders()` - Export all headers
- `ExportEnumsHeader()` - Export enums header

#### ControlsManager - UI Controls Management

Creates and manages all UI controls:

- Search box and buttons
- Tree view
- Tab control
- List view
- RichEdit control
- Status bar
- Splitter

#### MenuManager - Menu Management

Manages menu bar and context menus:

- Dynamic menu rebuilding (supports multi-language)
- Offset display mode context menu
- Search history menu
- PDB file association functionality

#### DragDropManager - Drag-and-Drop Management

Supports drag-and-drop to open PDB files:

- `Initialize()` - Initialize drag-drop support
- `HandleDropFiles()` - Handle dropped files

#### DPIManager - DPI Scaling Management

Handles high DPI display:

- `ScaleX/ScaleY()` - Scale coordinates
- `SetProcessDPIAware()` - Set DPI awareness

#### FontManager - Font Management

Manages application fonts:

- Header view specific font (monospace)
- Default UI font

#### SettingsManager - Settings Window Management

Manages export settings dialog:

- Flatten namespaces
- Remove void parameters
- IDA compatible format
- Include enums in Enums.h

---

### 3. Core Modules (src/Core/)

#### PDBViewer.cpp - Application Entry

WinMain entry point:

1. Set DPI awareness
2. Initialize common controls
3. Load configuration
4. Initialize language manager
5. Register window class
6. Create main window
7. Initialize drag-drop support
8. Handle command line arguments (auto-open PDB)
9. Run message loop

#### MainWindow.cpp - Main Window

Main window implementation:

- `MyRegisterClass()` - Register window class
- `InitInstance()` - Initialize instance
- `WndProc()` - Window message handling
- `About()` - About dialog

Message handling:
- `WM_COMMAND` - Menu commands
- `WM_NOTIFY` - Control notifications
- `WM_SIZE` - Window resize
- `WM_DROPFILES` - Dropped files
- `WM_CONTEXTMENU` - Context menu
- `WM_DESTROY` - Window destruction

---

## Features

### 1. PDB File Viewing

- **Tree View Browsing**: Browse all types, functions, classes, structs, unions, enums, and global variables
- **Details View**: View detailed properties of selected items
- **Header View**: Auto-generate C++ header declarations

### 2. Syntax Highlighting

Header view supports C++ syntax highlighting:
- Keywords (class, struct, public, private, etc.)
- Type names (int, void, char, etc.)
- Comments
- Numbers
- Strings

### 3. Search Functionality

- Search symbols in tree view
- Search history support
- Search history dropdown menu

### 4. Export Functionality

Supports multiple export formats:
- CSV format (all, functions, classes)
- XML format
- C++ header files (single class, all classes, enums)
- IDA-compatible pure C structs

### 5. Multi-language Support

- Chinese (Simplified)
- English
- Auto-detect system language
- Dynamic language switching

### 6. User Interface

- Resizable splitter
- Status bar with loading information
- Keyboard shortcuts (Ctrl+O open, Ctrl+C copy, Ctrl+F search)
- Context menus
- Drag-and-drop file opening

### 7. Number Display Mode

- Hexadecimal
- Decimal
- Show both

### 8. PDB File Association

- Associate PDB files with application
- Double-click PDB file to auto-open
- Unassociate

---

## Data Flow

### PDB Loading Flow

```
User Action → Open PDB File
    ↓
PDBParser::LoadPDB()
    ↓
Create DIA Data Source → Load PDB → Open Session → Get Global Symbol
    ↓
PDBParser::ParseModule()
    ↓
Parse Functions → Parse Classes → Parse Structs → Parse Unions → Parse Enums → Parse Global Variables
    ↓
PopulateTreeView()
    ↓
Display in Tree View
```

### Selection Display Flow

```
User Selects Tree Node → TVN_SELCHANGED Notification
    ↓
Check Current Tab
    ↓
[Details Tab] → PopulateListView() → Display Property List
[Header Tab] → ShowHeaderView() → Generate and Display Header
```

---

## Build Instructions

### Building with Visual Studio 2022

1. Open `PDBViewer.sln` solution file
2. Select configuration (Debug or Release) and platform (x64)
3. Click "Build" → "Build Solution" or press F7

### Building with Command Line

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" PDBViewer.sln /p:Configuration=Release /p:Platform=x64
```

---

## Usage Guide

### Basic Operations

1. Launch `PDB Insight.exe`
2. Click "File" → "Open" or press Ctrl+O to select a PDB file
3. Browse and select items in the left tree view
4. View detailed properties in the right "Details" tab
5. View auto-generated headers in the right "Header View" tab
6. Use the top search box to search for symbols
7. Right-click on tree view, details table, or header view for additional functions

### Command Line Parameters

PDB Insight supports the following command line parameters:

```
PDBInsight.exe [options] [file]
```

**Options**:
| Parameter | Description |
|-----------|-------------|
| `-h, --help, /?` | Show help message |
| `-q, --quiet` | Quiet mode (no UI, for batch processing) |
| `-d, --download-pdb` | Auto-download PDB for DLL/EXE files |
| `-e, --export <format>` | Export format: csv, xml, header, allheaders |
| `-o, --output <path>` | Output file/directory path |
| `-c, --class <name>` | Export specific class/struct |
| `-a, --all` | Export all classes/structs |
| `-l, --list` | List all classes in PDB |

**File**:
| Type | Description |
|------|-------------|
| `<pdb_file>` | Open PDB file directly |
| `<dll/exe_file>` | Open DLL/EXE file, auto-download corresponding PDB |

**Examples**:
```bash
# Open PDB file directly
PDBInsight.exe mylib.pdb

# Open DLL and auto-download PDB
PDBInsight.exe -d kernel32.dll

# Export to CSV format
PDBInsight.exe -e csv -o output.csv mylib.pdb

# Export specific class header
PDBInsight.exe -e header -c MyClass -o MyClass.h mylib.pdb

# Export all class headers to directory
PDBInsight.exe -e allheaders -o ./headers mylib.pdb

# List all classes in PDB
PDBInsight.exe -l mylib.pdb

# Show help
PDBInsight.exe --help
```

### DLL/EXE Auto-Download PDB Feature

PDB Insight supports directly opening Windows system DLL or EXE files, automatically downloading the corresponding PDB file from Microsoft symbol server:

1. **How it works**:
   - Read PE debug information from DLL/EXE file
   - Get PDB file name, GUID and Age
   - Build Microsoft symbol server URL
   - Download PDB file to DLL directory
   - Automatically open the downloaded PDB file

2. **Usage**:
   - Command line: `PDBInsight.exe -d kernel32.dll`
   - Drag and drop DLL file to program window
   - Menu: File → Open DLL/EXE File

3. **Supported file types**:
   - Windows system DLLs (e.g., kernel32.dll, ntdll.dll)
   - Windows system EXE files
   - Third-party DLL/EXE (must have debug information)

4. **Notes**:
   - Requires network connection to access Microsoft symbol server
   - PDB file is downloaded to DLL/EXE directory
   - File must contain debug information (Debug Directory)

### Keyboard Shortcuts

| Shortcut | Function |
|----------|----------|
| Ctrl+O | Open PDB file |
| Ctrl+C | Copy selected content |
| Ctrl+F | Search |

### Context Menu Functions

#### Tree View
- Toggle offset display format (hex/decimal/both)

#### Details Table
- Copy - Copy selected cell content
- Search - Search using selected cell content

#### Header View
- Copy - Copy selected text
- Search - Search using selected text

---

## Extension and Customization

### Adding New Language Support

1. Create new JSON language file (e.g., `ja-JP.json`)
2. Copy structure from `en-US.json`
3. Translate all string values
4. Add language menu item in `MenuManager.cpp`
5. Add language code mapping in `LanguageManager.cpp`

### Adding New Export Format

1. Add new export method in `PDBExporter.h/cpp`
2. Add corresponding export management function in `ExportManager.h/cpp`
3. Add menu command handling in `MainWindow.cpp` `WM_COMMAND` handler
4. Add menu item ID in resource file

### Customizing Header Generation Format

Modify generation logic in `PDBHeaderGenerator.cpp`:
- Modify indentation style
- Modify comment format
- Modify type handling logic


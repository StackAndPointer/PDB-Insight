# PDB Insight

A powerful, lightweight PDB (Program Database) viewer tool.

## Features

- **Tree View** - Browse all types, functions, classes, structs, unions, enums, and global variables in the PDB file
- **Details View** - View detailed properties of selected items
- **Header View** - Automatically generate C++ header declarations for classes/structs
- **Syntax Highlighting** - C++ syntax highlighting in header view
- **Search Function** - Search for symbols in the tree view
- **Export Function** - Support for exporting to CSV, XML, and header files
- **Multi-language Support** - Chinese and English interface
- **Resizable Splitter** - Flexibly adjust the width of each view
- **Keyboard Shortcuts** - Ctrl+C for copy, Ctrl+F for search


## System Requirements

- Windows 7 or later

## Build Instructions

### Building with Visual Studio 2022

1. Open the `PDBViewer.sln` solution file
2. Select configuration (Debug or Release) and platform (x64)
3. Click "Build" → "Build Solution" or press F7

### Building with Command Line

```powershell
# Build Release version using MSBuild
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" PDBViewer.sln /p:Configuration=Release /p:Platform=x64
```

## Project Structure

```
PDBViewer/
├── src/
│   ├── Core/           # Core window and application logic
│   ├── Managers/       # Various manager modules
│   └── PDB/            # PDB parsing and export functionality
├── PDBViewer.sln       # Visual Studio solution
├── PDBViewer.vcxproj   # Project file
└── PDBViewer.rc        # Resource file
```

## Usage

1. Launch `PDB Insight.exe`
2. Click "File" → "Open" or press Ctrl+O to select a PDB file
3. Browse and select items in the left tree view
4. View detailed properties in the right "Details" tab
5. View automatically generated headers in the right "Header View" tab
6. Use the top search box to search for symbols
7. Right-click on tree view, details table, or header view to access additional functions

## Keyboard Shortcuts

- **Ctrl+O** - Open PDB file
- **Ctrl+C** - Copy selected content (details and header views)
- **Ctrl+F** - Search (details and header views)

## Right-Click Menu Functions

### Tree View
- Toggle offset display format (hex/decimal/both)

### Details Table
- Copy - Copy the content of the selected cell
- Search - Search using the content of the selected cell

### Header View
- Copy - Copy selected text
- Search - Search using selected text

## Version History




### 1.0.0.0 (2026)
- Initial release
- Complete PDB viewing functionality
- Tree view, details view, and header view
- Export functionality (CSV, XML, headers)
- Multi-language support
- Resizable splitter
- Keyboard shortcuts and right-click menus

## License

This project is open source.

## Technology Stack

- C++/Win32 API
- Visual Studio 2022
- DIA SDK (Debug Interface Access)
<<<<<<< HEAD
- RichEdit control
=======
- RichEdit 控件

## 联系方式


>>>>>>> 013680e50c06d2b222125a07491cbe87332f972a

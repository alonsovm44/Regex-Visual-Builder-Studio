# Regex Studio — Visual Regex Builder for Developers

**Regex Studio** is a **visual, node-based developer tool** for building, testing, and debugging regular expressions.  
Instead of writing fragile and hard-to-read regex strings manually, developers can **construct patterns visually**, inspect matches step-by-step, and export production-ready code for multiple languages.

Built in **C++** using **Raylib**, Regex Studio is designed to be **lightweight, fast, and developer-focused**.

---

## Why Regex Studio

Regular expressions are powerful but notoriously difficult to:
- Read
- Debug
- Maintain

Regex Studio addresses this by turning regex construction into a **visual graph**, while still producing **exact, standards-compliant regex output** for real-world use.

This makes it useful for:
- Developers working with complex patterns
- Log analysis and data mining
- Teaching and learning regex
- Debugging legacy expressions

---

## Core Features

### Visual Node Editor
Build regex patterns using a drag-and-drop canvas:
- **Anchors**: Start (`^`), End (`$`)
- **Character Classes**: Digits, Words, Whitespace, Text (and negations)
- **Quantifiers**: `*`, `+`, `?`
- **Structure**: Capture Groups, Logical OR
- **Custom Nodes**: Insert raw or specialized regex fragments

Nodes are **color-coded by category** for clarity.

---

### Live Playground & Visual Debugger

- **Real-Time Highlighting**  
  Test input strings and instantly see matches update as the graph changes.

- **Step-by-Step Debugger**  
  Navigate through individual matches and inspect capture groups visually, showing *exactly* which node produced each match.

---

### Multi-Language Code Export

Regex Studio automatically handles string escaping and exports regex code ready to paste into:

- **C++** (`std::regex`)
- **Python** (raw strings)
- **JavaScript** (regex literals)
- **C#** (verbatim strings)
- **Java** (`Pattern.compile`)

---

### Native File Scanner

A built-in terminal allows scanning **real files and directories** on disk using the current visual pattern:
- Recursive directory scanning
- Match count reporting
- Useful for log analysis and data exploration

---

### Productivity & Workflow Tools

- Undo / Redo with full history
- Copy, cut, and paste node groups
- Multi-select and group dragging
- Save and load projects (`.vreg`)
- Built-in templates (Emails, URLs, Dates, IPv4, etc.)

---

## Controls (Quick Reference)

- **Left Click**: Select / Drag node
- **Right Click**: Create connection
- **Middle Click / Space + Drag**: Pan
- **Mouse Wheel**: Zoom
- **Ctrl + Z / Ctrl + Y**: Undo / Redo
- **T**: Toggle file scanner terminal
- **?**: Toggle help overlay

---

## Build from Source

### Requirements
- C++17-compatible compiler
- Raylib

### Linux
```bash
g++ main.cpp -lraylib -lGL -lm -lpthread -ldl -o RegexStudio
Windows (MinGW)
g++ main.cpp -lraylib -lopengl32 -lgdi32 -lwinmm -o RegexStudio.exe
Ensure the sources/ directory (including font.ttf) is present next to the executable.
````
Technical Overview
Language: C++17

Rendering / UI: Raylib (immediate-mode GUI)

Regex Engine: C++ Standard Library (<regex>)

Filesystem: C++ <filesystem> & <fstream>

Project Status
Regex Studio is an experimental developer productivity tool.
It is intended for learning, exploration, and real-world debugging—not as a regex engine replacement.

License
MIT

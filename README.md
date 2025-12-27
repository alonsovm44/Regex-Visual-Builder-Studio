Visual Regex Builder 

Regex Visual Builder a node-based development environment designed to simplify the creation, testing, and debugging of Regular Expressions. Instead of writing complex and cryptic syntax strings manually, users can construct logic visually by connecting nodes, making Regex more accessible, readable, and maintainable.

Built from scratch using C++ and the Raylib library, it offers a lightweight and high-performance experience with a distinctive visual interface.

Key Features

Visual Node Editor

Construct patterns using a drag-and-drop interface. Nodes are color-coded by category for easy identification:

Anchors: Start of line, End of line.

Character Classes: Text, Digits, Words, Whitespace, and their negations.

Quantifiers: Zero or more (*), One or more (+), Optional (?).

Structure: Capture Groups and Logical OR.

Custom: Editable nodes for inserting raw text or specific regex fragments.

Live Playground & Debugger

Real-time Highlighting: A side panel allows users to type test strings and see matches highlighted instantly as the node graph is modified.

Visual Debugger: A step-by-step inspection mode that allows users to navigate through individual matches. It provides a detailed breakdown of capture groups, showing exactly what part of the text is being captured by specific sections of the pattern.

Multi-Language Code Export

The application solves the common issue of string escaping. Users can view the full generated pattern and export it as a code snippet ready for copy-pasting into various programming languages:

C++ (std::regex with escaped backslashes)

Python (raw strings)

JavaScript (regex literals)

C# (verbatim strings)

Java (Pattern.compile with escaped backslashes)

Native File Scanner (Terminal)

A built-in drop-down console (toggle with 'T') allows users to input a directory path to scan real files on the local hard drive. The application recursively reads files and reports match counts based on the current visual pattern, making it a powerful tool for log analysis or data mining.

Professional Workflow Tools

Undo/Redo System: Full history stack for reverting changes.

Clipboard: Copy, Cut, and Paste groups of nodes while maintaining relative structure.

Persistence: Save and Load projects to custom .vreg files.

Templates: A library of drag-and-drop presets for common patterns like Email addresses, URLs, Dates (ISO 8601), and IPv4 addresses.

Multi-Select: Box selection and group dragging for organizing complex logic.

Controls and Shortcuts

Canvas Interaction

Left Click: Select node / Drag node.

Left Drag (Empty Space): Box selection (multi-select).

Shift + Left Click: Add node to selection.

Right Click: Create connection between nodes.

Middle Click (or Space + Drag): Pan view.

Mouse Wheel: Zoom in / Zoom out.

Editing

ENTER: Edit the selected node (for Custom Text nodes).

DELETE: Delete selected nodes and connections.

Ctrl + C: Copy selected nodes.

Ctrl + X: Cut selected nodes.

Ctrl + V: Paste nodes.

Ctrl + Z: Undo.

Ctrl + Y / Shift + Ctrl + Z: Redo.

Tools

T: Toggle the File Scanner Terminal.

?: Toggle the Help overlay.

Building from Source

This project requires a C++ compiler (like g++) and the Raylib library.

Dependencies

Raylib: A simple and easy-to-use library to enjoy videogames programming.

Compilation Command (Example for MinGW/Windows)

g++ main.cpp -lraylib -lopengl32 -lgdi32 -lwinmm -o VisualRegex.exe


Compilation Command (Example for Linux)

g++ main.cpp -lraylib -lGL -lm -lpthread -ldl -rt -Xlinker -zmuldefs -o VisualRegex


Ensure that the sources folder containing font.ttf is present in the same directory as the executable for text rendering to work correctly.

Technical Details

Language: C++ (Standard 17 or higher recommended for filesystem support).

Graphics Engine: Raylib (Immediate Mode GUI implementation).

Regex Engine: C++ Standard Library (<regex>).

File System: C++ Standard Library (<filesystem> and <fstream>)
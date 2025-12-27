/**
 * VISUAL REGEX BUILDER (Pro Edition - Final Fixes + Templates)
 * Language: C++
 * Library: Raylib
 * Description: Node editor with robust Command Parsing, High-Visibility UI, and Regex Templates.
 */

#include "raylib.h"
#include "raymath.h" 
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <regex> 
#include <fstream>
#include <filesystem>
#include <map>
#include <set>

// ----------------------------------------------------------------------------------
// Data Structures
// ----------------------------------------------------------------------------------

enum NodeType {
    NODE_START, NODE_END,
    NODE_TEXT, NODE_DIGIT, NODE_WHITESPACE,
    NODE_ANY, NODE_WORD, NODE_SYMBOL, NODE_CUSTOM,
    NODE_NOT_DIGIT, NODE_NOT_WHITESPACE, NODE_NOT_WORD,
    NODE_ZERO_OR_MORE, NODE_ONE_OR_MORE, NODE_OPTIONAL,
    NODE_GROUP_START, NODE_GROUP_END, NODE_OR
};

struct Node {
    int id;
    Rectangle rect;
    NodeType type;
    std::string title;
    std::string regexValue;
    Color color;
    bool isEditing;
    bool selected; 
    Vector2 dragOffset; 
};

struct Connection {
    int fromNodeId;
    int toNodeId;
};

// Clipboard Structure 
struct ClipboardData {
    std::vector<Node> nodes;
    std::vector<Connection> connections;
};

// Debugger Structures
struct DebugGroup {
    std::string content;
    int start;
    int length;
};

struct DebugMatch {
    int start;
    int length;
    std::string fullMatch;
    std::vector<DebugGroup> groups;
};

// UNDO/REDO STATE (Feature 2)
struct AppState {
    std::vector<Node> nodes;
    std::vector<Connection> connections;
    int nextNodeId;
};

// EXPORT LANGUAGES (Feature 1)
enum ExportLang { LANG_RAW, LANG_CPP, LANG_PYTHON, LANG_JS, LANG_CSHARP, LANG_JAVA };

// TEMPLATES (New Feature)
enum TemplateType { TPL_EMAIL, TPL_DATE_ISO, TPL_PHONE_US, TPL_URL_SIMPLE, TPL_IP_V4 };

// ----------------------------------------------------------------------------------
// Global Variables
// ----------------------------------------------------------------------------------
std::vector<Node> nodes;
std::vector<Connection> connections;
int nextNodeId = 0;

// Undo/Redo Stacks
std::vector<AppState> undoStack;
std::vector<AppState> redoStack;

// Interaction State
bool isCreatingConnection = false;
int connectionStartNodeId = -1;
bool isDraggingNodes = false;
bool isBoxSelecting = false; 
Vector2 boxSelectionStart = { 0, 0 };

ClipboardData clipboard; 

Camera2D camera = { 0 };
Font mainFont = { 0 };

// SYSTEMS STATE
bool showConsole = false;
bool showPlayground = false;
bool showHelp = false;
bool showFullRegex = false; 
bool showTemplates = false; // NEW: Templates Window
bool isDebugging = false; 

// Export State
ExportLang currentExportLang = LANG_RAW;
// Full View Scroll State
float fullRegexScroll = 0.0f;
bool isDraggingFullRegexScroll = false;

// CONSOLE DATA
std::string consoleInput = "";
std::vector<std::string> consoleLog;
int consoleScrollIndex = 0;
bool isDraggingScrollbar = false;

// PLAYGROUND & DEBUGGER DATA
std::string playgroundText = "Hello World! Contact: test@email.com. Date: 2023-10-27.";
Rectangle playgroundRect = { 0, 0, 0, 0 };
float playgroundScrollOffset = 0.0f;
bool isDraggingPlaygroundScroll = false;

// Debugger State
std::vector<DebugMatch> currentDebugMatches;
int currentDebugMatchIndex = 0;

// INPUT TIMING
float cursorBlinkTimer = 0.0f;
float keyRepeatTimer = 0.0f;
const float KEY_REPEAT_DELAY = 0.5f;
const float KEY_REPEAT_RATE = 0.05f;

// COLORS
const Color COL_BG = { 20, 24, 35, 255 };      
const Color COL_GRID = { 40, 45, 60, 255 };    
const Color COL_WIRE = { 200, 200, 200, 150 };       
const Color COL_WIRE_ACTIVE = { 255, 255, 0, 255 };  
const Color COL_SELECTION_BOX = { 0, 228, 48, 50 }; 
const Color COL_SELECTION_BORDER = { 0, 228, 48, 200 };

// Node Colors
const Color COL_CAT_ANCHOR = { 255, 100, 100, 255 }; 
const Color COL_CAT_CHAR = { 0, 228, 48, 255 };      
const Color COL_CAT_DIGIT = { 0, 121, 241, 255 };    
const Color COL_CAT_SPECIAL = { 255, 161, 0, 255 };  
const Color COL_CAT_NEGATED = { 100, 100, 100, 255 };
const Color COL_CAT_QUANT = { 255, 255, 0, 255 };    
const Color COL_CAT_STRUCT = { 180, 80, 255, 255 };  
const Color COL_CAT_CUSTOM = { 255, 0, 255, 255 };   

// Debugger Group Colors
const Color COL_GRP_0 = { 0, 255, 0, 100 };   
const Color COL_GRP_1 = { 0, 121, 241, 150 }; 
const Color COL_GRP_2 = { 255, 161, 0, 150 }; 
const Color COL_GRP_3 = { 255, 0, 255, 150 }; 

// ----------------------------------------------------------------------------------
// Core Logic & Persistence
// ----------------------------------------------------------------------------------

bool GuiButton(Rectangle bounds, const char* text, Color borderColor = BLACK);
void AddLog(std::string msg);
void AddNode(NodeType type, float x, float y); // Forward declare
void SaveState(); // Forward declare

// TEMPLATE GENERATOR
void AddTemplate(TemplateType type, float startX, float startY) {
    SaveState(); // Undo point
    
    int prevNodeId = -1;
    float currentX = startX;
    float spacingX = 180.0f; // Distance between nodes

    // Helper lambda to chain nodes
    auto Append = [&](NodeType t, std::string customVal = "") {
        AddNode(t, currentX, startY);
        Node& n = nodes.back();
        
        // If Custom Node, set the value immediately
        if (t == NODE_CUSTOM && !customVal.empty()) {
            n.regexValue = customVal;
            n.title = customVal; // Show value on node
        }

        if (prevNodeId != -1) {
            connections.push_back({prevNodeId, n.id});
        }
        prevNodeId = n.id;
        currentX += spacingX;
    };

    switch (type) {
        case TPL_EMAIL: // \w+@\w+\.\w+
            Append(NODE_WORD);
            Append(NODE_ONE_OR_MORE);
            Append(NODE_SYMBOL); // @
            Append(NODE_WORD);
            Append(NODE_ONE_OR_MORE);
            Append(NODE_CUSTOM, "."); // literal dot
            Append(NODE_WORD);
            Append(NODE_ONE_OR_MORE);
            break;
            
        case TPL_DATE_ISO: // \d{4}-\d{2}-\d{2}
            Append(NODE_DIGIT);
            Append(NODE_CUSTOM, "{4}");
            Append(NODE_CUSTOM, "-");
            Append(NODE_DIGIT);
            Append(NODE_CUSTOM, "{2}");
            Append(NODE_CUSTOM, "-");
            Append(NODE_DIGIT);
            Append(NODE_CUSTOM, "{2}");
            break;
            
        case TPL_PHONE_US: // \d{3}-\d{3}-\d{4}
            Append(NODE_DIGIT);
            Append(NODE_CUSTOM, "{3}");
            Append(NODE_CUSTOM, "-");
            Append(NODE_DIGIT);
            Append(NODE_CUSTOM, "{3}");
            Append(NODE_CUSTOM, "-");
            Append(NODE_DIGIT);
            Append(NODE_CUSTOM, "{4}");
            break;

        case TPL_URL_SIMPLE: // https?://\w+\.\w+
            Append(NODE_CUSTOM, "http");
            Append(NODE_CUSTOM, "s");
            Append(NODE_OPTIONAL);
            Append(NODE_CUSTOM, "://");
            Append(NODE_WORD);
            Append(NODE_ONE_OR_MORE);
            Append(NODE_CUSTOM, ".");
            Append(NODE_WORD);
            Append(NODE_ONE_OR_MORE);
            break;

        case TPL_IP_V4: // \d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}
            for(int i=0; i<3; i++) {
                Append(NODE_DIGIT);
                Append(NODE_CUSTOM, "{1,3}");
                Append(NODE_CUSTOM, ".");
            }
            Append(NODE_DIGIT);
            Append(NODE_CUSTOM, "{1,3}");
            break;
    }
    
    AddLog("[TEMPLATE] Added preset pattern.");
    showTemplates = false;
}

// Helper to write string with length prefix to avoid space issues
void WriteString(std::ofstream& file, const std::string& s) {
    size_t len = s.length();
    file << len << " " << s << " ";
}

// Helper to read string with length prefix
std::string ReadString(std::ifstream& file) {
    size_t len;
    file >> len;
    char temp; 
    file.get(temp); 
    std::string s(len, '\0');
    file.read(&s[0], len);
    return s;
}

void SaveProject(const std::string& filename) {
    std::string path = filename;
    if (path.find(".vreg") == std::string::npos) path += ".vreg";

    std::ofstream file(path);
    if (!file.is_open()) {
        AddLog("[ERROR] Could not create file: " + path);
        return;
    }

    file << "VREGEX_1.0" << std::endl;
    file << nodes.size() << std::endl;
    for (const auto& n : nodes) {
        file << n.id << " " << (int)n.type << " " 
             << n.rect.x << " " << n.rect.y << " " 
             << (int)n.color.r << " " << (int)n.color.g << " " << (int)n.color.b << " " << (int)n.color.a << " ";
        WriteString(file, n.title);
        WriteString(file, n.regexValue);
        file << std::endl;
    }
    file << connections.size() << std::endl;
    for (const auto& c : connections) {
        file << c.fromNodeId << " " << c.toNodeId << std::endl;
    }
    file << nextNodeId << std::endl;
    file.close();
    AddLog("[SUCCESS] Project saved to: " + std::filesystem::absolute(path).string());
}

void LoadProject(const std::string& filename) {
    std::string path = filename;
    if (path.find(".vreg") == std::string::npos) path += ".vreg";

    if (!std::filesystem::exists(path)) {
        AddLog("[ERROR] File not found: " + path);
        return;
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        AddLog("[ERROR] Could not open file.");
        return;
    }

    std::string header;
    file >> header;
    if (header != "VREGEX_1.0") {
        AddLog("[ERROR] Invalid file format.");
        return;
    }

    nodes.clear();
    connections.clear();

    size_t nodeCount;
    file >> nodeCount;
    for (size_t i = 0; i < nodeCount; ++i) {
        Node n;
        int typeInt, r, g, b, a;
        file >> n.id >> typeInt >> n.rect.x >> n.rect.y >> r >> g >> b >> a;
        n.type = (NodeType)typeInt;
        n.color = { (unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a };
        n.title = ReadString(file);
        n.regexValue = ReadString(file);
        
        n.rect.width = 160; 
        n.rect.height = 60;
        n.isEditing = false;
        n.selected = false;
        
        nodes.push_back(n);
    }

    size_t connCount;
    file >> connCount;
    for (size_t i = 0; i < connCount; ++i) {
        Connection c;
        file >> c.fromNodeId >> c.toNodeId;
        connections.push_back(c);
    }

    file >> nextNodeId;
    file.close();
    AddLog("[SUCCESS] Project Loaded.");
}

void AddNode(NodeType type, float x, float y) {
    Node n;
    n.id = nextNodeId++;
    n.rect = { x, y, 160, 60 };
    n.type = type;
    n.isEditing = false;
    n.selected = false;

    switch (type) {
        case NODE_CUSTOM: n.title = "Custom Text"; n.regexValue = "abc"; n.color = COL_CAT_CUSTOM; break;
        case NODE_START: n.title = "Start of Line"; n.regexValue = "^"; n.color = COL_CAT_ANCHOR; break;
        case NODE_END: n.title = "End of Line"; n.regexValue = "$"; n.color = COL_CAT_ANCHOR; break;
        case NODE_TEXT: n.title = "Letters"; n.regexValue = "[a-zA-Z]+"; n.color = COL_CAT_CHAR; break;
        case NODE_DIGIT: n.title = "Numbers"; n.regexValue = "\\d"; n.color = COL_CAT_DIGIT; break;
        case NODE_WHITESPACE: n.title = "Whitespace"; n.regexValue = "\\s"; n.color = COL_CAT_SPECIAL; break;
        case NODE_ANY: n.title = "Any Char"; n.regexValue = "."; n.color = COL_CAT_SPECIAL; break;
        case NODE_WORD: n.title = "Word Chars"; n.regexValue = "\\w"; n.color = COL_CAT_CHAR; break;
        case NODE_SYMBOL: n.title = "Symbol @"; n.regexValue = "@"; n.color = PURPLE; break;
        case NODE_NOT_DIGIT: n.title = "Non-Number"; n.regexValue = "\\D"; n.color = COL_CAT_NEGATED; break;
        case NODE_NOT_WHITESPACE: n.title = "Non-Space"; n.regexValue = "\\S"; n.color = COL_CAT_NEGATED; break;
        case NODE_NOT_WORD: n.title = "Non-Word"; n.regexValue = "\\W"; n.color = COL_CAT_NEGATED; break;
        case NODE_ZERO_OR_MORE: n.title = "Repeat (0+)"; n.regexValue = "*"; n.color = COL_CAT_QUANT; break;
        case NODE_ONE_OR_MORE: n.title = "Repeat (1+)"; n.regexValue = "+"; n.color = COL_CAT_QUANT; break;
        case NODE_OPTIONAL: n.title = "Optional"; n.regexValue = "?"; n.color = COL_CAT_QUANT; break;
        case NODE_GROUP_START: n.title = "Start Group"; n.regexValue = "("; n.color = COL_CAT_STRUCT; break;
        case NODE_GROUP_END: n.title = "End Group"; n.regexValue = ")"; n.color = COL_CAT_STRUCT; break;
        case NODE_OR: n.title = "OR (Either)"; n.regexValue = "|"; n.color = COL_CAT_STRUCT; break;
    }
    nodes.push_back(n);
}

std::string GenerateRegex() {
    std::stringstream ss;
    int currentNodeId = -1;
    for (const auto& node : nodes) {
        if (node.type == NODE_START) { currentNodeId = node.id; ss << node.regexValue; break; }
    }
    if (currentNodeId == -1 && !nodes.empty()) {
         for (const auto& n : nodes) {
             bool hasInput = false;
             for (const auto& c : connections) { if (c.toNodeId == n.id) hasInput = true; }
             if (!hasInput) { currentNodeId = n.id; ss << n.regexValue; break; }
         }
    }
    if (currentNodeId == -1) return "";

    int safety = 0;
    bool foundNext = true;
    while (foundNext && safety < 100) {
        foundNext = false; safety++;
        for (const auto& conn : connections) {
            if (conn.fromNodeId == currentNodeId) {
                currentNodeId = conn.toNodeId;
                for (const auto& n : nodes) { if (n.id == currentNodeId) { ss << n.regexValue; break; } }
                foundNext = true; break; 
            }
        }
    }
    return ss.str();
}

void DrawGrid2D(int slices, float spacing) {
    Vector2 topLeft = GetScreenToWorld2D({0, 0}, camera);
    Vector2 bottomRight = GetScreenToWorld2D({(float)GetScreenWidth(), (float)GetScreenHeight()}, camera);
    float startX = floor(topLeft.x / spacing) * spacing;
    float startY = floor(topLeft.y / spacing) * spacing;
    for (float x = startX; x < bottomRight.x + spacing; x += spacing) DrawLineV({x, topLeft.y}, {x, bottomRight.y}, COL_GRID);
    for (float y = startY; y < bottomRight.y + spacing; y += spacing) DrawLineV({topLeft.x, y}, {bottomRight.x, y}, COL_GRID);
}

// CONSOLE & DEBUGGER UTILS
void AddLog(std::string msg) {
    consoleLog.push_back(msg);
    if (consoleLog.size() > 1000) consoleLog.erase(consoleLog.begin());
    consoleScrollIndex = consoleLog.size(); 
}

void ProcessConsoleCommand() {
    if (consoleInput.empty()) return;
    
    // Trim spaces
    std::string cleanInput = consoleInput;
    cleanInput.erase(0, cleanInput.find_first_not_of(" \t\n\r"));
    cleanInput.erase(cleanInput.find_last_not_of(" \t\n\r") + 1);
    
    AddLog("> " + cleanInput);
    
    std::stringstream ss(cleanInput);
    std::string command;
    ss >> command; 

    if (command == "save") {
        std::string filename;
        std::getline(ss, filename); 
        if (!filename.empty()) {
            filename.erase(0, filename.find_first_not_of(" \t\n\r"));
            filename.erase(filename.find_last_not_of(" \t\n\r") + 1);
        }
        if (filename.empty()) AddLog("[USAGE] save <filename>");
        else SaveProject(filename);
    }
    else if (command == "load") {
        std::string filename;
        std::getline(ss, filename);
        if (!filename.empty()) {
            filename.erase(0, filename.find_first_not_of(" \t\n\r"));
            filename.erase(filename.find_last_not_of(" \t\n\r") + 1);
        }
        if (filename.empty()) AddLog("[USAGE] load <filename>");
        else LoadProject(filename);
    }
    else {
        // Fallback: Treat whole string as Path
        std::string regStr = GenerateRegex();
        if (regStr.empty()) { AddLog("[ERROR] Empty Regex (Add nodes first)."); consoleInput = ""; return; }

        std::filesystem::path path(cleanInput);
        std::error_code ec;
        if (!std::filesystem::exists(path, ec)) { 
            AddLog("[ERROR] Unknown command or Path not found: " + cleanInput); 
            consoleInput = ""; 
            return; 
        }

        AddLog("Scanning path: " + cleanInput);
        try {
            std::regex pattern(regStr);
            int totalMatches = 0;
            int filesScanned = 0;
            auto processFile = [&](const std::filesystem::path& filePath) {
                std::ifstream file(filePath);
                if (!file.is_open()) return;
                std::stringstream buffer; buffer << file.rdbuf(); std::string content = buffer.str();
                auto b = std::sregex_iterator(content.begin(), content.end(), pattern);
                auto e = std::sregex_iterator();
                int count = std::distance(b, e);
                if (count > 0) { AddLog("HIT: " + filePath.filename().string() + " (" + std::to_string(count) + ")"); totalMatches += count; }
                filesScanned++;
            };
            if (std::filesystem::is_directory(path)) {
                for (const auto& entry : std::filesystem::directory_iterator(path)) if (entry.is_regular_file()) processFile(entry.path());
            } else if (std::filesystem::is_regular_file(path)) processFile(path);
            AddLog("[DONE] Scanned " + std::to_string(filesScanned) + " files. Matches: " + std::to_string(totalMatches));
        } catch (const std::exception& e) { AddLog("[ERROR] Regex Engine: " + std::string(e.what())); }
    }
    consoleInput = "";
}

void AnalyzeMatchesForDebug(const std::string& patternStr) {
    currentDebugMatches.clear();
    try {
        std::regex pattern(patternStr);
        auto words_begin = std::sregex_iterator(playgroundText.begin(), playgroundText.end(), pattern);
        auto words_end = std::sregex_iterator();

        for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
            std::smatch match = *i;
            DebugMatch dm;
            dm.fullMatch = match.str();
            dm.start = match.position();
            dm.length = match.length();
            
            for (size_t k = 1; k < match.size(); ++k) {
                DebugGroup dg;
                dg.content = match[k].str();
                dg.start = match.position(k);
                dg.length = match.length(k);
                dm.groups.push_back(dg);
            }
            currentDebugMatches.push_back(dm);
        }
    } catch (...) {}
    if (currentDebugMatchIndex >= (int)currentDebugMatches.size()) currentDebugMatchIndex = 0;
}

float CalculateWrappedHeight(const std::string& text, float fontSize, float maxWidth) {
    float x = 0; float y = fontSize; 
    for (size_t i = 0; i < text.length(); ++i) {
        if (text[i] == '\n') { x = 0; y += fontSize; continue; }
        char b[2] = { text[i], '\0' };
        float w = MeasureTextEx(mainFont, b, fontSize, 1.0f).x;
        if (x + w > maxWidth) { x = 0; y += fontSize; }
        x += w;
    }
    return y;
}

// Updated DrawTextWrapped to handle scrollOffset
void DrawTextWrapped(Font font, const std::string& text, Rectangle rec, float fontSize, Color color, float scrollOffset) {
    float x = 0; float y = 0;
    for (size_t i = 0; i < text.length(); ++i) {
        char c = text[i];
        if (c == '\n') { x = 0; y += fontSize; continue; }
        char b[2] = { c, '\0' };
        float w = MeasureTextEx(font, b, fontSize, 1.0f).x;
        
        if (x + w > rec.width) { x = 0; y += fontSize; }
        
        // Drawing with culling
        float drawY = rec.y + y - scrollOffset;
        if (drawY + fontSize > rec.y && drawY < rec.y + rec.height) {
            DrawTextEx(font, b, {rec.x + x, drawY}, fontSize, 1.0f, color);
        }
        x += w;
    }
}

Vector2 GetTextPos(const std::string& text, size_t index, float fontSize) {
    int line = 0; size_t lastNewLine = 0;
    for (size_t i = 0; i < index && i < text.length(); i++) {
        if (text[i] == '\n') { line++; lastNewLine = i + 1; }
    }
    std::string currentLinePrefix = text.substr(lastNewLine, index - lastNewLine);
    float x = MeasureTextEx(mainFont, currentLinePrefix.c_str(), fontSize, 1.0f).x;
    return {x, line * fontSize};
}

// UNDO / REDO IMPLEMENTATION (RESTORED)
void SaveState() {
    AppState state;
    state.nodes = nodes;
    state.connections = connections;
    state.nextNodeId = nextNodeId;
    undoStack.push_back(state);
    if (undoStack.size() > 50) undoStack.erase(undoStack.begin());
    redoStack.clear(); 
}

void Undo() {
    if (undoStack.empty()) return;
    AppState current;
    current.nodes = nodes;
    current.connections = connections;
    current.nextNodeId = nextNodeId;
    redoStack.push_back(current);
    AppState prev = undoStack.back();
    undoStack.pop_back();
    nodes = prev.nodes;
    connections = prev.connections;
    nextNodeId = prev.nextNodeId;
    AddLog("[UNDO]");
}

void Redo() {
    if (redoStack.empty()) return;
    AppState current;
    current.nodes = nodes;
    current.connections = connections;
    current.nextNodeId = nextNodeId;
    undoStack.push_back(current);
    AppState next = redoStack.back();
    redoStack.pop_back();
    nodes = next.nodes;
    connections = next.connections;
    nextNodeId = next.nextNodeId;
    AddLog("[REDO]");
}

// CODE EXPORT IMPLEMENTATION (RESTORED)
std::string GetExportCode(std::string regex, ExportLang lang) {
    std::string out = "";
    switch (lang) {
        case LANG_RAW: return regex;
        case LANG_CPP: 
            for(char c : regex) {
                if(c == '\\') out += "\\\\";
                else if(c == '"') out += "\\\"";
                else out += c;
            }
            return "std::regex pattern(\"" + out + "\");";
        case LANG_PYTHON:
            return "pattern = re.compile(r\"" + regex + "\")";
        case LANG_JS:
            return "const pattern = /" + regex + "/;";
        case LANG_CSHARP:
            for(char c : regex) {
                if(c == '"') out += "\"\"";
                else out += c;
            }
            return "Regex pattern = new Regex(@\"" + out + "\");";
        case LANG_JAVA:
            for(char c : regex) {
                if(c == '\\') out += "\\\\";
                else if(c == '"') out += "\\\"";
                else out += c;
            }
            return "Pattern pattern = Pattern.compile(\"" + out + "\");";
    }
    return regex;
}

// ----------------------------------------------------------------------------------
// COPY / PASTE / CLIPBOARD LOGIC
// ----------------------------------------------------------------------------------
void CopyToClipboard() {
    clipboard.nodes.clear();
    clipboard.connections.clear();
    std::set<int> selectedIds;

    for (const auto& n : nodes) {
        if (n.selected) {
            clipboard.nodes.push_back(n);
            selectedIds.insert(n.id);
        }
    }

    for (const auto& conn : connections) {
        if (selectedIds.count(conn.fromNodeId) && selectedIds.count(conn.toNodeId)) {
            clipboard.connections.push_back(conn);
        }
    }
    AddLog("[CLIPBOARD] Copied " + std::to_string(clipboard.nodes.size()) + " nodes.");
}

void PasteFromClipboard(Vector2 pastePos) {
    if (clipboard.nodes.empty()) return;
    SaveState(); // UNDO POINT

    std::map<int, int> idMap; 
    for (auto& n : nodes) n.selected = false;

    Vector2 avgPos = {0,0};
    for (const auto& n : clipboard.nodes) { avgPos.x += n.rect.x; avgPos.y += n.rect.y; }
    avgPos.x /= clipboard.nodes.size();
    avgPos.y /= clipboard.nodes.size();

    for (const auto& clipNode : clipboard.nodes) {
        Node newNode = clipNode;
        newNode.id = nextNodeId++;
        newNode.selected = true; 
        Vector2 relPos = { clipNode.rect.x - avgPos.x, clipNode.rect.y - avgPos.y };
        newNode.rect.x = pastePos.x + relPos.x;
        newNode.rect.y = pastePos.y + relPos.y;
        nodes.push_back(newNode);
        idMap[clipNode.id] = newNode.id;
    }

    for (const auto& clipConn : clipboard.connections) {
        Connection newConn;
        newConn.fromNodeId = idMap[clipConn.fromNodeId];
        newConn.toNodeId = idMap[clipConn.toNodeId];
        connections.push_back(newConn);
    }
    AddLog("[CLIPBOARD] Pasted.");
}

void DeleteSelected() {
    if (nodes.empty() && connections.empty()) return;
    SaveState(); // UNDO POINT

    for (int i = connections.size() - 1; i >= 0; i--) {
        bool fromSel = false, toSel = false;
        for (const auto& n : nodes) {
            if (n.id == connections[i].fromNodeId && n.selected) fromSel = true;
            if (n.id == connections[i].toNodeId && n.selected) toSel = true;
        }
        if (fromSel || toSel) connections.erase(connections.begin() + i);
    }
    for (int i = nodes.size() - 1; i >= 0; i--) {
        if (nodes[i].selected) nodes.erase(nodes.begin() + i);
    }
}

// ----------------------------------------------------------------------------------
// Main
// ----------------------------------------------------------------------------------

int main() {
    const int screenWidth = 1280;
    const int screenHeight = 900;
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE); 
    InitWindow(screenWidth, screenHeight, "Visual Regex - Pro Edition");

    mainFont = LoadFontEx("sources/font.ttf", 32, 0, 0); 
    if (mainFont.texture.id == 0) mainFont = GetFontDefault();

    SetTargetFPS(60);
    camera.zoom = 1.0f;
    AddNode(NODE_START, 100, 300);
    
    int editingNodeId = -1;
    float copyFeedbackTimer = 0.0f;
    AddLog("Ready. Type 'save <name>' or 'load <name>' in terminal.");

    SetExitKey(KEY_NULL);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (copyFeedbackTimer > 0) copyFeedbackTimer -= dt;
        cursorBlinkTimer += dt;

        int curW = GetScreenWidth(); int curH = GetScreenHeight(); // DYNAMIC SIZE
        Vector2 mouseScreen = GetMousePosition();
        Vector2 mouseWorld = GetScreenToWorld2D(mouseScreen, camera);
        
        int key = GetCharPressed(); 
        bool inputConsumed = false;

        auto HandleBackspace = [&](std::string& target) {
            if (IsKeyPressed(KEY_BACKSPACE)) {
                if (!target.empty()) target.pop_back();
                keyRepeatTimer = KEY_REPEAT_DELAY; 
            } else if (IsKeyDown(KEY_BACKSPACE)) {
                keyRepeatTimer -= dt;
                if (keyRepeatTimer <= 0) {
                    if (!target.empty()) target.pop_back();
                    keyRepeatTimer = KEY_REPEAT_RATE; 
                }
            } else { keyRepeatTimer = 0; }
        };

        // INPUT: GLOBAL SHORTCUTS
        bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
        if (ctrl && IsKeyPressed(KEY_Z)) Undo();
        if (ctrl && (IsKeyPressed(KEY_Y) || (IsKeyDown(KEY_LEFT_SHIFT) && IsKeyPressed(KEY_Z)))) Redo();

        // 1. OVERLAYS (Help/FullView)
        if (showHelp || showFullRegex || showTemplates) {
            bool clickedOutside = false;
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (showFullRegex) {
                     Rectangle fullRect = { (float)curW/2 - 350, (float)curH/2 - 250, 700, 500 };
                     if (!CheckCollisionPointRec(mouseScreen, fullRect)) clickedOutside = true;
                } else if (showHelp) {
                     Rectangle helpRect = { (float)curW/2 - 250, (float)curH/2 - 200, 500, 400 };
                     if (!CheckCollisionPointRec(mouseScreen, helpRect)) clickedOutside = true;
                } else if (showTemplates) {
                    Rectangle tplRect = { (float)curW/2 - 200, (float)curH/2 - 200, 400, 400 };
                    if (!CheckCollisionPointRec(mouseScreen, tplRect)) clickedOutside = true;
                }
            }

            if (IsKeyPressed(KEY_ESCAPE) || clickedOutside) { 
                showHelp = false; showFullRegex = false; showTemplates = false;
            }
            inputConsumed = true;
        }
        // 2. CONSOLE
        else if (showConsole) {
            if (IsKeyPressed(KEY_ESCAPE)) showConsole = false;
            if (IsKeyPressed(KEY_ENTER)) ProcessConsoleCommand();
            if (ctrl && IsKeyPressed(KEY_V)) {
                const char* clip = GetClipboardText();
                if (clip) consoleInput += std::string(clip);
            }
            while (key > 0) {
                if ((key >= 32) && (key <= 125)) consoleInput += (char)key;
                key = GetCharPressed();
            }
            HandleBackspace(consoleInput); 
            inputConsumed = true; 
        }
        else if (IsKeyPressed(KEY_T) && editingNodeId == -1 && !showConsole && !showPlayground) {
            showConsole = true;
            inputConsumed = true;
        }

        // 3. PLAYGROUND
        bool mouseOverPlayground = showPlayground && CheckCollisionPointRec(mouseScreen, playgroundRect);
        if (!inputConsumed && showPlayground && mouseOverPlayground) {
            if (ctrl && IsKeyPressed(KEY_V)) {
                const char* clip = GetClipboardText();
                if (clip) playgroundText += std::string(clip);
                if (isDebugging) AnalyzeMatchesForDebug(GenerateRegex()); 
            }
            while (key > 0) {
                if ((key >= 32) && (key <= 125)) playgroundText += (char)key;
                key = GetCharPressed();
                if (isDebugging) AnalyzeMatchesForDebug(GenerateRegex());
            }
            if (IsKeyPressed(KEY_BACKSPACE) || IsKeyDown(KEY_BACKSPACE)) {
               HandleBackspace(playgroundText);
               if (isDebugging) AnalyzeMatchesForDebug(GenerateRegex());
            }
            if (IsKeyPressed(KEY_ENTER)) { playgroundText += '\n'; if (isDebugging) AnalyzeMatchesForDebug(GenerateRegex()); }
            inputConsumed = true; 
        }

        // 4. NODE EDITING
        if (!inputConsumed && editingNodeId != -1) {
            if (IsKeyPressed(KEY_ENTER) || (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !CheckCollisionPointRec(mouseWorld, nodes[0].rect))) { 
                SaveState(); // UNDO POINT: Finish Edit
                editingNodeId = -1;
                for(auto& n : nodes) n.isEditing = false;
            } else {
                while (key > 0) {
                    if ((key >= 32) && (key <= 125)) {
                        for(auto& n : nodes) if (n.id == editingNodeId) {
                            n.regexValue += (char)key;
                            if(n.type == NODE_CUSTOM) n.title = n.regexValue; 
                        }
                    }
                    key = GetCharPressed();
                }
                if (IsKeyPressed(KEY_BACKSPACE)) {
                    for(auto& n : nodes) if (n.id == editingNodeId && !n.regexValue.empty()) {
                        n.regexValue.pop_back();
                        if(n.type == NODE_CUSTOM) n.title = n.regexValue;
                    }
                    keyRepeatTimer = KEY_REPEAT_DELAY;
                } else if (IsKeyDown(KEY_BACKSPACE)) {
                    keyRepeatTimer -= dt;
                    if (keyRepeatTimer <= 0) {
                        for(auto& n : nodes) if (n.id == editingNodeId && !n.regexValue.empty()) {
                            n.regexValue.pop_back();
                            if(n.type == NODE_CUSTOM) n.title = n.regexValue;
                        }
                        keyRepeatTimer = KEY_REPEAT_RATE;
                    }
                }
            }
            inputConsumed = true;
        }

        // 5. CANVAS NAVIGATION & INTERACTION
        if (!inputConsumed && !mouseOverPlayground) {
            // Camera
            float wheel = GetMouseWheelMove();
            if (wheel != 0) {
                Vector2 mouseWorldBeforeZoom = GetScreenToWorld2D(mouseScreen, camera);
                camera.zoom += (wheel * 0.125f);
                if (camera.zoom < 0.2f) camera.zoom = 0.2f; if (camera.zoom > 3.0f) camera.zoom = 3.0f;
                Vector2 mouseWorldAfterZoom = GetScreenToWorld2D(mouseScreen, camera);
                camera.target.x += (mouseWorldBeforeZoom.x - mouseWorldAfterZoom.x);
                camera.target.y += (mouseWorldBeforeZoom.y - mouseWorldAfterZoom.y);
            }
            if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) || (IsKeyDown(KEY_SPACE) && IsMouseButtonDown(MOUSE_LEFT_BUTTON))) {
                Vector2 delta = GetMouseDelta();
                camera.target.x -= delta.x / camera.zoom;
                camera.target.y -= delta.y / camera.zoom;
            }

            // COPY / CUT / PASTE SHORTCUTS
            if (ctrl && IsKeyPressed(KEY_C)) CopyToClipboard();
            if (ctrl && IsKeyPressed(KEY_V)) PasteFromClipboard(mouseWorld);
            if (ctrl && IsKeyPressed(KEY_X)) { CopyToClipboard(); DeleteSelected(); }
            if (IsKeyPressed(KEY_DELETE)) DeleteSelected();

            // MOUSE LEFT CLICK LOGIC (Selection & Drag)
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && mouseScreen.y > 80 && mouseScreen.y < curH - 210) { // Check zones with dynamic height
                bool clickedNode = false;
                for (int i = nodes.size() - 1; i >= 0; i--) {
                    if (CheckCollisionPointRec(mouseWorld, nodes[i].rect)) {
                        clickedNode = true;
                        bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
                        if (shift) {
                            nodes[i].selected = !nodes[i].selected;
                        } else {
                            if (!nodes[i].selected) {
                                for (auto& n : nodes) n.selected = false;
                                nodes[i].selected = true;
                            }
                        }
                        isDraggingNodes = true;
                        SaveState(); // UNDO POINT: Start Drag
                        for (auto& n : nodes) {
                            if (n.selected) {
                                n.dragOffset = { mouseWorld.x - n.rect.x, mouseWorld.y - n.rect.y };
                            }
                        }
                        break;
                    }
                }

                if (!clickedNode) {
                    bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
                    if (!shift) for (auto& n : nodes) n.selected = false;
                    isBoxSelecting = true;
                    boxSelectionStart = mouseWorld;
                }
            }

            // MOUSE DRAG LOGIC
            if (isDraggingNodes && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                for (auto& n : nodes) {
                    if (n.selected) {
                        n.rect.x = mouseWorld.x - n.dragOffset.x;
                        n.rect.y = mouseWorld.y - n.dragOffset.y;
                    }
                }
            } else {
                isDraggingNodes = false;
            }

            // BOX SELECTION LOGIC
            if (isBoxSelecting) {
                if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                    isBoxSelecting = false;
                    float x = std::min(boxSelectionStart.x, mouseWorld.x);
                    float y = std::min(boxSelectionStart.y, mouseWorld.y);
                    float w = std::abs(mouseWorld.x - boxSelectionStart.x);
                    float h = std::abs(mouseWorld.y - boxSelectionStart.y);
                    Rectangle selRect = {x,y,w,h};
                    for (auto& n : nodes) if (CheckCollisionRecs(selRect, n.rect)) n.selected = true;
                }
            }

            // ENTER TO EDIT
            if (IsKeyPressed(KEY_ENTER)) {
                for (auto& n : nodes) {
                    if (n.selected) {
                        editingNodeId = n.id;
                        SaveState(); // UNDO POINT: Before Edit
                        n.isEditing = true;
                        break; 
                    }
                }
            }

            // CONNECTIONS
            if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
                for (const auto& n : nodes) if (CheckCollisionPointRec(mouseWorld, n.rect)) { isCreatingConnection = true; connectionStartNodeId = n.id; break; }
            }
            if (IsMouseButtonReleased(MOUSE_RIGHT_BUTTON) && isCreatingConnection) {
                isCreatingConnection = false;
                for (const auto& n : nodes) if (CheckCollisionPointRec(mouseWorld, n.rect) && n.id != connectionStartNodeId) {
                    SaveState(); // UNDO POINT: New Connection
                    connections.push_back({connectionStartNodeId, n.id}); break;
                }
            }
        }

        // --- DRAW ---
        BeginDrawing();
        ClearBackground(COL_BG);

        BeginMode2D(camera);
            DrawGrid2D(100, 40.0f);
            
            for (const auto& conn : connections) {
                Vector2 s, e;
                for (const auto& n : nodes) {
                    if (n.id == conn.fromNodeId) s = { n.rect.x + n.rect.width, n.rect.y + n.rect.height/2 };
                    if (n.id == conn.toNodeId) e = { n.rect.x, n.rect.y + n.rect.height/2 };
                }
                DrawLineBezier(s, e, 3.0f, COL_WIRE);
            }
            if (isCreatingConnection) {
                Vector2 s;
                for (const auto& n : nodes) if (n.id == connectionStartNodeId) s = { n.rect.x + n.rect.width, n.rect.y + n.rect.height/2 };
                DrawLineBezier(s, mouseWorld, 3.0f, COL_WIRE_ACTIVE);
            }

            for (const auto& n : nodes) {
                DrawRectangleRounded(n.rect, 0.2f, 8, n.isEditing ? RED : n.color);
                if (n.selected) DrawRectangleRoundedLines(n.rect, 0.2f, 8, WHITE);
                else DrawRectangleRoundedLines(n.rect, 0.2f, 8, BLACK);
                
                std::string displayStr = n.title;
                if (n.isEditing && n.type != NODE_CUSTOM) displayStr = n.regexValue;
                DrawTextEx(mainFont, displayStr.c_str(), {n.rect.x + 10, n.rect.y + 20}, 18, 1.0f, BLACK);
            }

            if (isBoxSelecting) {
                float x = std::min(boxSelectionStart.x, mouseWorld.x);
                float y = std::min(boxSelectionStart.y, mouseWorld.y);
                float w = std::abs(mouseWorld.x - boxSelectionStart.x);
                float h = std::abs(mouseWorld.y - boxSelectionStart.y);
                DrawRectangleRec({x,y,w,h}, COL_SELECTION_BOX);
                DrawRectangleLinesEx({x,y,w,h}, 1, COL_SELECTION_BORDER);
            }

        EndMode2D();

        // UI HEADERS
        DrawRectangle(0, 0, curW, 80, Fade(BLACK, 0.9f));
        std::string regStr = GenerateRegex();
        DrawTextEx(mainFont, "REGEX:", {20, 30}, 20, 1.0f, LIGHTGRAY);
        
        float textStartX = 100;
        float buttonStartX = (float)curW - 540; // Adjusted for extra buttons (Templates)
        float availableWidth = buttonStartX - textStartX - 20;
        float fontSize = 30;
        float textWidth = MeasureTextEx(mainFont, regStr.c_str(), fontSize, 1.0f).x;
        if (textWidth > availableWidth) {
            fontSize = 30 * (availableWidth / textWidth);
            if (fontSize < 16) fontSize = 16;
        }
        BeginScissorMode((int)textStartX, 0, (int)availableWidth, 80);
            DrawTextEx(mainFont, regStr.c_str(), {textStartX, 25}, fontSize, 1.0f, YELLOW);
        EndScissorMode();
        
        // --- BUTTONS ---
        float btnX = (float)curW - 540;
        if (GuiButton({btnX, 20, 60, 40}, "SAVE", YELLOW)) { showConsole = true; consoleInput = "save "; } btnX += 70;
        if (GuiButton({btnX, 20, 60, 40}, "LOAD", SKYBLUE)) { showConsole = true; consoleInput = "load "; } btnX += 70;
        if (GuiButton({btnX, 20, 60, 40}, "TEMPL", ORANGE)) showTemplates = !showTemplates; btnX += 70;
        if (GuiButton({btnX, 20, 60, 40}, "FULL")) showFullRegex = !showFullRegex; btnX += 70;

        if (!showConsole) {
            if (GuiButton({btnX, 20, 100, 40}, showPlayground ? "HIDE" : "TEST")) {
                showPlayground = !showPlayground;
                if (showPlayground) isDebugging = false; 
            }
        } else {
            DrawRectangleRec({btnX, 20, 100, 40}, Fade(GRAY, 0.5f));
            DrawTextEx(mainFont, "TEST", {btnX + 30, 30}, 18, 1.0f, DARKGRAY);
        }
        btnX += 110;

        if (GuiButton({btnX, 20, 80, 40}, copyFeedbackTimer > 0 ? "OK!" : "COPY")) {
            SetClipboardText(regStr.c_str()); copyFeedbackTimer = 2.0f;
        } btnX += 90;
        if (GuiButton({btnX, 20, 40, 40}, "?")) showHelp = !showHelp;

        // PLAYGROUND PANEL
        if (showPlayground) {
            float pgWidth = 400;
            float headerH = 80;
            float footerH = 210; // UPDATED to match bottom panel
            playgroundRect = { (float)curW - pgWidth, headerH, pgWidth, (float)curH - headerH - footerH };
            
            DrawRectangleRec(playgroundRect, Fade(COL_BG, 0.95f));
            DrawRectangleLinesEx(playgroundRect, 2, BLUE);
            
            // Header
            DrawRectangle(playgroundRect.x, playgroundRect.y, playgroundRect.width, 40, Fade(BLUE, 0.2f));
            DrawTextEx(mainFont, isDebugging ? "MATCH DEBUGGER" : "PLAYGROUND", {playgroundRect.x + 20, playgroundRect.y + 10}, 18, 1.0f, BLUE);
            
            // NEW: ERASE BUTTON
            if (GuiButton({playgroundRect.x + playgroundRect.width - 190, playgroundRect.y + 5, 80, 30}, "ERASE")) {
                playgroundText = "";
                if (isDebugging) AnalyzeMatchesForDebug(regStr);
            }
            
            if (GuiButton({playgroundRect.x + playgroundRect.width - 100, playgroundRect.y + 5, 80, 30}, isDebugging ? "EXIT" : "DEBUG")) {
                isDebugging = !isDebugging;
                if (isDebugging) AnalyzeMatchesForDebug(regStr);
            }

            float fontSize = 20.0f;
            Rectangle textArea = { playgroundRect.x + 10, playgroundRect.y + 50, playgroundRect.width - 35, playgroundRect.height - 60 };
            if (isDebugging) textArea.height -= 100; 

            float totalHeight = CalculateWrappedHeight(playgroundText, fontSize, textArea.width);
            float maxScroll = std::max(0.0f, totalHeight - textArea.height);
            
            if (mouseOverPlayground) {
                float wheel = GetMouseWheelMove();
                if (wheel != 0) {
                    playgroundScrollOffset -= wheel * 30.0f;
                    if (playgroundScrollOffset < 0) playgroundScrollOffset = 0;
                    if (playgroundScrollOffset > maxScroll) playgroundScrollOffset = maxScroll;
                }
            }

            Rectangle scrollTrack = { textArea.x + textArea.width + 5, textArea.y, 15, textArea.height };
            DrawRectangleRec(scrollTrack, Fade(BLUE, 0.1f));
            if (maxScroll > 0) {
                float viewRatio = textArea.height / totalHeight;
                float thumbH = std::max(20.0f, scrollTrack.height * viewRatio);
                float scrollRatio = playgroundScrollOffset / maxScroll;
                float thumbY = scrollTrack.y + (scrollTrack.height - thumbH) * scrollRatio;
                Rectangle scrollThumb = { scrollTrack.x, thumbY, scrollTrack.width, thumbH };
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouseScreen, scrollTrack)) isDraggingPlaygroundScroll = true;
                if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) isDraggingPlaygroundScroll = false;
                if (isDraggingPlaygroundScroll) {
                    float relativeY = mouseScreen.y - scrollTrack.y - thumbH/2;
                    float ratio = relativeY / (scrollTrack.height - thumbH);
                    playgroundScrollOffset = ratio * maxScroll;
                    if (playgroundScrollOffset < 0) playgroundScrollOffset = 0;
                    if (playgroundScrollOffset > maxScroll) playgroundScrollOffset = maxScroll;
                }
                DrawRectangleRec(scrollThumb, isDraggingPlaygroundScroll ? BLUE : Fade(BLUE, 0.5f));
            } else playgroundScrollOffset = 0;

            BeginScissorMode((int)textArea.x, (int)textArea.y, (int)textArea.width, (int)textArea.height);
                std::vector<bool> isMatched(playgroundText.length(), false);
                std::vector<int> matchColors(playgroundText.length(), 0);
                
                if (!regStr.empty()) {
                    try {
                        std::regex pattern(regStr);
                        auto wb = std::sregex_iterator(playgroundText.begin(), playgroundText.end(), pattern);
                        auto we = std::sregex_iterator();
                        for (auto i = wb; i != we; ++i) {
                            std::smatch match = *i;
                            if (isDebugging) {
                                if (currentDebugMatches.empty()) continue;
                                DebugMatch& dm = currentDebugMatches[currentDebugMatchIndex];
                                if (match.position() == dm.start) {
                                    for (int k = 0; k < match.length(); k++) {
                                        isMatched[match.position() + k] = true;
                                        matchColors[match.position() + k] = 1;
                                    }
                                    int gIdx = 0;
                                    for (auto& grp : dm.groups) {
                                        int gColorID = 2 + (gIdx % 3);
                                        for (int k = 0; k < grp.length; k++) matchColors[grp.start + k] = gColorID;
                                        gIdx++;
                                    }
                                }
                            } else {
                                for (int k = 0; k < match.length(); k++) {
                                    isMatched[match.position() + k] = true;
                                    matchColors[match.position() + k] = 1; 
                                }
                            }
                        }
                    } catch (...) {}
                }

                float tx = 0; float ty = 0; 
                for (size_t i = 0; i < playgroundText.length(); i++) {
                    char c = playgroundText[i];
                    char b[2] = { c, '\0' };
                    float cw = MeasureTextEx(mainFont, b, fontSize, 1.0f).x;
                    if (c == '\n') { tx = 0; ty += fontSize; continue; }
                    if (tx + cw > textArea.width) { tx = 0; ty += fontSize; }
                    if (isMatched[i]) {
                        Color hc = Fade(GREEN, 0.4f);
                        if (isDebugging) {
                            if (matchColors[i] == 1) hc = COL_GRP_0;
                            else if (matchColors[i] == 2) hc = COL_GRP_1;
                            else if (matchColors[i] == 3) hc = COL_GRP_2;
                            else if (matchColors[i] == 4) hc = COL_GRP_3;
                        }
                        DrawRectangle(textArea.x + tx, textArea.y + ty - playgroundScrollOffset, cw, fontSize, hc);
                    }
                    DrawTextEx(mainFont, b, {textArea.x + tx, textArea.y + ty - playgroundScrollOffset}, fontSize, 1.0f, WHITE);
                    tx += cw;
                }
                if (mouseOverPlayground && ((int)(cursorBlinkTimer*2)%2==0)) {
                    DrawRectangle(textArea.x + tx + 2, textArea.y + ty - playgroundScrollOffset, 2, fontSize, WHITE);
                }
            EndScissorMode();

            if (isDebugging) {
                float debugY = textArea.y + textArea.height + 10;
                DrawLine(textArea.x, debugY - 5, textArea.x + textArea.width, debugY - 5, BLUE);
                if (currentDebugMatches.empty()) {
                    DrawTextEx(mainFont, "No matches found.", {textArea.x, debugY}, 18, 1.0f, RED);
                } else {
                    if (GuiButton({textArea.x, debugY, 30, 30}, "<")) {
                        currentDebugMatchIndex--;
                        if (currentDebugMatchIndex < 0) currentDebugMatchIndex = currentDebugMatches.size() - 1;
                    }
                    std::string counter = "Match " + std::to_string(currentDebugMatchIndex + 1) + " / " + std::to_string(currentDebugMatches.size());
                    DrawTextEx(mainFont, counter.c_str(), {textArea.x + 40, debugY + 5}, 18, 1.0f, WHITE);
                    if (GuiButton({textArea.x + 160, debugY, 30, 30}, ">")) {
                        currentDebugMatchIndex = (currentDebugMatchIndex + 1) % currentDebugMatches.size();
                    }
                    DebugMatch& dm = currentDebugMatches[currentDebugMatchIndex];
                    float grpY = debugY + 40;
                    DrawTextEx(mainFont, "Groups:", {textArea.x, grpY}, 16, 1.0f, GRAY); grpY += 20;
                    int gCount = 1;
                    for (auto& grp : dm.groups) {
                        std::string gInfo = "G" + std::to_string(gCount) + ": " + grp.content;
                        Color gc = COL_GRP_1;
                        if ((gCount-1)%3 == 1) gc = COL_GRP_2;
                        if ((gCount-1)%3 == 2) gc = COL_GRP_3;
                        DrawTextEx(mainFont, gInfo.c_str(), {textArea.x, grpY}, 16, 1.0f, gc);
                        grpY += 20; gCount++;
                    }
                }
            }
        } else { playgroundRect = {0,0,0,0}; }

        // Bottom UI Panel
        int panelHeight = 220; // Fixed panel height
        DrawRectangle(0, curH - panelHeight, curW, panelHeight, Fade(BLACK, 0.9f));
        
        DrawTextEx(mainFont, "Pan: Mid-Click | Zoom: Wheel | R-Click: Connect | Del: Delete | Enter: Edit | T: Terminal", {20, (float)curH - panelHeight + 15}, 16, 1.0f, GRAY);
        DrawTextEx(mainFont, "Shift+Click: Multi-Select | Drag: Select Area | Ctrl+C/V/X: Clipboard | SAVE/LOAD: Project", {20, (float)curH - panelHeight + 35}, 16, 1.0f, DARKGRAY);

        Vector2 c = GetScreenToWorld2D({ (float)curW/2, (float)curH/2 }, camera);
        int startX = 20; 
        int startY = curH - panelHeight + 70;
        int btnW = 140; int btnH = 35; int gapX = 150; int gapY = 45;

        // Button Grid...
        int x = startX; int y = startY;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "CUSTOM TEXT")) { SaveState(); AddNode(NODE_CUSTOM, c.x, c.y); } x+=gapX;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Letters")) { SaveState(); AddNode(NODE_TEXT, c.x, c.y); } x+=gapX;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Numbers")) { SaveState(); AddNode(NODE_DIGIT, c.x, c.y); } x+=gapX;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Word Chars")) { SaveState(); AddNode(NODE_WORD, c.x, c.y); } x+=gapX;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Whitespace")) { SaveState(); AddNode(NODE_WHITESPACE, c.x, c.y); } x+=gapX;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Any Char")) { SaveState(); AddNode(NODE_ANY, c.x, c.y); } x+=gapX;
        
        x = startX; y += gapY;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Symbol @")) { SaveState(); AddNode(NODE_SYMBOL, c.x, c.y); } x+=gapX;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Non-Number")) { SaveState(); AddNode(NODE_NOT_DIGIT, c.x, c.y); } x+=gapX;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Non-Word Char")) { SaveState(); AddNode(NODE_NOT_WORD, c.x, c.y); } x+=gapX;
        x += 20; 
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Repeat (0+)")) { SaveState(); AddNode(NODE_ZERO_OR_MORE, c.x, c.y); } x+=gapX;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Repeat (1+)")) { SaveState(); AddNode(NODE_ONE_OR_MORE, c.x, c.y); } x+=gapX;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Make Optional")) { SaveState(); AddNode(NODE_OPTIONAL, c.x, c.y); } x+=gapX;

        x = startX; y += gapY;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Start Line")) { SaveState(); AddNode(NODE_START, c.x, c.y); } x+=gapX;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "End Line")) { SaveState(); AddNode(NODE_END, c.x, c.y); } x+=gapX;
        x += 20; 
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Begin Group")) { SaveState(); AddNode(NODE_GROUP_START, c.x, c.y); } x+=gapX;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Finish Group")) { SaveState(); AddNode(NODE_GROUP_END, c.x, c.y); } x+=gapX;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "OR (Either)")) { SaveState(); AddNode(NODE_OR, c.x, c.y); } x+=gapX;

        // CONSOLE (Draw Last if open)
        if (showConsole) {
            float conH = 400;
            DrawRectangle(0, 0, curW, curH, Fade(BLACK, 0.6f));
            Rectangle conRect = { (float)curW/2 - 300, 100, 600, conH };
            DrawRectangleRec(conRect, Fade(BLACK, 0.95f));
            DrawRectangleLinesEx(conRect, 2, GREEN);
            DrawRectangle(conRect.x, conRect.y, conRect.width, 40, Fade(GREEN, 0.2f));
            DrawTextEx(mainFont, "TERMINAL - SAVE / LOAD / SCAN", {conRect.x + 20, conRect.y + 10}, 20, 1.0f, GREEN);
            if (GuiButton({conRect.x + conRect.width - 40, conRect.y, 40, 40}, "X")) showConsole = false;

            float contentAreaHeight = conH - 100;
            int totalLines = (int)consoleLog.size();
            int visibleLines = (int)(contentAreaHeight / 25.0f);
            int maxScroll = std::max(0, totalLines - visibleLines);
            if (consoleScrollIndex < 0) consoleScrollIndex = 0;
            if (consoleScrollIndex > maxScroll) consoleScrollIndex = maxScroll;
            float wheel = GetMouseWheelMove();
            if (wheel != 0) { consoleScrollIndex -= (int)wheel; if (consoleScrollIndex < 0) consoleScrollIndex = 0; if (consoleScrollIndex > maxScroll) consoleScrollIndex = maxScroll; }

            int startLine = consoleScrollIndex;
            int endLine = std::min((int)consoleLog.size(), startLine + visibleLines);
            float logY = conRect.y + 50;
            for (int i = startLine; i < endLine; i++) {
                Color c = GREEN;
                if (consoleLog[i].find("[ERROR]") != std::string::npos) c = RED;
                else if (consoleLog[i].find("HIT:") != std::string::npos) c = ORANGE;
                else if (consoleLog[i].find("[SUCCESS]") != std::string::npos) c = YELLOW;
                else if (consoleLog[i].find("[USAGE]") != std::string::npos) c = SKYBLUE;
                
                DrawTextEx(mainFont, consoleLog[i].c_str(), {conRect.x + 20, logY}, 18, 1.0f, c);
                logY += 25;
            }
            
            float inputY = conRect.y + conH - 50;
            DrawRectangle(conRect.x + 10, inputY, conRect.width - 20, 40, Fade(GREEN, 0.1f));
            DrawTextEx(mainFont, ">", {conRect.x + 20, inputY + 10}, 20, 1.0f, GREEN);
            BeginScissorMode((int)conRect.x + 45, (int)inputY, (int)conRect.width - 60, 40);
                DrawTextEx(mainFont, (consoleInput + (((int)(cursorBlinkTimer*2)%2==0)?"_":"")).c_str(), {conRect.x + 45, inputY + 10}, 20, 1.0f, WHITE);
            EndScissorMode();
            DrawTextEx(mainFont, "ESC: Close | ENTER: Execute | Ctrl+V: Paste", {conRect.x + 20, conRect.y + conH + 10}, 16, 1.0f, WHITE);
        }

        // HELP OVERLAY
        if (showHelp) {
            DrawRectangle(0, 0, curW, curH, Fade(BLACK, 0.7f));
            Rectangle helpRect = { (float)curW/2 - 250, (float)curH/2 - 200, 500, 400 };
            DrawRectangleRec(helpRect, COL_BG);
            DrawRectangleLinesEx(helpRect, 2, WHITE);
            DrawTextEx(mainFont, "HELP & SHORTCUTS", {helpRect.x + 20, helpRect.y + 20}, 24, 1.0f, YELLOW);
            int ly = helpRect.y + 70; int lh = 30;
            DrawTextEx(mainFont, "- Left Click: Drag / Select (Shift to Add)", {helpRect.x + 30, (float)ly}, 20, 1.0f, WHITE); ly += lh;
            DrawTextEx(mainFont, "- Left Drag (Empty): Box Select", {helpRect.x + 30, (float)ly}, 20, 1.0f, WHITE); ly += lh;
            DrawTextEx(mainFont, "- Right Click: Connect Nodes", {helpRect.x + 30, (float)ly}, 20, 1.0f, WHITE); ly += lh;
            DrawTextEx(mainFont, "- Middle Click: Pan View", {helpRect.x + 30, (float)ly}, 20, 1.0f, WHITE); ly += lh;
            DrawTextEx(mainFont, "- Wheel: Zoom In / Out", {helpRect.x + 30, (float)ly}, 20, 1.0f, WHITE); ly += lh;
            DrawTextEx(mainFont, "- ENTER: Edit Selected Node", {helpRect.x + 30, (float)ly}, 20, 1.0f, WHITE); ly += lh;
            DrawTextEx(mainFont, "- DEL: Delete Selected", {helpRect.x + 30, (float)ly}, 20, 1.0f, WHITE); ly += lh;
            DrawTextEx(mainFont, "- Ctrl+C/X/V: Copy / Cut / Paste", {helpRect.x + 30, (float)ly}, 20, 1.0f, WHITE); ly += lh;
            DrawTextEx(mainFont, "- SAVE / LOAD: Use top buttons", {helpRect.x + 30, (float)ly}, 20, 1.0f, WHITE); ly += lh;
            
            DrawTextEx(mainFont, "Press ESC to Close", {helpRect.x + 150, helpRect.y + 360}, 18, 1.0f, GRAY);
        }

        // FULL REGEX VIEW MODAL (WITH CODE EXPORT AND SCROLLBAR)
        if (showFullRegex) {
            DrawRectangle(0, 0, curW, curH, Fade(BLACK, 0.8f));
            Rectangle fullRect = { (float)curW/2 - 350, (float)curH/2 - 250, 700, 500 };
            DrawRectangleRec(fullRect, COL_BG);
            DrawRectangleLinesEx(fullRect, 2, YELLOW);
            
            DrawTextEx(mainFont, "CODE EXPORT", {fullRect.x + 20, fullRect.y + 20}, 24, 1.0f, YELLOW);
            
            // Language Selection
            float langX = fullRect.x + 20;
            float langY = fullRect.y + 60;
            if (GuiButton({langX, langY, 80, 30}, "RAW", currentExportLang == LANG_RAW ? YELLOW : BLACK)) currentExportLang = LANG_RAW; langX += 90;
            if (GuiButton({langX, langY, 80, 30}, "C++", currentExportLang == LANG_CPP ? YELLOW : BLACK)) currentExportLang = LANG_CPP; langX += 90;
            if (GuiButton({langX, langY, 80, 30}, "PYTHON", currentExportLang == LANG_PYTHON ? YELLOW : BLACK)) currentExportLang = LANG_PYTHON; langX += 90;
            if (GuiButton({langX, langY, 80, 30}, "JS", currentExportLang == LANG_JS ? YELLOW : BLACK)) currentExportLang = LANG_JS; langX += 90;
            if (GuiButton({langX, langY, 80, 30}, "C#", currentExportLang == LANG_CSHARP ? YELLOW : BLACK)) currentExportLang = LANG_CSHARP; langX += 90;
            if (GuiButton({langX, langY, 80, 30}, "JAVA", currentExportLang == LANG_JAVA ? YELLOW : BLACK)) currentExportLang = LANG_JAVA;

            // Generate Code
            std::string codeStr = GetExportCode(regStr, currentExportLang);

            Rectangle textRect = { fullRect.x + 20, fullRect.y + 110, fullRect.width - 40, 300 };
            Rectangle viewRect = { textRect.x, textRect.y, textRect.width - 20, textRect.height }; // Slightly smaller to fit scrollbar
            
            DrawRectangleRec(textRect, Fade(BLACK, 0.5f));
            DrawRectangleLinesEx(textRect, 1, DARKGRAY);
            
            // Calculate Total Height
            float fontSize = 24.0f;
            float totalHeight = CalculateWrappedHeight(codeStr, fontSize, viewRect.width);
            float maxScroll = std::max(0.0f, totalHeight - viewRect.height);

            // Handle Scroll Interaction
            bool isMouseOver = CheckCollisionPointRec(GetMousePosition(), textRect);
            if (isMouseOver) {
                float wheel = GetMouseWheelMove();
                if (wheel != 0) {
                    fullRegexScroll -= wheel * 30.0f;
                    if (fullRegexScroll < 0) fullRegexScroll = 0;
                    if (fullRegexScroll > maxScroll) fullRegexScroll = maxScroll;
                }
            }

            // Draw Scrollbar
            Rectangle scrollTrack = { viewRect.x + viewRect.width + 5, viewRect.y, 15, viewRect.height };
            DrawRectangleRec(scrollTrack, Fade(YELLOW, 0.1f));
            
            if (maxScroll > 0) {
                float viewRatio = viewRect.height / totalHeight;
                float thumbH = std::max(30.0f, scrollTrack.height * viewRatio);
                float scrollRatio = fullRegexScroll / maxScroll;
                float thumbY = scrollTrack.y + (scrollTrack.height - thumbH) * scrollRatio;
                Rectangle scrollThumb = { scrollTrack.x, thumbY, scrollTrack.width, thumbH };
                
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), scrollTrack)) isDraggingFullRegexScroll = true;
                if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) isDraggingFullRegexScroll = false;
                
                if (isDraggingFullRegexScroll) {
                    float relativeY = GetMousePosition().y - scrollTrack.y - thumbH/2;
                    float ratio = relativeY / (scrollTrack.height - thumbH);
                    fullRegexScroll = ratio * maxScroll;
                    if (fullRegexScroll < 0) fullRegexScroll = 0;
                    if (fullRegexScroll > maxScroll) fullRegexScroll = maxScroll;
                }
                
                DrawRectangleRec(scrollThumb, isDraggingFullRegexScroll ? YELLOW : Fade(YELLOW, 0.5f));
            } else {
                fullRegexScroll = 0;
            }

            // Draw Text with Scroll Offset
            BeginScissorMode((int)viewRect.x, (int)viewRect.y, (int)viewRect.width, (int)viewRect.height);
                DrawTextWrapped(mainFont, codeStr, viewRect, fontSize, WHITE, fullRegexScroll);
            EndScissorMode();
            
            if (GuiButton({fullRect.x + 20, fullRect.y + 430, 270, 50}, "COPY CODE")) {
                SetClipboardText(codeStr.c_str());
            }
            if (GuiButton({fullRect.x + 310, fullRect.y + 430, 270, 50}, "CLOSE")) {
                showFullRegex = false;
            }
        }
        
        // TEMPLATES MODAL (NEW)
        if (showTemplates) {
            DrawRectangle(0, 0, curW, curH, Fade(BLACK, 0.8f));
            Rectangle tplRect = { (float)curW/2 - 200, (float)curH/2 - 200, 400, 400 };
            DrawRectangleRec(tplRect, COL_BG);
            DrawRectangleLinesEx(tplRect, 2, ORANGE);
            
            DrawTextEx(mainFont, "SELECT TEMPLATE", {tplRect.x + 20, tplRect.y + 20}, 24, 1.0f, ORANGE);
            
            Vector2 camCenter = { -(camera.target.x - curW/2.0f)/camera.zoom, -(camera.target.y - curH/2.0f)/camera.zoom };
            
            float btnY = tplRect.y + 70;
            if (GuiButton({tplRect.x + 50, btnY, 300, 40}, "EMAIL ADDRESS")) AddTemplate(TPL_EMAIL, camCenter.x, camCenter.y); btnY += 50;
            if (GuiButton({tplRect.x + 50, btnY, 300, 40}, "DATE (YYYY-MM-DD)")) AddTemplate(TPL_DATE_ISO, camCenter.x, camCenter.y); btnY += 50;
            if (GuiButton({tplRect.x + 50, btnY, 300, 40}, "PHONE (US)")) AddTemplate(TPL_PHONE_US, camCenter.x, camCenter.y); btnY += 50;
            if (GuiButton({tplRect.x + 50, btnY, 300, 40}, "URL (SIMPLE)")) AddTemplate(TPL_URL_SIMPLE, camCenter.x, camCenter.y); btnY += 50;
            if (GuiButton({tplRect.x + 50, btnY, 300, 40}, "IPv4 ADDRESS")) AddTemplate(TPL_IP_V4, camCenter.x, camCenter.y); btnY += 50;
            
            if (GuiButton({tplRect.x + 50, tplRect.y + 330, 300, 40}, "CANCEL", RED)) showTemplates = false;
        }

        EndDrawing();
    }
    CloseWindow();
    return 0;
}

bool GuiButton(Rectangle bounds, const char* text, Color borderColor) {
    Vector2 m = GetMousePosition();
    bool pressed = false;
    Color c = {60, 60, 60, 255};
    if (CheckCollisionPointRec(m, bounds)) {
        c = {80, 80, 80, 255};
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) c = {40, 40, 40, 255};
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) pressed = true;
    }
    DrawRectangleRec(bounds, c);
    DrawRectangleLinesEx(bounds, 2, borderColor);
    Vector2 size = MeasureTextEx(mainFont, text, 18, 1.0f);
    DrawTextEx(mainFont, text, {bounds.x + bounds.width/2 - size.x/2, bounds.y + bounds.height/2 - size.y/2}, 18, 1.0f, WHITE);
    return pressed;
}
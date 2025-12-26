/**
 * VISUAL REGEX BUILDER (Pro Edition - Live Playground)
 * Language: C++
 * Library: Raylib
 * Description: Node editor with Console, Live Regex Testing Playground, and continuous input handling.
 */

#include "raylib.h"
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <regex> 
#include <fstream>
#include <filesystem>

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
    bool isDragging;
    bool isEditing;
};

struct Connection {
    int fromNodeId;
    int toNodeId;
};

// ----------------------------------------------------------------------------------
// Global Variables
// ----------------------------------------------------------------------------------
std::vector<Node> nodes;
std::vector<Connection> connections;
int nextNodeId = 0;
int selectedNodeId = -1;
Vector2 mouseOffset = { 0, 0 };
bool isCreatingConnection = false;
int connectionStartNodeId = -1;
Camera2D camera = { 0 };
Font mainFont = { 0 };

// SYSTEMS STATE
bool showConsole = false;
bool showPlayground = false;
bool showHelp = false; // POINT 2: Help Toggle

// CONSOLE DATA
std::string consoleInput = "";
std::vector<std::string> consoleLog;
int consoleScrollIndex = 0;
bool isDraggingScrollbar = false;

// PLAYGROUND DATA
std::string playgroundText = "Hello World! Type here to test.\nUser123, Admin99.\nMulti-line support added.";
Rectangle playgroundRect = { 0, 0, 0, 0 };
float playgroundScrollOffset = 0.0f;
bool isDraggingPlaygroundScroll = false;

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

// Node Colors
const Color COL_CAT_ANCHOR = { 255, 100, 100, 255 }; 
const Color COL_CAT_CHAR = { 0, 228, 48, 255 };      
const Color COL_CAT_DIGIT = { 0, 121, 241, 255 };    
const Color COL_CAT_SPECIAL = { 255, 161, 0, 255 };  
const Color COL_CAT_NEGATED = { 100, 100, 100, 255 };
const Color COL_CAT_QUANT = { 255, 255, 0, 255 };    
const Color COL_CAT_STRUCT = { 180, 80, 255, 255 };  
const Color COL_CAT_CUSTOM = { 255, 0, 255, 255 };   

// ----------------------------------------------------------------------------------
// Core Logic
// ----------------------------------------------------------------------------------

bool GuiButton(Rectangle bounds, const char* text);

void AddNode(NodeType type, float x, float y) {
    Node n;
    n.id = nextNodeId++;
    n.rect = { x, y, 160, 60 };
    n.type = type;
    n.isDragging = false;
    n.isEditing = false;

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

// CONSOLE LOGIC
void AddLog(std::string msg) {
    consoleLog.push_back(msg);
    if (consoleLog.size() > 1000) consoleLog.erase(consoleLog.begin());
    consoleScrollIndex = consoleLog.size(); 
}

void ProcessConsoleCommand() {
    if (consoleInput.empty()) return;
    AddLog("> " + consoleInput);
    
    std::string regStr = GenerateRegex();
    if (regStr.empty()) { AddLog("[ERROR] Empty Regex."); consoleInput = ""; return; }

    std::filesystem::path path(consoleInput);
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) { AddLog("[ERROR] Path not found."); consoleInput = ""; return; }

    AddLog("Scanning...");
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
    } catch (const std::exception& e) { AddLog("[ERROR] " + std::string(e.what())); }
    consoleInput = "";
}

// HELPER: Get X,Y coords of text index for simple highlighting
Vector2 GetTextPos(const std::string& text, size_t index, float fontSize) {
    int line = 0;
    size_t lastNewLine = 0;
    for (size_t i = 0; i < index && i < text.length(); i++) {
        if (text[i] == '\n') {
            line++;
            lastNewLine = i + 1;
        }
    }
    std::string currentLinePrefix = text.substr(lastNewLine, index - lastNewLine);
    float x = MeasureTextEx(mainFont, currentLinePrefix.c_str(), fontSize, 1.0f).x;
    float y = line * fontSize; 
    return {x, y};
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
    AddLog("Visual Regex Console v1.0 Ready.");

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (copyFeedbackTimer > 0) copyFeedbackTimer -= dt;
        cursorBlinkTimer += dt;

        Vector2 mouseScreen = GetMousePosition();
        Vector2 mouseWorld = GetScreenToWorld2D(mouseScreen, camera);
        
        // INPUT ROUTING
        int key = GetCharPressed(); 
        bool inputConsumed = false;

        // --- BACKSPACE LOGIC HELPER ---
        auto HandleBackspace = [&](std::string& target) {
            if (IsKeyPressed(KEY_BACKSPACE)) {
                if (!target.empty()) target.pop_back();
                keyRepeatTimer = KEY_REPEAT_DELAY; // Initial delay
            } else if (IsKeyDown(KEY_BACKSPACE)) {
                keyRepeatTimer -= dt;
                if (keyRepeatTimer <= 0) {
                    if (!target.empty()) target.pop_back();
                    keyRepeatTimer = KEY_REPEAT_RATE; // Fast repeat
                }
            } else {
                keyRepeatTimer = 0; // Reset
            }
        };

        // 1. HELP OVERLAY (Top Priority)
        if (showHelp) {
            if (IsKeyPressed(KEY_ESCAPE) || (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && mouseScreen.y > 80)) {
                showHelp = false;
            }
            inputConsumed = true;
        }

        // 2. CONSOLE OVERLAY (Exclusive Input)
        else if (showConsole) {
            if (IsKeyPressed(KEY_ESCAPE)) showConsole = false;
            if (IsKeyPressed(KEY_ENTER)) ProcessConsoleCommand();
            if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_V)) {
                const char* clip = GetClipboardText();
                if (clip) consoleInput += std::string(clip);
            }
            while (key > 0) {
                if ((key >= 32) && (key <= 125)) consoleInput += (char)key;
                key = GetCharPressed();
            }
            
            HandleBackspace(consoleInput); // POINT 1: Continuous Backspace
            inputConsumed = true; 
        }
        // TOGGLE TERMINAL
        else if (IsKeyPressed(KEY_T) && editingNodeId == -1 && !showConsole && !showPlayground) {
            showConsole = true;
            inputConsumed = true;
        }

        // 3. PLAYGROUND PANEL (Contextual Input)
        bool mouseOverPlayground = showPlayground && CheckCollisionPointRec(mouseScreen, playgroundRect);
        
        if (!inputConsumed && showPlayground && mouseOverPlayground) {
            if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_V)) {
                const char* clip = GetClipboardText();
                if (clip) playgroundText += std::string(clip);
            }
            while (key > 0) {
                if ((key >= 32) && (key <= 125)) playgroundText += (char)key;
                key = GetCharPressed();
            }
            
            HandleBackspace(playgroundText); // POINT 1: Continuous Backspace
            
            if (IsKeyPressed(KEY_ENTER)) playgroundText += '\n';
            
            inputConsumed = true; 
        }

        // 4. NODE EDITING (Contextual Input)
        if (!inputConsumed && editingNodeId != -1) {
            if (IsKeyPressed(KEY_ENTER) || (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !CheckCollisionPointRec(mouseWorld, nodes[0].rect))) { 
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
                
                // Manual Backspace handling for Node, since helper uses reference to string, 
                // we need to access the specific node string.
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

        // 5. CANVAS NAVIGATION (Only if no overlay active)
        if (!inputConsumed && !mouseOverPlayground) {
            float wheel = GetMouseWheelMove();
            if (wheel != 0) {
                Vector2 mouseWorldBeforeZoom = GetScreenToWorld2D(mouseScreen, camera);
                camera.zoom += (wheel * 0.125f);
                if (camera.zoom < 0.2f) camera.zoom = 0.2f;
                if (camera.zoom > 3.0f) camera.zoom = 3.0f;
                Vector2 mouseWorldAfterZoom = GetScreenToWorld2D(mouseScreen, camera);
                camera.target.x += (mouseWorldBeforeZoom.x - mouseWorldAfterZoom.x);
                camera.target.y += (mouseWorldBeforeZoom.y - mouseWorldAfterZoom.y);
            }
            if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) || (IsKeyDown(KEY_SPACE) && IsMouseButtonDown(MOUSE_LEFT_BUTTON))) {
                Vector2 delta = GetMouseDelta();
                camera.target.x -= delta.x / camera.zoom;
                camera.target.y -= delta.y / camera.zoom;
            }

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && mouseScreen.y > 80 && mouseScreen.y < GetScreenHeight() - 180) { 
                bool clicked = false;
                for (int i = nodes.size() - 1; i >= 0; i--) {
                    if (CheckCollisionPointRec(mouseWorld, nodes[i].rect)) {
                        selectedNodeId = nodes[i].id;
                        nodes[i].isDragging = true;
                        mouseOffset = { mouseWorld.x - nodes[i].rect.x, mouseWorld.y - nodes[i].rect.y };
                        clicked = true; break;
                    }
                }
                if (!clicked) selectedNodeId = -1;
            }

            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) { for (auto& n : nodes) n.isDragging = false; }
            if (selectedNodeId != -1) {
                for (auto& n : nodes) {
                    if (n.id == selectedNodeId && n.isDragging) {
                        n.rect.x = mouseWorld.x - mouseOffset.x;
                        n.rect.y = mouseWorld.y - mouseOffset.y;
                    }
                }
                if (IsKeyPressed(KEY_ENTER)) {
                    editingNodeId = selectedNodeId;
                    for(auto& n : nodes) if(n.id == editingNodeId) n.isEditing = true;
                }
                if (IsKeyPressed(KEY_DELETE)) {
                    for (int i = connections.size() - 1; i >= 0; i--) {
                        if (connections[i].fromNodeId == selectedNodeId || connections[i].toNodeId == selectedNodeId) connections.erase(connections.begin() + i);
                    }
                    for (int i = nodes.size() - 1; i >= 0; i--) {
                        if (nodes[i].id == selectedNodeId) { nodes.erase(nodes.begin() + i); selectedNodeId = -1; break; }
                    }
                }
            }

            if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
                for (const auto& n : nodes) if (CheckCollisionPointRec(mouseWorld, n.rect)) { isCreatingConnection = true; connectionStartNodeId = n.id; break; }
            }
            if (IsMouseButtonReleased(MOUSE_RIGHT_BUTTON) && isCreatingConnection) {
                isCreatingConnection = false;
                for (const auto& n : nodes) if (CheckCollisionPointRec(mouseWorld, n.rect) && n.id != connectionStartNodeId) {
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
                DrawRectangleRoundedLines(n.rect, 0.2f, 8, (n.id == selectedNodeId) ? WHITE : BLACK);
                std::string displayStr = n.title;
                if (n.isEditing && n.type != NODE_CUSTOM) displayStr = n.regexValue;
                DrawTextEx(mainFont, displayStr.c_str(), {n.rect.x + 10, n.rect.y + 20}, 18, 1.0f, BLACK);
            }
        EndMode2D();

        // UI HEADERS & PANELS
        int curW = GetScreenWidth(); int curH = GetScreenHeight();
        
        // Header
        DrawRectangle(0, 0, curW, 80, Fade(BLACK, 0.9f));
        std::string regStr = GenerateRegex();
        DrawTextEx(mainFont, "REGEX:", {20, 30}, 20, 1.0f, LIGHTGRAY);
        DrawTextEx(mainFont, regStr.c_str(), {100, 25}, 30, 1.0f, YELLOW);
        
        // POINT 2: Block Playground opening if Console is active
        if (!showConsole) {
            if (GuiButton({(float)curW - 320, 20, 140, 40}, showPlayground ? "HIDE TEST" : "PLAYGROUND")) {
                showPlayground = !showPlayground;
            }
        } else {
            // Disabled look for button
            DrawRectangleRec({(float)curW - 320, 20, 140, 40}, Fade(GRAY, 0.5f));
            DrawTextEx(mainFont, "PLAYGROUND", {(float)curW - 320 + 20, 30}, 18, 1.0f, DARKGRAY);
        }

        if (GuiButton({(float)curW - 170, 20, 100, 40}, copyFeedbackTimer > 0 ? "COPIED!" : "COPY")) {
            SetClipboardText(regStr.c_str()); copyFeedbackTimer = 2.0f;
        }

        // POINT 2: HELP BUTTON
        if (GuiButton({(float)curW - 60, 20, 40, 40}, "?")) {
            showHelp = !showHelp;
        }

        // POINT 1 & 3: PLAYGROUND PANEL
        if (showPlayground) {
            float pgWidth = 400;
            float headerH = 80;
            float footerH = 180;
            playgroundRect = { (float)curW - pgWidth, headerH, pgWidth, (float)curH - headerH - footerH };
            
            DrawRectangleRec(playgroundRect, Fade(COL_BG, 0.95f));
            DrawRectangleLinesEx(playgroundRect, 2, BLUE);
            
            // Header
            DrawRectangle(playgroundRect.x, playgroundRect.y, playgroundRect.width, 40, Fade(BLUE, 0.2f));
            DrawTextEx(mainFont, "REAL-TIME PLAYGROUND", {playgroundRect.x + 20, playgroundRect.y + 10}, 18, 1.0f, BLUE);
            DrawTextEx(mainFont, (mouseOverPlayground ? "[TYPE HERE]" : ""), {playgroundRect.x + 250, playgroundRect.y + 10}, 14, 1.0f, LIGHTGRAY);

            // Scrollbar Logic
            float fontSize = 20.0f;
            Rectangle textArea = { playgroundRect.x + 10, playgroundRect.y + 50, playgroundRect.width - 35, playgroundRect.height - 60 };
            
            int lines = 1;
            for(char c : playgroundText) if(c == '\n') lines++;
            float totalHeight = lines * fontSize;
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
            } else {
                playgroundScrollOffset = 0;
            }

            BeginScissorMode((int)textArea.x, (int)textArea.y, (int)textArea.width, (int)textArea.height);
                if (!regStr.empty()) {
                    try {
                        std::regex pattern(regStr);
                        auto words_begin = std::sregex_iterator(playgroundText.begin(), playgroundText.end(), pattern);
                        auto words_end = std::sregex_iterator();
                        for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
                            std::smatch match = *i;
                            Vector2 pos = GetTextPos(playgroundText, match.position(), fontSize);
                            pos.x += textArea.x;
                            pos.y += textArea.y - playgroundScrollOffset;
                            float matchW = MeasureTextEx(mainFont, match.str().c_str(), fontSize, 1.0f).x;
                            DrawRectangle(pos.x, pos.y, matchW, fontSize, Fade(GREEN, 0.4f));
                        }
                    } catch (...) {}
                }
                DrawTextEx(mainFont, playgroundText.c_str(), {textArea.x, textArea.y - playgroundScrollOffset}, fontSize, 1.0f, WHITE);
                if (mouseOverPlayground && ((int)(cursorBlinkTimer*2)%2==0)) {
                    Vector2 cursorPos = GetTextPos(playgroundText, playgroundText.length(), fontSize);
                    DrawRectangle(textArea.x + cursorPos.x + 2, textArea.y + cursorPos.y - playgroundScrollOffset, 2, fontSize, WHITE);
                }
            EndScissorMode();
        } else {
            playgroundRect = {0,0,0,0}; 
        }

        // Bottom UI Panel
        DrawRectangle(0, curH - 180, curW, 180, Fade(BLACK, 0.9f));
        DrawTextEx(mainFont, "Pan: Mid-Click | Zoom: Wheel | R-Click: Connect | Del: Delete | Enter: Edit | T: Terminal", {20, (float)curH - 170}, 16, 1.0f, GRAY);

        Vector2 c = GetScreenToWorld2D({ (float)curW/2, (float)curH/2 }, camera);
        int startX = 20; int startY = curH - 140; int btnW = 140; int btnH = 35; int gapX = 150; int gapY = 45;

        int x = startX; int y = startY;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "CUSTOM TEXT")) AddNode(NODE_CUSTOM, c.x, c.y); x+=gapX;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Letters")) AddNode(NODE_TEXT, c.x, c.y); x+=gapX;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Numbers")) AddNode(NODE_DIGIT, c.x, c.y); x+=gapX;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Word Chars")) AddNode(NODE_WORD, c.x, c.y); x+=gapX;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Whitespace")) AddNode(NODE_WHITESPACE, c.x, c.y); x+=gapX;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Any Char")) AddNode(NODE_ANY, c.x, c.y); x+=gapX;
        
        x = startX; y += gapY;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Symbol @")) AddNode(NODE_SYMBOL, c.x, c.y); x+=gapX;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Non-Number")) AddNode(NODE_NOT_DIGIT, c.x, c.y); x+=gapX;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Non-Word Char")) AddNode(NODE_NOT_WORD, c.x, c.y); x+=gapX;
        x += 20; 
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Repeat (0+)")) AddNode(NODE_ZERO_OR_MORE, c.x, c.y); x+=gapX;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Repeat (1+)")) AddNode(NODE_ONE_OR_MORE, c.x, c.y); x+=gapX;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Make Optional")) AddNode(NODE_OPTIONAL, c.x, c.y); x+=gapX;

        x = startX; y += gapY;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Start Line")) AddNode(NODE_START, c.x, c.y); x+=gapX;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "End Line")) AddNode(NODE_END, c.x, c.y); x+=gapX;
        x += 20; 
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Begin Group")) AddNode(NODE_GROUP_START, c.x, c.y); x+=gapX;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Finish Group")) AddNode(NODE_GROUP_END, c.x, c.y); x+=gapX;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "OR (Either)")) AddNode(NODE_OR, c.x, c.y); x+=gapX;

        // CONSOLE (Draw Last to be on top of everything if open)
        if (showConsole) {
            float conH = 400;
            DrawRectangle(0, 0, curW, curH, Fade(BLACK, 0.6f));
            Rectangle conRect = { (float)curW/2 - 300, 100, 600, conH };
            DrawRectangleRec(conRect, Fade(BLACK, 0.95f));
            DrawRectangleLinesEx(conRect, 2, GREEN);
            DrawRectangle(conRect.x, conRect.y, conRect.width, 40, Fade(GREEN, 0.2f));
            DrawTextEx(mainFont, "TERMINAL - DIRECTORY SCANNER", {conRect.x + 20, conRect.y + 10}, 20, 1.0f, GREEN);
            if (GuiButton({conRect.x + conRect.width - 40, conRect.y, 40, 40}, "X")) showConsole = false;

            // Simple Scrollbar (Reusing logic)
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
                DrawTextEx(mainFont, consoleLog[i].c_str(), {conRect.x + 20, logY}, 18, 1.0f, c);
                logY += 25;
            }
            
            float inputY = conRect.y + conH - 50;
            DrawRectangle(conRect.x + 10, inputY, conRect.width - 20, 40, Fade(GREEN, 0.1f));
            DrawTextEx(mainFont, ">", {conRect.x + 20, inputY + 10}, 20, 1.0f, GREEN);
            BeginScissorMode((int)conRect.x + 45, (int)inputY, (int)conRect.width - 60, 40);
                DrawTextEx(mainFont, (consoleInput + (((int)(cursorBlinkTimer*2)%2==0)?"_":"")).c_str(), {conRect.x + 45, inputY + 10}, 20, 1.0f, WHITE);
            EndScissorMode();
            DrawTextEx(mainFont, "ESC: Close | ENTER: Scan | Ctrl+V: Paste", {conRect.x + 20, conRect.y + conH + 10}, 16, 1.0f, WHITE);
        }

        // POINT 2: HELP OVERLAY
        if (showHelp) {
            DrawRectangle(0, 0, curW, curH, Fade(BLACK, 0.7f));
            Rectangle helpRect = { (float)curW/2 - 250, (float)curH/2 - 200, 500, 400 };
            DrawRectangleRec(helpRect, COL_BG);
            DrawRectangleLinesEx(helpRect, 2, WHITE);
            DrawTextEx(mainFont, "HELP & SHORTCUTS", {helpRect.x + 20, helpRect.y + 20}, 24, 1.0f, YELLOW);
            
            int ly = helpRect.y + 70;
            int lh = 30;
            DrawTextEx(mainFont, "- Left Click: Drag Nodes / Select", {helpRect.x + 30, (float)ly}, 20, 1.0f, WHITE); ly += lh;
            DrawTextEx(mainFont, "- Right Click: Connect Nodes", {helpRect.x + 30, (float)ly}, 20, 1.0f, WHITE); ly += lh;
            DrawTextEx(mainFont, "- Middle Click: Pan View", {helpRect.x + 30, (float)ly}, 20, 1.0f, WHITE); ly += lh;
            DrawTextEx(mainFont, "- Wheel: Zoom In / Out", {helpRect.x + 30, (float)ly}, 20, 1.0f, WHITE); ly += lh;
            DrawTextEx(mainFont, "- ENTER: Edit Selected Node", {helpRect.x + 30, (float)ly}, 20, 1.0f, WHITE); ly += lh;
            DrawTextEx(mainFont, "- DEL: Delete Selected Node", {helpRect.x + 30, (float)ly}, 20, 1.0f, WHITE); ly += lh;
            DrawTextEx(mainFont, "- T: Toggle File Scanner Console", {helpRect.x + 30, (float)ly}, 20, 1.0f, WHITE); ly += lh;
            DrawTextEx(mainFont, "- Ctrl+V: Paste text", {helpRect.x + 30, (float)ly}, 20, 1.0f, WHITE); ly += lh;
            
            DrawTextEx(mainFont, "Press ESC to Close", {helpRect.x + 150, helpRect.y + 360}, 18, 1.0f, GRAY);
        }

        EndDrawing();
    }
    CloseWindow();
    return 0;
}

bool GuiButton(Rectangle bounds, const char* text) {
    Vector2 m = GetMousePosition();
    bool pressed = false;
    Color c = {60, 60, 60, 255};
    if (CheckCollisionPointRec(m, bounds)) {
        c = {80, 80, 80, 255};
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) c = {40, 40, 40, 255};
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) pressed = true;
    }
    DrawRectangleRec(bounds, c);
    DrawRectangleLinesEx(bounds, 1, BLACK);
    Vector2 size = MeasureTextEx(mainFont, text, 18, 1.0f);
    DrawTextEx(mainFont, text, {bounds.x + bounds.width/2 - size.x/2, bounds.y + bounds.height/2 - size.y/2}, 18, 1.0f, WHITE);
    return pressed;
}
/**
 * VISUAL REGEX BUILDER (MVP Prototype)
 * Language: C++
 * Library: Raylib
 * Description: A simple node editor to visually build Regular Expressions.
 */

#include "raylib.h"
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <cmath>

// ----------------------------------------------------------------------------------
// Data Structures
// ----------------------------------------------------------------------------------

enum NodeType {
    // Anchors
    NODE_START,         // ^
    NODE_END,           // $

    // Character Classes
    NODE_TEXT,          // [a-z]
    NODE_DIGIT,         // \d
    NODE_WHITESPACE,    // \s
    NODE_ANY,           // .
    NODE_WORD,          // \w
    NODE_SYMBOL,        // @ (Placeholder)

    // Negated Classes
    NODE_NOT_DIGIT,     // \D
    NODE_NOT_WHITESPACE,// \S
    NODE_NOT_WORD,      // \W

    // Quantifiers
    NODE_ZERO_OR_MORE,  // *
    NODE_ONE_OR_MORE,   // +
    NODE_OPTIONAL,      // ?

    // Structure
    NODE_GROUP_START,   // (
    NODE_GROUP_END,     // )
    NODE_OR             // |
};

struct Node {
    int id;
    Rectangle rect;
    NodeType type;
    std::string title;
    std::string regexValue; // The regex code this node contributes
    Color color;
    bool isDragging;
    
    // Connection points (visual)
    Vector2 inputPos;
    Vector2 outputPos;
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
int selectedNodeId = -1; // -1 means none
Vector2 mouseOffset = { 0, 0 };

// Variables for creating connections
bool isCreatingConnection = false;
int connectionStartNodeId = -1;

// Camera System
Camera2D camera = { 0 };

// Font System
Font mainFont = { 0 }; // Global font to use instead of default

// Cyberpunk Style Colors
const Color COL_BG = { 20, 24, 35, 255 };      // Very dark blue
const Color COL_GRID = { 40, 45, 60, 255 };    // Blueish gray
const Color COL_WIRE = { 200, 200, 200, 150 };       // Gray wire
const Color COL_WIRE_ACTIVE = { 255, 255, 0, 255 };  // Yellow wire (creating)

// Node Colors by Category
const Color COL_CAT_ANCHOR = { 255, 100, 100, 255 }; // Soft red
const Color COL_CAT_CHAR = { 0, 228, 48, 255 };      // Matrix green
const Color COL_CAT_DIGIT = { 0, 121, 241, 255 };    // Electric blue
const Color COL_CAT_SPECIAL = { 255, 161, 0, 255 };  // Orange
const Color COL_CAT_NEGATED = { 100, 100, 100, 255 };// Dark Gray
const Color COL_CAT_QUANT = { 255, 255, 0, 255 };    // Yellow
const Color COL_CAT_STRUCT = { 180, 80, 255, 255 };  // Purple

// ----------------------------------------------------------------------------------
// Prototypes and Helper Functions
// ----------------------------------------------------------------------------------

// Prototype needed for main() to recognize the function
bool GuiButton(Rectangle bounds, const char* text);

void AddNode(NodeType type, float x, float y) {
    Node n;
    n.id = nextNodeId++;
    n.rect = { x, y, 140, 60 };
    n.type = type;
    n.isDragging = false;

    switch (type) {
        // --- ANCHORS ---
        case NODE_START:
            n.title = "Start of Line";
            n.regexValue = "^";
            n.color = COL_CAT_ANCHOR;
            break;
        case NODE_END:
            n.title = "End of Line";
            n.regexValue = "$";
            n.color = COL_CAT_ANCHOR;
            break;

        // --- CLASSES ---
        case NODE_TEXT:
            n.title = "Letters (a-z)";
            n.regexValue = "[a-zA-Z]+";
            n.color = COL_CAT_CHAR;
            break;
        case NODE_DIGIT:
            n.title = "Number (0-9)";
            n.regexValue = "\\d";
            n.color = COL_CAT_DIGIT;
            break;
        case NODE_WHITESPACE:
            n.title = "Whitespace";
            n.regexValue = "\\s";
            n.color = COL_CAT_SPECIAL;
            break;
        case NODE_ANY:
            n.title = "Anything";
            n.regexValue = ".";
            n.color = COL_CAT_SPECIAL;
            break;
        case NODE_WORD:
            n.title = "Word Char";
            n.regexValue = "\\w";
            n.color = COL_CAT_CHAR;
            break;
        case NODE_SYMBOL:
            n.title = "Specific Symbol";
            n.regexValue = "@";
            n.color = PURPLE;
            break;

        // --- NEGATED ---
        case NODE_NOT_DIGIT:
            n.title = "Not a Number";
            n.regexValue = "\\D";
            n.color = COL_CAT_NEGATED;
            break;
        case NODE_NOT_WHITESPACE:
            n.title = "Not Whitespace";
            n.regexValue = "\\S";
            n.color = COL_CAT_NEGATED;
            break;
        case NODE_NOT_WORD:
            n.title = "Not Word Char";
            n.regexValue = "\\W";
            n.color = COL_CAT_NEGATED;
            break;

        // --- QUANTIFIERS ---
        case NODE_ZERO_OR_MORE:
            n.title = "Repeat (0+)";
            n.regexValue = "*";
            n.color = COL_CAT_QUANT;
            break;
        case NODE_ONE_OR_MORE:
            n.title = "Repeat (1+)";
            n.regexValue = "+";
            n.color = COL_CAT_QUANT;
            break;
        case NODE_OPTIONAL:
            n.title = "Optional";
            n.regexValue = "?";
            n.color = COL_CAT_QUANT;
            break;

        // --- STRUCTURE ---
        case NODE_GROUP_START:
            n.title = "Start Group";
            n.regexValue = "(";
            n.color = COL_CAT_STRUCT;
            break;
        case NODE_GROUP_END:
            n.title = "End Group";
            n.regexValue = ")";
            n.color = COL_CAT_STRUCT;
            break;
        case NODE_OR:
            n.title = "OR (Either)";
            n.regexValue = "|";
            n.color = COL_CAT_STRUCT;
            break;
    }
    nodes.push_back(n);
}

// Generates the Regex string by traversing connections
std::string GenerateRegex() {
    std::stringstream ss;
    
    // 1. Find the START node
    // Note: With multiple paths or fragments, this simple traverser might need logic updates.
    // For now, we look for NODE_START. If not found, we look for the "left-most" node without inputs?
    // To keep it simple for MVP: We always start search from NODE_START.
    
    int currentNodeId = -1;
    for (const auto& node : nodes) {
        if (node.type == NODE_START) {
            currentNodeId = node.id;
            ss << node.regexValue;
            break;
        }
    }

    // If no start node, maybe start from the first node in list?
    if (currentNodeId == -1 && !nodes.empty()) {
        // Fallback: Start from first created node that isn't connected TO anything?
        // Simplified: Just take the first node in the vector for now if no explicit START anchor.
        // Or return empty. Let's return a hint.
        if (nodes.empty()) return "";
        // Let's try to find a root (no incoming connections)
         for (const auto& n : nodes) {
             bool hasInput = false;
             for (const auto& c : connections) {
                 if (c.toNodeId == n.id) hasInput = true;
             }
             if (!hasInput) {
                 currentNodeId = n.id;
                 ss << n.regexValue;
                 break;
             }
         }
    }

    if (currentNodeId == -1) return "Add nodes...";

    // 2. Traverse the chain (simplified: assumes 1-to-1 linearity)
    // We loop to find the 'next' connection.
    int safetyCounter = 0;
    bool foundNext = true;
    while (foundNext && safetyCounter < 1000) {
        foundNext = false;
        safetyCounter++;
        
        for (const auto& conn : connections) {
            if (conn.fromNodeId == currentNodeId) {
                // Found the next node
                currentNodeId = conn.toNodeId;
                
                // Find the node object to get its value
                for (const auto& n : nodes) {
                    if (n.id == currentNodeId) {
                        ss << n.regexValue;
                        break;
                    }
                }
                foundNext = true;
                break; // Only follow one path in this prototype
            }
        }
    }

    return ss.str();
}

// Helper to draw grid based on camera view
void DrawGrid2D(int slices, float spacing) {
    // Calculate visible area to draw grid only where needed
    Vector2 topLeft = GetScreenToWorld2D({0, 0}, camera);
    Vector2 bottomRight = GetScreenToWorld2D({(float)GetScreenWidth(), (float)GetScreenHeight()}, camera);
    
    // Snap to grid
    float startX = floor(topLeft.x / spacing) * spacing;
    float startY = floor(topLeft.y / spacing) * spacing;
    
    // Draw vertical lines
    for (float x = startX; x < bottomRight.x + spacing; x += spacing) {
        DrawLineV({x, topLeft.y}, {x, bottomRight.y}, COL_GRID);
    }
    
    // Draw horizontal lines
    for (float y = startY; y < bottomRight.y + spacing; y += spacing) {
        DrawLineV({topLeft.x, y}, {bottomRight.x, y}, COL_GRID);
    }
}

// Wrapper to draw text using our custom font
void DrawTextCustom(const char* text, float x, float y, float fontSize, Color color) {
    DrawTextEx(mainFont, text, {x, y}, fontSize, 1.0f, color);
}

// ----------------------------------------------------------------------------------
// Main
// ----------------------------------------------------------------------------------

int main() {
    // Initialization
    const int screenWidth = 1200;
    const int screenHeight = 900; // Increased height for more UI
    
    // 3. Make window resizable
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE); 
    InitWindow(screenWidth, screenHeight, "Visual Regex Builder - Raylib C++ Prototype");

    // FONT LOADING
    mainFont = LoadFontEx("sources/font.ttf", 32, 0, 0); 
    if (mainFont.texture.id == 0) {
        std::cout << "WARNING: Could not load sources/font.ttf, using default." << std::endl;
        mainFont = GetFontDefault();
    } else {
        SetTextureFilter(mainFont.texture, TEXTURE_FILTER_BILINEAR); 
    }

    SetTargetFPS(60);

    // Init Camera
    camera.zoom = 1.0f;
    camera.target = { 0, 0 };
    camera.offset = { 0, 0 };
    camera.rotation = 0.0f;

    // Add default start node
    AddNode(NODE_START, 100, 300);
    
    float copyFeedbackTimer = 0.0f;

    // Main Loop
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (copyFeedbackTimer > 0) copyFeedbackTimer -= dt;

        Vector2 mouseScreen = GetMousePosition();
        Vector2 mouseWorld = GetScreenToWorld2D(mouseScreen, camera);

        // --- CONTROLS ---
        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            Vector2 mouseWorldBeforeZoom = GetScreenToWorld2D(mouseScreen, camera);
            camera.zoom += (wheel * 0.125f);
            if (camera.zoom < 0.1f) camera.zoom = 0.1f;
            if (camera.zoom > 3.0f) camera.zoom = 3.0f;
            Vector2 mouseWorldAfterZoom = GetScreenToWorld2D(mouseScreen, camera);
            camera.target.x += (mouseWorldBeforeZoom.x - mouseWorldAfterZoom.x);
            camera.target.y += (mouseWorldBeforeZoom.y - mouseWorldAfterZoom.y);
        }

        if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) || (IsKeyDown(KEY_SPACE) && IsMouseButtonDown(MOUSE_LEFT_BUTTON))) {
            Vector2 delta = GetMouseDelta();
            delta.x = delta.x * (-1.0f / camera.zoom);
            delta.y = delta.y * (-1.0f / camera.zoom);
            camera.target.x += delta.x;
            camera.target.y += delta.y;
        }

        // --- INTERACTION ---
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !IsKeyDown(KEY_SPACE)) {
            // Only interact if NOT clicking on UI (simple check: mouse Y < height - 200?)
            // For now, we trust the Z-order drawing or assume simple intersection check.
            
            bool clickedNode = false;
            for (int i = nodes.size() - 1; i >= 0; i--) {
                if (CheckCollisionPointRec(mouseWorld, nodes[i].rect)) {
                    selectedNodeId = nodes[i].id;
                    nodes[i].isDragging = true;
                    mouseOffset.x = mouseWorld.x - nodes[i].rect.x;
                    mouseOffset.y = mouseWorld.y - nodes[i].rect.y;
                    clickedNode = true;
                    break; 
                }
            }
            if (!clickedNode) {
                // Check if clicking background to deselect?
                // But buttons consume clicks too.
                // Simple hack: if click is in UI area, don't deselect? 
                // Let's just deselect if not node.
                 if (mouseScreen.y < GetScreenHeight() - 180 && mouseScreen.y > 80)
                    selectedNodeId = -1;
            }
        }

        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            for (auto& node : nodes) node.isDragging = false;
        }

        if (selectedNodeId != -1) {
            for (auto& node : nodes) {
                if (node.id == selectedNodeId && node.isDragging) {
                    node.rect.x = mouseWorld.x - mouseOffset.x;
                    node.rect.y = mouseWorld.y - mouseOffset.y;
                }
            }
        }

        // Connect
        if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
            for (const auto& node : nodes) {
                if (CheckCollisionPointRec(mouseWorld, node.rect)) {
                    isCreatingConnection = true;
                    connectionStartNodeId = node.id;
                    break;
                }
            }
        }

        if (IsMouseButtonReleased(MOUSE_RIGHT_BUTTON) && isCreatingConnection) {
            isCreatingConnection = false;
            for (const auto& node : nodes) {
                if (CheckCollisionPointRec(mouseWorld, node.rect) && node.id != connectionStartNodeId) {
                    for (size_t k = 0; k < connections.size(); ) {
                        if (connections[k].fromNodeId == connectionStartNodeId) {
                            connections.erase(connections.begin() + k);
                        } else {
                            k++;
                        }
                    }
                    connections.push_back({connectionStartNodeId, node.id});
                    break;
                }
            }
            connectionStartNodeId = -1;
        }

        // Delete
        if (IsKeyPressed(KEY_DELETE) && selectedNodeId != -1) {
            for (int i = connections.size() - 1; i >= 0; i--) {
                if (connections[i].fromNodeId == selectedNodeId || connections[i].toNodeId == selectedNodeId) {
                    connections.erase(connections.begin() + i);
                }
            }
            for (int i = nodes.size() - 1; i >= 0; i--) {
                if (nodes[i].id == selectedNodeId) {
                     nodes.erase(nodes.begin() + i);
                     selectedNodeId = -1;
                    break;
                }
            }
        }

        // ------------------------------------------------------------------------------
        // 2. DRAW
        // ------------------------------------------------------------------------------
        BeginDrawing();
        ClearBackground(COL_BG);

        BeginMode2D(camera);
            DrawGrid2D(100, 40.0f);

            // Draw Connections
            for (const auto& conn : connections) {
                Vector2 start = {0,0}, end = {0,0};
                for (const auto& n : nodes) {
                    if (n.id == conn.fromNodeId) start = { n.rect.x + n.rect.width, n.rect.y + n.rect.height/2 };
                    if (n.id == conn.toNodeId) end = { n.rect.x, n.rect.y + n.rect.height/2 };
                }
                float thickness = 3.0f * (1.0f/camera.zoom > 1.0f ? 1.0f : 1.0f/camera.zoom);
                DrawLineBezier(start, end, thickness, COL_WIRE); 
            }

            if (isCreatingConnection) {
                Vector2 start = {0,0};
                for (const auto& n : nodes) {
                    if (n.id == connectionStartNodeId) {
                        start = { n.rect.x + n.rect.width, n.rect.y + n.rect.height/2 };
                        break;
                    }
                }
                DrawLineBezier(start, mouseWorld, 3.0f, COL_WIRE_ACTIVE);
                for (const auto& node : nodes) {
                    if (CheckCollisionPointRec(mouseWorld, node.rect) && node.id != connectionStartNodeId) {
                         DrawRectangleLinesEx(node.rect, 2.0f, YELLOW);
                    }
                }
            }

            // Draw Nodes
            for (const auto& node : nodes) {
                DrawRectangleRounded({node.rect.x + 4, node.rect.y + 4, node.rect.width, node.rect.height}, 0.3f, 6, Fade(BLACK, 0.5f));
                DrawRectangleRounded(node.rect, 0.3f, 6, node.color);
                
                if (node.id == selectedNodeId) 
                    DrawRectangleRoundedLines(node.rect, 0.3f, 6, WHITE);
                else 
                    DrawRectangleRoundedLines(node.rect, 0.3f, 6, Fade(BLACK, 0.5f));

                DrawTextCustom(node.title.c_str(), node.rect.x + 10, node.rect.y + 20, 20, BLACK);
                
                // Inputs/Outputs
                DrawCircle((int)node.rect.x, (int)node.rect.y + (int)node.rect.height/2, 5, WHITE);
                DrawCircle((int)node.rect.x + (int)node.rect.width, (int)node.rect.y + (int)node.rect.height/2, 5, WHITE);
            }
        EndMode2D();

        // --- UI ---
        int w = GetScreenWidth();
        int h = GetScreenHeight();
        
        // TOP PANEL
        DrawRectangle(0, 0, w, 80, Fade(BLACK, 0.85f));
        std::string regexResult = GenerateRegex();
        DrawTextCustom("REGEX:", 20, 30, 20, LIGHTGRAY);
        DrawTextCustom(regexResult.c_str(), 100, 30, 30, YELLOW);

        Rectangle copyBtnRec = { (float)w - 180, 20, 160, 40 };
        const char* btnText = (copyFeedbackTimer > 0) ? "COPIED!" : "COPY";
        if (GuiButton(copyBtnRec, btnText)) {
            SetClipboardText(regexResult.c_str());
            copyFeedbackTimer = 2.0f; 
        }

        // BOTTOM PANEL (Expanded for more buttons)
        int panelHeight = 180;
        DrawRectangle(0, h - panelHeight, w, panelHeight, Fade(BLACK, 0.9f));
        DrawTextCustom("Pan: Mid-Click | Zoom: Wheel | R-Click: Connect | Del: Delete", 20, h - panelHeight + 10, 16, GRAY);

        Vector2 c = GetScreenToWorld2D({ (float)w/2, (float)h/2 }, camera);
        
        // BUTTON LAYOUT
        // Grid configuration
        int startX = 20;
        int startY = h - panelHeight + 40;
        int btnW = 140;
        int btnH = 35;
        int gapX = 150;
        int gapY = 45;

        // Row 1: Basic Classes
        int x = startX; int y = startY;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Letters")) AddNode(NODE_TEXT, c.x, c.y); x+=gapX;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Numbers")) AddNode(NODE_DIGIT, c.x, c.y); x+=gapX;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Word Chars")) AddNode(NODE_WORD, c.x, c.y); x+=gapX;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Whitespace")) AddNode(NODE_WHITESPACE, c.x, c.y); x+=gapX;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Any Char")) AddNode(NODE_ANY, c.x, c.y); x+=gapX;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Exact Symbol")) AddNode(NODE_SYMBOL, c.x, c.y); x+=gapX;
        
        // Row 2: Negated & Quantifiers
        x = startX; y += gapY;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Non-Number")) AddNode(NODE_NOT_DIGIT, c.x, c.y); x+=gapX;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Non-Word Char")) AddNode(NODE_NOT_WORD, c.x, c.y); x+=gapX;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Non-Whitespace")) AddNode(NODE_NOT_WHITESPACE, c.x, c.y); x+=gapX;
        // Separator
        x += 20; 
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Repeat (0+)")) AddNode(NODE_ZERO_OR_MORE, c.x, c.y); x+=gapX;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Repeat (1+)")) AddNode(NODE_ONE_OR_MORE, c.x, c.y); x+=gapX;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Make Optional")) AddNode(NODE_OPTIONAL, c.x, c.y); x+=gapX;

        // Row 3: Structure & Anchors
        x = startX; y += gapY;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Start of Line")) AddNode(NODE_START, c.x, c.y); x+=gapX;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "End of Line")) AddNode(NODE_END, c.x, c.y); x+=gapX;
        x += 20;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Begin Group")) AddNode(NODE_GROUP_START, c.x, c.y); x+=gapX;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "Finish Group")) AddNode(NODE_GROUP_END, c.x, c.y); x+=gapX;
        if (GuiButton({(float)x, (float)y, (float)btnW, (float)btnH}, "OR (Either)")) AddNode(NODE_OR, c.x, c.y); x+=gapX;

        EndDrawing();
    }

    if (mainFont.texture.id != GetFontDefault().texture.id) UnloadFont(mainFont); 
    CloseWindow();
    return 0;
}

// ----------------------------------------------------------------------------------
// UI Implementation
// ----------------------------------------------------------------------------------
bool GuiButton(Rectangle bounds, const char* text) {
    Vector2 mousePoint = GetMousePosition();
    bool isPressed = false;
    Color color = { 60, 60, 60, 255 }; // Dark grey base

    if (CheckCollisionPointRec(mousePoint, bounds)) {
        color = { 80, 80, 80, 255 };
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) color = { 40, 40, 40, 255 };
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) isPressed = true;
    }

    DrawRectangleRec(bounds, color);
    DrawRectangleLinesEx(bounds, 1, BLACK);
    
    Vector2 textSize = MeasureTextEx(mainFont, text, 18, 1.0f);
    DrawTextEx(mainFont, text, {bounds.x + bounds.width/2 - textSize.x/2, bounds.y + bounds.height/2 - textSize.y/2}, 18, 1.0f, WHITE);

    return isPressed;
}
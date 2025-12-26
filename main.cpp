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
    NODE_START,
    NODE_TEXT,      // Represents [a-z]+
    NODE_DIGIT,     // Represents \d
    NODE_WHITESPACE,// Represents \s
    NODE_ANY,       // Represents .
    NODE_WORD,      // Represents \w
    NODE_SYMBOL     // Represents @ (or custom)
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
const Color COL_NODE_START = { 255, 100, 100, 255 }; // Soft red
const Color COL_NODE_TEXT = { 0, 228, 48, 255 };     // Matrix green
const Color COL_NODE_DIGIT = { 0, 121, 241, 255 };   // Electric blue
const Color COL_NODE_SPECIAL = { 255, 161, 0, 255 }; // Orange for specials
const Color COL_WIRE = { 200, 200, 200, 150 };       // Gray wire
const Color COL_WIRE_ACTIVE = { 255, 255, 0, 255 };  // Yellow wire (creating)

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
        case NODE_START:
            n.title = "START";
            n.regexValue = "^";
            n.color = COL_NODE_START;
            break;
        case NODE_TEXT:
            n.title = "Text (a-z)";
            n.regexValue = "[a-zA-Z]+";
            n.color = COL_NODE_TEXT;
            break;
        case NODE_DIGIT:
            n.title = "Digit (0-9)";
            n.regexValue = "\\d+";
            n.color = COL_NODE_DIGIT;
            break;
        case NODE_WHITESPACE:
            n.title = "Whitespace";
            n.regexValue = "\\s";
            n.color = ORANGE;
            break;
        case NODE_ANY:
            n.title = "Any Char (.)";
            n.regexValue = ".";
            n.color = COL_NODE_SPECIAL;
            break;
        case NODE_WORD:
            n.title = "Word (\\w)";
            n.regexValue = "\\w+";
            n.color = COL_NODE_TEXT;
            break;
        case NODE_SYMBOL:
            n.title = "Symbol (@)";
            n.regexValue = "@";
            n.color = PURPLE;
            break;
    }
    nodes.push_back(n);
}

// Generates the Regex string by traversing connections
std::string GenerateRegex() {
    std::stringstream ss;
    
    // 1. Find the START node
    int currentNodeId = -1;
    for (const auto& node : nodes) {
        if (node.type == NODE_START) {
            currentNodeId = node.id;
            ss << node.regexValue;
            break;
        }
    }

    if (currentNodeId == -1) return "Missing START node";

    // 2. Traverse the chain (simplified: assumes 1-to-1 linearity)
    bool foundNext = true;
    while (foundNext) {
        foundNext = false;
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
    const int screenHeight = 800;
    
    // 3. Make window resizable
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE); 
    InitWindow(screenWidth, screenHeight, "Visual Regex Builder - Raylib C++ Prototype");

    // FONT LOADING
    // 1. Try to load custom font from sources/font.ttf
    // We load it with size 32 (large enough for scaling)
    mainFont = LoadFontEx("sources/font.ttf", 32, 0, 0); // Load all chars
    
    // Fallback if failed
    if (mainFont.texture.id == 0) {
        std::cout << "WARNING: Could not load sources/font.ttf, using default." << std::endl;
        mainFont = GetFontDefault();
    } else {
        // Set texture filter for better scaling
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
    
    // Feedback timer for "Copied!" message
    float copyFeedbackTimer = 0.0f;

    // Main Loop
    while (!WindowShouldClose()) {
        // ------------------------------------------------------------------------------
        // 1. UPDATE (Logic)
        // ------------------------------------------------------------------------------
        
        float dt = GetFrameTime();
        if (copyFeedbackTimer > 0) copyFeedbackTimer -= dt;

        // IMPORTANT: Get mouse position in World Space (relative to camera)
        Vector2 mouseScreen = GetMousePosition();
        Vector2 mouseWorld = GetScreenToWorld2D(mouseScreen, camera);

        // --- ZOOM & PAN CONTROLS ---
        
        // Zoom with Mouse Wheel
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

        // Pan with Middle Mouse or SPACE + Left Click
        if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) || (IsKeyDown(KEY_SPACE) && IsMouseButtonDown(MOUSE_LEFT_BUTTON))) {
            Vector2 delta = GetMouseDelta();
            delta.x = delta.x * (-1.0f / camera.zoom);
            delta.y = delta.y * (-1.0f / camera.zoom);
            camera.target.x += delta.x;
            camera.target.y += delta.y;
        }

        // --- INTERACTION LOGIC (Using mouseWorld) ---

        // Drag Nodes
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !IsKeyDown(KEY_SPACE)) { // Prevent conflict with Pan
            bool clickedNode = false;
            // Iterate backwards to select the one drawn on top (last)
            for (int i = nodes.size() - 1; i >= 0; i--) {
                if (CheckCollisionPointRec(mouseWorld, nodes[i].rect)) {
                    selectedNodeId = nodes[i].id;
                    nodes[i].isDragging = true;
                    // Store offset relative to node origin
                    mouseOffset.x = mouseWorld.x - nodes[i].rect.x;
                    mouseOffset.y = mouseWorld.y - nodes[i].rect.y;
                    clickedNode = true;
                    break; 
                }
            }
            if (!clickedNode) selectedNodeId = -1;
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

        // Connection Logic (Right Click)
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
            // Check if dropped over another node (using World coordinates)
            for (const auto& node : nodes) {
                if (CheckCollisionPointRec(mouseWorld, node.rect) && node.id != connectionStartNodeId) {
                    // Create connection
                    // First remove existing connections from source (enforce 1 output per node)
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

        // 2. Delete connections or nodes (Delete Key)
        if (IsKeyPressed(KEY_DELETE) && selectedNodeId != -1) {
            // First remove connections involving this node
            for (int i = connections.size() - 1; i >= 0; i--) {
                if (connections[i].fromNodeId == selectedNodeId || connections[i].toNodeId == selectedNodeId) {
                    connections.erase(connections.begin() + i);
                }
            }
            
            // Then remove the node itself
            for (int i = nodes.size() - 1; i >= 0; i--) {
                if (nodes[i].id == selectedNodeId) {
                    // Don't allow deleting the START node if we want to enforce structure (optional logic)
                    // But for now, let's allow it but warn or check logic elsewhere.
                    // Actually, let's PROTECT the START node to avoid breaking the graph logic
                    if (nodes[i].type != NODE_START) {
                        nodes.erase(nodes.begin() + i);
                        selectedNodeId = -1;
                    }
                    break;
                }
            }
        }

        // ------------------------------------------------------------------------------
        // 2. DRAW
        // ------------------------------------------------------------------------------
        BeginDrawing();
        ClearBackground(COL_BG);

        // --- WORLD SPACE DRAWING (Inside Camera) ---
        BeginMode2D(camera);

            // Draw Infinite Grid
            DrawGrid2D(100, 40.0f);

            // Draw Connections
            for (const auto& conn : connections) {
                Vector2 start = {0,0}, end = {0,0};
                // Find positions
                for (const auto& n : nodes) {
                    if (n.id == conn.fromNodeId) start = { n.rect.x + n.rect.width, n.rect.y + n.rect.height/2 };
                    if (n.id == conn.toNodeId) end = { n.rect.x, n.rect.y + n.rect.height/2 };
                }
                
                // Draw Bezier curve for wire
                float thickness = 3.0f * (1.0f/camera.zoom > 1.0f ? 1.0f : 1.0f/camera.zoom);
                DrawLineBezier(start, end, thickness, COL_WIRE); 
            }

            // Draw temp wire (while dragging right click)
            if (isCreatingConnection) {
                Vector2 start = {0,0};
                for (const auto& n : nodes) {
                    if (n.id == connectionStartNodeId) {
                        start = { n.rect.x + n.rect.width, n.rect.y + n.rect.height/2 };
                        break;
                    }
                }
                // Draw to World Mouse Position
                DrawLineBezier(start, mouseWorld, 3.0f, COL_WIRE_ACTIVE);
                
                // Highlight target node if hovering
                for (const auto& node : nodes) {
                    if (CheckCollisionPointRec(mouseWorld, node.rect) && node.id != connectionStartNodeId) {
                         DrawRectangleLinesEx(node.rect, 2.0f, YELLOW);
                    }
                }
            }

            // Draw Nodes
            for (const auto& node : nodes) {
                // Shadow
                DrawRectangleRounded({node.rect.x + 4, node.rect.y + 4, node.rect.width, node.rect.height}, 0.3f, 6, Fade(BLACK, 0.5f));
                // Body
                DrawRectangleRounded(node.rect, 0.3f, 6, node.color);
                // Border (bright if selected)
                if (node.id == selectedNodeId) 
                    DrawRectangleRoundedLines(node.rect, 0.3f, 6, WHITE);
                else 
                    DrawRectangleRoundedLines(node.rect, 0.3f, 6, Fade(BLACK, 0.5f));

                // Node Text (Using Custom Font)
                DrawTextCustom(node.title.c_str(), node.rect.x + 10, node.rect.y + 20, 20, BLACK); // Increased size to 20
                
                // Visual connectors (input/output circles)
                if (node.type != NODE_START) // Start has no input
                    DrawCircle((int)node.rect.x, (int)node.rect.y + (int)node.rect.height/2, 5, WHITE);
                
                DrawCircle((int)node.rect.x + (int)node.rect.width, (int)node.rect.y + (int)node.rect.height/2, 5, WHITE);
            }

        EndMode2D();
        // --- END WORLD SPACE ---

        // --- UI DRAWING (Screen Space - Static) ---
        // Use GetScreenWidth/Height for dynamic resizing
        int currentScreenWidth = GetScreenWidth();
        int currentScreenHeight = GetScreenHeight();
        
        // UI: Top Panel (Generation)
        DrawRectangle(0, 0, currentScreenWidth, 80, Fade(BLACK, 0.8f));
        std::string regexResult = GenerateRegex();
        DrawTextCustom("GENERATED REGEX:", 20, 30, 20, WHITE);
        
        // Output Value
        DrawTextCustom(regexResult.c_str(), 240, 30, 30, YELLOW);

        // Copy Button
        Rectangle copyBtnRec = { (float)currentScreenWidth - 180, 20, 160, 40 };
        const char* btnText = (copyFeedbackTimer > 0) ? "COPIED!" : "COPY CLIPBOARD";
        if (GuiButton(copyBtnRec, btnText)) {
            SetClipboardText(regexResult.c_str());
            copyFeedbackTimer = 2.0f; // Show "Copied" for 2 seconds
        }

        // UI: Bottom Panel (Tools)
        DrawRectangle(0, currentScreenHeight - 140, currentScreenWidth, 140, Fade(BLACK, 0.8f));
        DrawTextCustom("Pan: Middle-Click | Zoom: Wheel | R-Click: Connect | Del: Delete", 20, currentScreenHeight - 125, 20, LIGHTGRAY);

        // Buttons need to use World Coordinates for spawning nodes centered on screen
        Vector2 centerScreenWorld = GetScreenToWorld2D({ (float)currentScreenWidth/2, (float)currentScreenHeight/2 }, camera);
        
        // Row 1
        int btnX = 20;
        int btnY = currentScreenHeight - 90;
        int btnW = 150;
        int btnH = 40;
        int gap = 170;
        
        if (GuiButton({(float)btnX, (float)btnY, (float)btnW, (float)btnH}, "+ TEXT (a-z)")) AddNode(NODE_TEXT, centerScreenWorld.x, centerScreenWorld.y);
        btnX += gap;
        if (GuiButton({(float)btnX, (float)btnY, (float)btnW, (float)btnH}, "+ DIGIT (0-9)")) AddNode(NODE_DIGIT, centerScreenWorld.x, centerScreenWorld.y);
        btnX += gap;
        if (GuiButton({(float)btnX, (float)btnY, (float)btnW, (float)btnH}, "+ WORD (\\w)")) AddNode(NODE_WORD, centerScreenWorld.x, centerScreenWorld.y);
        
        // Row 2
        btnX = 20;
        btnY += 50;
        if (GuiButton({(float)btnX, (float)btnY, (float)btnW, (float)btnH}, "+ SPACE")) AddNode(NODE_WHITESPACE, centerScreenWorld.x, centerScreenWorld.y);
        btnX += gap;
        if (GuiButton({(float)btnX, (float)btnY, (float)btnW, (float)btnH}, "+ ANY (.)")) AddNode(NODE_ANY, centerScreenWorld.x, centerScreenWorld.y);
        btnX += gap;
        if (GuiButton({(float)btnX, (float)btnY, (float)btnW, (float)btnH}, "+ SYMBOL (@)")) AddNode(NODE_SYMBOL, centerScreenWorld.x, centerScreenWorld.y);

        EndDrawing();
    }

    // Unload font if it was loaded externally
    if (mainFont.texture.id != GetFontDefault().texture.id) {
         UnloadFont(mainFont); 
    }

    CloseWindow();
    return 0;
}

// ----------------------------------------------------------------------------------
// Minimal Button Implementation
// ----------------------------------------------------------------------------------
bool GuiButton(Rectangle bounds, const char* text) {
    Vector2 mousePoint = GetMousePosition(); // UI is in Screen Space
    bool isPressed = false;
    Color color = GRAY;

    if (CheckCollisionPointRec(mousePoint, bounds)) {
        color = LIGHTGRAY;
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) color = DARKGRAY;
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) isPressed = true;
    }

    DrawRectangleRec(bounds, color);
    DrawRectangleLinesEx(bounds, 1, BLACK);
    
    // Center Text using custom font
    Vector2 textSize = MeasureTextEx(mainFont, text, 20, 1.0f);
    DrawTextEx(mainFont, text, {bounds.x + bounds.width/2 - textSize.x/2, bounds.y + bounds.height/2 - textSize.y/2}, 20, 1.0f, BLACK);

    return isPressed;
}
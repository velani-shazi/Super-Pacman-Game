#include "raylib.h"
#include <vector>
#include <cmath>
#include <array>
#include <fstream>
#include <iostream>

const int screenWidth = 800;
const int screenHeight = 600;
const int tileSize = 40;
const int mapWidth = 20;
const int mapHeight = 15;

enum TileType { EMPTY, WALL, DOOR, KEY, MAIZE, POWER_PELLET, STAR, SUPER_PELLET, FRUIT };

struct Entity {
    Vector2 position;
    Vector2 direction;
    float speed;
    float radius;
    Color color;
    bool isPoweredUp;
    float powerUpTimer;
    bool isVulnerable;
    Vector2 startPosition;
    bool isFlattened;
    int spriteIndex;
};

Entity player;
std::vector<Entity> ghosts;
int score = 0;
float starTimer = 0;
const float starInterval = 10.0f; // Stars appear every 10 seconds

// New variables for player lives
int playerLives = 3;
const int maxLives = 5;

// New variables for super pellets
const float superPelletDuration = 15.0f;
const float normalSpeed = 2.0f;
const float superSpeed = 3.0f;

// New variables for center box symbols
std::array<int, 2> centerBoxSymbols = {0, 0};
float symbolChangeTimer = 0;
const float symbolChangeInterval = 1.0f; // Symbols change every second
const int numSymbols = 6; // Number of different symbols

// Sprite sheet and rectangles
Texture2D spriteSheet;
Rectangle spriteRects[16];

TileType map[mapHeight][mapWidth];

// Function to load the map from a text file
void LoadMapFromFile(const char* filename) {
    std::ifstream mapFile(filename);
    
    if (!mapFile.is_open()) {
        std::cerr << "Error: Unable to open the file " << filename << std::endl;
        return;
    }

    std::string line;
    int row = 0;

    while (std::getline(mapFile, line)) {
        for (int col = 0; col < line.length() && col < mapWidth; col++) {
            char tile = line[col];
            switch (tile) {
                case '#': map[row][col] = WALL; break;
                case '.': map[row][col] = EMPTY; break;
                case 'K': map[row][col] = KEY; break;
                case 'M': map[row][col] = MAIZE; break;
                case 'P': map[row][col] = POWER_PELLET; break;
                case 'S': map[row][col] = STAR; break;
                case 'U': map[row][col] = SUPER_PELLET; break;
                case 'F': map[row][col] = FRUIT; break;
                case 'D': map[row][col] = DOOR; break;
                default: map[row][col] = EMPTY; break; // Default case for unknown characters
            }
        }
        row++;
        if (row >= mapHeight) break; // Prevent going out of bounds
    }

    mapFile.close();
}


void InitGame() {
    LoadMapFromFile("maze.txt");
    player.position = {tileSize * 1.5f, tileSize * 1.5f};
    player.direction = {1, 0};
    player.speed = normalSpeed;
    player.radius = tileSize / 3.0f;
    player.color = YELLOW;
    player.isPoweredUp = false;
    player.powerUpTimer = 0;
    player.startPosition = player.position;

    ghosts.clear();
    for (int i = 0; i < 4; i++) {
        Entity ghost;
        ghost.startPosition = {tileSize * (10 + i), tileSize * 8};
        ghost.position = ghost.startPosition;
        ghost.direction = {(float)(GetRandomValue(0, 1) * 2 - 1), 0};
        ghost.speed = 1.5f;
        ghost.radius = tileSize / 3.0f;
        ghost.color = WHITE;
        ghost.isVulnerable = false;
        ghost.isFlattened = false;
        ghost.spriteIndex = i;
        ghosts.push_back(ghost);
    }

    score = 0;
    starTimer = 0;
    playerLives = 3;

    // Initialize center box symbols
    centerBoxSymbols[0] = GetRandomValue(0, numSymbols - 1);
    centerBoxSymbols[1] = GetRandomValue(0, numSymbols - 1);
    symbolChangeTimer = 0;

    // Add two super pellets
    int superPelletCount = 0;
    while (superPelletCount < 2) {
        int x = GetRandomValue(1, mapWidth - 2);
        int y = GetRandomValue(1, mapHeight - 2);
        if (map[y][x] == EMPTY) {
            map[y][x] = SUPER_PELLET;
            superPelletCount++;
        }
    }

    // Add fruit enclosed by two doors
    map[7][9] = DOOR;
    map[7][10] = FRUIT;
    map[7][11] = DOOR;

    // Load sprite sheet
    spriteSheet = LoadTexture("sprites.png");
    
    // Define sprite rectangles for each tile type
    spriteRects[KEY] = (Rectangle){ 296, 100, 14, 26 };
    spriteRects[MAIZE] = (Rectangle){ 68, 100, 24, 24 };
    spriteRects[FRUIT] = (Rectangle){ 164, 100, 24, 24 };
    
    // Define ghost sprite rectangles
    spriteRects[9] = (Rectangle){ 98, 130, 28, 28 };   // Red ghost
    spriteRects[10] = (Rectangle){ 98, 162, 28, 28 }; // Pink ghost
    spriteRects[11] = (Rectangle){ 98, 194, 28, 28 }; // Blue ghost
    spriteRects[12] = (Rectangle){ 98, 226, 28, 28 }; // Orange ghost
    spriteRects[13] = (Rectangle){ 258, 130, 28, 28 }; // Vulnerable ghost
    spriteRects[14] = (Rectangle){ 162, 130, 28, 28 }; // Flattened ghost
    spriteRects[15] = (Rectangle){ 194, 130, 28, 28 }; // Eyes only (when eaten)
}

void ResetPlayerPosition() {
    player.position = player.startPosition;
    player.direction = {1, 0};
    player.isPoweredUp = false;
    player.powerUpTimer = 0;
    player.speed = normalSpeed;

    for (auto& ghost : ghosts) {
        ghost.position = ghost.startPosition;
        ghost.direction = {(float)(GetRandomValue(0, 1) * 2 - 1), 0};
        ghost.isVulnerable = false;
        ghost.isFlattened = false;
    }
}

void UpdateGame() {
    // Player movement
    if (IsKeyDown(KEY_RIGHT)) player.direction = {1, 0};
    if (IsKeyDown(KEY_LEFT)) player.direction = {-1, 0};
    if (IsKeyDown(KEY_DOWN)) player.direction = {0, 1};
    if (IsKeyDown(KEY_UP)) player.direction = {0, -1};

    Vector2 newPos = {
        player.position.x + player.direction.x * player.speed,
        player.position.y + player.direction.y * player.speed
    };

    int cellX = (int)(newPos.x / tileSize);
    int cellY = (int)(newPos.y / tileSize);

    if (cellX >= 0 && cellX < mapWidth && cellY >= 0 && cellY < mapHeight) {
        if (map[cellY][cellX] == EMPTY || map[cellY][cellX] == MAIZE || map[cellY][cellX] == KEY || 
            map[cellY][cellX] == POWER_PELLET || map[cellY][cellX] == STAR || map[cellY][cellX] == SUPER_PELLET ||
            map[cellY][cellX] == FRUIT || (map[cellY][cellX] == DOOR && player.isPoweredUp)) {
            player.position = newPos;

            // Collect items
            if (map[cellY][cellX] == MAIZE) {
                map[cellY][cellX] = EMPTY;
                score += 100;
            } else if (map[cellY][cellX] == KEY) {
                map[cellY][cellX] = EMPTY;
                score += 500;
                // Unlock doors here
            } else if (map[cellY][cellX] == POWER_PELLET) {
                map[cellY][cellX] = EMPTY;
                player.isPoweredUp = true;
                player.powerUpTimer = 10.0f; // 10 seconds of power-up
                for (auto& ghost : ghosts) {
                    ghost.isVulnerable = true;
                }
                score += 50;
            } else if (map[cellY][cellX] == STAR) {
                map[cellY][cellX] = EMPTY;
                // Check for bonus scoring
                if (centerBoxSymbols[0] == centerBoxSymbols[1]) {
                    bool symbolsMatchMaze = false;
                    for (int y = 0; y < mapHeight; y++) {
                        for (int x = 0; x < mapWidth; x++) {
                            if (map[y][x] == (TileType)centerBoxSymbols[0]) {
                                symbolsMatchMaze = true;
                                break;
                            }
                        }
                        if (symbolsMatchMaze) break;
                    }
                    if (symbolsMatchMaze) {
                        score += 5000; // Matching symbols are the same as other symbols in the maze
                    } else {
                        score += 2000; // Matching symbols, but not found in the maze
                    }
                } else {
                    score += 500; // Symbols do not match
                }
            } else if (map[cellY][cellX] == SUPER_PELLET) {
                map[cellY][cellX] = EMPTY;
                player.isPoweredUp = true;
                player.powerUpTimer = superPelletDuration;
                player.speed = superSpeed;
                player.radius = tileSize / 2.0f;
                for (auto& ghost : ghosts) {
                    ghost.isFlattened = true;
                }
                score += 100;
            } else if (map[cellY][cellX] == FRUIT) {
                map[cellY][cellX] = EMPTY;
                score += 1000;
            } else if (map[cellY][cellX] == DOOR && player.isPoweredUp) {
                map[cellY][cellX] = EMPTY; // Open the door
            }
        }
    }

    // Update power-up status
    if (player.isPoweredUp) {
        player.powerUpTimer -= GetFrameTime();
        if (player.powerUpTimer <= 0) {
            player.isPoweredUp = false;
            player.radius = tileSize / 3.0f;
            player.speed = normalSpeed;
            for (auto& ghost : ghosts) {
                ghost.isVulnerable = false;
                ghost.isFlattened = false;
            }
        }
    }

    // Ghost movement and interaction
    for (auto& ghost : ghosts) {
        Vector2 newGhostPos = {
            ghost.position.x + ghost.direction.x * ghost.speed,
            ghost.position.y + ghost.direction.y * ghost.speed
        };

        int ghostCellX = (int)(newGhostPos.x / tileSize);
        int ghostCellY = (int)(newGhostPos.y / tileSize);

        if (ghostCellX >= 0 && ghostCellX < mapWidth && ghostCellY >= 0 && ghostCellY < mapHeight && 
            map[ghostCellY][ghostCellX] != WALL && map[ghostCellY][ghostCellX] != DOOR) {
            ghost.position = newGhostPos;
        } else {
            // Change direction randomly
            if (GetRandomValue(0, 1) == 0) {
                ghost.direction = {(float)(GetRandomValue(0, 1) * 2 - 1), 0};
            } else {
                ghost.direction = {0, (float)(GetRandomValue(0, 1) * 2 - 1)};
            }
        }

        // Check collision with player
        if (CheckCollisionCircles(player.position, player.radius, ghost.position, ghost.radius)) {
            if (player.isPoweredUp && ghost.isVulnerable) {
                // Player eats the ghost
                ghost.position = ghost.startPosition;
                ghost.isVulnerable = false;
                ghost.isFlattened = false;
                score += 200;
            } else if (!player.isPoweredUp && !ghost.isFlattened) {
                playerLives--;
                if (playerLives > 0) {
                    ResetPlayerPosition();
                } else {
                    InitGame();
                }
            }
        }
    }

    // Star spawning logic
    starTimer += GetFrameTime();
    if (starTimer >= starInterval) {
        starTimer = 0;
        int starX = GetRandomValue(9, 10);
        int starY = GetRandomValue(6, 8);
        if (map[starY][starX] == EMPTY) {
            map[starY][starX] = STAR;
        }
    }

    // Update center box symbols
    symbolChangeTimer += GetFrameTime();
    if (symbolChangeTimer >= symbolChangeInterval) {
        symbolChangeTimer = 0;
        centerBoxSymbols[0] = GetRandomValue(0, numSymbols - 1);
        centerBoxSymbols[1] = GetRandomValue(0, numSymbols - 1);
    }

    // Extra life logic
    if (score >= 10000 && playerLives < maxLives) {
        playerLives++;
        score -= 10000;
    }
}

void DrawGame() {
    BeginDrawing();
    ClearBackground(BLACK);

    // Draw map
    for (int y = 0; y < mapHeight; y++) {
        for (int x = 0; x < mapWidth; x++) {
            switch (map[y][x]) {
                case WALL:
                    DrawRectangle(x * tileSize, y * tileSize, tileSize, tileSize, DARKPURPLE);
                    break;
                case DOOR:
                    DrawRectangle(x * tileSize, y * tileSize, tileSize, tileSize, PINK);
                    break;
                case KEY:
                    DrawTexturePro(spriteSheet, spriteRects[KEY], 
                        (Rectangle){x * tileSize, y * tileSize, tileSize, tileSize}, 
                        (Vector2){0, 0}, 0, WHITE);
                    break;
                case MAIZE:
                    DrawTexturePro(spriteSheet, spriteRects[MAIZE], 
                        (Rectangle){x * tileSize, y * tileSize, tileSize, tileSize}, 
                        (Vector2){0, 0}, 0, WHITE);
                    break;
                case POWER_PELLET:
                    DrawCircle(x * tileSize + tileSize/2, y * tileSize + tileSize/2, tileSize/4, SKYBLUE);
                    break;
                case STAR:
                    DrawPoly(Vector2{x * tileSize + tileSize/2, y * tileSize + tileSize/2}, 5, tileSize/3, 0, YELLOW);
                    break;
                case SUPER_PELLET:
                    DrawCircle(x * tileSize + tileSize/2, y * tileSize + tileSize/2, tileSize/3, ORANGE);
                    break;
                case FRUIT:
                    DrawTexturePro(spriteSheet, spriteRects[FRUIT], 
                        (Rectangle){x * tileSize, y * tileSize, tileSize, tileSize}, 
                        (Vector2){0, 0}, 0, WHITE);
                    break;
            }
        }
    }

    // Draw center boxes with symbols
    DrawRectangle(9 * tileSize, 7 * tileSize, tileSize, tileSize, GRAY);
    DrawRectangle(10 * tileSize, 7 * tileSize, tileSize, tileSize, GRAY);
    
    // Draw symbols in center boxes
    const char* symbols[] = {"A", "B", "C", "D", "E", "F"};
    DrawText(symbols[centerBoxSymbols[0]], 9 * tileSize + tileSize/4, 7 * tileSize + tileSize/4, tileSize/2, RED);
    DrawText(symbols[centerBoxSymbols[1]], 10 * tileSize + tileSize/4, 7 * tileSize + tileSize/4, tileSize/2, RED);

    // Draw player (Pac-Man)
    DrawCircle(player.position.x, player.position.y, player.radius, player.color);
    
    // Draw Pac-Man's mouth
    float mouthAngle = 45.0f * sinf(GetTime() * 10);
    DrawCircleSector(player.position, player.radius, 
                     (atan2f(player.direction.y, player.direction.x) * RAD2DEG) - mouthAngle, 
                     (atan2f(player.direction.y, player.direction.x) * RAD2DEG) + mouthAngle, 0, BLACK);

    // Draw ghosts
    for (const auto& ghost : ghosts) {
        Rectangle ghostRect;
        if (ghost.isVulnerable) {
            ghostRect = spriteRects[13]; // Vulnerable ghost sprite
        } else if (ghost.isFlattened) {
            ghostRect = spriteRects[14]; // Flattened ghost sprite
        } else {
            ghostRect = spriteRects[9 + ghost.spriteIndex]; // Normal ghost sprite
        }
        
        DrawTexturePro(spriteSheet, ghostRect, 
            (Rectangle){ghost.position.x - ghost.radius, ghost.position.y - ghost.radius, ghost.radius * 2, ghost.radius * 2}, 
            (Vector2){0, 0}, 0, WHITE);
    }

    // Draw score
    DrawText(TextFormat("SCORE: %d", score), 10, 10, 20, WHITE);
    DrawText("HIGH SCORE: 30000", screenWidth - 200, 10, 20, WHITE);

    // Draw remaining lives
    for (int i = 0; i < playerLives; i++) {
        DrawCircle(20 + i * 30, screenHeight - 20, 10, YELLOW);
    }

    EndDrawing();
}

int main() {
    InitWindow(screenWidth, screenHeight, "Super Pac-Man");
    SetTargetFPS(60);

    InitGame();

    while (!WindowShouldClose()) {
        UpdateGame();
        DrawGame();
    }

    UnloadTexture(spriteSheet);
    CloseWindow();
    return 0;
}
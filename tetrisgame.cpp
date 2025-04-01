#include <iostream>
#include <vector>
#include <ctime>
#include <conio.h>
#include <windows.h>
#include <algorithm>
#include <string>

using namespace std;

// Game constants
const int WIDTH = 10;                // Width of the playfield
const int HEIGHT = 20;               // Total height of the playfield (including invisible top)
const int VISIBLE_HEIGHT = 20;       // Visible height of the playfield
const int INITIAL_SPEEDS[] = {1000, 800, 600, 400}; // Initial speeds for different difficulty levels
const int LEVEL_UP_LINES = 10;       // Lines needed to level up
const int SCORE_TABLE[] = {0, 40, 100, 300, 1200}; // Score for 1, 2, 3, 4 lines

HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

// Color definitions for the pieces
enum Colors {
    BLACK = 0,
    BLUE = 1,
    GREEN = 2,
    CYAN = 3,
    RED = 4,
    MAGENTA = 5,
    YELLOW = 6,
    WHITE = 7,
    GRAY = 8,
    BRIGHT_BLUE = 9,
    BRIGHT_GREEN = 10,
    BRIGHT_CYAN = 11,
    BRIGHT_RED = 12,
    BRIGHT_MAGENTA = 13,
    BRIGHT_YELLOW = 14,
    BRIGHT_WHITE = 15
};

// Helper function to set console colors
void setColor(int textColor, int bgColor = BLACK) {
    SetConsoleTextAttribute(hConsole, (bgColor << 4) | textColor);
}

// Simple point structure for coordinates
struct Point {
    int x, y;
    Point(int x = 0, int y = 0) : x(x), y(y) {}
};

// Tetromino structure containing shape, color and position
struct Tetromino {
    vector<vector<int>> shape;  // 2D array representing the piece shape
    vector<Point> wallKicks;    // Wall kick tests for rotation
    int color;                  // Color of the piece
    int x, y;                   // Position on the board
};

// All Tetromino definitions with their shapes and wall kicks
vector<Tetromino> tetrominoes = {
    // I piece (cyan)
    {
        {{0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0}},
        {{0,0}, {-2,0}, {1,0}, {-2,-1}, {1,2}},
        CYAN,
        0, 0
    },
    // O piece (yellow)
    {
        {{1,1}, {1,1}},
        {{0,0}}, // O piece doesn't rotate
        YELLOW,
        0, 0
    },
    // T piece (magenta)
    {
        {{0,1,0}, {1,1,1}, {0,0,0}},
        {{0,0}, {-1,0}, {-1,1}, {0,-2}, {-1,-2}},
        MAGENTA,
        0, 0
    },
    // L piece (bright blue)
    {
        {{0,0,1}, {1,1,1}, {0,0,0}},
        {{0,0}, {-1,0}, {-1,1}, {0,-2}, {-1,-2}},
        BRIGHT_BLUE,
        0, 0
    },
    // J piece (blue)
    {
        {{1,0,0}, {1,1,1}, {0,0,0}},
        {{0,0}, {-1,0}, {-1,1}, {0,-2}, {-1,-2}},
        BLUE,
        0, 0
    },
    // S piece (green)
    {
        {{0,1,1}, {1,1,0}, {0,0,0}},
        {{0,0}, {-1,0}, {-1,1}, {0,-2}, {-1,-2}},
        GREEN,
        0, 0
    },
    // Z piece (red)
    {
        {{1,1,0}, {0,1,1}, {0,0,0}},
        {{0,0}, {-1,0}, {-1,1}, {0,-2}, {-1,-2}},
        RED,
        0, 0
    }
};

class Game {
private:
    int board[HEIGHT][WIDTH] = {0};  // Game board (0 = empty, other values = colors)
    Tetromino current, next, hold;   // Current, next and held pieces
    bool canHold = true;             // Flag to prevent holding twice in a row
    int score = 0;                   // Current score
    int level = 1;                   // Current level
    int linesCleared = 0;            // Total lines cleared
    int speed;                       // Current fall speed
    bool gameOver = false;           // Game over flag
    clock_t lastFall = 0;            // Time of last fall
    bool needsRedraw = true;         // Flag to optimize drawing
    int difficulty = 1;              // Difficulty level (1-4)

public:
    Game(int diff) : difficulty(diff) {
        speed = INITIAL_SPEEDS[difficulty-1]; // Set initial speed based on difficulty
        srand(time(0));  // Seed random number generator
        next = getRandomTetromino(); // Get first next piece
        spawnNewPiece(); // Spawn first piece
    }

    // Returns a random tetromino
    Tetromino getRandomTetromino() {
        Tetromino t = tetrominoes[rand() % tetrominoes.size()];
        t.x = 0;
        t.y = 0;
        return t;
    }

    // Spawns a new piece at the top of the board
    void spawnNewPiece() {
        current = next;
        next = getRandomTetromino();
        current.x = WIDTH / 2 - current.shape[0].size() / 2; // Center the piece
        current.y = 0;
        canHold = true;
        needsRedraw = true;
        
        // Check if game over (new piece can't be placed)
        if (!isValidPosition(current.shape, current.x, current.y)) {
            gameOver = true;
        }
    }

    // Checks if a piece can be placed at a certain position
    bool isValidPosition(const vector<vector<int>>& shape, int x, int y) {
        for (int i = 0; i < shape.size(); i++) {
            for (int j = 0; j < shape[i].size(); j++) {
                if (shape[i][j]) {
                    int newX = x + j;
                    int newY = y + i;
                    if (newX < 0 || newX >= WIDTH || newY >= HEIGHT || (newY >= 0 && board[newY][newX])) {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    // Rotates the current piece with wall kicks
    void rotatePiece() {
        if (current.shape.size() == 4) { // I piece special case
            vector<vector<int>> rotated(4, vector<int>(4, 0));
            for (int i = 0; i < 4; i++) {
                for (int j = 0; j < 4; j++) {
                    rotated[j][3 - i] = current.shape[i][j];
                }
            }
            
            // Try wall kicks
            for (const auto& kick : current.wallKicks) {
                if (isValidPosition(rotated, current.x + kick.x, current.y + kick.y)) {
                    current.shape = rotated;
                    current.x += kick.x;
                    current.y += kick.y;
                    needsRedraw = true;
                    return;
                }
            }
        } else if (current.shape.size() > 1) { // Other pieces
            vector<vector<int>> rotated(current.shape[0].size(), vector<int>(current.shape.size(), 0));
            for (int i = 0; i < current.shape.size(); i++) {
                for (int j = 0; j < current.shape[i].size(); j++) {
                    rotated[j][current.shape.size() - i - 1] = current.shape[i][j];
                }
            }
            
            // Try wall kicks
            for (const auto& kick : current.wallKicks) {
                if (isValidPosition(rotated, current.x + kick.x, current.y + kick.y)) {
                    current.shape = rotated;
                    current.x += kick.x;
                    current.y += kick.y;
                    needsRedraw = true;
                    return;
                }
            }
        }
    }

    // Moves the current piece by dx, dy if possible
    void movePiece(int dx, int dy) {
        if (isValidPosition(current.shape, current.x + dx, current.y + dy)) {
            current.x += dx;
            current.y += dy;
            needsRedraw = true;
        }
    }

    // Drops the piece immediately to the bottom
    void hardDrop() {
        while (isValidPosition(current.shape, current.x, current.y + 1)) {
            current.y++;
        }
        lockPiece();
    }

    // Locks the current piece in place and checks for completed lines
    void lockPiece() {
        // Add piece to board
        for (int i = 0; i < current.shape.size(); i++) {
            for (int j = 0; j < current.shape[i].size(); j++) {
                if (current.shape[i][j]) {
                    int y = current.y + i;
                    int x = current.x + j;
                    if (y >= 0) { // Only lock if it's in the visible area
                        board[y][x] = current.color;
                    }
                }
            }
        }
        
        // Check for completed lines
        int lines = 0;
        for (int i = HEIGHT - 1; i >= 0; i--) {
            bool complete = true;
            for (int j = 0; j < WIDTH; j++) {
                if (!board[i][j]) {
                    complete = false;
                    break;
                }
            }
            
            if (complete) {
                // Remove the line
                for (int k = i; k > 0; k--) {
                    for (int j = 0; j < WIDTH; j++) {
                        board[k][j] = board[k-1][j];
                    }
                }
                // Clear top line
                for (int j = 0; j < WIDTH; j++) {
                    board[0][j] = 0;
                }
                lines++;
                i++; // Check the same line again (now with new content)
            }
        }
        
        // Update score and level
        if (lines > 0) {
            score += SCORE_TABLE[lines] * level;
            linesCleared += lines;
            level = linesCleared / LEVEL_UP_LINES + 1;
            speed = max(100, INITIAL_SPEEDS[difficulty-1] - (level * 50));
            needsRedraw = true;
        }
        
        spawnNewPiece();
    }

    // Holds the current piece and swaps with the held piece
    void holdPiece() {
        if (!canHold) return;
        
        if (hold.shape.empty()) {
            hold = current;
            spawnNewPiece();
        } else {
            Tetromino temp = current;
            current = hold;
            current.x = WIDTH / 2 - current.shape[0].size() / 2;
            current.y = 0;
            hold = temp;
        }
        canHold = false;
        needsRedraw = true;
    }

    // Draws the game screen
    void draw() {
        if (!needsRedraw) return;
        
        system("cls");
        
        // Draw score and level info
        setColor(WHITE);
        cout << "Score: " << score << "  Level: " << level << "  Lines: " << linesCleared << endl << endl;
        
        // Draw playfield
        for (int i = 0; i < VISIBLE_HEIGHT; i++) {
            // Left border
            setColor(WHITE, WHITE);
            cout << "  ";
            
            // Playfield
            for (int j = 0; j < WIDTH; j++) {
                if (board[i][j]) {
                    setColor(BLACK, board[i][j]);
                    cout << "  ";
                } else {
                    setColor(BLACK, BLACK);
                    cout << "  ";
                }
            }
            
            // Right border and hold/next area
            setColor(WHITE, WHITE);
            cout << "  ";
            
            // Right side info
            setColor(WHITE);
            if (i == 1) cout << "  Hold:";
            if (i == 4 && !hold.shape.empty()) {
                for (int j = 0; j < hold.shape[0].size(); j++) {
                    setColor(BLACK, hold.color);
                    cout << "  ";
                }
            }
            
            if (i == 8) cout << "  Next:";
            if (i == 11) {
                for (int j = 0; j < next.shape[0].size(); j++) {
                    setColor(BLACK, next.color);
                    cout << "  ";
                }
            }
            
            cout << endl;
        }
        
        // Bottom border
        setColor(WHITE, WHITE);
        for (int j = 0; j < WIDTH + 2; j++) {
            cout << "  ";
        }
        cout << endl;
        
        // Draw current piece
        for (int i = 0; i < current.shape.size(); i++) {
            for (int j = 0; j < current.shape[i].size(); j++) {
                if (current.shape[i][j]) {
                    int y = current.y + i;
                    int x = current.x + j;
                    if (y >= 0 && y < VISIBLE_HEIGHT) {
                        COORD pos = {static_cast<SHORT>((x + 1) * 2), static_cast<SHORT>(y + 3)};
                        SetConsoleCursorPosition(hConsole, pos);
                        setColor(BLACK, current.color);
                        cout << "  ";
                    }
                }
            }
        }
        
        // Reset cursor and color
        COORD pos = {0, static_cast<SHORT>(VISIBLE_HEIGHT + 5)};
        SetConsoleCursorPosition(hConsole, pos);
        setColor(WHITE);
        
        if (gameOver) {
            cout << "GAME OVER! Final Score: " << score << endl;
            cout << "Press ESC to exit" << endl;
        } else {
            cout << "Controls:" << endl;
            cout << "Left/Right Arrow: Move" << endl;
            cout << "Up Arrow: Rotate" << endl;
            cout << "Down Arrow: Soft Drop" << endl;
            cout << "Space: Hard Drop" << endl;
            cout << "C: Hold" << endl;
            cout << "ESC: Quit" << endl;
        }
        
        needsRedraw = false;
    }

    // Updates game state (piece falling)
    void update() {
        if (gameOver) return;
        
        clock_t now = clock();
        if ((now - lastFall) * 1000 / CLOCKS_PER_SEC >= speed) {
            if (isValidPosition(current.shape, current.x, current.y + 1)) {
                current.y++;
                needsRedraw = true;
            } else {
                lockPiece();
            }
            lastFall = now;
        }
    }

    // Handles keyboard input
    void handleInput() {
        if (_kbhit()) {
            int key = _getch();
            
            if (gameOver) {
                if (key == 27) exit(0);
                return;
            }
            
            switch (key) {
                case 75: movePiece(-1, 0); break; // Left arrow
                case 77: movePiece(1, 0); break;  // Right arrow
                case 80: movePiece(0, 1); break;  // Down arrow (soft drop)
                case 72: rotatePiece(); break;    // Up arrow (rotate)
                case 32: hardDrop(); break;       // Space (hard drop)
                case 'c':
                case 'C': holdPiece(); break;     // C key (hold)
                case 27: exit(0);                // ESC (quit)
            }
        }
    }

    // Returns game over state
    bool isGameOver() const {
        return gameOver;
    }
};

// Function to display welcome screen
void showWelcomeScreen() {
    system("cls");
    setColor(WHITE);
    
    cout << R"(
  _______ _______ _______ _______ 
 |__   _|_   _|_   _|_   __|
    | |     | |     | |     | |   
    | |     | |     | |     | |   
    | |     | |     | |     | |   
    ||     ||     ||     ||   
)" << endl;

    cout << "\nWelcome to TETRIS GAME!\n\n";
    cout << "Instructions:\n";
    cout << "- Use arrow keys to move and rotate pieces\n";
    cout << "- Complete lines to score points\n";
    cout << "- The game gets faster as you level up\n\n";
    cout << "Press any key to continue...";
    
    _getch();
}

// Function to select difficulty level
int selectDifficulty() {
    system("cls");
    setColor(WHITE);
    
    cout << "Select Difficulty Level:\n\n";
    cout << "1. Easy (Slow)\n";
    cout << "2. Medium\n";
    cout << "3. Hard\n";
    cout << "4. Expert (Very Fast)\n\n";
    cout << "Enter choice (1-4): ";
    
    int choice = 1;
    cin >> choice;
    while (choice < 1 || choice > 4) {
        cout << "Invalid choice. Please enter 1-4: ";
        cin >> choice;
    }
    
    return choice;
}

int main() {
    // Set up console window
    system("mode con: cols=40 lines=30");
    SetConsoleTitle("Tetris");
    
    // Show welcome screen
    showWelcomeScreen();
    
    // Select difficulty
    int difficulty = selectDifficulty();
    
    // Create and run game
    Game game(difficulty);
    
    // Main game loop
    while (!game.isGameOver()) {
        game.handleInput();
        game.update();
        game.draw();
        Sleep(16); // ~60 FPS
    }
    
    // Game over loop (wait for ESC)
    while (true) {
        if (_kbhit() && _getch() == 27) {
            break;
        }
        Sleep(100);
    }
    
    return 0;
}
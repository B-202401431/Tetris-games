#include <iostream>
#include <vector>
#include <ctime>
#include <conio.h>
#include <windows.h>
#include <algorithm>
#include <string>

using namespace std;

// Game constants
const int WIDTH = 10;
const int HEIGHT = 20;
const int VISIBLE_HEIGHT = 20;
const int INITIAL_SPEEDS[] = {1000, 800, 600, 400};
const int LEVEL_UP_LINES = 10;
const int SCORE_TABLE[] = {0, 40, 100, 300, 1200};

HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

// Helper functions for cursor control
void gotoxy(int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(hConsole, coord);
}

void hideCursor() {
    CONSOLE_CURSOR_INFO cursorInfo;
    cursorInfo.dwSize = 1;
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
}

void clearScreen() {
    system("cls");
}

// Color definitions
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

void setColor(int textColor, int bgColor = BLACK) {
    SetConsoleTextAttribute(hConsole, (bgColor << 4) | textColor);
}

struct Point {
    int x, y;
    Point(int x = 0, int y = 0) : x(x), y(y) {}
};

struct Tetromino {
    vector<vector<int>> shape;
    vector<Point> wallKicks;
    int color;
    int x, y;
};

vector<Tetromino> tetrominoes = {
    // I piece
    {
        {{0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0}},
        {{0,0}, {-2,0}, {1,0}, {-2,-1}, {1,2}},
        CYAN,
        0, 0
    },
    // O piece
    {
        {{1,1}, {1,1}},
        {{0,0}},
        YELLOW,
        0, 0
    },
    // T piece
    {
        {{0,1,0}, {1,1,1}, {0,0,0}},
        {{0,0}, {-1,0}, {-1,1}, {0,-2}, {-1,-2}},
        MAGENTA,
        0, 0
    },
    // L piece
    {
        {{0,0,1}, {1,1,1}, {0,0,0}},
        {{0,0}, {-1,0}, {-1,1}, {0,-2}, {-1,-2}},
        BRIGHT_BLUE,
        0, 0
    },
    // J piece
    {
        {{1,0,0}, {1,1,1}, {0,0,0}},
        {{0,0}, {-1,0}, {-1,1}, {0,-2}, {-1,-2}},
        BLUE,
        0, 0
    },
    // S piece
    {
        {{0,1,1}, {1,1,0}, {0,0,0}},
        {{0,0}, {-1,0}, {-1,1}, {0,-2}, {-1,-2}},
        GREEN,
        0, 0
    },
    // Z piece
    {
        {{1,1,0}, {0,1,1}, {0,0,0}},
        {{0,0}, {-1,0}, {-1,1}, {0,-2}, {-1,-2}},
        RED,
        0, 0
    }
};

class Game {
private:
    int board[HEIGHT][WIDTH] = {0};
    Tetromino current, next, hold;
    bool canHold = true;
    int score = 0;
    int level = 1;
    int linesCleared = 0;
    int speed;
    bool gameOver = false;
    clock_t lastFall = 0;
    bool needsRedraw = true;
    int difficulty = 1;
    vector<pair<int, int>> changedCells;
    vector<pair<int, int>> previousPiecePositions;
    clock_t lastCheatTime = 0;
    const int CHEAT_COOLDOWN = 5000; // 5 seconds

    void drawCell(int x, int y, int color) {
        gotoxy((x + 1) * 2, y + 2);
        setColor(BLACK, color);
        cout << "  ";
    }

    void clearPreviousPiece() {
        for (auto& pos : previousPiecePositions) {
            int x = pos.first;
            int y = pos.second;
            if (y >= 0 && y < VISIBLE_HEIGHT && x >= 0 && x < WIDTH) {
                if (board[y][x] == 0) {
                    drawCell(x, y, BLACK);
                }
            }
        }
        previousPiecePositions.clear();
    }

    void storeCurrentPiecePositions() {
        for (int i = 0; i < current.shape.size(); i++) {
            for (int j = 0; j < current.shape[i].size(); j++) {
                if (current.shape[i][j]) {
                    int y = current.y + i;
                    int x = current.x + j;
                    previousPiecePositions.emplace_back(x, y);
                }
            }
        }
    }

    void clearHoldArea() {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                gotoxy((WIDTH + 3 + j) * 2, 3 + i);
                setColor(BLACK, BLACK);
                cout << "  ";
            }
        }
    }

    void clearNextArea() {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                gotoxy((WIDTH + 3 + j) * 2, 10 + i);
                setColor(BLACK, BLACK);
                cout << "  ";
            }
        }
    }

    void clearBoard() {
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                board[y][x] = 0;
                drawCell(x, y, BLACK);
            }
        }
        needsRedraw = true;
    }

    void showCheatMessage(const string& message) {
        gotoxy(0, VISIBLE_HEIGHT + 3);
        setColor(BRIGHT_YELLOW, BLACK);
        cout << "CHEAT: " << message << string(20, ' '); // Clear any previous message
        Sleep(1000);
        gotoxy(0, VISIBLE_HEIGHT + 3);
        cout << string(30, ' '); // Clear the message line
    }

public:
    Game(int diff) : difficulty(diff) {
        speed = INITIAL_SPEEDS[difficulty-1];
        srand(time(0));
        next = getRandomTetromino();
        spawnNewPiece();
        initializeDisplay();
    }

    void initializeDisplay() {
        clearScreen();
        setColor(WHITE);
        gotoxy(0, 0);
        cout << "Score: " << score << "  Level: " << level << "  Lines: " << linesCleared;
        
        // Draw borders
        for (int y = 0; y < VISIBLE_HEIGHT + 2; y++) {
            gotoxy(0, y + 1);
            setColor(WHITE, WHITE);
            cout << "  ";
            
            gotoxy((WIDTH + 1) * 2, y + 1);
            setColor(WHITE, WHITE);
            cout << "  ";
        }
        
        gotoxy(0, VISIBLE_HEIGHT + 2);
        for (int j = 0; j < WIDTH + 2; j++) {
            setColor(WHITE, WHITE);
            cout << "  ";
        }
        
        // Draw empty board
        for (int y = 0; y < VISIBLE_HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                drawCell(x, y, BLACK);
            }
        }
        
        // Draw next/hold labels
        gotoxy((WIDTH + 3) * 2, 1);
        setColor(WHITE);
        cout << "Hold:";
        gotoxy((WIDTH + 3) * 2, 8);
        cout << "Next:";
    }

    Tetromino getRandomTetromino() {
        Tetromino t = tetrominoes[rand() % tetrominoes.size()];
        t.x = 0;
        t.y = 0;
        return t;
    }

    void spawnNewPiece() {
        current = next;
        next = getRandomTetromino();
        current.x = WIDTH / 2 - current.shape[0].size() / 2;
        current.y = 0;
        canHold = true;
        needsRedraw = true;
        
        if (!isValidPosition(current.shape, current.x, current.y)) {
            gameOver = true;
        }
    }

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

    void rotatePiece() {
        if (current.shape.size() == 4) {
            vector<vector<int>> rotated(4, vector<int>(4, 0));
            for (int i = 0; i < 4; i++) {
                for (int j = 0; j < 4; j++) {
                    rotated[j][3 - i] = current.shape[i][j];
                }
            }
            
            for (const auto& kick : current.wallKicks) {
                if (isValidPosition(rotated, current.x + kick.x, current.y + kick.y)) {
                    current.shape = rotated;
                    current.x += kick.x;
                    current.y += kick.y;
                    needsRedraw = true;
                    return;
                }
            }
        } else if (current.shape.size() > 1) {
            vector<vector<int>> rotated(current.shape[0].size(), vector<int>(current.shape.size(), 0));
            for (int i = 0; i < current.shape.size(); i++) {
                for (int j = 0; j < current.shape[i].size(); j++) {
                    rotated[j][current.shape.size() - i - 1] = current.shape[i][j];
                }
            }
            
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

    void movePiece(int dx, int dy) {
        if (isValidPosition(current.shape, current.x + dx, current.y + dy)) {
            current.x += dx;
            current.y += dy;
            needsRedraw = true;
        }
    }

    void hardDrop() {
        while (isValidPosition(current.shape, current.x, current.y + 1)) {
            current.y++;
        }
        lockPiece();
    }

    void lockPiece() {
        changedCells.clear();
        
        for (int i = 0; i < current.shape.size(); i++) {
            for (int j = 0; j < current.shape[i].size(); j++) {
                if (current.shape[i][j]) {
                    int y = current.y + i;
                    int x = current.x + j;
                    if (y >= 0) {
                        board[y][x] = current.color;
                        changedCells.emplace_back(x, y);
                    }
                }
            }
        }
        
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
                for (int k = i; k > 0; k--) {
                    for (int j = 0; j < WIDTH; j++) {
                        if (board[k][j] != board[k-1][j]) {
                            board[k][j] = board[k-1][j];
                            changedCells.emplace_back(j, k);
                        }
                    }
                }
                for (int j = 0; j < WIDTH; j++) {
                    board[0][j] = 0;
                    changedCells.emplace_back(j, 0);
                }
                lines++;
                i++;
            }
        }
        
        if (lines > 0) {
            score += SCORE_TABLE[lines] * level;
            linesCleared += lines;
            level = linesCleared / LEVEL_UP_LINES + 1;
            speed = max(100, INITIAL_SPEEDS[difficulty-1] - (level * 50));
            
            gotoxy(0, 0);
            setColor(WHITE);
            cout << "Score: " << score << "  Level: " << level << "  Lines: " << linesCleared;
            cout << string(10, ' ');
        }
        
        spawnNewPiece();
    }

    void holdPiece() {
        if (!canHold) return;
        
        clearHoldArea();
        
        if (hold.shape.empty()) {
            hold = current;
            spawnNewPiece();
        } else {
            Tetromino temp = current;
            current = hold;
            current.x = WIDTH / 2 - current.shape[0].size() / 2;
            current.y = 0;
            hold = temp;
            
            clearHoldArea();
        }
        canHold = false;
        needsRedraw = true;
    }

    void draw() {
        if (!needsRedraw) return;
        
        clearPreviousPiece();
        storeCurrentPiecePositions();
        
        for (auto& cell : changedCells) {
            int x = cell.first;
            int y = cell.second;
            drawCell(x, y, board[y][x]);
        }
        changedCells.clear();
        
        for (int i = 0; i < current.shape.size(); i++) {
            for (int j = 0; j < current.shape[i].size(); j++) {
                if (current.shape[i][j]) {
                    int y = current.y + i;
                    int x = current.x + j;
                    if (y >= 0 && y < VISIBLE_HEIGHT) {
                        drawCell(x, y, current.color);
                    }
                }
            }
        }
        
        clearNextArea();
        for (int i = 0; i < next.shape.size(); i++) {
            for (int j = 0; j < next.shape[i].size(); j++) {
                gotoxy((WIDTH + 3 + j) * 2, 10 + i);
                if (next.shape[i][j]) {
                    setColor(BLACK, next.color);
                    cout << "  ";
                } else {
                    setColor(BLACK, BLACK);
                    cout << "  ";
                }
            }
        }
        
        clearHoldArea();
        if (!hold.shape.empty()) {
            for (int i = 0; i < hold.shape.size(); i++) {
                for (int j = 0; j < hold.shape[i].size(); j++) {
                    gotoxy((WIDTH + 3 + j) * 2, 3 + i);
                    if (hold.shape[i][j]) {
                        setColor(BLACK, hold.color);
                        cout << "  ";
                    } else {
                        setColor(BLACK, BLACK);
                        cout << "  ";
                    }
                }
            }
        }
        
        needsRedraw = false;
    }

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

    void handleInput() {
        static string cheatBuffer;
        
        if (_kbhit()) {
            int key = _getch();
            
            // Cheat code detection
            cheatBuffer += toupper(key);
            if (cheatBuffer.size() > 10) {
                cheatBuffer.clear();
            }
            
            if (cheatBuffer.find("CLEAR") != string::npos && 
                clock() - lastCheatTime > CHEAT_COOLDOWN) {
                clearBoard();
                showCheatMessage("BOARD CLEARED!");
                lastCheatTime = clock();
                cheatBuffer.clear();
                return;
            }
            
            if (gameOver) {
                if (key == 27) exit(0);
                return;
            }
            
            switch (key) {
                case 75: movePiece(-1, 0); break;
                case 77: movePiece(1, 0); break;
                case 80: movePiece(0, 1); break;
                case 72: rotatePiece(); break;
                case 32: hardDrop(); break;
                case 'c':
                case 'C': holdPiece(); break;
                case 27: exit(0);
            }
        }
    }

    bool isGameOver() const {
        return gameOver;
    }

    int getScore() const {
        return score;
    }
};

void showWelcomeScreen() {
    clearScreen();
    setColor(WHITE);
    
    cout << R"(
        _____ _____ _____ _____ _____ _____ 
       |_   _|_   _|_   _|_   _|_   _|_   _|
         | |   | |   | |   | |   | |   | |  
         | |   | |   | |   | |   | |   | |  
         |_|   |_|   |_|   |_|   |_|   |_|  
       )" << endl;
    cout << "\nWelcome to TETRIS GAME!\n\n";
    cout << "Instructions:\n";
    cout << "- Use arrow keys to move and rotate pieces\n";
    cout << "- Complete lines to score points\n";
    cout << "- The game gets faster as you level up\n";
    cout << "- Cheat code: Type 'CLEAR' to clear the board\n\n";
    cout << "Press any key to continue...";
    
    _getch();
}

int selectDifficulty() {
    clearScreen();
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
    system("mode con: cols=40 lines=30");
    SetConsoleTitle("Tetris");
    hideCursor();
    
    showWelcomeScreen();
    int difficulty = selectDifficulty();
    
    clearScreen();
    
    Game game(difficulty);
    
    while (!game.isGameOver()) {
        game.handleInput();
        game.update();
        game.draw();
        Sleep(16);
    }
    
    clearScreen();
    gotoxy(0, 0);
    setColor(WHITE);
    cout << "GAME OVER! Final Score: " << game.getScore() << endl;
    cout << "Press ESC to exit" << endl;
    
    while (true) {
        if (_kbhit() && _getch() == 27) {
            break;
        }
        Sleep(100);
    }
    
    return 0;
}

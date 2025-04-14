#include <iostream>
#include <vector>
#include <unistd.h>
#include <termios.h>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <sys/ioctl.h> // ✅ Needed for FIONREAD on macOS

const int width = 10;
const int height = 20;

char field[height][width];
bool gameOver = false;
int score = 0;
int highScore = 0;

char blockTypes[] = { '#', 'O', '+', 'X', '*' };
char currentBlockChar;

std::vector<std::pair<int, int>> block;
bool paused = false;

// Setup non-blocking input
char getCharInput() {
    struct termios oldt, newt;
    char ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

bool kbhit() {
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    int bytesWaiting;
    ioctl(STDIN_FILENO, FIONREAD, &bytesWaiting); // ✅ works now
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return bytesWaiting > 0;
}

void loadHighScore() {
    std::ifstream file("highscore.txt");
    if (file.is_open()) {
        file >> highScore;
        file.close();
    }
}

void saveHighScore() {
    if (score > highScore) {
        std::ofstream file("highscore.txt");
        if (file.is_open()) {
            file << score;
            file.close();
        }
    }
}

std::string getColoredBlock(char blockChar) {
    std::string colorCode;
    switch (blockChar) {
        case '#': colorCode = "\033[31m"; break; // Red
        case 'O': colorCode = "\033[32m"; break; // Green
        case '+': colorCode = "\033[34m"; break; // Blue
        case 'X': colorCode = "\033[33m"; break; // Yellow
        case '*': colorCode = "\033[35m"; break; // Magenta
        default:  colorCode = "\033[0m";  break;
    }
    return colorCode + blockChar + "\033[0m";
}

void spawnNewBlock() {
    block.clear();
    int x = width / 2;
    int y = 0;
    currentBlockChar = blockTypes[rand() % 5];
    block.push_back({ y, x });
    block.push_back({ y, x + 1 });
    block.push_back({ y + 1, x });
    block.push_back({ y + 1, x + 1 });
}

bool canMove(int dy, int dx) {
    for (auto& [y, x] : block) {
        int newY = y + dy;
        int newX = x + dx;
        if (newX < 0 || newX >= width || newY >= height || field[newY][newX] != ' ')
            return false;
    }
    return true;
}

void moveBlock(int dy, int dx) {
    if (canMove(dy, dx)) {
        for (auto& p : block) {
            p.first += dy;
            p.second += dx;
        }
    }
}

void drawField() {
    system("clear");
    std::cout << "Score: " << score << "   High Score: " << highScore << "\n\n";
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            bool isBlock = false;
            for (auto& [by, bx] : block) {
                if (by == y && bx == x) {
                    std::cout << getColoredBlock(currentBlockChar);
                    isBlock = true;
                    break;
                }
            }
            if (!isBlock)
                std::cout << getColoredBlock(field[y][x]);
        }
        std::cout << "\n";
    }
}

void input() {
    if (kbhit()) {
        char key = getCharInput();
        if (key == 'a') moveBlock(0, -1);
        if (key == 'd') moveBlock(0, 1);
        if (key == 's') moveBlock(1, 0);
        if (key == 'p' || key == 'P') paused = !paused;
    }
}

void clearLines() {
    int linesCleared = 0;
    for (int y = height - 1; y >= 0; y--) {
        bool full = true;
        for (int x = 0; x < width; x++) {
            if (field[y][x] == ' ') {
                full = false;
                break;
            }
        }
        if (full) {
            linesCleared++;
            for (int row = y; row > 0; row--)
                for (int x = 0; x < width; x++)
                    field[row][x] = field[row - 1][x];
            for (int x = 0; x < width; x++)
                field[0][x] = ' ';
            y++; // Recheck current line after shift
        }
    }
    score += linesCleared * 100;
}

void lockBlock() {
    for (auto& [y, x] : block) {
        if (y < 0) {
            gameOver = true;
            return;
        }
        field[y][x] = currentBlockChar;
    }
    clearLines();
    spawnNewBlock();
    if (!canMove(0, 0))
        gameOver = true;
}

void logic() {
    if (canMove(1, 0)) {
        moveBlock(1, 0);
    } else {
        lockBlock();
    }
}

void showTitleScreen() {
    system("clear");
    std::cout << "==========================\n";
    std::cout << "      TETRIS GAME         \n";
    std::cout << "==========================\n";
    std::cout << "Press 'S' to Start\n";
    std::cout << "Press 'E' to Exit\n";

    char choice = getCharInput();
    if (choice == 's' || choice == 'S') return;
    else gameOver = true;
}

void showPauseScreen() {
    std::cout << "\n== PAUSED ==\nPress 'P' to resume, 'E' to exit\n";
    while (true) {
        if (kbhit()) {
            char key = getCharInput();
            if (key == 'p' || key == 'P') {
                paused = false;
                break;
            }
            if (key == 'e' || key == 'E') {
                gameOver = true;
                break;
            }
        }
        usleep(100000);
    }
}

int main() {
    srand(time(0));
    loadHighScore();
    showTitleScreen();

    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
            field[y][x] = ' ';

    spawnNewBlock();

    while (!gameOver) {
        if (paused) {
            showPauseScreen();
        } else {
            drawField();
            input();
            logic();
            usleep(300000); // 300 ms per frame
        }
    }

    saveHighScore();
    std::cout << "\nGame Over! Final Score: " << score << "\n";
    std::cout << "High Score: " << (score > highScore ? score : highScore) << "\n";
    return 0;
}

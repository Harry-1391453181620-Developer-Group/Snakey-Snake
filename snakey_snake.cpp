// snakey_snake.cpp : 定义应用程序的入口点。
//
#define NOMINMAX
#include "framework.h"
#include "snakey_snake.h"
#include <fstream>

#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>
#include <algorithm>

#define MAX_LOADSTRING 100

// 全局变量:
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

constexpr int WINDOW_WIDTH = 800;               // 窗口宽度
constexpr int WINDOW_HEIGHT = 600;              // 窗口高度
constexpr int GRID_ROWS = 20;                   // 网格行数
constexpr int GRID_COLS = 25;                   // 网格列数

struct Point {
    int x;
    int y;
    Point(int x = 0, int y = 0) : x(x), y(y) {}
};

std::vector<Point> snake;
Point food;
int score = 0;
int direction = 0; // 0 - up, 1 - right, 2 - down, 3 - left
bool gameOver = false;
float gridWidth = 0;
float gridHeight = 0;

// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

void InitializeGame(HWND hWnd);
void DrawSnake(HDC hdc);
void DrawFood(HDC hdc);
void MoveSnake();
void PlaceFood();
bool CheckCollision();
void GameOver(HWND hWnd);
void SaveHighScore();
void LoadHighScore();
void DrawScore(HDC hdc);
void ResizeGrid(HWND hWnd);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_SNAKEYSNAKE, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 执行应用程序初始化:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SNAKEYSNAKE));

    MSG msg;

    // 动态计算网格大小
    HWND hWnd = GetActiveWindow();
    ResizeGrid(hWnd);

    InitializeGame(hWnd);
    LoadHighScore();

    // 主消息循环:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    SaveHighScore();
    return (int)msg.wParam;
}

//
//  函数: MyRegisterClass()
//
//  目标: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SNAKEYSNAKE));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = CreateSolidBrush(RGB(173, 216, 230)); // 背景颜色调整为淡蓝色
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_SNAKEYSNAKE);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目标: 保存实例句柄并创建主窗口
//
//   注释:
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // 将实例句柄存储在全局变量中

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    SetTimer(hWnd, 1, 100, NULL); // 设置定时器，每100毫秒触发一次

    return TRUE;
}

void InitializeGame(HWND hWnd)
{
    srand(static_cast<unsigned int>(time(0))); // 设置随机种子

    snake.clear();
    RECT rect;
    GetClientRect(hWnd, &rect);
    snake.push_back({ rand() % GRID_COLS, rand() % GRID_ROWS }); // 随机初始位置
    direction = rand() % 4; // 随机初始方向
    score = 0;
    gameOver = false;
    PlaceFood();
}

void ResizeGrid(HWND hWnd)
{
    RECT rect;
    GetClientRect(hWnd, &rect);
    gridWidth = static_cast<float>(rect.right) / GRID_COLS;
    gridHeight = static_cast<float>(rect.bottom) / GRID_ROWS;
}

void PlaceFood()
{
    food.x = rand() % GRID_COLS;
    food.y = rand() % GRID_ROWS;
}

void DrawSnake(HDC hdc)
{
    HPEN hPen = CreatePen(PS_SOLID, 3, RGB(0, 0, 0)); // 固定颜色
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0)); // 固定颜色
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);

    for (const auto& segment : snake) {
        int centerX = static_cast<int>(segment.x * gridWidth + gridWidth / 2);
        int centerY = static_cast<int>(segment.y * gridHeight + gridHeight / 2);
        int radius = static_cast<int>(std::min(gridWidth, gridHeight) / 2 - 3); // 绘制小一些的正圆形
        Ellipse(hdc, centerX - radius, centerY - radius, centerX + radius, centerY + radius);
    }

    SelectObject(hdc, hOldPen);
    SelectObject(hdc, hOldBrush);
    DeleteObject(hPen);
    DeleteObject(hBrush);
}

void DrawFood(HDC hdc)
{
    HPEN hPen = CreatePen(PS_SOLID, 3, RGB(255, 0, 0)); // 固定颜色
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    HBRUSH hBrush = CreateSolidBrush(RGB(255, 0, 0)); // 固定颜色
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);

    int centerX = static_cast<int>(food.x * gridWidth + gridWidth / 2);
    int centerY = static_cast<int>(food.y * gridHeight + gridHeight / 2);
    int radius = static_cast<int>(std::min(gridWidth, gridHeight) / 2 - 3); // 绘制小一些的正圆形

    Ellipse(hdc, centerX - radius, centerY - radius, centerX + radius, centerY + radius);

    SelectObject(hdc, hOldPen);
    SelectObject(hdc, hOldBrush);
    DeleteObject(hPen);
    DeleteObject(hBrush);
}

void MoveSnake()
{
    if (gameOver) return;

    Point head = snake.front();
    Point newHead = head;

    switch (direction) {
    case 0: // 上
        newHead.y--;
        if (newHead.y < 0) newHead.y = GRID_ROWS - 1;
        break;
    case 1: // 右
        newHead.x++;
        if (newHead.x >= GRID_COLS) newHead.x = 0;
        break;
    case 2: // 下
        newHead.y++;
        if (newHead.y >= GRID_ROWS) newHead.y = 0;
        break;
    case 3: // 左
        newHead.x--;
        if (newHead.x < 0) newHead.x = GRID_COLS - 1;
        break;
    }

    if (CheckCollision()) {
        gameOver = true;
        GameOver(GetActiveWindow());
        return;
    }

    snake.insert(snake.begin(), newHead);

    if (newHead.x == food.x && newHead.y == food.y) {
        score++;
        PlaceFood();
    }
    else {
        snake.pop_back();
    }
}

bool CheckCollision()
{
    Point head = snake.front();
    for (size_t i = 1; i < snake.size(); ++i) {
        if (head.x == snake[i].x && head.y == snake[i].y) {
            return true;
        }
    }
    return false;
}

void GameOver(HWND hWnd)
{
    MessageBox(hWnd, L"Game Over", L"Game Over", MB_OK | MB_ICONINFORMATION);
}

void SaveHighScore()
{
    int highScore = 0;
    std::ifstream file("highscore.txt");
    file >> highScore;
    file.close();

    if (score > highScore) {
        std::ofstream outFile("highscore.txt");
        outFile << score;
        outFile.close();
    }
}

void LoadHighScore()
{
    std::ifstream file("highscore.txt");
    if (file.is_open()) {
        file >> score;
        file.close();
    }
    else {
        std::ofstream file("highscore.txt");
        file << 0;
        file.close();
    }
}

void DrawScore(HDC hdc)
{
    HFONT hFont = CreateFont(24, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");

    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    SetTextColor(hdc, RGB(0, 0, 0));
    SetBkMode(hdc, TRANSPARENT);

    std::wstring scoreText = L"Score: " + std::to_wstring(score);
    TextOut(hdc, 10, 10, scoreText.c_str(), scoreText.size());

    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目标: 处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//  WM_KEYDOWN  - 处理键盘输入
//  WM_TIMER    - 处理定时器消息
//  WM_SIZE     - 处理窗口大小改变
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // 分析菜单选择:
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        // 在此处添加使用 hdc 的任何绘图代码...
        DrawSnake(hdc);
        DrawFood(hdc);
        DrawScore(hdc); // 添加这一行来绘制分数
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_KEYDOWN:
        switch (wParam) {
        case VK_UP:
            if (direction != 2) direction = 0; // 防止反向移动
            break;
        case VK_DOWN:
            if (direction != 0) direction = 2;
            break;
        case VK_LEFT:
            if (direction != 1) direction = 3;
            break;
        case VK_RIGHT:
            if (direction != 3) direction = 1;
            break;
        }
        break;
    case WM_TIMER:
        MoveSnake();
        InvalidateRect(hWnd, NULL, TRUE); // 强制重绘窗口
        break;
    case WM_SIZE:
        ResizeGrid(hWnd);
        InvalidateRect(hWnd, NULL, TRUE); // 强制重绘窗口
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

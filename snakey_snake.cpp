#define NOMINMAX
#define SDL_MAIN_HANDLED

#include <Windows.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <deque>
#include <vector>
#include <string>
#include <fstream>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <algorithm>

constexpr int WINDOW_WIDTH = 1000;
constexpr int WINDOW_HEIGHT = 800;

constexpr int GRID_COLS = 25;
constexpr int GRID_ROWS = 20;

constexpr int CELL_SIZE = 32;

constexpr int BOARD_OFFSET_X = 100;
constexpr int BOARD_OFFSET_Y = 80;

constexpr float MOVE_INTERVAL = 0.10f;

struct Point
{
    int x;
    int y;

    bool operator==(const Point& other) const
    {
        return x == other.x && y == other.y;
    }
};

struct Particle
{
    float x;
    float y;

    float vx;
    float vy;

    float life;
    float maxLife;

    int radius;
};

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
TTF_Font* font = nullptr;

std::deque<Point> snake;

std::vector<Particle> particles;

Point food;

int direction = 1;

bool gameOver = false;

int score = 0;
int highScore = 0;

float accumulator = 0.0f;

Point previousHead;
Point currentHead;

bool showGameOverText = false;
float gameOverAlpha = 0.0f;

//============================================================
// Utility
//============================================================

float lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

SDL_FPoint GridToPixel(const Point& p)
{
    return {
        BOARD_OFFSET_X + p.x * CELL_SIZE + CELL_SIZE / 2.0f,
        BOARD_OFFSET_Y + p.y * CELL_SIZE + CELL_SIZE / 2.0f
    };
}

void SaveHighScore()
{
    std::ofstream out("highscore.txt");

    if (out.is_open())
    {
        out << highScore;
    }
}

void LoadHighScore()
{
    std::ifstream in("highscore.txt");

    if (in.is_open())
    {
        in >> highScore;
    }
    else
    {
        highScore = 0;
    }
}

bool SnakeContains(const Point& p)
{
    for (const auto& s : snake)
    {
        if (s == p)
        {
            return true;
        }
    }

    return false;
}

void PlaceFood()
{
    while (true)
    {
        Point p;

        p.x = rand() % GRID_COLS;
        p.y = rand() % GRID_ROWS;

        if (!SnakeContains(p))
        {
            food = p;
            return;
        }
    }
}

void InitializeGame()
{
    snake.clear();
    particles.clear();

    Point start;

    start.x = GRID_COLS / 2;
    start.y = GRID_ROWS / 2;

    snake.push_back(start);

    previousHead = start;
    currentHead = start;

    direction = 1;

    score = 0;

    gameOver = false;

    gameOverAlpha = 0.0f;

    showGameOverText = false;

    PlaceFood();
}

bool CheckCollision(const Point& p)
{
    for (const auto& s : snake)
    {
        if (p == s)
        {
            return true;
        }
    }

    return false;
}

//============================================================
// Particle System
//============================================================

void SpawnEatParticles(const Point& p)
{
    SDL_FPoint pixel = GridToPixel(p);

    for (int i = 0; i < 25; ++i)
    {
        float angle =
            ((float)rand() / RAND_MAX) * 6.28318f;

        float speed =
            60.0f +
            ((float)rand() / RAND_MAX) * 140.0f;

        Particle particle;

        particle.x = pixel.x;
        particle.y = pixel.y;

        particle.vx = cos(angle) * speed;
        particle.vy = sin(angle) * speed;

        particle.life = 0.6f;
        particle.maxLife = 0.6f;

        particle.radius =
            2 + rand() % 4;

        particles.push_back(particle);
    }
}

void UpdateParticles(float deltaTime)
{
    for (auto& p : particles)
    {
        p.life -= deltaTime;

        p.x += p.vx * deltaTime;
        p.y += p.vy * deltaTime;

        p.vy += 120.0f * deltaTime;

        p.vx *= 0.98f;
        p.vy *= 0.98f;
    }

    particles.erase(
        std::remove_if(
            particles.begin(),
            particles.end(),
            [](const Particle& p)
            {
                return p.life <= 0.0f;
            }),
        particles.end());
}

//============================================================
// Circle Rendering
//============================================================

void DrawFilledCircle(int cx, int cy, int radius)
{
    for (int w = -radius; w <= radius; ++w)
    {
        for (int h = -radius; h <= radius; ++h)
        {
            if (w * w + h * h <= radius * radius)
            {
                SDL_RenderDrawPoint(renderer, cx + w, cy + h);
            }
        }
    }
}

//============================================================
// Text Rendering
//============================================================

void DrawText(
    const std::string& text,
    int x,
    int y,
    SDL_Color color)
{
    SDL_Surface* surface =
        TTF_RenderUTF8_Blended(
            font,
            text.c_str(),
            color);

    SDL_Texture* texture =
        SDL_CreateTextureFromSurface(
            renderer,
            surface);

    SDL_Rect rect;

    rect.x = x;
    rect.y = y;
    rect.w = surface->w;
    rect.h = surface->h;

    SDL_RenderCopy(
        renderer,
        texture,
        nullptr,
        &rect);

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

//============================================================
// Board
//============================================================

void DrawBoard()
{
    for (int y = 0; y < GRID_ROWS; ++y)
    {
        for (int x = 0; x < GRID_COLS; ++x)
        {
            bool dark = (x + y) % 2;

            if (dark)
            {
                SDL_SetRenderDrawColor(
                    renderer,
                    120,
                    190,
                    120,
                    255);
            }
            else
            {
                SDL_SetRenderDrawColor(
                    renderer,
                    140,
                    210,
                    140,
                    255);
            }

            SDL_Rect rect;

            rect.x =
                BOARD_OFFSET_X + x * CELL_SIZE;

            rect.y =
                BOARD_OFFSET_Y + y * CELL_SIZE;

            rect.w = CELL_SIZE;
            rect.h = CELL_SIZE;

            SDL_RenderFillRect(renderer, &rect);
        }
    }
}

//============================================================
// Snake
//============================================================

void DrawSnake(float alpha)
{
    for (size_t i = 0; i < snake.size(); ++i)
    {
        Point p = snake[i];

        SDL_FPoint pixel;

        if (i == 0)
        {
            SDL_FPoint prevPixel =
                GridToPixel(previousHead);

            SDL_FPoint currPixel =
                GridToPixel(currentHead);

            pixel.x =
                lerp(prevPixel.x, currPixel.x, alpha);

            pixel.y =
                lerp(prevPixel.y, currPixel.y, alpha);
        }
        else
        {
            pixel = GridToPixel(p);
        }

        int radius = CELL_SIZE / 2 - 3;

        // shadow
        SDL_SetRenderDrawColor(
            renderer,
            0,
            0,
            0,
            70);

        DrawFilledCircle(
            (int)pixel.x + 3,
            (int)pixel.y + 3,
            radius);

        Uint8 shade =
            static_cast<Uint8>(
                std::max(40, 220 - (int)i * 8));

        SDL_SetRenderDrawColor(
            renderer,
            30,
            shade,
            30,
            255);

        if (i == 0)
        {
            SDL_SetRenderDrawColor(
                renderer,
                20,
                255,
                20,
                255);
        }

        DrawFilledCircle(
            (int)pixel.x,
            (int)pixel.y,
            radius);

        // eyes
        if (i == 0)
        {
            SDL_SetRenderDrawColor(
                renderer,
                255,
                255,
                255,
                255);

            int eyeOffsetX = 0;
            int eyeOffsetY = 0;

            switch (direction)
            {
            case 0:
                eyeOffsetY = -5;
                break;

            case 1:
                eyeOffsetX = 5;
                break;

            case 2:
                eyeOffsetY = 5;
                break;

            case 3:
                eyeOffsetX = -5;
                break;
            }

            DrawFilledCircle(
                (int)pixel.x - 5 + eyeOffsetX,
                (int)pixel.y - 5 + eyeOffsetY,
                3);

            DrawFilledCircle(
                (int)pixel.x + 5 + eyeOffsetX,
                (int)pixel.y - 5 + eyeOffsetY,
                3);
        }
    }
}

//============================================================
// Food
//============================================================

void DrawFood()
{
    SDL_FPoint pixel = GridToPixel(food);

    SDL_SetRenderDrawColor(
        renderer,
        0,
        0,
        0,
        80);

    DrawFilledCircle(
        (int)pixel.x + 3,
        (int)pixel.y + 3,
        CELL_SIZE / 2 - 5);

    SDL_SetRenderDrawColor(
        renderer,
        220,
        40,
        40,
        255);

    DrawFilledCircle(
        (int)pixel.x,
        (int)pixel.y,
        CELL_SIZE / 2 - 5);

    SDL_SetRenderDrawColor(
        renderer,
        30,
        180,
        30,
        255);

    SDL_Rect leaf;

    leaf.x = (int)pixel.x + 3;
    leaf.y = (int)pixel.y - 16;
    leaf.w = 8;
    leaf.h = 4;

    SDL_RenderFillRect(renderer, &leaf);

    SDL_SetRenderDrawColor(
        renderer,
        120,
        70,
        20,
        255);

    SDL_RenderDrawLine(
        renderer,
        (int)pixel.x,
        (int)pixel.y - 8,
        (int)pixel.x,
        (int)pixel.y - 15);
}

//============================================================
// Particles
//============================================================

void DrawParticles()
{
    SDL_SetRenderDrawBlendMode(
        renderer,
        SDL_BLENDMODE_BLEND);

    for (const auto& p : particles)
    {
        float t = p.life / p.maxLife;

        Uint8 alpha =
            (Uint8)(255 * t);

        SDL_SetRenderDrawColor(
            renderer,
            255,
            180,
            40,
            alpha);

        DrawFilledCircle(
            (int)p.x,
            (int)p.y,
            (int)(p.radius * t));
    }
}

//============================================================
// UI
//============================================================

void DrawUI()
{
    DrawText(
        "Score: " + std::to_string(score),
        40,
        20,
        { 255,255,255,255 });

    DrawText(
        "High Score: " + std::to_string(highScore),
        320,
        20,
        { 255,255,100,255 });
}

//============================================================
// Game Over Overlay
//============================================================

void DrawGameOverOverlay()
{
    if (!showGameOverText)
    {
        return;
    }

    gameOverAlpha += 3.0f;

    if (gameOverAlpha > 180)
    {
        gameOverAlpha = 180;
    }

    SDL_SetRenderDrawBlendMode(
        renderer,
        SDL_BLENDMODE_BLEND);

    SDL_SetRenderDrawColor(
        renderer,
        0,
        0,
        0,
        (Uint8)gameOverAlpha);

    SDL_Rect overlay;

    overlay.x = 0;
    overlay.y = 0;
    overlay.w = WINDOW_WIDTH;
    overlay.h = WINDOW_HEIGHT;

    SDL_RenderFillRect(renderer, &overlay);

    DrawText(
        "GAME OVER",
        WINDOW_WIDTH / 2 - 140,
        WINDOW_HEIGHT / 2 - 100,
        { 255,80,80,255 });

    DrawText(
        "Press R To Restart",
        WINDOW_WIDTH / 2 - 170,
        WINDOW_HEIGHT / 2,
        { 255,255,255,255 });
}

//============================================================
// Update
//============================================================

void UpdateSnake()
{
    if (gameOver)
    {
        return;
    }

    previousHead = snake.front();

    Point newHead = snake.front();

    switch (direction)
    {
    case 0:
        newHead.y--;
        break;

    case 1:
        newHead.x++;
        break;

    case 2:
        newHead.y++;
        break;

    case 3:
        newHead.x--;
        break;
    }

    if (newHead.x < 0)
    {
        newHead.x = GRID_COLS - 1;
    }

    if (newHead.x >= GRID_COLS)
    {
        newHead.x = 0;
    }

    if (newHead.y < 0)
    {
        newHead.y = GRID_ROWS - 1;
    }

    if (newHead.y >= GRID_ROWS)
    {
        newHead.y = 0;
    }

    if (CheckCollision(newHead))
    {
        gameOver = true;

        showGameOverText = true;

        SaveHighScore();

        return;
    }

    snake.push_front(newHead);

    currentHead = newHead;

    if (newHead == food)
    {
        SpawnEatParticles(food);

        score++;

        if (score > highScore)
        {
            highScore = score;
        }

        PlaceFood();
    }
    else
    {
        snake.pop_back();
    }
}

//============================================================
// Main
//============================================================

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{
    srand((unsigned int)time(nullptr));

    SDL_Init(SDL_INIT_VIDEO);

    TTF_Init();

    window = SDL_CreateWindow(
        "Enhanced Snake SDL2",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN);

    renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED |
        SDL_RENDERER_PRESENTVSYNC);

    font = TTF_OpenFont(
        "C:/Windows/Fonts/arial.ttf",
        36);

    if (!font)
    {
        return -1;
    }

    LoadHighScore();

    InitializeGame();

    bool running = true;

    Uint64 previousCounter =
        SDL_GetPerformanceCounter();

    while (running)
    {
        Uint64 currentCounter =
            SDL_GetPerformanceCounter();

        float deltaTime =
            (float)(currentCounter - previousCounter)
            / SDL_GetPerformanceFrequency();

        previousCounter = currentCounter;

        accumulator += deltaTime;

        UpdateParticles(deltaTime);

        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }

            if (event.type == SDL_KEYDOWN)
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_UP:

                    if (direction != 2)
                    {
                        direction = 0;
                    }

                    break;

                case SDLK_RIGHT:

                    if (direction != 3)
                    {
                        direction = 1;
                    }

                    break;

                case SDLK_DOWN:

                    if (direction != 0)
                    {
                        direction = 2;
                    }

                    break;

                case SDLK_LEFT:

                    if (direction != 1)
                    {
                        direction = 3;
                    }

                    break;

                case SDLK_r:

                    if (gameOver)
                    {
                        InitializeGame();

                        accumulator = 0.0f;

                        particles.clear();
                    }

                    break;
                }
            }
        }

        while (accumulator >= MOVE_INTERVAL)
        {
            UpdateSnake();

            accumulator -= MOVE_INTERVAL;
        }

        float alpha =
            accumulator / MOVE_INTERVAL;

        SDL_SetRenderDrawColor(
            renderer,
            25,
            25,
            35,
            255);

        SDL_RenderClear(renderer);

        DrawBoard();

        DrawFood();

        DrawParticles();

        DrawSnake(alpha);

        DrawUI();

        DrawGameOverOverlay();

        SDL_RenderPresent(renderer);
    }

    SaveHighScore();

    TTF_CloseFont(font);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    TTF_Quit();
    SDL_Quit();

    return 0;
}

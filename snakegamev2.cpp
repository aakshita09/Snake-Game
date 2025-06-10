#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include <deque>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int GRID_SIZE = 20;
const int GRID_WIDTH = SCREEN_WIDTH / GRID_SIZE;
const int GRID_HEIGHT = SCREEN_HEIGHT / GRID_SIZE;

class Snake {
public:
    enum Direction { UP, DOWN, LEFT, RIGHT };

    Snake(int startX, int startY, Direction startDirection) {
        // Initialize snake at the specified position
        body.push_back({startX, startY});
        direction = startDirection;
    }

    void move() {
        SDL_Point newHead = body.front();

        switch (direction) {
            case UP: newHead.y--; break;
            case DOWN: newHead.y++; break;
            case LEFT: newHead.x--; break;
            case RIGHT: newHead.x++; break;
        }

        // Wrap around the screen boundaries
        if (newHead.x < 0) newHead.x = GRID_WIDTH - 1;
        if (newHead.x >= GRID_WIDTH) newHead.x = 0;
        if (newHead.y < 0) newHead.y = GRID_HEIGHT - 1;
        if (newHead.y >= GRID_HEIGHT) newHead.y = 0;

        // Insert new head at the front
        body.push_front(newHead);

        // Remove tail if no food eaten
        if (!growing) {
            body.pop_back();
        }
        growing = false;
    }

    void grow() {
        growing = true;
    }

    bool checkSelfCollision() {
        SDL_Point head = body.front();
        for (auto it = body.begin() + 1; it != body.end(); ++it) {
            if (head.x == it->x && head.y == it->y) {
                return true;
            }
        }
        return false;
    }

    std::deque<SDL_Point>& getBody() { return body; }
    Direction getDirection() { return direction; }
    void setDirection(Direction newDir) {
        // Prevent 180-degree turns
        if ((direction == UP && newDir == DOWN) ||
            (direction == DOWN && newDir == UP) ||
            (direction == LEFT && newDir == RIGHT) ||
            (direction == RIGHT && newDir == LEFT)) {
            return;
        }
        direction = newDir;
    }

private:
    std::deque<SDL_Point> body;
    Direction direction;
    bool growing = false;
};

class Game {
public:
    Game() {
        // Initialize SDL
        SDL_Init(SDL_INIT_VIDEO);
        TTF_Init();

        // Create window and renderer
        window = SDL_CreateWindow("Snake Game", 
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
            SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

        // Load font
        font = TTF_OpenFont("arial.ttf", 24);
        if (!font) {
            throw std::runtime_error("Failed to load font: " + std::string(TTF_GetError()));
        }

        // Seed random number generator
        srand(time(nullptr));

        // Place initial food
        placeFood();
        placeBonusFood();
    }

    ~Game() {
        // Cleanup SDL resources
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
    }

    void run() {
        bool quit = false;
        SDL_Event e;
        Uint32 frameStart, frameTime;
        const int FPS = 10; // Game speed
        const int frameDelay = 1000 / FPS;

        while (!quit) {
            frameStart = SDL_GetTicks();

            // Handle events
            while (SDL_PollEvent(&e) != 0) {
                if (e.type == SDL_QUIT) {
                    quit = true;
                }
                handleInput(e);
            }

            // Game logic
            update();

            // Render
            render();

            // Frame timing
            frameTime = SDL_GetTicks() - frameStart;
            if (frameDelay > frameTime) {
                SDL_Delay(frameDelay - frameTime);
            }
        }
    }

private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font;
    Snake snake1{GRID_WIDTH / 4, GRID_HEIGHT / 2, Snake::RIGHT};
    Snake snake2{3 * GRID_WIDTH / 4, GRID_HEIGHT / 2, Snake::LEFT};
    SDL_Point food;
    SDL_Point bonusFood;
    int score1 = 0, score2 = 0;
    int bonusFoodTimer = 0;

    void handleInput(SDL_Event& e) {
        if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_w: snake1.setDirection(Snake::UP); break;
                case SDLK_s: snake1.setDirection(Snake::DOWN); break;
                case SDLK_a: snake1.setDirection(Snake::LEFT); break;
                case SDLK_d: snake1.setDirection(Snake::RIGHT); break;
                case SDLK_UP: snake2.setDirection(Snake::UP); break;
                case SDLK_DOWN: snake2.setDirection(Snake::DOWN); break;
                case SDLK_LEFT: snake2.setDirection(Snake::LEFT); break;
                case SDLK_RIGHT: snake2.setDirection(Snake::RIGHT); break;
            }
        }
    }

    void update() {
        snake1.move();
        snake2.move();

        // Check for food collision for both snakes
        checkFoodCollision(snake1, score1);
        checkFoodCollision(snake2, score2);

        // Check for bonus food collision
        SDL_Point head1 = snake1.getBody().front();
        SDL_Point head2 = snake2.getBody().front();
        if (head1.x == bonusFood.x && head1.y == bonusFood.y) {
            snake1.grow();
            score1 += 2;  // Bonus food gives more points
            placeBonusFood();
        }
        if (head2.x == bonusFood.x && head2.y == bonusFood.y) {
            snake2.grow();
            score2 += 2;  // Bonus food gives more points
            placeBonusFood();
        }

        // Check for self-collision for both snakes
        if (snake1.checkSelfCollision()) {
            resetGame("Player 1 (Green) died by self-collision. Player 2 (Blue) wins!");
            return;
        }
        if (snake2.checkSelfCollision()) {
            resetGame("Player 2 (Blue) died by self-collision. Player 1 (Green) wins!");
            return;
        }

        // Check for collision between the snakes
        for (const auto& segment : snake1.getBody()) {
            if (head2.x == segment.x && head2.y == segment.y) {
                resetGame("Player 2 (Blue) died by colliding into Player 1 (Green). Player 1 wins!");
                return;
            }
        }
        for (const auto& segment : snake2.getBody()) {
            if (head1.x == segment.x && head1.y == segment.y) {
                resetGame("Player 1 (Green) died by colliding into Player 2 (Blue). Player 2 wins!");
                return;
            }
        }

        // Bonus food timer
        bonusFoodTimer++;
        if (bonusFoodTimer > 1000) { // Show bonus food after some time
            placeBonusFood();
            bonusFoodTimer = 0;
        }
    }

    void checkFoodCollision(Snake& snake, int& score) {
        SDL_Point head = snake.getBody().front();
        if (head.x == food.x && head.y == food.y) {
            snake.grow();
            score++;
            placeFood();
        }
    }

    void render() {
        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw snakes
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        for (const auto& segment : snake1.getBody()) {
            SDL_Rect rect = {
                segment.x * GRID_SIZE, 
                segment.y * GRID_SIZE, 
                GRID_SIZE, 
                GRID_SIZE
            };
            SDL_RenderFillRect(renderer, &rect);
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        for (const auto& segment : snake2.getBody()) {
            SDL_Rect rect = {
                segment.x * GRID_SIZE, 
                segment.y * GRID_SIZE, 
                GRID_SIZE, 
                GRID_SIZE
            };
            SDL_RenderFillRect(renderer, &rect);
        }

        // Draw food
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_Rect foodRect = {
            food.x * GRID_SIZE, 
            food.y * GRID_SIZE, 
            GRID_SIZE, 
            GRID_SIZE
        };
        SDL_RenderFillRect(renderer, &foodRect);

        // Draw bonus food
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        SDL_Rect bonusFoodRect = {
            bonusFood.x * GRID_SIZE, 
            bonusFood.y * GRID_SIZE, 
            GRID_SIZE, 
            GRID_SIZE
        };
        SDL_RenderFillRect(renderer, &bonusFoodRect);

        // Draw scores
        renderScore();

        // Present renderer
        SDL_RenderPresent(renderer);
    }

    void renderScore() {
        // Render score text
        SDL_Color white = {255, 255, 255, 255};
        std::string scoreText = "Player 1: " + std::to_string(score1) + " | Player 2: " + std::to_string(score2);
        SDL_Surface* surface = TTF_RenderText_Solid(font, scoreText.c_str(), white);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

        SDL_Rect textRect = {10, 10, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, nullptr, &textRect);

        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }

    void placeFood() {
        // Place food in a random unoccupied grid cell
        while (true) {
            food.x = rand() % GRID_WIDTH;
            food.y = rand() % GRID_HEIGHT;

            bool validPosition = true;
            for (const auto& segment : snake1.getBody()) {
                if (food.x == segment.x && food.y == segment.y) {
                    validPosition = false;
                    break;
                }
            }
            for (const auto& segment : snake2.getBody()) {
                if (food.x == segment.x && food.y == segment.y) {
                    validPosition = false;
                    break;
                }
            }

            if (validPosition) break;
        }
    }

    void placeBonusFood() {
        // Place bonus food in a random unoccupied grid cell
        while (true) {
            bonusFood.x = rand() % GRID_WIDTH;
            bonusFood.y = rand() % GRID_HEIGHT;

            bool validPosition = true;
            for (const auto& segment : snake1.getBody()) {
                if (bonusFood.x == segment.x && bonusFood.y == segment.y) {
                    validPosition = false;
                    break;
                }
            }
            for (const auto& segment : snake2.getBody()) {
                if (bonusFood.x == segment.x && bonusFood.y == segment.y) {
                    validPosition = false;
                    break;
                }
            }

            if (validPosition) break;
        }
    }

    void renderMessage(const std::string& message) {
        SDL_Color white = {255, 255, 255, 255};
        SDL_Surface* surface = TTF_RenderText_Solid(font, message.c_str(), white);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

        SDL_Rect textRect = {
            SCREEN_WIDTH / 2 - surface->w / 2,
            SCREEN_HEIGHT / 2 - surface->h / 2,
            surface->w,
            surface->h
        };

        // Render message to the screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);  // Clear screen
        SDL_RenderCopy(renderer, texture, nullptr, &textRect);
        SDL_RenderPresent(renderer); // Update the screen

        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }
    void resetGame(const std::string& message) {
    renderMessage(message); // Show the message on the screen
    SDL_Delay(3000);        // Pause for 3 seconds to let the players read the message

    // Reset snakes and scores
    snake1 = Snake(GRID_WIDTH / 4, GRID_HEIGHT / 2, Snake::RIGHT);
    snake2 = Snake(3 * GRID_WIDTH / 4, GRID_HEIGHT / 2, Snake::LEFT);
    score1 = 0;
    score2 = 0;
    placeFood();
    placeBonusFood();
    }

};

#ifdef _WIN32
#include <windows.h>
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    Game game;
    game.run();
    return 0;
}
#else
int main() {
    Game game;
    game.Run();
    return 0;
}
#endif
//g++ snakegamev2.cpp -o snakegamev2 -I"SDL2/include" -L"SDL2/lib" -lSDL2main -lSDL2 -lSDL2_ttf -lmingw32 -mwindows
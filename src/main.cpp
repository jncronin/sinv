#include <SDL.h>
#include <vector>
#include <list>

float player_x = 320.0f;
const float player_speed = 80.0f;   // per second
const int player_y = 440;
const int player_w = 64;
const int player_h = 32;
const int baddy_w = 24;
const int baddy_h = 24;
const int n_baddies_x = 10;
const int n_baddies_y = 5;
const int baddy_advance = 12;

bool game_is_still_running = true;

bool keypress[256] = { false };

Uint64 last_fire_time = 0;
const Uint64 fire_delay = 500;

struct bullet
{
    int x;
    float y;
    float speed;
    bool is_player;

    SDL_Rect r() const
    {
        SDL_Rect ret;
        ret.x = x - 1;
        ret.y = y - 4;
        ret.w = 2;
        ret.h = 8;
        return ret;
    }
};

std::list<bullet> bullets;

struct baddy
{
    float x;
    float y;
    Uint64 last_fire_time;

    SDL_Rect r() const
    {
        SDL_Rect ret;
        ret.x = x - baddy_w / 2;
        ret.y = y - baddy_h / 2;
        ret.w = baddy_w;
        ret.h = baddy_h;
        return ret;
    }
};

std::list<baddy> baddies;

float baddy_speed = 20.0f;

SDL_Rect pr()
{
    SDL_Rect ret;
    ret.x = player_x - player_w / 2;
    ret.y = player_y - player_h / 2;
    ret.w = player_w;
    ret.h = player_h;
    return ret;
}

Uint64 lticks;
bool baddy_move_left;

void reset_game()
{
    // Add baddies
    lticks = SDL_GetTicks64();

    baddy_move_left = true;

    baddies.clear();
    bullets.clear();

    int x_start = 320 - (n_baddies_x - 1) * baddy_w;
    int y_start = 40;

    for (int y = 0; y < n_baddies_y; y++)
    {
        for (int x = 0; x < n_baddies_x; x++)
        {
            baddy b;
            b.x = x_start + x * baddy_w * 2;
            b.y = y_start + y * baddy_h * 2;
            b.last_fire_time = lticks;
            baddies.push_front(b);
        }
    }

    player_x = 320;
    baddy_speed = 20.0f;
}

int main(int argc, char *argv[])
{
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *window = SDL_CreateWindow(
        "SDL2Test",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        640,
        480,
        0
    );

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    reset_game();

    while (game_is_still_running)
    {
        auto cticks = SDL_GetTicks64();
        auto ticks = cticks - lticks;
        lticks = cticks;

        // Event handling
        bool fire_pressed = false;

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_QUIT:
                    game_is_still_running = false;
                    break;

                case SDL_KEYDOWN:
                    keypress[event.key.keysym.scancode] = true;

                    if (event.key.keysym.scancode == SDL_SCANCODE_LCTRL ||
                        event.key.keysym.scancode == SDL_SCANCODE_A)
                        fire_pressed = true;
                    
                    break;

                case SDL_KEYUP:
                    keypress[event.key.keysym.scancode] = false;
                    break;
            }
        }

        // Game logic
        if (keypress[SDL_SCANCODE_LEFT])
        {
            player_x -= (float)ticks * player_speed / 1000.0f;
        }
        if (keypress[SDL_SCANCODE_RIGHT])
        {
            player_x += (float)ticks * player_speed / 1000.0f;
        }
        auto prect = pr();

        if (fire_pressed && (cticks - last_fire_time) >= fire_delay)
        {
            // spawn new bullet
            bullet b;
            b.x = player_x;
            b.y = player_y;
            b.speed = -200.0;
            b.is_player = true;
            bullets.push_front(b);

            last_fire_time = cticks;
        }

        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(renderer);

        // Get baddy extents
        int baddy_left = 640;
        int baddy_right = 0;
        int baddy_top = 480;
        int baddy_bottom = 0;
        for (const auto& b : baddies)
        {
            auto br = b.r();
            if (br.x < baddy_left) baddy_left = br.x;
            if (br.x + br.w > baddy_right) baddy_right = br.x + br.w;
            if (br.y < baddy_top) baddy_top = br.y;
            if (br.y + br.h > baddy_bottom) baddy_bottom = br.y + br.h;
        }
        SDL_Rect baddy_r;
        baddy_r.x = baddy_left;
        baddy_r.y = baddy_top;
        baddy_r.w = baddy_right - baddy_left;
        baddy_r.h = baddy_bottom - baddy_top;

        bool maybe_baddie_hit_player = SDL_HasIntersection(&prect, &baddy_r);

        // Animate baddies
        bool advance = false;
        if (baddy_move_left && baddy_left < 20)
        {
            baddy_move_left = false;
            advance = true;
        }
        else if (!baddy_move_left && baddy_right >= 620)
        {
            baddy_move_left = true;
            advance = true;
        }

        for (auto& b : baddies)
        {
            if (baddy_move_left)
            {
                b.x -= baddy_speed * (float)ticks / 1000.0f;
            }
            else
            {
                b.x += baddy_speed * (float)ticks / 1000.0f;
            }

            if (advance)
            {
                b.y += baddy_advance;
            }

            if (maybe_baddie_hit_player)
            {
                auto br = b.r();
                if (SDL_HasIntersection(&prect, &br))
                {
                    reset_game();
                    break;
                }
            }

            // Display baddy
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
            SDL_Rect r = b.r();
            SDL_RenderFillRect(renderer, &r);

        }

        // Animate bullets
        {
            auto b = bullets.begin();
            while (b != bullets.end())
            {
                b->y += b->speed * (float)ticks / 1000.0f;
                if (b->y < 0.0f || b->y >= 480.0f)
                {
                    b = bullets.erase(b);
                    continue;
                }

                auto br = b->r();

                bool for_bullet_destroy = false;
                bool for_next_level = false;
                if (b->is_player &&
                    b->x >= baddy_left && b->x <= baddy_right &&
                    b->y >= baddy_top && b->y <= baddy_bottom)
                {
                    // test for baddy hit
                    auto biter = baddies.begin();
                    while (biter != baddies.end())
                    {
                        auto bdr = biter->r();
                        if (SDL_HasIntersection(&br, &bdr))
                        {
                            for_bullet_destroy = true;
                            baddies.erase(biter);
                            baddy_speed += 1.0f;

                            if (baddies.empty())
                            {
                                for_next_level = true;
                            }
                            break;
                        }
                        biter++;
                    }
                }

                if (for_next_level)
                {
                    reset_game();
                    break;
                }

                if (for_bullet_destroy)
                {
                    b = bullets.erase(b);
                    continue;
                }                

                // Display bullet
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
                SDL_RenderFillRect(renderer, &br);

                b++;
            }
        }


        SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
        SDL_Rect r;
        r.x = player_x - player_w / 2;
        r.y = player_y - player_h / 2;
        r.w = player_w;
        r.h = player_h;
        SDL_RenderFillRect(renderer, &r);

        SDL_RenderPresent(renderer);
    }


    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

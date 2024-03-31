#include <SDL.h>
#include <vector>
#include <list>

float player_x = 320.0f;
const float player_speed = 80.0f;   // per second
const float baddy_fire_interval = 4000.0f;
const int player_y = 440;
const int player_w = 24;
const int player_h = 12;
const int baddy_w = 24;
const int baddy_h = 24;
const int n_baddies_x = 10;
const int n_baddies_y = 5;
const int baddy_advance = 12;
const int shield_w = 8;
const int shield_h = 8;
const int shield_scale = 8;
const int n_shields = 4;

bool game_is_still_running = true;

bool keypress[256] = { false };

Uint64 last_fire_time = 0;
Uint64 last_baddy_fire_check = 0;
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
        ret.y = (int)y - 4;
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
        ret.x = (int)x - baddy_w / 2;
        ret.y = (int)y - baddy_h / 2;
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
    ret.x = (int)player_x - player_w / 2;
    ret.y = player_y - player_h / 2;
    ret.w = player_w;
    ret.h = player_h;
    return ret;
}

Uint64 lticks;
bool baddy_move_left;

struct shield
{
    bool parts[shield_w * shield_h];
    int x, y;

    bool hit(const SDL_Rect& other);
    void draw(SDL_Renderer* r);
    void reset();
};

shield shields[n_shields];

void reset_game()
{
    // Add baddies
    lticks = SDL_GetTicks64();

    last_baddy_fire_check = 0;
    last_fire_time = 0;

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
            b.x = (float)(x_start + x * baddy_w * 2);
            b.y = (float)(y_start + y * baddy_h * 2);
            b.last_fire_time = lticks;
            baddies.push_front(b);
        }
    }

    for (int i = 0; i < n_shields; i++)
    {
        shields[i].x = 64 + (640 - 64*2) / (n_shields - 1) * i;
        shields[i].y = player_y - 64;
        shields[i].reset();
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


#if 0
    // Try and select video mode
    SDL_DisplayMode mymode, newmode;
    SDL_GetCurrentDisplayMode(0, &mymode);
    mymode.format = SDL_PIXELFORMAT_RGB565;
    if(SDL_GetClosestDisplayMode(0, &mymode, &newmode))
    {
        SDL_SetWindowDisplayMode(window, &newmode);
    } 
#endif

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_Surface *s_inv = SDL_LoadBMP("inv.bmp");
    if(!s_inv)
    {
        printf("SDL_LoadBMP failed\n");
    }
    auto s_tex = SDL_CreateTextureFromSurface(renderer, s_inv);
    if(!s_tex)
    {
        printf("SDL_CreateTextureFromSurface failed\n");
    }

    auto p_tex = SDL_CreateTextureFromSurface(renderer, SDL_LoadBMP("player.bmp"));

    reset_game();

    while (game_is_still_running)
    {
        auto cticks = SDL_GetTicks64();
        auto ticks = cticks - lticks;
        lticks = cticks;

        printf("sinv: ticks: %u, fps: %.2f\n", (unsigned int)ticks, 1000.0f / (float)ticks);

        bool baddy_fire_check = false;
        Uint64 bfire_ticks = 0;
        if (cticks > (last_baddy_fire_check + 100))
        {
            bfire_ticks = cticks - last_baddy_fire_check;
            last_baddy_fire_check = cticks;
            baddy_fire_check = true;
        }

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
            if (player_x < player_w / 2 + 5)
                player_x = player_w / 2 + 5;
        }
        if (keypress[SDL_SCANCODE_RIGHT])
        {
            player_x += (float)ticks * player_speed / 1000.0f;
            if (player_x >= 635 - player_w / 2)
                player_x = 635 - player_w / 2;
        }
        auto prect = pr();

        if (fire_pressed && (cticks - last_fire_time) >= fire_delay)
        {
            // spawn new bullet
            bullet b;
            b.x = (int)player_x;
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

        // proportion of baddies killed, used to increase firing rate
        float prop_baddies_killed = (float)(n_baddies_x * n_baddies_y - baddies.size()) /
            (float)baddies.size();
        float baddie_fire_chance = std::max(prop_baddies_killed, 0.05f);

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

            auto br = b.r();
            if (maybe_baddie_hit_player)
            {
                if (SDL_HasIntersection(&prect, &br))
                {
                    reset_game();
                    break;
                }
            }

            for (int i = 0; i < n_shields; i++)
            {
                // don't destroy baddy on shield hit, just the baddie itself
                shields[i].hit(br);
            }

            // Fire bullet?
            if (baddy_fire_check && cticks >= (b.last_fire_time + fire_delay))
            {
                // has potential to fire
                float fire_chance = (float)bfire_ticks / baddy_fire_interval * baddie_fire_chance;
                
                int fc_int = (int)(fire_chance * (float)RAND_MAX);
                if (fc_int == 0) fc_int = 1;
                if (rand() < fc_int)
                {
                    // fire
                    bullet blt;
                    blt.x = (int)b.x;
                    blt.y = b.y;
                    blt.speed = 200.0f;
                    blt.is_player = false;
                    bullets.push_back(blt);
                    b.last_fire_time = cticks;
                }
            }

            // Display baddy
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
            SDL_Rect r = b.r();
            //SDL_RenderFillRect(renderer, &r);
            SDL_RenderCopy(renderer, s_tex, NULL, &r);

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
                            baddy_speed += 2.0f;

                            if (baddies.empty())
                            {
                                for_next_level = true;
                            }
                            break;
                        }
                        biter++;
                    }
                }

                if (!for_bullet_destroy)
                {
                    for (int i = 0; i < n_shields; i++)
                    {
                        if (shields[i].hit(br))
                        {
                            for_bullet_destroy = true;
                            break;
                        }
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

        // Display shields
        for (int i = 0; i < n_shields; i++)
        {
            shields[i].draw(renderer);
        }


        SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
        SDL_Rect r;
        r.x = (int)player_x - player_w / 2;
        r.y = player_y - player_h / 2;
        r.w = player_w;
        r.h = player_h;
        //SDL_RenderFillRect(renderer, &r);
        SDL_RenderCopy(renderer, p_tex, NULL, &r);

        //printf("sinv: call SDL_RenderPresent\n");
        SDL_RenderPresent(renderer);
    }


    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

void shield::reset()
{
    int nparts[] = {
        0,0,0,1,1,0,0,0,
        0,1,1,1,1,1,1,0,
        1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,
        1,1,1,0,0,1,1,1,
        1,1,0,0,0,0,1,1,
        1,1,0,0,0,0,1,1
    };

    for (int i = 0; i < sizeof(nparts) / sizeof(int); i++)
        parts[i] = nparts[i] != 0;
}

void shield::draw(SDL_Renderer* r)
{
    SDL_SetRenderDrawColor(r, 255, 255, 255, SDL_ALPHA_OPAQUE);
    int s_x = x - (shield_w * shield_scale) / 2;
    int s_y = y - (shield_h * shield_scale) / 2;
    for (int y = 0; y < shield_h; y++)
    {
        for (int x = 0; x < shield_w; x++)
        {
            if (parts[x + y * shield_w])
            {
                SDL_Rect rct;
                rct.x = s_x + x * shield_scale;
                rct.y = s_y + y * shield_scale;
                rct.w = shield_scale;
                rct.h = shield_scale;
                SDL_RenderFillRect(r, &rct);
            }
        }
    }
}

bool shield::hit(const SDL_Rect& other)
{
    SDL_Rect mr;
    mr.x = x - (shield_w * shield_scale) / 2;
    mr.y = y - (shield_h * shield_scale) / 2;
    mr.w = shield_w * shield_scale;
    mr.h = shield_h * shield_scale;

    // First just check for gross hit
    SDL_Rect isect;
    if (!SDL_IntersectRect(&other, &mr, &isect))
        return false;

    /* Then get all overlapping squares */
    isect.x -= std::max(mr.x, 0);
    isect.y -= std::max(mr.y, 0);
    isect.x /= shield_scale;
    isect.y /= shield_scale;
    isect.w /= shield_scale;
    isect.h /= shield_scale;

    bool ret = false;
    for (int cy = isect.y; (cy < shield_h) && (cy <= isect.y + isect.h); cy++)
    {
        for (int cx = isect.x; (cx < shield_w) && (cx <= isect.x + isect.w); cx++)
        {
            if (parts[cx + cy * shield_w])
            {
                ret = true;
                parts[cx + cy * shield_w] = false;
            }
        }
    }
    return ret;
}

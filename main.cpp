#include <iostream>
#include <vector>
#include <random>
#include <algorithm>

#include <raylib-cpp.hpp>

#define RAYGUI_IMPLEMENTATION
#include <raygui.h>

bool debugEnabled = false;

raylib::Sound pop_sound;

enum WALK {UP, DOWN, LEFT, RIGHT};

const int SCREEN_WIDTH = 2500;
const int SCREEN_HEIGHT = 900;

float FPS = 60;
float gameTick = 10; // 10 ms = 1 tick

std::default_random_engine engine;

void setGameTick(float t)
{
    gameTick = t;
}

int convertFromGameTick(int targetFPS = 60)
{
    int e = (gameTick / 100 * targetFPS);
    return e == 0 ? 1 : e;
}

struct Circle
{
    float radius;
    raylib::Vector2 center;

    Circle() {}
    Circle(float r, raylib::Vector2 c) : radius(r), center(c) {}

};

class Creature;

struct Stats
{
    float hp, atk;
    Circle sight_area;
    Circle range_area;
    raylib::Rectangle pos;
    Creature *go_after;
    Creature *attack;
    bool is_alive;
    raylib::Color col;

    Stats() {}
    Stats(float hp, float atk, Circle sight_area, Circle range_area, raylib::Rectangle pos = {0, 0, 10, 10}, bool is_alive = true, 
        Creature *go_after = nullptr, 
        Creature *attack = nullptr, 
        raylib::Color col = raylib::Color::Red())
    : hp(hp), atk(atk), sight_area(sight_area), range_area(range_area), pos(pos), is_alive(is_alive), go_after(go_after), attack(attack), col(col) {}
};

class Creature
{
    public:
    Stats stats;
    static float walkLength;
    int wayCount[4] = {0};

    static int creature_size;

    Creature() {creature_size++;}

    Creature(float hp, float atk, Circle sight_area, Circle range_area, raylib::Rectangle pos = {0, 0, 10, 10}, bool is_alive = true, 
        Creature *go_after = nullptr, 
        Creature *attack = nullptr, 
        raylib::Color col = raylib::Color::Red()) 
    : stats(hp, atk, sight_area, range_area, pos, is_alive, go_after, attack, col)
    {   
        creature_size++; 
        if(!pop_sound.IsPlaying())
            pop_sound.Play();
    }

    Creature(Stats stats) : stats(stats)
    {   
        creature_size++; 
        if(!pop_sound.IsPlaying())
            pop_sound.Play();
    }

    virtual ~Creature(){creature_size--;}

    friend void addCreature(std::vector<Creature *> &creatures, int count);
    friend void deleteCreature(std::vector<Creature *> &creatures, int count);
    friend void FightBetween(Creature *c1, Creature *c2);

    void setRandomColor()
    {
        stats.col = raylib::Color(
            static_cast<unsigned char>(GetRandomValue(100, 255)), 
            static_cast<unsigned char>(GetRandomValue(100, 255)),
            static_cast<unsigned char>(GetRandomValue(100, 255)),
            255
        );
    }
    void Wander() 
    {
        int walk_direction = -1;
        std::vector<int> possible_directions;

        if(stats.pos.y - 1 > 0)
            possible_directions.push_back(UP);
        if(stats.pos.y + 1 < SCREEN_HEIGHT)
            possible_directions.push_back(DOWN);
        if(stats.pos.x - 1 > 0)
            possible_directions.push_back(LEFT);
        if(stats.pos.x + 1 < SCREEN_WIDTH)
            possible_directions.push_back(RIGHT);
        
        float deltaTime = GetFrameTime();

        walk_direction = possible_directions[GetRandomValue(0, static_cast<int>(possible_directions.size() - 1))];

        switch(walk_direction)
        {
            case UP:
                stats.pos.y -= walkLength * deltaTime;
                break;
            case DOWN:
                stats.pos.y += walkLength * deltaTime;
                break;
            case LEFT:
                stats.pos.x -= walkLength * deltaTime;
                break;
            case RIGHT:
                stats.pos.x += walkLength * deltaTime;
                break;
        }
    }
    void DrawCreature()
    {
        raylib::Vector2 center(stats.pos.x - stats.pos.width/2, stats.pos.y - stats.pos.height/2);
        raylib::Rectangle(center.x, center.y, stats.pos.width, stats.pos.height).Draw(stats.col);
    }
    void checkSight(std::vector<Creature *> &creatures)
    {
        for(size_t i = 0; i < creatures.size(); i++)
        {
            if(creatures[i]->stats.is_alive && !(stats.pos.x == creatures[i]->stats.pos.x && stats.pos.y == creatures[i]->stats.pos.y) && CheckCollisionCircleRec(stats.sight_area.center, stats.sight_area.radius, creatures[i]->stats.pos))
            {
                stats.go_after = creatures[i]; // always go after first encountered crtr in the array (inappropriate but basic enough)
                return;
            }
        }
        stats.go_after = nullptr;
    }
    void GoAfter()
    {
        float real_walk_length = walkLength * GetFrameTime();
        raylib::Vector2 start {stats.pos.x, stats.pos.y}, end {stats.go_after->stats.pos.x, stats.go_after->stats.pos.y};
        raylib::Vector2 moved = start.MoveTowards(end, real_walk_length);
        stats.pos.x = moved.x;
        stats.pos.y = moved.y;
    }
};

class Human : public Creature
{
    public:

    Human() {}

    ~Human() {}

    Human(Stats stats) 
    : Creature(stats) {}
};

int Creature::creature_size = 0;
float Creature::walkLength = 800;

/* Friend functions*/
void addCreature(std::vector<Creature *> &creatures, int count = 1)
{
    for(int i = 0; i < count; i++)
    {
        raylib::Vector2 rand_pos{static_cast<float>(GetRandomValue(2, SCREEN_WIDTH - 2)), static_cast<float>(GetRandomValue(2, SCREEN_HEIGHT - 2))};
        creatures.push_back(new Human( 
        Stats{
            static_cast<float>(GetRandomValue(1, 20)),
            static_cast<float>(GetRandomValue(1, 10)),
            Circle(150, rand_pos),
            Circle(30, rand_pos),
            raylib::Rectangle(rand_pos.x, rand_pos.y, 5, 5),
            true,
            nullptr,
            nullptr,
            raylib::Color
            (static_cast<unsigned char>(GetRandomValue(100, 255)), 
            static_cast<unsigned char>(GetRandomValue(100, 255)),
            static_cast<unsigned char>(GetRandomValue(100, 255)),
            255)
            }
        ));
    }
}
void deleteCreature(std::vector<Creature *> &creatures, int count = 1)
{
    int delete_count = Creature::creature_size > count ? count : Creature::creature_size;
    for(int i = 0; i < delete_count; i++)
    {
        auto last_elem_p = (creatures.end() - 1);
        delete *last_elem_p;
        creatures.erase(last_elem_p);
    }
}
void FightBetween(Creature *c1, Creature *c2)
{
    c1->stats.hp -= c2->stats.atk;
    c2->stats.hp -= c1->stats.atk;

    if (c1->stats.hp <= 0) 
    {
        c1->stats.is_alive = false;
        c2->stats.go_after = nullptr;
        c1->stats.attack = nullptr;
        c1->stats.hp = 0;
    }
    if (c2->stats.hp <= 0)
    {
        c2->stats.is_alive = false;
        c2->stats.go_after = nullptr;
        c2->stats.attack = nullptr;
        c2->stats.hp = 0;
    }
}

int main() 
{
    raylib::Window window(SCREEN_WIDTH, SCREEN_HEIGHT, "Creature simulator");
    window.SetTargetFPS(FPS);
    engine.seed(time(0));

    raylib::AudioDevice audio;
    
    if(!audio.IsReady())
    {
        std::cerr << "Sound couldn't be initialized.\n";
        return 1;
    }

    pop_sound = raylib::Sound("my_pop_sound.mp3");

    std::vector<Creature*> creatures;

    addCreature(creatures, 10);

    int i = 0;
    bool showUI = true;
    float old_FPS;

    char inputText1[64] = "", inputText2[64] = ""; 
    bool editMode1 = false, editMode2 = false;

    while (!window.ShouldClose())
    {
        DrawText("Add", 150, 30, 18, GREEN);

        if (GuiTextBox((Rectangle){100, 50, 200, 30}, inputText1, 64,
                       editMode1))
        {
            editMode1 = !editMode1;
        }

        if (GuiButton((Rectangle){150, 85, 100, 30}, "Gonder"))
        {

            int x = atoi(inputText1);

            addCreature(creatures, x);
        }
        /*************/
        DrawText("Delete", 140, 120, 18, GREEN);

        if (GuiTextBox((Rectangle){100, 140, 200, 30}, inputText2, 64, editMode2))
        {
            editMode2 = !editMode2;
        }

        if (GuiButton((Rectangle){150, 175, 100, 30}, "Gonder"))
        {

            int x = atoi(inputText2);

            deleteCreature(creatures, x);
        }

        old_FPS = FPS;
        GuiSliderBar({1200, 20, 200, 20}, "30", "300", &FPS, 30, 300);
        if(static_cast<int>(old_FPS) != static_cast<int>(FPS))
            window.SetTargetFPS(FPS);

        window.BeginDrawing();
            window.ClearBackground(raylib::Color::Black());
            GuiSliderBar({50, 500, 200, 30}, "1", "100", &gameTick, 1.0f, 100.0f);
            GuiSliderBar({50, 400, 200, 30}, "500", "10000", &Creature::walkLength, 500.0f, 10000.0f);
            DrawText(TextFormat("%f", gameTick), 50, 450, 25, GREEN);
            DrawText(TextFormat("getfps: %d, fps: %f", window.GetFPS(), FPS), 1200, 40, 20, YELLOW);
            
            if(debugEnabled)
                DrawText(TextFormat("UP: %4d, DOWN: %4d, LEFT: %4d, RIGHT: %4d",
                    creatures[0]->wayCount[0],
                    creatures[0]->wayCount[1],
                    creatures[0]->wayCount[2], 
                    creatures[0]->wayCount[3]), 10, 370, 25, GREEN);
            
            if(debugEnabled)
                DrawText(TextFormat("Creature::creature_size: %d", Creature::creature_size), 300, 300, 20, RED);
            
            for(int j = 0; j < Creature::creature_size; j++) // draw sight area circles
            {
                if(creatures[j]->stats.is_alive) // draw utilities
                {
                    DrawCircle(creatures[j]->stats.sight_area.center.x, creatures[j]->stats.sight_area.center.y,
                        creatures[j]->stats.sight_area.radius, raylib::Color{ 253, 249, 0, 100 });
                    DrawCircle(creatures[j]->stats.range_area.center.x, creatures[j]->stats.range_area.center.y,
                        creatures[j]->stats.range_area.radius, raylib::Color{ 0, 228, 48, 100 });
                    if(creatures[j]->stats.go_after && creatures[j]->stats.go_after->stats.is_alive)
                        DrawText("!", creatures[j]->stats.pos.x + 15, creatures[j]->stats.pos.y - 15, 30, WHITE);
                    if(creatures[j]->stats.attack && creatures[j]->stats.attack->stats.is_alive)
                        DrawText("!", creatures[j]->stats.pos.x + 20, creatures[j]->stats.pos.y - 15, 30, WHITE);
                    DrawText(TextFormat("hp: %.0f", creatures[j]->stats.hp), creatures[j]->stats.pos.x - 5, creatures[j]->stats.pos.y - 15, 5, GREEN);
                    DrawText(TextFormat("atk: %.0f", creatures[j]->stats.atk), creatures[j]->stats.pos.x - 5, creatures[j]->stats.pos.y - 10, 5, GREEN);
                }
            }

            if(i++ % convertFromGameTick(FPS) == 0) // where the game progresses
            {
                for(int j = 0; j < Creature::creature_size; j++)
                {
                    if (creatures[j]->stats.is_alive)
                    {
                        creatures[j]->checkSight(creatures);
                        if (creatures[j]->stats.go_after && creatures[j]->stats.go_after->stats.is_alive)
                        {
                            if (CheckCollisionCircleRec(
                            creatures[j]->stats.range_area.center,
                            creatures[j]->stats.range_area.radius,
                            creatures[j]->stats.go_after->stats.pos)) // whether j's go_after, touching j's range circle or not
                            {
                                creatures[j]->stats.attack = creatures[j]->stats.go_after;
                                creatures[j]->stats.go_after->stats.attack = creatures[j];
                                FightBetween(creatures[j], creatures[j]->stats.go_after);
                            }

                            else
                            {
                                creatures[j]->stats.attack = creatures[j]->stats.go_after->stats.attack = nullptr;
                                creatures[j]->GoAfter();
                            }
                        }
                        else
                            creatures[j]->Wander();
                        if (creatures[j]->stats.is_alive)
                            creatures[j]->stats.sight_area.center = 
                            creatures[j]->stats.range_area.center = raylib::Vector2(creatures[j]->stats.pos.x, creatures[j]->stats.pos.y);
                    }
                }
            }
            
            for(int j = 0; j < Creature::creature_size; j++)
            {
                if (creatures[j]->stats.is_alive)
                    creatures[j]->DrawCreature();
            }
        window.EndDrawing();
    }

    for (int j = 0; j < Creature::creature_size; j++)
    {
        delete creatures[j];
    }
    creatures.clear();
    
    return 0; // RAII destroys window, audio, and unloads sound objects cleanly automatically.
}
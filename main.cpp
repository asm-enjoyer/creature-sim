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
const int SCREEN_HEIGHT = 1300;

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
    raylib::Rectangle pos;
    raylib::Color col;
    Creature *kill;
    Stats() {}
    Stats(float hp, float atk, Circle sight_area, Creature *kill = nullptr, raylib::Rectangle pos = {0, 0, 10, 10}, raylib::Color col = raylib::Color::Red())
    : hp(hp), atk(atk), sight_area(sight_area), kill(kill), pos(pos), col(col) {}
};

class Creature
{
    public:
    Stats stats;
    static float walkLength;
    int wayCount[4] = {0};

    static int creature_size;

    Creature() {creature_size++;}

    Creature(float hp, float atk, Circle sight_area, Creature *kill = nullptr, raylib::Rectangle pos = {0, 0, 10, 10}, raylib::Color col = raylib::Color::Red()) 
    : stats(hp, atk, sight_area, kill, pos, col)
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
            if(!(stats.pos.x == creatures[i]->stats.pos.x && stats.pos.y == creatures[i]->stats.pos.y) && CheckCollisionCircleRec(stats.sight_area.center, stats.sight_area.radius, creatures[i]->stats.pos))
            {
                stats.kill = creatures[i]; // go after first encountered dot in the array (inappropriate but basic enough)
                return;
            }
        }
        stats.kill = nullptr;
    }

    void GoAfter()
    {
        float real_walk_length = walkLength * GetFrameTime();
        raylib::Vector2 start {stats.pos.x, stats.pos.y}, end {stats.kill->stats.pos.x, stats.kill->stats.pos.y};
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

void addCreature(std::vector<Creature *> &creatures, int count = 1)
{
    std::uniform_real_distribution<> randomCoords(2, std::max(SCREEN_HEIGHT - 2, SCREEN_WIDTH - 2));
    for(int i = 0; i < count; i++)
    {
        raylib::Vector2 rand_pos{static_cast<float>(randomCoords(engine)), static_cast<float>(randomCoords(engine))};
        creatures.push_back(new Human( 
        Stats{
            static_cast<float>(GetRandomValue(1, 20)),
            static_cast<float>(GetRandomValue(1, 10)),
            Circle(150, rand_pos),
            nullptr,
            raylib::Rectangle(rand_pos.x, rand_pos.y, 5, 5)
            }
        ));
        creatures.back()->setRandomColor();

        
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

    while (!window.ShouldClose())
    {   
        if(GuiButton({700, 300, 90, 30}, "add 1 point"))
            addCreature(creatures);
        if(GuiButton({700, 330, 90, 30}, "add 10 points"))
            addCreature(creatures, 10);
        if(GuiButton({700, 360, 90, 30}, "add 100 points"))
            addCreature(creatures, 100);
        if(GuiButton({700, 390, 90, 30}, "add 1000 points"))
            addCreature(creatures, 1000);
        if(GuiButton({700, 420, 90, 30}, "add 10000 points"))
            addCreature(creatures, 10000);
        if(GuiButton({790, 300, 90, 30}, "delete 1 point"))
            deleteCreature(creatures);
        if(GuiButton({790, 330, 90, 30}, "delete 10 points"))
            deleteCreature(creatures, 10);
        if(GuiButton({790, 360, 90, 30}, "delete 100 points"))
            deleteCreature(creatures, 100);
        if(GuiButton({790, 390, 90, 30}, "delete 1000 points"))
            deleteCreature(creatures, 1000);
        
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
            
            for(int j = 0; j < Creature::creature_size; j++)
            {
                DrawCircle(creatures[j]->stats.sight_area.center.x, creatures[j]->stats.sight_area.center.y,
                creatures[j]->stats.sight_area.radius, raylib::Color{ 253, 249, 0, 100 });
                if(creatures[j]->stats.kill)
                    DrawText("!", creatures[j]->stats.pos.x + 15, creatures[j]->stats.pos.y - 15, 30, WHITE);
            }

            if(i++ % convertFromGameTick(FPS) == 0)
            {
                for(int j = 0; j < Creature::creature_size; j++)
                {
                    creatures[j]->checkSight(creatures);
                    if(creatures[j]->stats.kill)
                    {
                        creatures[j]->GoAfter();
                    }
                    else
                        creatures[j]->Wander();
                    creatures[j]->stats.sight_area.center = raylib::Vector2(creatures[j]->stats.pos.x, creatures[j]->stats.pos.y);
                }
            }
            
            for(int j = 0; j < Creature::creature_size; j++)
            {
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
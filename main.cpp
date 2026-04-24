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

class Creature
{
    protected:
    float view_distance, hp, atk;
    raylib::Rectangle pos;
    raylib::Color col;

    int counter = 0, interval_selector = 0;
    
    public:

    static float walkLength;
    int wayCount[4] = {0};

    static int creature_size;

    Creature() {creature_size++;}

    Creature(float hp, float atk, float v_d = 50, raylib::Rectangle pos = {0, 0, 10, 10}, raylib::Color col = raylib::Color::Red()) 
    : view_distance(v_d), hp(hp), atk(atk), pos(pos), col(col) 
    {   
        creature_size++; 
        if(!pop_sound.IsPlaying())
            pop_sound.Play();
    }

    virtual ~Creature(){creature_size--;}

    friend void addPoints(std::vector<Creature *> &creatures, struct preDefInfo &a, int count);

    void setWalkLength(int n)
    {
        walkLength = n;
    }

    void setRandomColor()
    {
        col = raylib::Color(
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

        if(pos.y - 1 > 0)
            possible_directions.push_back(UP);
        if(pos.y + 1 < SCREEN_HEIGHT)
            possible_directions.push_back(DOWN);
        if(pos.x - 1 > 0)
            possible_directions.push_back(LEFT);
        if(pos.x + 1 < SCREEN_WIDTH)
            possible_directions.push_back(RIGHT);
        
        float deltaTime = GetFrameTime();

        walk_direction = possible_directions[GetRandomValue(0, static_cast<int>(possible_directions.size() - 1))];

        switch(walk_direction)
        {
            case UP:
                pos.y -= walkLength * deltaTime;
                break;
            case DOWN:
                pos.y += walkLength * deltaTime;
                break;
            case LEFT:
                pos.x -= walkLength * deltaTime;
                break;
            case RIGHT:
                pos.x += walkLength * deltaTime;
                break;
        }
    }

    void DrawCreature()
    {
        raylib::Vector2 center(pos.x - pos.width/2, pos.y - pos.height/2);
        raylib::Rectangle(center.x, center.y, pos.width, pos.height).Draw(col);
    }

    friend void deleteCreature(std::vector<Creature *> &creatures, struct preDefInfo &a, int count);
};

class Human : public Creature
{
    public:

    Human() {}

    ~Human() {}

    Human(float hp, float atk, float v_d = 50, raylib::Rectangle pos = {0, 0, 10, 10}) 
    : Creature(hp, atk, v_d, pos) {}
};

int Creature::creature_size = 0;
float Creature::walkLength = 800;

struct preDefInfo
{
    float hp, atk, view_dist;
    raylib::Rectangle body;
} preDefInfoVal {10, 2, 50, {400, 300, 5, 5}};

void addPoints(std::vector<Creature *> &creatures, struct preDefInfo &a, int count = 1)
{
    std::uniform_real_distribution<> randomCoords(2, std::max(SCREEN_HEIGHT - 2, SCREEN_WIDTH - 2));
    for(int i = 0; i < count; i++)
    {
        creatures.push_back(new Human(a.hp, a.atk, a.view_dist, raylib::Rectangle(static_cast<float>(randomCoords(engine)), static_cast<float>(randomCoords(engine)), a.body.width, a.body.height)));
        creatures.back()->setRandomColor();
    }
}

void deleteCreature(std::vector<Creature *> &creatures, struct preDefInfo &a, int count = 1)
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

    addPoints(creatures, preDefInfoVal, 10);

    int i = 0;
    bool showUI = true;
    float old_FPS;

    while (!window.ShouldClose())
    {   
        if(GuiButton({700, 300, 90, 30}, "add 1 point"))
            addPoints(creatures, preDefInfoVal);
        if(GuiButton({700, 330, 90, 30}, "add 10 points"))
            addPoints(creatures, preDefInfoVal, 10);
        if(GuiButton({700, 360, 90, 30}, "add 100 points"))
            addPoints(creatures, preDefInfoVal, 100);
        if(GuiButton({700, 390, 90, 30}, "add 1000 points"))
            addPoints(creatures, preDefInfoVal, 1000);
        if(GuiButton({700, 420, 90, 30}, "add 10000 points"))
            addPoints(creatures, preDefInfoVal, 10000);
        if(GuiButton({790, 300, 90, 30}, "delete 1 point"))
            deleteCreature(creatures, preDefInfoVal);
        if(GuiButton({790, 330, 90, 30}, "delete 10 points"))
            deleteCreature(creatures, preDefInfoVal, 10);
        if(GuiButton({790, 360, 90, 30}, "delete 100 points"))
            deleteCreature(creatures, preDefInfoVal, 100);
        if(GuiButton({790, 390, 90, 30}, "delete 1000 points"))
            deleteCreature(creatures, preDefInfoVal, 1000);
        
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
            
            if(i++ % convertFromGameTick(FPS) == 0)
            {
                for(int j = 0; j < Creature::creature_size; j++)
                {
                    creatures[j]->Wander();
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
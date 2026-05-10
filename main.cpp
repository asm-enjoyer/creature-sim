#include <iostream>
#include <string>
#include <algorithm>
#include <sstream>
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
const int SCREEN_HEIGHT = 1100;

float FPS = 60;
float gameTick = 10; // 10 ms = 1 tick

void setGameTick(float t)
{
    gameTick = t;
}
int convertFromGameTick(int targetFPS = 60)
{
    int e = (gameTick / 100 * targetFPS);
    return e == 0 ? 1 : e;
}
int removePoint(float num) {
    // clean string of num
    std::ostringstream oss;
    oss << num;
    std::string str_num = oss.str();

    // delete point
    str_num.erase(std::remove(str_num.begin(), str_num.end(), '.'), str_num.end());

    // for overflow
    return std::stoll(str_num);
}
/// @brief returns random float in [min, max] with given precision
/// @param a min
/// @param b max
/// @param c precision
/// @return the random value
float randf(float a = 0.0f, float b = 1.0f, int c = 100)
{
    return GetRandomValue(c * removePoint(a), c * removePoint(b)) / static_cast<float>(c);
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
    float hp, atk, hit_chance;
    Circle sight_area;
    Circle range_area;
    raylib::Rectangle pos;
    Creature *go_after;
    Creature *attack;
    bool is_alive;
    raylib::Color col;
    bool human_controlled;

    Stats(float hp = 1, float atk = 1, float hit_ch = 0.7, Circle sight_area = Circle(150, raylib::Vector2{0, 0}),
     Circle range_area = Circle(30, raylib::Vector2{0, 0}), raylib::Rectangle pos = {0, 0, 10, 10}, bool is_alive = true, 
        Creature *go_after = nullptr, 
        Creature *attack = nullptr, 
        raylib::Color col = raylib::Color::Red(), bool human_cont = false)
    : hp(hp), atk(atk), hit_chance(hit_ch), sight_area(sight_area),
    range_area(range_area), pos(pos), is_alive(is_alive),
    go_after(go_after), attack(attack), col(col), human_controlled(human_cont) {}
};

class Creature
{
    public:
    Stats stats;
    static float walkLength; // TODO if big (near 10k), most of the time 1 crtr jumps between 2 other crtrs and never takes damage nor makes the other 2 give damage, an infinite loop.
    static raylib::Vector2 wander_limit;
    int wayCount[4] = {0};

    static int creature_size;

    Creature() {creature_size++;}

    Creature(float hp, float atk, float hit_ch, Circle sight_area, Circle range_area, raylib::Rectangle pos = {0, 0, 10, 10}, bool is_alive = true, 
        Creature *go_after = nullptr, 
        Creature *attack = nullptr, 
        raylib::Color col = raylib::Color::Red(), bool human_cont = false) 
    : stats(hp, atk, hit_ch, sight_area, range_area, pos, is_alive, go_after, attack, col, human_cont)
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

    friend void addCreature(std::vector<Creature *> &creatures, int count, Stats *stats_local, bool human_cont);
    friend void deleteCreature(std::vector<Creature *> &creatures, int count);
    friend void FightBetween(Creature *c1, Creature *c2);
    friend void deleteKilled(std::vector<Creature *> &creatures);

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
        if(stats.pos.y + 1 < Creature::wander_limit.y)
            possible_directions.push_back(DOWN);
        if(stats.pos.x - 1 > 0)
            possible_directions.push_back(LEFT);
        if(stats.pos.x + 1 < Creature::wander_limit.x)
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
        float min_distance_squ = INFINITY;
        float new_dist_squ;
        Creature *closest = nullptr;
        for(size_t i = 0; i < creatures.size(); i++)
        {
            if(creatures[i]->stats.is_alive && !(stats.pos.x == creatures[i]->stats.pos.x && stats.pos.y == creatures[i]->stats.pos.y) && CheckCollisionCircleRec(stats.sight_area.center, stats.sight_area.radius, creatures[i]->stats.pos))
            {
                float x = creatures[i]->stats.pos.x;
                float y = creatures[i]->stats.pos.y;
                float dx = stats.pos.x - x, dy = stats.pos.y - y;
                float squaredx = dx * dx, squaredy = dy * dy;
                // don't use sqrt, just compare squared distances (more efficient)
                if((new_dist_squ = squaredx + squaredy) < min_distance_squ)
                {
                    min_distance_squ = new_dist_squ;
                    closest = creatures[i];
                }
            }
        }
        
        stats.go_after = closest;
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
struct Grid
{
    std::vector< std::vector< std::pair< raylib::Rectangle, std::vector<Creature *> > > > cells;
    int sq_num; // number of squares in one side of the grid
    raylib::Rectangle total_area;
    float side_length;

    Grid(int sq_num = 10, raylib::Rectangle total_area = raylib::Rectangle(-20, -20, wander_limit.x + 20, wander_limit.y + 20))
    : sq_num(sq_num), total_area(total_area), side_length(total_area.width / sq_num)
    {
        cells.resize(sq_num); 

        for(int y = 0; y < sq_num; ++y)
        {
            float coordY = total_area.y + (y * side_length);
            for(int x = 0; x < sq_num; ++x)
            {
                float coordX = total_area.x + (x * side_length);
                
                cells[y].push_back(std::pair<raylib::Rectangle, std::vector<Creature*>>(
                    raylib::Rectangle(coordX, coordY, side_length, side_length), 
                    std::vector<Creature*>()
                ));
            }
        }
    }

    void UpdateCells(const std::vector<Creature *> &creatures)
    {
        // clear cell info from previous frame
        for(int y = 0; y < sq_num; ++y)
        {
            for(int x = 0; x < sq_num; ++x)
            {
                cells[y][x].second.clear();
            }
        }

        for(size_t i = 0; i < creatures.size(); i++)
        {
            if(creatures[i]->stats.is_alive)
            {
                int inX = static_cast<int>((creatures[i]->stats.pos.x - total_area.x) / side_length);
                int inY = static_cast<int>((creatures[i]->stats.pos.y - total_area.y) / side_length);

                // check if out of grid. or game crashes.
                if (inX >= 0 && inX < sq_num && inY >= 0 && inY < sq_num)
                {
                    cells[inY][inX].second.push_back(creatures[i]);
                }
            }
        }
    }

    /// @brief More optimized sight checker. 
    /// CheckSight takes the return vector and traverses it instead of all creatures.
    /// @param center_crtr 
    /// @return return vector with creatures who center creature can see
    std::vector<Creature *> GridCheckSight(Creature *&center_crtr)
    {
        auto center = center_crtr->stats; // shortcut for long writing
        raylib::Vector2 min_point(center.pos.x - center.sight_area.radius, center.pos.y - center.sight_area.radius),
                        max_point(center.pos.x + center.sight_area.radius, center.pos.y + center.sight_area.radius);
        
        int start_x = std::max(0,          static_cast<int>((min_point.x - total_area.x) / side_length));
        int end_x   = std::min(sq_num - 1, static_cast<int>((max_point.x - total_area.x) / side_length));
        int start_y = std::max(0,          static_cast<int>((min_point.y - total_area.y) / side_length));
        int end_y   = std::min(sq_num - 1, static_cast<int>((max_point.y - total_area.y) / side_length));

        float radius_sq = center.sight_area.radius * center.sight_area.radius;

        std::vector<Creature *> creatures_in_range;

        for (int y = start_y; y <= end_y; ++y)
        {
            for (int x = start_x; x <= end_x; ++x)
            {
                for (Creature *other : cells[y][x].second)
                {
                    if (other == center_crtr) 
                        continue;

                    float dx = other->stats.pos.x - center_crtr->stats.pos.x;
                    float dy = other->stats.pos.y - center_crtr->stats.pos.y;

                    if ((dx * dx + dy * dy) <= radius_sq)
                    {
                        creatures_in_range.push_back(other);
                    }
                }
            }
        }

        return creatures_in_range;
    }

    void DrawGrid()
    {
        for(int i = 0; i < cells.size(); i++)
        {
            for(int j = 0; j < cells[i].size(); j++)
            {
                cells[i][j].first.DrawLines(WHITE);
            }
        }
    }
};

int Creature::creature_size = 0;
float Creature::walkLength = 800;
raylib::Vector2 Creature::wander_limit(2000, 1500);

/* Friend functions*/
void addCreature(std::vector<Creature *> &creatures, int count = 1, Stats *stats_local = nullptr, bool human_cont = false)
{
    for(int i = 0; i < count; i++)
    {
        if(stats_local)
        {
            creatures.push_back(new Human(*stats_local));
        }

        else
        {
            raylib::Vector2 rand_pos{
                static_cast<float>(GetRandomValue(2, Creature::wander_limit.x - 2)),
                static_cast<float>(GetRandomValue(2, Creature::wander_limit.y - 2))};
            creatures.push_back(new Human(
                Stats{static_cast<float>(GetRandomValue(1, 20)),
                      static_cast<float>(GetRandomValue(1, 10)),
                      randf(0.01, 1),
                      Circle(150, rand_pos), Circle(30, rand_pos),
                      raylib::Rectangle(rand_pos.x, rand_pos.y, 5, 5), true,
                      nullptr, nullptr,
                      raylib::Color(
                          static_cast<unsigned char>(GetRandomValue(100, 255)),
                          static_cast<unsigned char>(GetRandomValue(100, 255)),
                          static_cast<unsigned char>(GetRandomValue(100, 255)),
                          255), false}));
        }
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
void FightBetween(Creature *c1, Creature *c2) // c1 must be the one who sees first
{
    if(c1->stats.range_area.radius <= c2->stats.range_area.radius && randf() > c2->stats.hit_chance)
        c1->stats.hp -= c2->stats.atk;
    if(randf() < c1->stats.hit_chance)
        c2->stats.hp -= c1->stats.atk;

    if (c1->stats.hp <= 0) 
    {
        c1->stats.is_alive = false;
        c2->stats.go_after = nullptr;
        c2->stats.attack = nullptr;
        c1->stats.hp = 0;
    }
    if (c2->stats.hp <= 0)
    {
        c2->stats.is_alive = false;
        c1->stats.go_after = nullptr;
        c1->stats.attack = nullptr;
        c2->stats.hp = 0;
    }
}

void swap_pop(std::vector<Creature *> &creatures, size_t i);

void deleteKilled(std::vector<Creature *> &creatures)
{
    for(size_t i = 0; i < creatures.size(); )
    {
        if(creatures[i] && !creatures[i]->stats.is_alive)
        {
            swap_pop(creatures, i);
        }
        else i++;
    }
}
void swap_pop(std::vector<Creature *> &creatures, size_t i)
{
    delete creatures[i];
    creatures[i] = creatures.back();
    creatures.pop_back();
}
/***************** */
void MoveByKey(std::vector<Creature *> &creatures, int j)
{
    float deltatime_walk = Creature::walkLength * GetFrameTime();
    if (IsKeyDown(KEY_W)) creatures[j]->stats.pos.y -= deltatime_walk;
    if (IsKeyDown(KEY_A)) creatures[j]->stats.pos.x -= deltatime_walk;
    if (IsKeyDown(KEY_S)) creatures[j]->stats.pos.y += deltatime_walk;
    if (IsKeyDown(KEY_D)) creatures[j]->stats.pos.x += deltatime_walk;
}

void ParseAndAddSpecialCreature(char inputText[128], std::vector<Creature *> &creatures)
{
    Stats new_stats;
    std::string input_text(inputText), token;
    std::stringstream ss(input_text);

    std::vector<std::string> string_vec;

    while(ss >> token)
    {
        if(token == "hp")
        {
            ss >> token;
            new_stats.hp = std::stof(token);
        }

        else if(token == "atk")
        {
            ss >> token;
            new_stats.atk = std::stof(token);
        }

        else if(token == "hit_chance" || token == "hc")
        {
            ss >> token;
            new_stats.hit_chance = std::stof(token);
        }
        
        else if(token == "human_controlled" || token == "humc")
        {
            new_stats.human_controlled = true;
        }

        else if(token == "sight_r" || token == "sr")
        {
            ss >> token;
            new_stats.sight_area.radius = std::stof(token);
        }
        
        else if(token == "range_r" || token == "rr")
        {
            ss >> token;
            new_stats.range_area.radius = std::stof(token);
        }
        
    }

    new_stats.pos =
        raylib::Rectangle(GetRandomValue(2, Creature::wander_limit.x - 2),
                          GetRandomValue(2, Creature::wander_limit.y - 2), 5, 5);
    addCreature(creatures, 1, &new_stats);
}

raylib::Rectangle GetCameraVisibleArea2D(Camera2D camera) 
{
    Vector2 topLeft = GetScreenToWorld2D((Vector2){ 0.0f, 0.0f }, camera);
    Vector2 bottomRight = GetScreenToWorld2D((Vector2){ (float)GetScreenWidth(), (float)GetScreenHeight() }, camera);

    return raylib::Rectangle{ 
        topLeft.x, 
        topLeft.y, 
        bottomRight.x - topLeft.x, 
        bottomRight.y - topLeft.y 
    };
}

int main()
{
    SetRandomSeed(time(0));

    raylib::Window window(SCREEN_WIDTH, SCREEN_HEIGHT, "Creature simulator");
    window.SetTargetFPS(FPS);

    raylib::AudioDevice audio;
    if(!audio.IsReady())
    {
        std::cerr << "Sound couldn't be initialized.\n";
        return 1;
    }
    pop_sound = raylib::Sound("my_pop_sound.mp3");

    raylib::Camera2D cam2d = raylib::Camera2D(Vector2{SCREEN_WIDTH / 2.0, SCREEN_HEIGHT / 2.0}, {0, 0});

    std::vector<Creature*> creatures;
    addCreature(creatures, 10);

    Grid grid;

    int i = 0;
    bool showUI = true;
    float old_FPS;
    int j = 0;

    char inputText[3][128] = {"500", "500", ""};
    bool editMode[3] = {0};

    bool bool_draw_circle = true, bool_draw_grid = false;
    raylib::Vector2 old_m_pos(0, 0), mouse_pos;
    float mwheel_move;

    bool moved_before;

    while (!window.ShouldClose())
    {
        /************* CAMERA ************/
        {
        mouse_pos = GetMousePosition();
        mwheel_move = GetMouseWheelMove();

        if(mwheel_move)
        {
            // Get the world point that is under the mouse
            Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), cam2d);

            // Set the offset to where the mouse is
            cam2d.offset = GetMousePosition();

            // Set the target to match, so that the camera maps the world space
            // point under the cursor to the screen space point under the cursor
            // at any zoom
            cam2d.target = mouseWorldPos;

            // Zoom increment
            // Uses log scaling to provide consistent zoom speed
            float scale = 0.2f * mwheel_move;
            cam2d.zoom = (expf(logf(cam2d.zoom) + scale));
        }
        
        cam2d.zoom = expf(logf(cam2d.zoom) + (mwheel_move*0.1f));
        
        if(IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            cam2d.offset.x += mouse_pos.x - old_m_pos.x;
            cam2d.offset.y += mouse_pos.y - old_m_pos.y;
        }
        old_m_pos = mouse_pos;
        }
        {
        /************* GUI ************/
        DrawText("Add", 150, 30, 18, GREEN);

        if (GuiTextBox((Rectangle){100, 50, 200, 30}, inputText[0], 64, editMode[0]))
        {
            editMode[0] = !editMode[0];
        }

        if (GuiButton((Rectangle){150, 85, 100, 30}, "OK"))
        {

            int x = atoi(inputText[0]);

            addCreature(creatures, x);
        }
        /*************/
        DrawText("Delete", 140, 120, 18, GREEN);

        if (GuiTextBox((Rectangle){100, 140, 200, 30}, inputText[1], 64, editMode[1]))
        {
            editMode[1] = !editMode[1];
        }

        if (GuiButton((Rectangle){150, 175, 100, 30}, "OK"))
        {
            int x = atoi(inputText[1]);
            deleteCreature(creatures, x);
        }
        /****************/
        DrawText("Add special (ex: atk 20 hp 10 humc hc 0.3)", 80, 210, 18, GREEN);

        if (GuiTextBox((Rectangle){100, 230, 200, 30}, inputText[2], 64, editMode[2]))
        {
            editMode[2] = !editMode[2];
        }

        if (GuiButton((Rectangle){150, 265, 100, 30}, "OK"))
        {
            ParseAndAddSpecialCreature(inputText[2], creatures);
        }

        if(GuiButton({150, 300, 100, 30}, "Toggle circles"))
        {
            bool_draw_circle = !bool_draw_circle;
        }

        if (GuiButton((Rectangle){130, 335, 200, 30}, "Delete 0 health crtrs"))
        {
            deleteKilled(creatures);
            // creatures.shrink_to_fit();
        }
        
        if(GuiButton({150, 370, 100, 30}, "Toggle grid"))
        {
            bool_draw_grid = !bool_draw_grid;
        }

        old_FPS = FPS;
        GuiSliderBar({1200, 20, 200, 20}, "30", "300", &FPS, 30, 300);
        if(static_cast<int>(old_FPS) != static_cast<int>(FPS))
            window.SetTargetFPS(FPS);
        }

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
            
            if(1)
                DrawText(TextFormat("Creature::creature_size: %d", Creature::creature_size), 300, 300, 20, RED);
            
        cam2d.BeginMode();
            raylib::Rectangle cam_visible_area = GetCameraVisibleArea2D(cam2d);
            for(int j = 0; j < Creature::creature_size; j++) // draw zone
            {
                if(bool_draw_grid)
                        grid.DrawGrid();
                if(creatures[j]->stats.is_alive && CheckCollisionRecs(creatures[j]->stats.pos, cam_visible_area))
                {
                    if(bool_draw_circle)
                    {
                        DrawCircle(creatures[j]->stats.sight_area.center.x, creatures[j]->stats.sight_area.center.y,
                            creatures[j]->stats.sight_area.radius, raylib::Color{ 253, 249, 0, 100 });
                        DrawCircle(creatures[j]->stats.range_area.center.x, creatures[j]->stats.range_area.center.y,
                            creatures[j]->stats.range_area.radius, raylib::Color{ 0, 228, 48, 100 });
                    }
                    if(creatures[j]->stats.go_after && creatures[j]->stats.go_after->stats.is_alive)
                        DrawText("!", creatures[j]->stats.pos.x + 15, creatures[j]->stats.pos.y - 15, 30, WHITE);
                    if(creatures[j]->stats.attack && creatures[j]->stats.attack->stats.is_alive)
                        DrawText("!", creatures[j]->stats.pos.x + 20, creatures[j]->stats.pos.y - 15, 30, WHITE);
                    DrawText(TextFormat("  %d", j), creatures[j]->stats.pos.x - 5, creatures[j]->stats.pos.y - 45, 5, GREEN);
                    DrawText(TextFormat("hp: %.0f", creatures[j]->stats.hp), creatures[j]->stats.pos.x - 5, creatures[j]->stats.pos.y - 35, 5, GREEN);
                    DrawText(TextFormat("atk: %.0f", creatures[j]->stats.atk), creatures[j]->stats.pos.x - 5, creatures[j]->stats.pos.y - 25, 5, GREEN);
                    DrawText(TextFormat("hit ch: %.2f", creatures[j]->stats.hit_chance), creatures[j]->stats.pos.x - 5, creatures[j]->stats.pos.y - 15, 5, GREEN);
                }
            }

            if(j++ % 50 == 0) // auto delete killed
            {
                j = 1;
                deleteKilled(creatures);
            }

            if(i++ % convertFromGameTick(FPS) == 0) // game progresses here
            {
                moved_before = false;
                i = 1;
                grid.UpdateCells(creatures);
                for(int j = 0; j < Creature::creature_size; j++)
                {
                    if (creatures[j]->stats.is_alive)
                    {
                        creatures_in_range = grid.GridCheckSight(creatures[j]);
                        creatures[j]->checkSight(creatures_in_range); // determine if j sees any crtr
                        if (creatures[j]->stats.go_after && creatures[j]->stats.go_after->stats.is_alive) // j has someone to go after or not?
                        {
                            // can j attack the crtr it sees?
                            if (CheckCollisionCircleRec(
                            creatures[j]->stats.range_area.center,
                            creatures[j]->stats.range_area.radius,
                            creatures[j]->stats.go_after->stats.pos))
                            {
                                creatures[j]->stats.attack = creatures[j]->stats.go_after;
                                creatures[j]->stats.go_after->stats.attack = creatures[j];
                                // fight between them
                                FightBetween(creatures[j], creatures[j]->stats.go_after);
                                if(creatures[j]->stats.human_controlled)
                                {
                                    MoveByKey(creatures, j);
                                    moved_before = true;
                                }
                            }

                            else
                            {
                                // j can't attack it, so goes after it 
                                creatures[j]->stats.attack = creatures[j]->stats.go_after->stats.attack = nullptr;
                                if(!creatures[j]->stats.human_controlled)
                                    creatures[j]->GoAfter();
                                else
                                {
                                    MoveByKey(creatures, j);
                                    moved_before = true;
                                }
                            }
                        }
                        else if(!creatures[j]->stats.human_controlled)
                            creatures[j]->Wander(); // can't see anyone, wander randomly
                        else if(!moved_before)
                            MoveByKey(creatures, j);
                        if (creatures[j]->stats.is_alive) // update sight and range area centers
                            creatures[j]->stats.sight_area.center = 
                            creatures[j]->stats.range_area.center = raylib::Vector2(creatures[j]->stats.pos.x, creatures[j]->stats.pos.y);
                    }
                }
            }
            
            for(int j = 0; j < Creature::creature_size; j++)
            {
                if (creatures[j]->stats.is_alive && CheckCollisionRecs(creatures[j]->stats.pos, cam_visible_area))
                    creatures[j]->DrawCreature();
            }
        cam2d.EndMode();
        window.EndDrawing();
    }

    for (int j = 0; j < Creature::creature_size; j++)
    {
        delete creatures[j];
    }
    creatures.clear();
    
    return 0; // RAII destroys window, audio, and unloads sound objects cleanly automatically.
}
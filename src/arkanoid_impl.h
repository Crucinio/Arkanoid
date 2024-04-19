#pragma once

#include "arkanoid.h"
#define USE_ARKANOID_IMPL

#include "actors.h"

#include <unordered_map>
#include <string>



// data about the ball-brick hit
struct BrickHitData 
{
    Vect position = Vect(0.0f);
    Vect next_vect = Vect(1.0f);
    int row = 0;
    int column = 0;
};

class ArkanoidImpl : public Arkanoid
{
public:
    // general
    void reset(const ArkanoidSettings& settings, ArkanoidDebugData& debug_data) override;
    void update(ImGuiIO& io, ArkanoidDebugData& debug_data, float elapsed) override;
    void draw(ImGuiIO& io, ImDrawList& draw_list) override;
    ArkanoidImpl() = default;
    ~ArkanoidImpl(); // for bricks

    ArkanoidImpl(ArkanoidImpl& other) = delete;
    void operator = (ArkanoidImpl& other) = delete;
private:
    // debug
    void initialize_brick_collisions(ArkanoidDebugData& debug_data);
    void update_debug_brick_collision(ArkanoidDebugData& debug_data, int i, int j);
    void update_all_debug_brick_collisions(ArkanoidDebugData& debug_data);
    void add_debug_hit(ArkanoidDebugData& debug_data, const Vect& pos, const Vect& normal);
    void add_debug_aim_helper(ArkanoidDebugData& debug_data, float rad, const Vect& pos, const Vect& prediction);

    // logic
    Vect next_hit(const Vect& pos, const Vect& velocity, float radius);
    BrickHitData process_brick_hit(Ball& ball, Vect prev_pos);
    void transform_collisions(float radius, float prev_trans, float new_trans);
    Brick* create_brick(Vect position, Vect size, BrickType type); // factory method to keep track of the init. sequence (pos + size before collision)
    void spawn_ball();
    void spawn_bonus_from_brick(const Brick& brick);
    void process_possible_explosion(int i, int j);
    void apply_explosion(int i, int j, int damage);
    void destroy_brick(int i, int j);
    void execute_bonus(const Bonus& bonus);
    void jackpot(); // all bricks -> 300p
    void turn_random_brick_to_random_explosive(int rows, int columns);

    void initiate_game_over();

    // algebra
    Vect calulate_bounce_vector(float speed, float width, float dist);
    Vect find_intersection(Vect p1, Vect p2, Vect p3, Vect p4);
    float dist_qdr(Vect p1, Vect p2);
    

    // statistics
    int bricks_at_start = 0;
    int bricks_left = 0;
    int bricks_destroyed_100 = 0;
    int bricks_destroyed_200 = 0;
    int bricks_destroyed_300 = 0;
    int explosive_bricks_destroyed = 0;
    int affected_by_explosion = 0;
    long long ball_hits = 0;


    // game mode state
    bool game_start = true;
    bool multiplier_on = true;
    int lives = 3;
    int max_lives = 5;
    int max_balls = 3;
    int hits_to_destroy = 1;
    const int ticks_before_explosion = 5;
    float score_multiplier = 1.0f;
    float muliplier_from_speed = 0;
    float multiplier_from_racket_width = 0;

    unsigned long long score = 0;
    unsigned long long highest_score = 0;

    float ball_radius = 0;
    float ball_initial_speed = 0;
    Vect ball_start_position = Vect(0.0f);

    float bonus_falling_speed = 200.0f;
    float bonus_drop_chance = 0.02f;


    //actors
    Racket racket;
    std::vector<Ball> balls;
    std::vector<Bonus> bonuses;
    std::vector<std::vector<Brick*> > bricks;

    // world
    Vect world_size = Vect(0.0f);
    Vect world_to_screen = Vect(0.0f);
    Vect world_scale = Vect(1.0f);

    // specials
    float world_to_screen_diff_xy = 0.0f;
    float to_horizontal_radius = 0.0f; // specail for ball hits
    const float spec = 0.5f;

    // drawing
    const std::unordered_map<int, ImColor> score_to_color = {
        {100, ImColor(150, 150, 250)},
        {200, ImColor(80, 100, 180)},
        {300, ImColor(255, 200, 0)}
    };
    const ImColor my_text_color = ImColor(255, 255, 200);
    const ImColor new_record_color = ImColor(255, 215, 0);
    const ImColor ball_color = ImColor(100, 100, 200);
    const ImColor ball_outline_color = ImColor(ImColor(0, 0, 50));
    const ImColor racket_color = ImColor(100, 100, 200);
    const ImColor bonus_color = ImColor(173, 216, 230);
    const ImColor inner_bonus = ImColor(100, 180, 200);

    // paddings
    float padding_lives_by_rad = 16.0f;
    float padding_lives_from_bottom = 20.0f;
    float padding_score_text_from_top = 10.0f;
    float padding_playzone_from_top = 40.0f;
    float padding_text_y = 13.0f;
    float max_text_shift_x = 100.0f;
    float text_shift_from_center = 100.0f;
    float padding_x = 20.0f;
    int precision = 2;

    // for optimized brick hit check
    float brick_width = 0.0f;
    float brick_height = 0.0f;
    float brick_padding_x = 0.0f;
    float brick_padding_y = 0.0f; 
};

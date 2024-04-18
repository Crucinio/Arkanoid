#pragma once

#include "base.h"
#include <vector>
#include <array>

struct ArkanoidSettings
{
    // Display
    float display_w; //todo default value
    float display_h; //todo default value

    // World
    bool show_world_settings = false;
    Vect world_size = Vect(800.0f, 600.f);

    // Bricks
    bool show_bricks_settings = false;

    int bricks_columns_count = 15;
    int bricks_rows_count = 7;

    float bricks_columns_padding = 5.0f;
    float bricks_rows_padding = 5.0f;

    static constexpr int bricks_columns_min = 10;
    static constexpr int bricks_columns_max = 30;
    static constexpr int bricks_rows_min = 3;
    static constexpr int bricks_rows_max = 10;

    static constexpr float bricks_columns_padding_min = 5.0f;
    static constexpr float bricks_columns_padding_max = 20.0f;
    static constexpr float bricks_rows_padding_min = 5.0f;
    static constexpr float bricks_rows_padding_max = 20.0f;

   
    static constexpr float explosive_brick_chance_min = 0.0f;
    static constexpr float explosive_brick_chance_max = 1.0f;
    
    float explosive_brick_chance = 0.05f;

    // Bonuses
    bool show_bonuses_settings = false;
    static constexpr float bonus_chance_min = 0.0f;
    static constexpr float bonus_chance_max = 1.0f;

    float bonus_chance_choice = 0.02f;

    static constexpr float bonus_falling_speed_min = 50.0f;
    static constexpr float bonus_falling_speed_max = 500.0f;

    float bonus_speed_choice = 200.0f;

    // Ball
    bool show_ball_settings = false;
    float ball_radius = 10.0f;
    float ball_speed = 300.0f;

    static constexpr unsigned int ball_maximum_qnt = 3;
    static constexpr float ball_radius_min = 5.0f;
    static constexpr float ball_radius_max = 50.0f;
    static constexpr float ball_speed_min = 1.0f;
    static constexpr float ball_speed_max = 1000.0f;

    // Carriege
    bool show_carriege_settings = false;
    float racket_width = 100.0f;
    float carriege_sens = 7.0f;

    static constexpr float racket_width_min = 50.0f;
    static constexpr float racket_sens_min = 1.0f;
    static constexpr float racket_sens_max = 20.0f;

    // Game mode
    bool show_game_mode_settings = false;
    bool random_bricks = false;
    bool multiplier = true;
    bool explosive_bricks = true;

    int starting_lives = 3;
    static constexpr int starting_lives_min = 1;
    static constexpr int starting_lives_max = 5;

    int hits_for_brick_to_destroy = 1;
    static constexpr int hits_for_brick_to_destroy_min = 1;
    static constexpr int hits_for_brick_to_destroy_max = 3;
};

struct ArkanoidDebugData
{
    struct Hit
    {
        Vect screen_pos;        // Hit position, in screen space
        Vect normal;            // Hit normal
        float time = 0.0f;      // leave it default
    };

    struct AimHelper {
        Vect screen_p1;
        Vect screen_p2;
        Vect screen_p3 = Vect(0.0f);
        float screen_radius;
    };

    struct BrickCollisionDebug {
        std::array<Vect, 8> screen_points;
        bool is_visible = true; //todo rename visible
    };
    
    bool god_mode = false;

    std::vector<Hit> hits;
    std::vector<AimHelper> aim_helpers;
    std::vector < std::vector<BrickCollisionDebug> > bricks_collisions;
    //std::vector<BrickCenterCollision> brick_collisions;
};

class Arkanoid
{
protected:
    bool game_over = false;

public:
    virtual ~Arkanoid() = default;
    // added debug data to initialise brick collisions
    virtual void reset(const ArkanoidSettings& settings, ArkanoidDebugData& debug_data) = 0;
    virtual void draw(ImGuiIO& io, ImDrawList& draw_list) = 0;
    virtual void update(ImGuiIO& io, ArkanoidDebugData& debug_data, float elapsed) = 0;

    //todo обычно пишут на bool метод, is -> is_game_over() ...
    bool get_game_over() { return game_over; };
};

extern Arkanoid* create_arkanoid();


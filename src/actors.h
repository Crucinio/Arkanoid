#pragma once

#include <unordered_map>
#include <string>
#include <array>
#include "base.h"

enum class BrickType {
    Normal,
    Explosive
};

enum class ExplosionType {
    Vertical,
    Horizontal,
    Diagonal,
    Radial,
};

enum class BonusType {
    AnotherBall,
    ExtraLife,
    TurnBrickIntoExplosive,
    Jackpot
    //DoubleWidth - not good
};

// default brick
struct Brick
{

    struct BrickCollision
    {
        BrickCollision() = default;
        BrickCollision(const Brick& brick, float radius, float to_horizontal_radius);

        std::array<Vect, 8> points;
        bool visible = true;
        void update(float radius, float prev_radius_multiplier_x, float new_radius_multiplier_x);
   };

    Brick() = default;
    Brick(const Brick& brick);
    
    virtual BrickType get_brick_type();

    BrickCollision collision;
    Vect brick_pos = Vect(0.0f);
    Vect brick_size = Vect(0.0f);

    bool can_be_damaged = true; // obstacle or not
    int hits_left = 1;
    int score = 100;
    int ticks_before_explosion = -1;
};

// deals damage by ExplosionType pattern within the distance of expolosion_dist
struct ExplosiveBrick : Brick 
{
    ExplosiveBrick() = default;
    ExplosiveBrick(const Brick& brick) : Brick::Brick(brick) {};

    BrickType get_brick_type() override;

    int explosion_dist = 1;
    unsigned int damage = 1;
    ExplosionType expl_type = ExplosionType::Radial;
};

// ball, we can have multiple balls
struct Ball
{
    Vect position = Vect(0.0f);
    Vect velocity = Vect(0.0f);
    float radius = 0.0f;
    float initial_speed = 0.0f;
    float speed = 0;

    bool active = true;
    bool on_start = true;
};

// our racket
struct Racket
{
    Vect position = Vect(0.0f);
    float sensitivity = 0.0f;
    float width = 0.0f;
    float basic_height = 0.0f;
};

// bonus, falling, can be catched 
struct Bonus
{
    Bonus() = default;
    Bonus(const Brick& brick);

    Vect position = Vect(0.0f);
    Vect size = Vect(0.0f);
    float falling_speed = 0.0f;
    BonusType type = BonusType::AnotherBall;
};


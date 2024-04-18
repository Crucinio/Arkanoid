#pragma once

#include <unordered_map>
#include <string>
#include <array>
#include "base.h"

enum ExplosionType {
    Vertical,
    Horizontal,
    Diagonal,
    Radial,
};

enum BonusType {
    AnotherBall,
    ExtraLife,
    TurnBrickIntoExplosive,
    Jackpot
    //DoubleWidth - not good
};

// default brick
struct Brick {

    struct BrickCollision {
        std::array<Vect, 8> points;
        bool is_visible = true;
        BrickCollision() = default;
        BrickCollision(const Brick& brick, float radius, float to_horizontal_radius);
        void to_new_horizontal_radius(float rad, float prev_trans, float trans_radius);
    };

    virtual std::string get_brick_type();

    Brick(const Brick& brick);
    Brick() = default;

    BrickCollision collision;
    Vect brick_pos = Vect(0.0f);
    Vect brick_size = Vect(0.0f);
    bool can_be_damaged = true; // obstacle or not
    int hits_left = 1;
    int score = 100;

    int ticks_before_explosion = -1; // for explosion
};

// deals damage by ExplosionType pattern within the distance of expolosion_dist
struct ExplosiveBrick : Brick {
    ExplosiveBrick(const Brick& brick) : Brick::Brick(brick) {};
    std::string get_brick_type() override;
    int explosion_dist = 1;
    unsigned int damage = 1;
    ExplosionType type = Radial;
};

// ball, we can have multiple balls
struct Ball {
    Vect position = Vect(0.0f);
    Vect velocity = Vect(0.0f);
    float radius = 0.0f;
    float initial_speed = 0.0f;
    float speed = 0;

    bool is_active = true;
    bool on_start = true;
};

// our racket
struct Racket {
    Vect position = Vect(0.0f);
    float sensitivity = 0.0f;
    float width = 0.0f;
    float basic_height = 0.0f;
};

// bonus, falling, can be catched 
struct Bonus {
    Bonus() = default;
    Bonus(const Brick& brick);
    Vect position = Vect(0.0f);
    Vect size = Vect(0.0f);
    float falling_speed = 0.0f;
    BonusType type = AnotherBall;
};


#include "actors.h"

Brick::BrickCollision::BrickCollision(const Brick& brick, float radius, float to_horizontal_radius)
{
    points[0] = Vect(brick.brick_pos.x, brick.brick_pos.y - radius);
    points[1] = points[0] + Vect(-radius * to_horizontal_radius, radius);
    points[2] = points[1] + Vect(0, brick.brick_size.y);
    points[3] = points[2] + Vect(radius * to_horizontal_radius, radius);
    points[4] = points[3] + Vect(brick.brick_size.x, 0);
    points[5] = points[4] + Vect(radius * to_horizontal_radius, -radius);
    points[6] = points[5] + Vect(0, -brick.brick_size.y);
    points[7] = points[6] + Vect(-radius * to_horizontal_radius, -radius);
}

void Brick::BrickCollision::update(float radius, float prev_radius_multiplier_x, float new_radius_multiplier_x)
{
    float diff = radius * (prev_radius_multiplier_x - new_radius_multiplier_x);
    points[1].x += diff;
    points[2].x += diff;
    points[5].x -= diff;
    points[6].x -= diff;
}

std::string Brick::get_brick_type()
{
    return "Brick";
}

Brick::Brick(const Brick& brick)
{
    collision = brick.collision;
    brick_pos = brick.brick_pos;
    brick_size = brick.brick_size;
    score = brick.score;
    hits_left = brick.hits_left;
    can_be_damaged = brick.can_be_damaged;
}

std::string ExplosiveBrick::get_brick_type()
{
    return "ExplosiveBrick";
}

Bonus::Bonus(const Brick& brick)
{
    size = Vect(brick.brick_size.x, brick.brick_size.y * 2.0f);
    position = brick.brick_pos;
}
#include "actors.h"
#include "actors.h"
#include "actors.h"
#include "actors.h"

Brick::BrickCollision::BrickCollision(const Brick& brick, float radius, float to_horizontal_radius)
{
    points[0] = Vect(brick.position.x, brick.position.y - radius);
    points[1] = points[0] + Vect(-radius * to_horizontal_radius, radius);
    points[2] = points[1] + Vect(0, brick.size.y);
    points[3] = points[2] + Vect(radius * to_horizontal_radius, radius);
    points[4] = points[3] + Vect(brick.size.x, 0);
    points[5] = points[4] + Vect(radius * to_horizontal_radius, -radius);
    points[6] = points[5] + Vect(0, -brick.size.y);
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

BrickType Brick::get_brick_type()
{
    return BrickType::Normal;
}

Brick::Brick(const Brick& brick)
{
    collision = brick.collision;
    position = brick.position;
    size = brick.size;
    score = brick.score;
    hits_left = brick.hits_left;
}

ExplosiveBrick::ExplosiveBrick(ExplosionType _explosion_type)
{
    explosion_type = _explosion_type;
}

BrickType ExplosiveBrick::get_brick_type()
{
    return BrickType::Explosive;
}

Bonus::Bonus(const Brick& brick)
{
    size = Vect(brick.size.x, brick.size.y * 2.0f);
    position = brick.position;
}

Bonus::Bonus(const Brick& brick, float _falling_speed, BonusType _type) : Bonus(brick)
{
    falling_speed = _falling_speed;
    type = _type;
}

Ball::Ball(Vect _position, Vect _velocity, float _radius, float _initial_speed)
{
    position = _position;
    velocity = _velocity;
    radius = _radius;
    initial_speed = _initial_speed;
}
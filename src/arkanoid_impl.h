#pragma once

#include "arkanoid.h"

#define USE_ARKANOID_IMPL

class ArkanoidImpl : public Arkanoid
{
public:
    void reset(const ArkanoidSettings& settings) override;
    void update(ImGuiIO& io, ArkanoidDebugData& debug_data, float elapsed) override;
    void draw(ImGuiIO& io, ImDrawList& draw_list) override;

private:
    void demo_update(ImGuiIO& io, ArkanoidDebugData& debug_data, float elapsed);
    void demo_draw(ImGuiIO& io, ImDrawList& draw_list);
    void demo_add_debug_hit(ArkanoidDebugData& debug_data, const Vect& pos, const Vect& normal);
    
    Vect demo_world_size = Vect(0.0f);
    Vect demo_world_to_screen = Vect(0.0f);
    Vect demo_ball_position = Vect(0.0f);
    Vect demo_ball_velocity = Vect(0.0f);
    float demo_ball_radius = 0.0f;
    float demo_ball_initial_speed = 0.0f;
};

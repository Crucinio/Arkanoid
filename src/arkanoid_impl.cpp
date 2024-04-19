#include "arkanoid_impl.h"

#include <GLFW/glfw3.h>
#include <math.h>
#include <random>
#include <sstream>
#include <memory>


#ifdef USE_ARKANOID_IMPL
Arkanoid* create_arkanoid()
{
    srand(time(NULL)); // random brick scores every session
    return new ArkanoidImpl();
}
#endif

// for Text in drawing
template <typename T>
std::string to_string_with_precision(const T value, const int n = 2)
{
    std::ostringstream out;
    out.precision(n);
    out << std::fixed << value;
    return std::move(out).str();
} 

void ArkanoidImpl::reset(const ArkanoidSettings& settings, ArkanoidDebugData& debug_data)
{
    // clearing containers
    endgame_clear();

    // Game mode
    hits_to_destroy = settings.hits_for_brick_to_destroy;
    lives = settings.starting_lives;
    if (score > highest_score)
        highest_score = score;

    score = 0;
    game_over = false;
    game_start = true;
    max_lives = settings.starting_lives_max;
    max_balls = settings.ball_maximum_qnt;

    // multiplier
    multiplier_on = settings.multiplier;
    if (multiplier_on)
    {
        muliplier_from_speed = (settings.ball_speed - 300.0f) / 1000.0f;
        if (settings.racket_width < 100.0f)
            multiplier_from_racket_width = (100.0f - settings.racket_width) / 100.0f;
        else if (settings.racket_width > 400.0f)
            multiplier_from_racket_width = -1.0f;
        else
            multiplier_from_racket_width = -((settings.racket_width - 100.0f) / 300.0f);

        score_multiplier = 1.0f + muliplier_from_speed + multiplier_from_racket_width;
        if (score_multiplier < 0.0f)
            score_multiplier = 0.0f;
    }
    else
    {
        score_multiplier = 1.0f;
    }

    // statistics
    bricks_at_start = settings.bricks_rows_count * settings.bricks_columns_count;
    bricks_left = bricks_at_start;
    bricks_destroyed_100 = 0;
    bricks_destroyed_200 = 0;
    bricks_destroyed_300 = 0;
    explosive_bricks_destroyed = 0;
    affected_by_explosion = 0;
    ball_hits = 0;

    // World  screen
    world_size.x = settings.world_size[0];
    world_size.y = settings.world_size[1];
    world_to_screen = Vect(settings.display_w / world_size.x, settings.display_h / world_size.y);
    world_to_screen_diff_xy = world_to_screen.x - world_to_screen.y;
    to_horizontal_radius = world_to_screen.y / world_to_screen.x;
    world_scale = Vect(800.0f / world_size.x, 600.0f / world_size.y);

    // Scaling
    padding_text_y = 13.0f / world_scale.y;
    max_text_shift_x = 100.0f / world_scale.x;
    padding_x = 20.0f / world_scale.x;
    text_shift_from_center = 100.0f * world_scale.x;
    padding_playzone_from_top = 40.0f / world_scale.y;
    padding_score_text_from_top = 10.0f / world_scale.y;
    brick_padding_x = settings.bricks_columns_padding / world_scale.x;
    brick_padding_y = settings.bricks_rows_padding / world_scale.y;
    racket.sensitivity = settings.carriege_sens / world_scale.x;
    padding_lives_by_radius = 16.0f / world_scale.y;
    padding_lives_from_bottom = 20.0f / world_scale.y;
    bonus_falling_speed = settings.bonus_speed_choice / world_scale.y;
    bonus_drop_chance = settings.bonus_chance_choice;

    // Racket
    racket.basic_height = (settings.world_size.y * 0.02f);
    racket.width = settings.racket_width;
    racket.position = Vect(settings.world_size.x * 0.5 - settings.racket_width * 0.5f, settings.world_size.y * 0.9f);

    // Ball(s)
    ball_radius = settings.ball_radius;
    ball_initial_speed = settings.ball_speed;
    spawn_ball();

    // Bricks init
    int rows = settings.bricks_rows_count;
    int columns = settings.bricks_columns_count;

    brick_width = (settings.world_size.x - (columns + 1) * brick_padding_x) / columns;
    brick_height = (settings.world_size.y * 0.3f) / rows;

    bricks = std::vector<std::vector<std::shared_ptr<Brick> > >(rows);
    for (int i = 0; i < rows; ++i)
    {
        bricks[i].reserve(columns);
        for (int j = 0; j < columns; ++j)
        {
            std::shared_ptr<Brick> brick;

            int chance = 100 * settings.explosive_brick_chance;
            if (settings.explosive_bricks && chance > (rand() % 100))
                brick = create_brick(Vect((j + 1) * brick_padding_x + j * brick_width, padding_playzone_from_top + i * (brick_padding_y + brick_height)),
                    Vect(brick_width, brick_height),
                    BrickType::Explosive);
            else
                brick = create_brick(Vect((j + 1) * brick_padding_x + j * brick_width, padding_playzone_from_top + i * (brick_padding_y + brick_height)),
                    Vect(brick_width, brick_height),
                    BrickType::Normal);

            brick->hits_left = hits_to_destroy;
            if (settings.random_bricks)
            {
                brick->score = 100 * (rand() % 3 + 1);
            }
            else
            {
                int r = settings.bricks_rows_count / 3;
                if (i < r)
                    brick->score = 300;
                else if (i < r * 2)
                    brick->score = 200;
                else
                    brick->score = 100;
            }

            bricks[i].push_back(brick);
        }
    }

    update_all_debug_brick_collisions(debug_data);
}

void ArkanoidImpl::update(ImGuiIO& io, ArkanoidDebugData& debug_data, float elapsed)
{
    // screen and transform
    world_to_screen = Vect(io.DisplaySize.x / world_size.x, io.DisplaySize.y / world_size.y);
    world_to_screen_diff_xy = world_to_screen.x - world_to_screen.y;
    float cur_hor_radius = to_horizontal_radius;
    to_horizontal_radius = world_to_screen.y / world_to_screen.x;
    if (cur_hor_radius != to_horizontal_radius)
    {
        update_collision_scaling(ball_radius, cur_hor_radius, to_horizontal_radius);
        update_all_debug_brick_collisions(debug_data);
    }

    if (game_over)
        return;

    // process user input
    {
        if (io.KeysDown[GLFW_KEY_SPACE])
        {
            for (Ball& ball : balls)
                ball.on_start = false;

            game_start = false;
        } // launching balls on racket

        bool ignore_ctrl = true;
        for (Ball& ball : balls)
            if (ball.on_start)
            {
                ignore_ctrl = false;
                break;
            } // Check if there are balls on racket

        if (!ignore_ctrl && io.KeysDown[GLFW_KEY_LEFT_CONTROL])
        {
            for (Ball& ball : balls)
            {
                if (ball.on_start)
                {
                    if (io.KeysDown[GLFW_KEY_A])
                    {
                        ball.position.x -= racket.sensitivity / 2;

                        if (ball.position.x < racket.position.x)
                            ball.position.x = racket.position.x;

                        if (ball.position.x < ball.radius * to_horizontal_radius)
                            ball.position.x = ball.radius * to_horizontal_radius;
                    }

                    if (io.KeysDown[GLFW_KEY_D])
                    {
                        ball.position.x += racket.sensitivity / 2;

                        if (ball.position.x > racket.position.x + racket.width)
                            ball.position.x = racket.position.x + racket.width;

                        if (ball.position.x > world_size.x - ball.radius * to_horizontal_radius)
                            ball.position.x = world_size.x - ball.radius * to_horizontal_radius;
                    }
                }
            }
        } // Control balls on racket
        else
        {
            if (io.KeysDown[GLFW_KEY_A])
            {
                racket.position.x -= racket.sensitivity;
                if (racket.position.x < 0)
                {
                    racket.position.x = 0;
                }
                else
                {
                    for (Ball& ball : balls)
                    {
                        if (ball.on_start)
                        {
                            ball.position.x -= racket.sensitivity;

                            if (ball.position.x < ball.radius * to_horizontal_radius)
                                ball.position.x = ball.radius * to_horizontal_radius;
                        }
                    }
                }
            }

            if (io.KeysDown[GLFW_KEY_D])
            {
                racket.position.x += racket.sensitivity;

                if (racket.position.x > world_size.x - racket.width)
                {
                    racket.position.x = world_size.x - racket.width;
                }
                else
                {
                    for (Ball& ball : balls)
                    {
                        if (ball.on_start)
                        {
                            ball.position.x += racket.sensitivity;

                            if (ball.position.x > world_size.x - ball.radius * to_horizontal_radius)
                                ball.position.x = world_size.x - ball.radius * to_horizontal_radius;
                        }
                    }
                }
            }
        } // Control Racket
    }

    // balls handling
    {
        std::vector<Ball>::iterator it = balls.begin();
        while (it != balls.end())
        {
            Vect previous_position = it->position;
            // update ball position according
            // its velocity and elapsed time
            if (!it->on_start)
            {
                it->position += it->velocity * elapsed;
            }
            else
            {
                it->velocity = calulate_bounce_vector(it->initial_speed, racket.width, racket.position.x + racket.width * 0.5f - it->position.x);
                Vect next = next_hit(it->position, it->velocity, it->radius);
                add_debug_aim_helper(debug_data, it->radius, it->position, next);
                ++it;
                continue;
            }

            // kill zone/active zone
            {
                if (!debug_data.god_mode)
                {
                    if (previous_position.y > world_size.y + it->radius)
                    {
                        it = balls.erase(it);
                        continue;
                    }

                    if (!it->active)
                    {
                        ++it;
                        continue;
                    }

                    if (previous_position.y > racket.position.y + 0.5f * racket.basic_height)
                    {
                        it->active = false;
                        ++it;
                        continue;
                    }
                }
            }

            // hit border processing;
            {
                // special scaling for horizontal since we scale radius in "draw" with Y
                // if we don't there is a scaling problem (hitting before or after the display hit)
                if (it->position.x < it->radius * to_horizontal_radius)
                {
                    it->position.x = it->radius * to_horizontal_radius;
                    it->velocity.x *= -1.0f;

                    add_debug_hit(debug_data, Vect(0, it->position.y), Vect(1, 0));
                }
                else if (it->position.x > (world_size.x - it->radius * to_horizontal_radius))
                {
                    it->position.x = world_size.x - it->radius * to_horizontal_radius;
                    it->velocity.x *= -1.0f;

                    add_debug_hit(debug_data, Vect(world_size.x, it->position.y), Vect(-1, 0));
                }

                if (it->position.y < it->radius)
                {
                    it->position.y += (it->radius - it->position.y) * 2.0f;
                    it->velocity.y *= -1.0f;

                    add_debug_hit(debug_data, Vect(it->position.x, 0), Vect(0, 1));
                }
                else if (debug_data.god_mode && it->position.y > (world_size.y - it->radius))
                {
                    it->position.y -= (it->position.y - (world_size.y - it->radius)) * 2.0f;
                    it->velocity.y *= -1.0f;

                    add_debug_hit(debug_data, Vect(it->position.x, world_size.y), Vect(0, -1));
                }
            }

            // racket hit processing
            if (racket.position.x - it->radius * to_horizontal_radius * spec < it->position.x
                && racket.position.x + racket.width + it->radius * to_horizontal_radius * spec > it->position.x
                && racket.position.y - it->radius < it->position.y)
            {
                // if we catch it below the racket line
                if (previous_position.y > racket.position.y - it->radius && it->position.y < racket.position.y + racket.basic_height * 0.5)
                {
                    it->position.y = racket.position.y - it->radius;
                }
                else
                {
                    double diff = it->position.y - racket.position.y + it->radius;
                    float diff_t = diff / it->velocity.y;
                    it->position.y = racket.position.y - it->radius;
                    it->position.x = it->position.x - diff_t * it->velocity.x;
                } // precise calculation of the hit pos for the formula

                it->velocity = calulate_bounce_vector(it->initial_speed, racket.width, racket.position.x + racket.width * 0.5f - it->position.x);

                add_debug_hit(debug_data, Vect(it->position.x, it->position.y), Vect(0, -1));
            }

            // hit bricks processing
            BrickHitData  data = process_brick_hit(*it, previous_position);

            float left = elapsed;
            while (data.next_vect != Vect(1.0f) && elapsed > 0)
            {
                --bricks[data.row][data.column]->hits_left;
                if (!bricks[data.row][data.column]->hits_left)
                {
                    bricks[data.row][data.column]->collision.visible = false;
                    score += bricks[data.row][data.column]->score * score_multiplier;
                    update_debug_brick_collision(debug_data, data.row, data.column);
                    process_possible_explosion(data.row, data.column);
                }

                it->position = data.position; // !
                // I want to not only place the ball into the intersection position,
                // but also calculate all the hits it could have possibly had between the ticks
                left -= fabsf(data.position.x - previous_position.x) / fabsf(it->velocity.x);
                it->velocity *= data.next_vect;
                previous_position = it->position;
                it->position += left * it->velocity;

                // debug brick hit
                if (data.next_vect.x < 0)
                {
                    if (it->velocity.x > 0)
                        add_debug_hit(debug_data, data.position, Vect(1, 0));
                    else
                        add_debug_hit(debug_data, data.position, Vect(-1, 0));
                }
                else
                {
                    if (it->velocity.y > 0)
                        add_debug_hit(debug_data, data.position, Vect(0, 1));
                    else
                        add_debug_hit(debug_data, data.position, Vect(0, -1));
                }

                data = process_brick_hit(*it, previous_position);
            }

            // debug aim helper
            Vect next_hit_world = next_hit(it->position, it->velocity, it->radius);
            add_debug_aim_helper(debug_data, it->radius, it->position, next_hit_world);
            if (next_hit_world.y == racket.position.y - it->radius)
                add_debug_aim_helper(debug_data, it->radius, next_hit_world, next_hit(next_hit_world, calulate_bounce_vector(it->initial_speed, racket.width, racket.position.x + racket.width * 0.5f - next_hit_world.x), it->radius));

            ++it;
        }

        balls.shrink_to_fit();
    }

    // bricks
    for (int i = 0; i < bricks.size(); ++i)
        for (int j = 0; j < bricks[0].size(); ++j)
        {
            if (!bricks[i][j])
                continue;

            if (bricks[i][j]->ticks_before_explosion > 0)
            {
                --bricks[i][j]->ticks_before_explosion;
                if (ticks_before_explosion == 0)
                    destroy_brick(debug_data, i, j);
            }
            else if (bricks[i][j]->hits_left == 0)
            {
                destroy_brick(debug_data, i, j);
            }
        }

    if (bricks_destroyed_100 + bricks_destroyed_200 + bricks_destroyed_300 == bricks_at_start) {
        game_over = true;
        endgame_clear();
        return;
    }

    // bonuses
    std::vector<Bonus>::iterator it = bonuses.begin();
    while (it != bonuses.end())
    {
        it->position.y += it->falling_speed * elapsed;
        if (it->position.y > world_size.y)
        {
            it = bonuses.erase(it);
            continue;
        }

        if (it->position.x < racket.position.x + racket.width &&
            it->position.x + it->size.x > racket.position.x &&
            it->position.y + it->size.y > racket.position.y &&
            it->position.y < racket.position.y + racket.basic_height)
        {

            execute_bonus(*it);
            it = bonuses.erase(it);
        }
        else
        {
            ++it;
        }
    }

    bonuses.shrink_to_fit();

    // lives
    if (balls.size() < 1)
    {
        --lives;
        if (lives == 0)
        {
            game_over = true;
            endgame_clear();
            return;
        }

        spawn_ball();
    }
}

void ArkanoidImpl::draw(ImGuiIO& io, ImDrawList &draw_list)
{
    draw_list.AddLine(Vect(0, racket.position.y + 0.5f * racket.basic_height) * world_to_screen, Vect(world_size.x, racket.position.y + 0.5f * racket.basic_height) * world_to_screen, ImColor(50, 50, 100), (2.0f * world_to_screen.y) / world_scale.y);
    
    // Text
    float text_size = padding_text_y * world_to_screen.y;
    float text_pos_x = (world_size.x / 2.0f - max_text_shift_x) * world_to_screen.x;
    float text_pos_y_center = (world_size.y / 2.0f) * world_to_screen.y;
   
    { 
        //drawing score
        draw_list.AddText(nullptr, padding_text_y * world_to_screen.y, Vect(padding_x, padding_score_text_from_top) * world_to_screen, my_text_color, ("HIGHEST SCORE: " + std::to_string(highest_score)).c_str());
        if (score > highest_score)
            draw_list.AddText(nullptr, padding_text_y * world_to_screen.y, Vect(padding_x, padding_score_text_from_top + padding_text_y) * world_to_screen, new_record_color, ("SCORE: " + std::to_string(score)).c_str());
        else
            draw_list.AddText(nullptr, padding_text_y * world_to_screen.y, Vect(padding_x, padding_score_text_from_top + padding_text_y) * world_to_screen, my_text_color, ("SCORE: " + std::to_string(score)).c_str());
        
        if (game_over)
        {
            int r = 0;
            draw_list.AddText(nullptr, text_size, Vect(text_pos_x, text_pos_y_center) + Vect(0, r++ * padding_text_y) * world_to_screen, my_text_color, "GAME OVER");
            draw_list.AddText(nullptr, text_size, Vect(text_pos_x, text_pos_y_center) + Vect(0, r++ * padding_text_y) * world_to_screen, my_text_color, "Statistics:");
            if (score >= highest_score)
                draw_list.AddText(nullptr, text_size, Vect(text_pos_x, text_pos_y_center) + Vect(0, r++ * padding_text_y) * world_to_screen, my_text_color, ("Final score: " + std::to_string(score) + " (Your highest!)").c_str());
            else
                draw_list.AddText(nullptr, text_size, Vect(text_pos_x, text_pos_y_center) + Vect(0, r++ * padding_text_y) * world_to_screen, my_text_color, ("Final score: " + std::to_string(score)).c_str());

            draw_list.AddText(nullptr, text_size, Vect(text_pos_x, text_pos_y_center) + Vect(0, r++ * padding_text_y) * world_to_screen, my_text_color, ("Bricks destroyed: " + std::to_string(bricks_destroyed_100 + bricks_destroyed_200 + bricks_destroyed_300) + "/" + std::to_string(bricks_at_start)).c_str());
            draw_list.AddText(nullptr, text_size, Vect(text_pos_x, text_pos_y_center) + Vect(0, r++ * padding_text_y) * world_to_screen, my_text_color, ("Bricks destroyed (100p): " + std::to_string(bricks_destroyed_100)).c_str());
            draw_list.AddText(nullptr, text_size, Vect(text_pos_x, text_pos_y_center) + Vect(0, r++ * padding_text_y) * world_to_screen, my_text_color, ("Bricks destroyed (200p): " + std::to_string(bricks_destroyed_200)).c_str());
            draw_list.AddText(nullptr, text_size, Vect(text_pos_x, text_pos_y_center) + Vect(0, r++ * padding_text_y) * world_to_screen, my_text_color, ("Bricks destroyed (300p): " + std::to_string(bricks_destroyed_300)).c_str());
            draw_list.AddText(nullptr, text_size, Vect(text_pos_x, text_pos_y_center) + Vect(0, r++ * padding_text_y) * world_to_screen, my_text_color, ("Explosions: " + std::to_string(explosive_bricks_destroyed)).c_str());
            draw_list.AddText(nullptr, text_size, Vect(text_pos_x, text_pos_y_center) + Vect(0, r++ * padding_text_y) * world_to_screen, my_text_color, ("Affected by explosions: " + std::to_string(affected_by_explosion)).c_str());
            r += 2;
            draw_list.AddText(nullptr, text_size, Vect(text_pos_x, text_pos_y_center) + Vect(0, r++ * padding_text_y) * world_to_screen, my_text_color, "Press ENTER or RESET to start new game");
        } // Statistics (game over)
        else if (game_start)
        {
            int r = 0;
            draw_list.AddText(nullptr, text_size, Vect(text_pos_x, text_pos_y_center) + Vect(0, r++ * padding_text_y) * world_to_screen, my_text_color, "Controls:");
            draw_list.AddText(nullptr, text_size, Vect(text_pos_x, text_pos_y_center) + Vect(0, r++ * padding_text_y) * world_to_screen, my_text_color, "Use A/D keys to control the racket");
            draw_list.AddText(nullptr, text_size, Vect(text_pos_x, text_pos_y_center) + Vect(0, r++ * padding_text_y) * world_to_screen, my_text_color, "Hold CTRL + A/D to control starting position of the ball");
            draw_list.AddText(nullptr, text_size, Vect(text_pos_x, text_pos_y_center) + Vect(0, r++ * padding_text_y) * world_to_screen, my_text_color, "Press SPACEBAR to launch the ball");
            ++r;

            draw_list.AddText(nullptr, text_size, Vect(text_pos_x, text_pos_y_center) + Vect(0, r++ * padding_text_y) * world_to_screen, my_text_color, "Rules:");
            draw_list.AddText(nullptr, text_size, Vect(text_pos_x, text_pos_y_center) + Vect(0, r++ * padding_text_y) * world_to_screen, my_text_color, ("Recommended world size    W:800.00, H:600.00  (currently: W:" + to_string_with_precision(world_size.x, precision) + ", H:" + to_string_with_precision(world_size.y, precision)).c_str());
            draw_list.AddText(nullptr, text_size, Vect(text_pos_x, text_pos_y_center) + Vect(0, r++ * padding_text_y) * world_to_screen, my_text_color, ("Recommended ball speed    300.00  (currently: " + to_string_with_precision(ball_initial_speed, precision) + ")").c_str());
            draw_list.AddText(nullptr, text_size, Vect(text_pos_x, text_pos_y_center) + Vect(0, r++ * padding_text_y) * world_to_screen, my_text_color, ("Recommended racket width  100.00  (currently: " + to_string_with_precision(racket.width, precision) + ")").c_str());
            
            if (!multiplier_on)
                draw_list.AddText(nullptr, text_size, Vect(text_pos_x, text_pos_y_center) + Vect(0, r++ * padding_text_y) * world_to_screen, ImColor(255, 255, 0), ("Your score multiplier for this round: " + to_string_with_precision(score_multiplier, precision) + "(multiplier is turned off)").c_str());
            else if (score_multiplier > 0.6f && score_multiplier < 1.0f)
                draw_list.AddText(nullptr, text_size, Vect(text_pos_x, text_pos_y_center) + Vect(0, r++ * padding_text_y) * world_to_screen, ImColor(255, 255, 0), ("Your score multiplier for this round: " + to_string_with_precision(score_multiplier, precision)).c_str());
            else if (score_multiplier >= 1.0f && score_multiplier < 1.4f)
                draw_list.AddText(nullptr, text_size, Vect(text_pos_x, text_pos_y_center) + Vect(0, r++ * padding_text_y) * world_to_screen, ImColor(0, 255, 0), ("Your score multiplier for this round: " + to_string_with_precision(score_multiplier, precision)).c_str());
            else if (score_multiplier >= 1.4f)
                draw_list.AddText(nullptr, text_size, Vect(text_pos_x, text_pos_y_center) + Vect(0, r++ * padding_text_y) * world_to_screen, ImColor(100, 100, 255), ("Your score multiplier for this round: " + to_string_with_precision(score_multiplier, precision)).c_str());
            else
                draw_list.AddText(nullptr, text_size, Vect(text_pos_x, text_pos_y_center) + Vect(0, r++ * padding_text_y) * world_to_screen, ImColor(255, 0, 0), ("Your score multiplier for this round: " + to_string_with_precision(score_multiplier, precision)).c_str());

            draw_list.AddText(nullptr, text_size, Vect(text_pos_x, text_pos_y_center) + Vect(0, r++ * padding_text_y) * world_to_screen, my_text_color, "Light purple bricks   100 points");
            draw_list.AddText(nullptr, text_size, Vect(text_pos_x, text_pos_y_center) + Vect(0, r++ * padding_text_y) * world_to_screen, my_text_color, "Dark blue bricks      200 points");
            draw_list.AddText(nullptr, text_size, Vect(text_pos_x, text_pos_y_center) + Vect(0, r++ * padding_text_y) * world_to_screen, my_text_color, "Yellow bricks         300 points");
            draw_list.AddText(nullptr, text_size, Vect(text_pos_x, text_pos_y_center) + Vect(0, r++ * padding_text_y) * world_to_screen, my_text_color, "Enable \"Random brick scores\" option to randomise the bricks layout");
            draw_list.AddText(nullptr, text_size, Vect(text_pos_x, text_pos_y_center) + Vect(0, r++ * padding_text_y) * world_to_screen, my_text_color, "Have fun!");
        } // Rules
    }

    // drawing lives
    float y = world_size.y - padding_lives_from_bottom;
    for (int i = 0; i < lives - 1; ++i)
    {
        draw_list.AddCircleFilled(Vect(padding_x * world_to_screen.x + (padding_lives_by_radius * i * world_to_screen.y), y * world_to_screen.y), (padding_lives_by_radius  / 2.0f) * world_to_screen.y, ball_color);
        draw_list.AddCircle(Vect(padding_x * world_to_screen.x + (padding_lives_by_radius * i * world_to_screen.y), y * world_to_screen.y), (padding_lives_by_radius / 2.0f) * world_to_screen.y, ball_outline_color, 0, 1.0f);
    }

    // drawing bricks
    for (int i = 0; i < bricks.size(); ++i)
    {
        for (int j = 0; j < bricks[0].size(); ++j)
        {
            if (!bricks[i][j])
                continue;

            if (bricks[i][j]->hits_left > 0 && bricks[i][j]->ticks_before_explosion <= 0)
            {
                draw_list.AddRectFilled(bricks[i][j]->position * world_to_screen, (bricks[i][j]->position + bricks[i][j]->size) * world_to_screen, score_to_color.at(bricks[i][j]->score));
                if (bricks[i][j]->get_brick_type() == BrickType::Explosive)
                {
                    ExplosionType type = dynamic_cast<ExplosiveBrick*>(bricks[i][j].get())->explosion_type;
                    if (type == ExplosionType::Radial) 
                    {
                        draw_list.AddCircleFilled((bricks[i][j]->position + bricks[i][j]->size / 2.0f) * world_to_screen, bricks[i][j]->size.y * world_to_screen.y / 3.0f, ImColor(0, 0, 0));
                    }
                    else if (type == ExplosionType::Diagonal)
                    {
                        draw_list.AddLine(bricks[i][j]->position * world_to_screen, (bricks[i][j]->position + bricks[i][j]->size) * world_to_screen, ImColor(0, 0, 0), 2.0f);
                        draw_list.AddLine((bricks[i][j]->position + Vect(0, bricks[i][j]->size.y)) * world_to_screen, (bricks[i][j]->position + Vect(bricks[i][j]->size.x, 0)) * world_to_screen, ImColor(0, 0, 0), 2.0f);
                    }
                    else if (type == ExplosionType::Horizontal)
                    {
                        draw_list.AddLine((bricks[i][j]->position + Vect(0, bricks[i][j]->size.y / 2.0f)) * world_to_screen, (bricks[i][j]->position + Vect(bricks[i][j]->size.x, bricks[i][j]->size.y / 2.0f)) * world_to_screen, ImColor(0, 0, 0), 2.0f);
                    }
                    else if (type == ExplosionType::Vertical)
                    {
                        draw_list.AddLine((bricks[i][j]->position + Vect(bricks[i][j]->size.x / 2.0f, 0)) * world_to_screen, (bricks[i][j]->position + Vect(bricks[i][j]->size.x / 2.0f, bricks[i][j]->size.y)) * world_to_screen, ImColor(0, 0, 0), 2.0f);

                    }
                }
            }
            else if (bricks[i][j]->ticks_before_explosion > 0)
                draw_list.AddRectFilled(bricks[i][j]->position * world_to_screen, (bricks[i][j]->position + bricks[i][j]->size) * world_to_screen, ImColor(255, 0, 0));
        }
    }

    // drawing bonuses
    for (int i = 0; i < bonuses.size(); ++i)
    {
        draw_list.AddRectFilled(bonuses[i].position * world_to_screen, (bonuses[i].position + bonuses[i].size) * world_to_screen, bonus_color);
        if (bonuses[i].type == BonusType::AnotherBall)
            draw_list.AddCircleFilled((bonuses[i].position + bonuses[i].size / 2.0f) * world_to_screen, bonuses[i].size.x / 4.0f, inner_bonus);
        else if (bonuses[i].type == BonusType::ExtraLife)
            draw_list.AddText(nullptr, text_size * 2, ((bonuses[i].position + bonuses[i].size / 2.0f) - Vect(padding_x, text_size)) * world_to_screen, inner_bonus, "+ 1");
        else if (bonuses[i].type == BonusType::Jackpot)
            draw_list.AddCircleFilled((bonuses[i].position + bonuses[i].size / 2.0f) * world_to_screen, bonuses[i].size.x / 4.0f, score_to_color.at(300));
        else if (bonuses[i].type == BonusType::TurnBrickIntoExplosive)
            draw_list.AddText(nullptr, text_size * 2, ((bonuses[i].position + bonuses[i].size / 2.0f) - Vect(padding_x, text_size)) * world_to_screen, inner_bonus, "BOOM");

        draw_list.AddRect(bonuses[i].position * world_to_screen, (bonuses[i].position + bonuses[i].size) * world_to_screen, inner_bonus, 0, 0, (4.0f * world_to_screen.y / world_scale.y));
    }

    // drawing carriege
    Vect screen_pos = racket.position * world_to_screen;
    float screen_width = racket.width * world_to_screen.x;
    float screen_height = racket.basic_height * world_to_screen.y;
    draw_list.AddRectFilled(screen_pos, screen_pos + Vect(screen_width, screen_height), racket_color);
    draw_list.AddRect(screen_pos, screen_pos + Vect(screen_width, screen_height), ImColor(50, 50, 200), 0, 0, 3.0f);

    // drawing balls
    for (auto& ball : balls)
    {
        Vect screen_pos = ball.position * world_to_screen;
        float screen_radius = ball.radius * world_to_screen.y;
        draw_list.AddCircleFilled(screen_pos, screen_radius, ball_color);
        draw_list.AddCircle(screen_pos, screen_radius, ball_outline_color);
    }
}

ArkanoidImpl::~ArkanoidImpl()
{
    bricks.clear();
}

void ArkanoidImpl::update_debug_brick_collision(ArkanoidDebugData& debug_data, int i, int j)
{
    if (i < 0 || i > bricks.size() - 1 || j < 0 || j > bricks[i].size() - 1)
        return;

    if (!bricks[i][j])
        return;

    debug_data.bricks_collisions[i][j] = ArkanoidDebugData::BrickCollisionDebug(bricks[i][j]->collision.points, world_to_screen, bricks[i][j]->hits_left > 0);
}

void ArkanoidImpl::add_debug_hit(ArkanoidDebugData& debug_data, const Vect& world_pos, const Vect& normal)
{
    debug_data.hits.emplace_back(world_pos * world_to_screen, normal);
}

void ArkanoidImpl::update_all_debug_brick_collisions(ArkanoidDebugData& debug_data)
{
    debug_data.bricks_collisions = std::vector<std::vector<ArkanoidDebugData::BrickCollisionDebug> >(bricks.size());
    for (int i = 0; i < bricks.size(); ++i)
    {
        debug_data.bricks_collisions.reserve(bricks[i].size());
        for (int j = 0; j < bricks[i].size(); ++j)
        {
            if (!bricks[i][j]) {
                debug_data.bricks_collisions[i].emplace_back(false);
                continue;
            }

            debug_data.bricks_collisions[i].emplace_back(bricks[i][j]->collision.points, world_to_screen, true);
        }
    }
}

void ArkanoidImpl::add_debug_aim_helper(ArkanoidDebugData& debug_data, float rad, const Vect& pos, const Vect& prediction)
{
    debug_data.aim_helpers.emplace_back(pos * world_to_screen, prediction * world_to_screen, rad * world_to_screen.y);
}

Vect ArkanoidImpl::next_hit(const Vect& position, const Vect& velocity, float radius)
{
    // special cases
    if (velocity.y == 0)
    {
        if (velocity.x > 0)
            return Vect(world_size.x, position.y);
        else
            return Vect(0, position.y);
    }

    if (velocity.x == 0)
    {
        if (velocity.y > 0)
            return Vect(position.x, world_size.y);
        else
            return Vect(position.x, 0);
    }

    float x = 0.0f;
    float y = 0.0f;

    if (velocity.y < 0)
    {
        y = radius;
        x = ((y - position.y) * velocity.x + position.x * velocity.y) / velocity.y;
    } // upper
    else if (velocity.y > 0) {
        y = racket.position.y - radius;
        x = ((y - position.y) * velocity.x + position.x * velocity.y) / velocity.y;
        if (x < racket.position.x || x > racket.position.x + racket.width)
        {
            y = world_size.y - radius;
            x = ((y - position.y) * velocity.x + position.x * velocity.y) / velocity.y;
        }
    } // lower + carriege check

    if (x > radius * to_horizontal_radius && x < world_size.x - radius * to_horizontal_radius)
        return Vect(x, y);

    if (velocity.x < 0)
    {
        x = radius * to_horizontal_radius;
        y = ((x - position.x) * velocity.y + position.y * velocity.x) / velocity.x;
    } // left
    else if (velocity.x > 0)
    {
        x = world_size.x - radius * to_horizontal_radius;
        y = ((x - position.x) * velocity.y + position.y * velocity.x) / velocity.x;
    } // right

    return Vect(x, y);
}

Vect ArkanoidImpl::calulate_bounce_vector(float speed, float width, float dist)
{
    float angle = fabsf((1 - (fabsf(dist) / (width * 0.5f))) * (0.5f * 3.14f)); // arkanoid paddle bounce formula
    if (angle < 0.5f)
        angle = 0.5f;

    if (dist < 0)
        return speed * Vect(cos(angle) * cos(angle), -sin(angle) * sin(angle));
    else
        return speed * Vect(-cos(angle) * cos(angle), -sin(angle) * sin(angle));
}

Vect ArkanoidImpl::find_intersection(Vect p1, Vect p2, Vect p3, Vect p4)
{
    float d1 = (p2.y - p1.y) / (p2.x - p1.x);
    float d2 = (p4.y - p3.y) / (p4.x - p3.x);
    if (p2.x == p1.x)
    {
        if (p4.x == p3.x)
            return Vect(0.0f, 0.0f);
        else
            return Vect(p1.x, d2 * (p1.x - p3.x) + p3.y);
    }

    if (p3.x == p4.x)
        return Vect(p3.x, d1 * (p3.x - p1.x) + p1.y);

    float x = ((d1 * p1.x) + p3.y - p1.y - d2 * p3.x) / (d1 - d2);
    float y = d1 * x - d1 * p1.x + p1.y;
    return Vect(x, y);
}

BrickHitData ArkanoidImpl::process_brick_hit(Ball& ball, Vect prev_pos)
{
    // Step 1
    // Decide the zone affected by the passed ball 
    // (rough estimation)
    int min_i = (fminf(ball.position.y, prev_pos.y) - ball.radius - padding_playzone_from_top) / (brick_padding_y + brick_height);
    int max_i = (fmaxf(ball.position.y, prev_pos.y) + ball.radius - padding_playzone_from_top) / (brick_padding_y + brick_height) + 1;
    // ensuring row range safety
    if (max_i >= bricks.size())
        max_i = bricks.size() - 1;

    if (min_i < 0)
        min_i = 0;

    int min_j = (fminf(ball.position.x, prev_pos.x) - ball.radius * to_horizontal_radius) / (brick_padding_x + brick_width);
    int max_j = (fmaxf(ball.position.x, prev_pos.x) + ball.radius * to_horizontal_radius) / (brick_padding_x + brick_width) + 1;
    // ensuring column range safety
    if (max_j >= bricks[0].size())
        max_j = bricks[0].size() - 1;

    if (min_j < 0)
        min_j = 0;


    // Step 2
    // Given the direction of the ball, 
    // check 8 sides in the hexagon collision corresponding to a brick and keep closest hit point
    BrickHitData res;
    float dist = world_size.x * world_size.x + world_size.y * world_size.y;

    {
        for (int i = min_i; i <= max_i; ++i)
        {
            for (int j = min_j; j <= max_j; ++j)
            {
                if (!bricks[i][j])
                    continue;

                if (bricks[i][j]->hits_left < 1)
                    continue;

                float max_dist = dist_qdr(ball.position, prev_pos);
                float min_dist = max_dist;
                float dist = 0;
                float checker = (ball.radius * ball.radius * to_horizontal_radius) / (ball.radius + ball.radius * to_horizontal_radius);
                Vect intersection(0.0f);

                //  upper left
                Vect p1 = bricks[i][j]->collision.points[0];
                Vect p2 = bricks[i][j]->collision.points[1];
                intersection = find_intersection(p1, p2, prev_pos, ball.position);
                dist = dist_qdr(intersection, prev_pos);
                if (dist < min_dist && dist < max_dist && (ball.velocity.x > 0 || ball.velocity.y > 0) && intersection.x > p2.x && intersection.x < p1.x && intersection.y < p2.y && intersection.y > p1.y)
                {
                    if (ball.velocity.x < 0 || ball.velocity.y > 0 && intersection.y < bricks[i][j]->position.y - checker)
                        res.next_vect = Vect(1.0f, -1.0f);
                    else
                        res.next_vect = Vect(-1.0f, 1.0f);

                    res.position = intersection;
                    res.next_vect = Vect(1.0f, -1.0f);
                    min_dist = dist;
                }

                // left
                p1 = p2;
                p2 = bricks[i][j]->collision.points[2];
                intersection = find_intersection(p1, p2, prev_pos, ball.position);
                dist = dist_qdr(intersection, prev_pos);
                if (dist < min_dist && dist < max_dist && ball.velocity.x > 0 && intersection.y > p1.y && intersection.y < p2.y)
                {
                    res.position = intersection;
                    res.next_vect = Vect(-1.0f, 1.0f);
                    min_dist = dist;
                }

                // lower left
                p1 = p2;
                p2 = bricks[i][j]->collision.points[3];
                intersection = find_intersection(p1, p2, prev_pos, ball.position);
                dist = dist_qdr(intersection, prev_pos);
                if (dist < min_dist && dist < max_dist && (ball.velocity.x >= 0 || ball.velocity.y <= 0) && intersection.x > p1.x && intersection.x < p2.x && intersection.y > p1.y && intersection.y < p2.y)
                {
                    res.position = intersection;
                    if (ball.velocity.x < 0 || ball.velocity.y < 0 && intersection.y > bricks[i][j]->position.y + checker + bricks[i][j]->size.y)
                        res.next_vect = Vect(1.0f, -1.0f);
                    else
                        res.next_vect = Vect(-1.0f, 1.0f);

                    min_dist = dist;
                }

                // bottom
                p1 = p2;
                p2 = bricks[i][j]->collision.points[4];
                intersection = find_intersection(p1, p2, prev_pos, ball.position);
                dist = dist_qdr(intersection, prev_pos);
                if (dist < min_dist && dist < max_dist && ball.velocity.y < 0 && intersection.x > p1.x && intersection.x < p2.x)
                {
                    res.position = intersection;
                    res.next_vect = Vect(1.0f, -1.0f);
                    min_dist = dist;
                }

                // bottom right
                p1 = p2;
                p2 = bricks[i][j]->collision.points[5];
                intersection = find_intersection(p1, p2, prev_pos, ball.position);
                dist = dist_qdr(intersection, prev_pos);
                if (dist < min_dist && dist < max_dist && (ball.velocity.x < 0 || ball.velocity.y < 0) && intersection.x > p1.x && intersection.x < p2.x && intersection.y > p2.y && intersection.y < p1.y)
                {
                    res.position = intersection;
                    if (ball.velocity.x > 0 || ball.velocity.y < 0 && intersection.y > bricks[i][j]->position.y + checker + bricks[i][j]->size.y)
                        res.next_vect = Vect(1.0f, -1.0f);
                    else
                        res.next_vect = Vect(-1.0f, 1.0f);

                    min_dist = dist;
                }

                // right
                p1 = p2;
                p2 = bricks[i][j]->collision.points[6];
                intersection = find_intersection(p1, p2, prev_pos, ball.position);
                dist = dist_qdr(intersection, prev_pos);
                if (dist < min_dist && dist < max_dist && ball.velocity.x < 0 && intersection.y > p2.y && intersection.y < p1.y)
                {
                    res.position = intersection;
                    res.next_vect = Vect(-1.0f, 1.0f);
                    min_dist = dist;
                }

                // upper right
                p1 = p2;
                p2 = bricks[i][j]->collision.points[7];
                intersection = find_intersection(p1, p2, prev_pos, ball.position);
                dist = dist_qdr(intersection, prev_pos);
                if (dist < min_dist && dist < max_dist && (ball.velocity.x < 0 || ball.velocity.y > 0) && intersection.x > p2.x && intersection.x < p1.x && intersection.y > p2.y && intersection.y < p1.y)
                {
                    if (ball.velocity.x > 0 || ball.velocity.y > 0 && intersection.y < bricks[i][j]->position.y - checker)
                        res.next_vect = Vect(1.0f, -1.0f);
                    else
                        res.next_vect = Vect(-1.0f, 1.0f);

                    res.position = intersection;
                    min_dist = dist;
                }

                // upper
                p1 = p2;
                p2 = bricks[i][j]->collision.points[0];
                intersection = find_intersection(p1, p2, prev_pos, ball.position);
                dist = dist_qdr(intersection, prev_pos);
                if (dist < min_dist && dist < max_dist && ball.velocity.y > 0 && intersection.x > p2.x && intersection.x < p1.x)
                {
                    res.position = intersection;
                    res.next_vect = Vect(1.0f, -1.0f);
                    min_dist = dist;
                }

                if (min_dist != max_dist)
                {
                    res.column = j;
                    res.row = i;
                    return res;
                }
            }
        }
    }

    return res;
}

void ArkanoidImpl::update_collision_scaling(float radius, float prev_trans, float new_trans)
{
    for (int i = 0; i < bricks.size(); ++i)
        for (int j = 0; j < bricks[0].size(); ++j)
        {
            if (!bricks[i][j])
                continue;

            bricks[i][j]->collision.update(radius, prev_trans, new_trans);
        }
}

std::shared_ptr<Brick> ArkanoidImpl::create_brick(Vect position, Vect size, BrickType type)
{
    std::shared_ptr<Brick> brick;

    if (type == BrickType::Explosive)
        brick = std::make_shared<ExplosiveBrick>(ExplosiveBrick(static_cast<ExplosionType>(rand() % 4)));
    else
        brick = std::make_shared<Brick>(Brick());

    // before collision creation!!!
    brick->position = position;
    brick->size = size;

    // creation
    brick->collision = Brick::BrickCollision(*brick, ball_radius, to_horizontal_radius);
    return brick; // returning valid brick with position, size and always correct collision
}


float ArkanoidImpl::dist_qdr(Vect p1, Vect p2)
{
    return (p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y);
}

void ArkanoidImpl::spawn_ball()
{
    balls.emplace_back(
        Vect(racket.width / 2.0f + racket.position.x, racket.position.y - ball_radius),
        Vect(0.01 * ball_initial_speed, -ball_initial_speed),
        ball_radius,
        ball_initial_speed );
}

void ArkanoidImpl::spawn_bonus_from_brick(const Brick& brick)
{
    bonuses.emplace_back(brick, bonus_falling_speed, static_cast<BonusType>(rand() % 4));
}

void ArkanoidImpl::process_possible_explosion(int row, int column)
{
    if (row < 0 || column < 0 || row > bricks.size() - 1 || column > bricks[row].size() - 1)
        return;

    if (!bricks[row][column])
        return;

    if (bricks[row][column]->get_brick_type() == BrickType::Explosive)
    {
        ExplosionType type = dynamic_cast<ExplosiveBrick*>(bricks[row][column].get())->explosion_type;
        int d = dynamic_cast<ExplosiveBrick*>(bricks[row][column].get())->explosion_dist;
        int damage = dynamic_cast<ExplosiveBrick*>(bricks[row][column].get())->damage;
        if (type == ExplosionType::Radial)
        {
            for (int i = row - d; i < row + d + 1; ++i)
                apply_explosion(i, column, damage);

            for (int j = column - d; j < column + d + 1; ++j)
                apply_explosion(row, j, damage);
        }
        else if (type == ExplosionType::Vertical)
        {
            for (int i = row - d * 2; i < row + d * 2 + 1; ++i)
                apply_explosion(i, column, damage);
        }
        else if (type == ExplosionType::Horizontal)
        {
            for (int j = column - d * 2; j < column + d * 2 + 1; ++j)
                apply_explosion(row, j, damage);
        }
        else if (type == ExplosionType::Diagonal)
        {
            for (int i1 = -d; i1 < d + 1; ++i1)
            {
                apply_explosion(row + i1, column + i1, damage);
                apply_explosion(row + i1, column - i1, damage);
            }
        }
    }
}

void ArkanoidImpl::apply_explosion(int i, int j, int damage)
{
    if (i < 0 || i > bricks.size() - 1 || j < 0 || j > bricks[i].size() - 1 || damage <= 0)
        return;

    if (!bricks[i][j])
        return;

    if (bricks[i][j]->hits_left > 0)
    {
        bricks[i][j]->ticks_before_explosion = ticks_before_explosion;
        ++affected_by_explosion;
        --bricks[i][j]->hits_left;
        if (bricks[i][j]->hits_left == 0)
        {
            process_possible_explosion(i, j);
            score += bricks[i][j]->score;
        }
    }
}

void ArkanoidImpl::destroy_brick(ArkanoidDebugData& debug_data, int i, int j)
{
    if (i < 0 || i > bricks.size() - 1 || j < 0 || j > bricks[i].size() - 1)
        return;

    if (!bricks[i][j])
        return;

    if (bricks[i][j]->score == 100)
        ++bricks_destroyed_100;
    else if (bricks[i][j]->score == 200)
        ++bricks_destroyed_200;
    else if (bricks[i][j]->score == 300)
        ++bricks_destroyed_300;

    --bricks_left;
    if (bricks[i][j]->get_brick_type() == BrickType::Explosive)
        ++explosive_bricks_destroyed;

    int chance = bonus_drop_chance * 100;
    if (rand() % 100 < chance)
        spawn_bonus_from_brick(*bricks[i][j]);

    update_debug_brick_collision(debug_data, i, j);
    bricks[i][j].reset();
}

void ArkanoidImpl::execute_bonus(const Bonus& bonus)
{
    if (bonus.type == BonusType::AnotherBall && balls.size() < max_balls)
        spawn_ball();
    else if (bonus.type == BonusType::ExtraLife && lives < max_lives)
        ++lives;
    else if (bonus.type == BonusType::TurnBrickIntoExplosive)
        turn_random_brick_to_random_explosive(bricks.size(), bricks[0].size());
    else if (bonus.type == BonusType::Jackpot)
        jackpot();
}

void ArkanoidImpl::jackpot()
{
    for (int i = 0; i < bricks.size(); ++i)
        for (int j = 0; j < bricks[0].size(); ++j)
            if (bricks[i][j])
                bricks[i][j]->score = 300;
}

void ArkanoidImpl::turn_random_brick_to_random_explosive(int rows, int columns)
{
    std::vector<std::pair<int, int> > possibilities;

    // take random brick from every row and check if it is valid for transformation
    int terminate_counter = 100; // tries to choose the brick, chances of it not succeeding
    while (possibilities.size() == 0 && terminate_counter > 0)
    {
        for (int i = 0; i < rows; ++i)
        {
            int rand_j = rand() % columns;
            if (bricks[i][rand_j] && bricks[i][rand_j]->get_brick_type() != BrickType::Explosive)
                possibilities.emplace_back(i, rand_j);
        }

        --terminate_counter;
    }

    if (terminate_counter)
    {
        int k = rand() % possibilities.size();
        int i = possibilities[k].first;
        int j = possibilities[k].second;

        bricks[i][j].reset(new ExplosiveBrick(*bricks[i][j]));
        dynamic_cast<ExplosiveBrick*>(bricks[i][j].get())->explosion_type = static_cast<ExplosionType>(rand() % 4);
        dynamic_cast<ExplosiveBrick*>(bricks[i][j].get())->explosion_dist = 1;
        dynamic_cast<ExplosiveBrick*>(bricks[i][j].get())->damage = 1;
    }
    else
    {
        score += 1000 * score_multiplier;
    }
}

void ArkanoidImpl::endgame_clear()
{
    balls.clear();
    bonuses.clear();
    bricks.clear();
    lives = 0;
    racket.position = Vect(world_size.x * 0.5f, racket.position.y) - Vect(racket.width * 0.5f, 0);
}



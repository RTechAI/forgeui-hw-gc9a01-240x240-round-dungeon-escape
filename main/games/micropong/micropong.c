/* ForgeUI MicroPong - RTechAI. Circular court, no hardware dependencies. */
#include "micropong.h"
#include <math.h>
#include "lvgl.h"

#define DEG (0.01745329252f)
#define CONTACT_RADIUS 102.0f
#define PADDLE_HALF 18.0f
#define WIN_SCORE 7
#define STEP_MS 5

typedef enum { SERVING, RALLY, FINISHED } pong_phase_t;
static pong_phase_t phase;
static float player_angle, ai_angle, x, y, vx, vy, speed;
static unsigned player_score, ai_score, serve_number;
static uint32_t last_tick, accumulator, serve_started;
static bool previous_button, short_press_pending;
static uint32_t press_started;
static lv_obj_t *player, *opponent, *ball, *score, *message;

static float clamp(float value, float low, float high)
{
    return value < low ? low : value > high ? high : value;
}

static float angle_difference(float a, float b)
{
    float d = a - b;
    while (d > 180) d -= 360;
    while (d < -180) d += 360;
    return d;
}

static void paddle_position(lv_obj_t *arc, float angle)
{
    /* Rotation avoids crossing-zero angle ambiguity on the right paddle. */
    int rotation = (int)lroundf(angle - PADDLE_HALF);
    rotation = (rotation % 360 + 360) % 360;
    lv_arc_set_rotation(arc, rotation);
}

static void render(void)
{
    paddle_position(player, player_angle);
    paddle_position(opponent, ai_angle);
    lv_obj_set_pos(ball, (int)lroundf(120 + x) - 4, (int)lroundf(120 + y) - 4);
}

static void serve(bool toward_player)
{
    x = y = 0;
    speed = 112;
    const float tilt = (++serve_number & 1) ? 0.28f : -0.28f;
    vx = (toward_player ? -1 : 1) * speed * cosf(tilt);
    vy = speed * sinf(tilt);
    phase = SERVING;
    serve_started = lv_tick_get();
    lv_label_set_text(message, "READY");
}

static void reset_match(void)
{
    player_score = ai_score = serve_number = 0;
    player_angle = 180;
    ai_angle = 0;
    accumulator = 0;
    lv_label_set_text(score, "YOU 0 : 0 AI");
    serve(true);
}

static void point(bool player_won)
{
    if (player_won) ++player_score;
    else ++ai_score;
    lv_label_set_text_fmt(score, "YOU %u : %u AI", player_score, ai_score);
    if (player_score == WIN_SCORE || ai_score == WIN_SCORE) {
        phase = FINISHED;
        x = y = 0;
        lv_label_set_text(message, player_score == WIN_SCORE ? "YOU WIN\nPRESS TO RESET" : "AI WINS\nPRESS TO RESET");
    } else {
        serve(!player_won);
    }
}

static void step(float axis)
{
    const float dt = STEP_MS / 1000.0f;
    player_angle = clamp(player_angle - axis * 135 * dt, 132, 228);
    /* Speed-limited AI with a deadzone; it returns to center on outgoing balls. */
    float target = 0;
    if (phase == RALLY && vx > 0) target = clamp(atan2f(y, fmaxf(x, 35)) / DEG, -48, 48);
    const float error = target - ai_angle;
    if (fabsf(error) > 3) ai_angle += clamp(error, -72 * dt, 72 * dt);
    if (phase != RALLY) return;

    x += vx * dt;
    y += vy * dt;
    const float distance = sqrtf(x * x + y * y);
    if (distance < CONTACT_RADIUS) return;
    const float nx = x / distance, ny = y / distance;
    if (vx * nx + vy * ny <= 0) return;
    const float angle = atan2f(y, x) / DEG;
    const bool left = x < 0;
    const float paddle_angle = left ? player_angle : ai_angle;
    /* Top and bottom 48-degree sectors are walls; side sectors are goals. */
    const bool goal_sector = fabsf(nx) >= cosf(66 * DEG);
    if (goal_sector) {
        if (fabsf(angle_difference(angle, paddle_angle)) > PADDLE_HALF + 2) {
            point(!left);
            return;
        }
        speed = fminf(speed + 8, 220);
        const float paddle_y = CONTACT_RADIUS * sinf(paddle_angle * DEG);
        const float bend = clamp(atan2f(-ny, fabsf(nx)) +
                                 clamp((y - paddle_y) / 32, -0.4f, 0.4f),
                                 -1.05f, 1.05f);
        vx = (left ? 1 : -1) * speed * cosf(bend);
        vy = speed * sinf(bend);
    } else {
        const float dot = vx * nx + vy * ny;
        vx -= 2 * dot * nx;
        vy -= 2 * dot * ny;
    }
    /* Keep the ball inside after contact to avoid repeated collision events. */
    x = nx * (CONTACT_RADIUS - 1);
    y = ny * (CONTACT_RADIUS - 1);
}

static lv_obj_t *arc(lv_obj_t *screen, uint32_t color, bool paddle)
{
    lv_obj_t *obj = lv_arc_create(screen);
    lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj, 224, 224);
    lv_obj_center(obj);
    lv_obj_set_style_arc_color(obj, lv_color_hex(color), LV_PART_MAIN);
    lv_obj_set_style_arc_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_arc_width(obj, paddle ? 6 : 1, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(obj, true, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(obj, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_arc_set_bg_angles(obj, 0, paddle ? 36 : 360);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    return obj;
}

static lv_obj_t *label(lv_obj_t *screen, const char *text, int y_pos, uint32_t color)
{
    lv_obj_t *obj = lv_label_create(screen);
    lv_label_set_text(obj, text);
    lv_obj_set_style_text_color(obj, lv_color_hex(color), 0);
    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, y_pos);
    return obj;
}

void micropong_start(void)
{
    lv_obj_t *screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x07111F), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    arc(screen, 0x21404A, false);
    lv_obj_t *top_wall = arc(screen, 0x426373, true);
    lv_arc_set_bg_angles(top_wall, 0, 48);
    lv_arc_set_rotation(top_wall, 246);
    lv_obj_t *bottom_wall = arc(screen, 0x426373, true);
    lv_arc_set_bg_angles(bottom_wall, 0, 48);
    lv_arc_set_rotation(bottom_wall, 66);
    player = arc(screen, 0x21D4C2, true);
    opponent = arc(screen, 0x6FADCF, true);
    score = label(screen, "", 35, 0xDFF8F5);
    message = label(screen, "", 167, 0x21D4C2);
    label(screen, "HOLD: MENU", 206, 0x75929E);
    ball = lv_obj_create(screen);
    lv_obj_remove_style_all(ball);
    lv_obj_set_size(ball, 8, 8);
    lv_obj_set_style_radius(ball, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(ball, lv_color_hex(0xE1F7F8), 0);
    lv_obj_set_style_bg_opa(ball, LV_OPA_COVER, 0);
    lv_obj_clear_flag(ball, LV_OBJ_FLAG_SCROLLABLE);
    previous_button = true;
    short_press_pending = false;
    last_tick = lv_tick_get();
    reset_match();
    render();
}

void micropong_tick(const micro_input_state_t *input)
{
    uint32_t now = lv_tick_get();
    /* Short release resets; the host consumes a one-second hold as menu exit. */
    if (input->button_pressed && !previous_button) {
        press_started = now;
        short_press_pending = true;
    }
    if (!input->button_pressed && previous_button && short_press_pending) {
        if (now - press_started < 1000) {
            reset_match();
            now = lv_tick_get();
        }
        short_press_pending = false;
    }
    previous_button = input->button_pressed;
    float axis = 0;
    if (input->axes_valid) {
        const float raw = (input->y_raw - 2048) / 2048.0f;
        if (fabsf(raw) > 0.18f) axis = copysignf((fabsf(raw) - 0.18f) / 0.82f, raw);
    }
    uint32_t elapsed = now - last_tick;
    last_tick = now;
    /* Bound catch-up work after a stall; 5 ms steps prevent ball tunneling. */
    accumulator += elapsed > 40 ? 40 : elapsed;
    if (phase == SERVING && now - serve_started >= 900) {
        phase = RALLY;
        lv_label_set_text(message, "");
    }
    while (accumulator >= STEP_MS) {
        if (phase != FINISHED) step(axis);
        accumulator -= STEP_MS;
    }
    render();
}

void micropong_stop(void)
{
    /* The host owns the shared timer and deletes all screen children. */
    player = opponent = ball = score = message = NULL;
}

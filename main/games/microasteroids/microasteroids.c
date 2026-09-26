/* ForgeUI MicroAsteroids / RTechAI. Fixed pools, circular space, LVGL only. */
#include "microasteroids.h"
#include <math.h>
#include <string.h>
#include "lvgl.h"

#define PI 3.14159265f
#define ROCKS 12
#define SHOTS 8
#define SPARKS 16
#define STEP_MS 10
#define DT 0.01f

typedef struct { float x, y, vx, vy, radius, angle, spin; bool alive; } rock_t;
typedef struct { float x, y, vx, vy, life; } mote_t;
static rock_t rocks[ROCKS];
static mote_t shots[SHOTS], sparks[SPARKS];
static float x, y, vx, vy, heading, thrust, shield;
static unsigned score, best, wave;
static uint32_t random_state = 0x41535452, last_tick, last_frame, accumulator;
static bool dead, previous_button, press_pending;
static uint32_t press_started;
static lv_obj_t *field, *hud, *message, *hint;

static float random_unit(void)
{
    random_state = random_state * 1664525U + 1013904223U;
    return (random_state >> 8) / 16777216.0f;
}

static float axis(int raw)
{
    float value = (raw - 2048) / 2048.0f;
    return fabsf(value) < 0.18f ? 0 : copysignf((fabsf(value) - 0.18f) / 0.82f, value);
}

/* Antipodal wrap at the round bezel, preserving direction and overshoot.
 * Radius includes a body's extent so it disappears before re-entering. */
static void wrap(float *px, float *py, float radius)
{
    float distance = sqrtf(*px * *px + *py * *py);
    if (distance > radius) {
        float scale = (distance - 2 * radius) / distance;
        *px *= scale;
        *py *= scale;
    }
}

static void update_hud(void)
{
    lv_label_set_text_fmt(hud, "%05u  /  W%u", score, wave);
}

static void burst(float px, float py)
{
    for (int i = 0; i < SPARKS; ++i) {
        float angle = random_unit() * 2 * PI;
        float speed = 20 + random_unit() * 55;
        sparks[i] = (mote_t){px, py, cosf(angle) * speed, sinf(angle) * speed, 0.45f};
    }
}

static void spawn_wave(void)
{
    ++wave;
    unsigned count = wave + 2;
    if (count > 6) count = 6;
    memset(rocks, 0, sizeof(rocks));
    for (unsigned i = 0; i < count; ++i) {
        float a = 2 * PI * i / count + random_unit() * 0.4f;
        float travel = a + PI + (random_unit() - 0.5f);
        float speed = 20 + fminf(wave * 3, 25);
        rocks[i] = (rock_t){cosf(a) * 104, sinf(a) * 104,
            cosf(travel) * speed, sinf(travel) * speed, 15, a,
            (random_unit() - 0.5f) * 1.8f, true};
    }
    shield = 1.5f;
    update_hud();
}

static void reset(void)
{
    memset(shots, 0, sizeof(shots));
    memset(sparks, 0, sizeof(sparks));
    x = y = vx = vy = thrust = 0;
    heading = -PI / 2;
    score = wave = accumulator = 0;
    dead = false;
    spawn_wave();
    lv_label_set_text(message, "");
    lv_label_set_text(hint, "TAP: FIRE");
}

static void fire(void)
{
    for (int i = 0; i < SHOTS; ++i) {
        if (shots[i].life > 0) continue;
        shots[i] = (mote_t){x + cosf(heading) * 11, y + sinf(heading) * 11,
            vx + cosf(heading) * 180, vy + sinf(heading) * 180, 0.95f};
        break;
    }
}

static bool contact(float ax, float ay, float bx, float by, float radius)
{
    float dx = ax - bx, dy = ay - by;
    return dx * dx + dy * dy < radius * radius;
}

static void split(int index)
{
    rock_t old = rocks[index];
    rocks[index].alive = false;
    score += old.radius > 10 ? 20 : 50;
    if (score > best) best = score;
    update_hud();
    burst(old.x, old.y);
    if (old.radius < 10) return;
    /* Six large rocks can produce at most twelve fragments. */
    for (int child = 0; child < 2; ++child) {
        for (int j = 0; j < ROCKS; ++j) {
            if (rocks[j].alive) continue;
            float a = atan2f(old.vy, old.vx) + (child ? 0.65f : -0.65f);
            float speed = sqrtf(old.vx * old.vx + old.vy * old.vy) * 1.5f;
            rocks[j] = (rock_t){old.x, old.y, cosf(a) * speed, sinf(a) * speed,
                7, a, child ? 1.8f : -1.8f, true};
            break;
        }
    }
}

static void step(float rotation, float power)
{
    for (int i = 0; i < SPARKS; ++i) {
        if (sparks[i].life <= 0) continue;
        sparks[i].life -= DT;
        sparks[i].x += sparks[i].vx * DT;
        sparks[i].y += sparks[i].vy * DT;
    }
    if (dead) return;
    heading += rotation * 3.6f * DT;
    if (heading > PI) heading -= 2 * PI;
    if (heading < -PI) heading += 2 * PI;
    thrust = power;
    vx = (vx + cosf(heading) * power * 115 * DT) * 0.994f;
    vy = (vy + sinf(heading) * power * 115 * DT) * 0.994f;
    float speed = sqrtf(vx * vx + vy * vy);
    if (speed > 90) { vx *= 90 / speed; vy *= 90 / speed; }
    x += vx * DT;
    y += vy * DT;
    wrap(&x, &y, 128);
    shield = fmaxf(0, shield - DT);
    for (int i = 0; i < ROCKS; ++i) {
        rock_t *r = &rocks[i];
        if (!r->alive) continue;
        r->x += r->vx * DT;
        r->y += r->vy * DT;
        r->angle += r->spin * DT;
        wrap(&r->x, &r->y, 120 + r->radius);
    }
    for (int i = 0; i < SHOTS; ++i) {
        mote_t *s = &shots[i];
        if (s->life <= 0) continue;
        s->life -= DT;
        s->x += s->vx * DT;
        s->y += s->vy * DT;
        wrap(&s->x, &s->y, 122);
        for (int j = 0; j < ROCKS; ++j) {
            if (rocks[j].alive && contact(s->x, s->y, rocks[j].x, rocks[j].y, rocks[j].radius + 2)) {
                s->life = 0;
                split(j);
                break;
            }
        }
    }
    bool remaining = false;
    for (int i = 0; i < ROCKS; ++i) {
        if (!rocks[i].alive) continue;
        remaining = true;
        if (shield <= 0 && contact(x, y, rocks[i].x, rocks[i].y, rocks[i].radius + 5)) {
            dead = true;
            burst(x, y);
            /* A press that began before death cannot accidentally restart. */
            press_pending = false;
            lv_label_set_text_fmt(message, "GAME OVER\n%05u POINTS\nBEST %05u", score, best);
            lv_label_set_text(hint, "TAP: RETRY");
            break;
        }
    }
    if (!remaining && !dead) spawn_wave();
}

static void line(lv_draw_ctx_t *ctx, float ax, float ay, float bx, float by, uint32_t color, int width)
{
    lv_draw_line_dsc_t d;
    lv_draw_line_dsc_init(&d);
    d.color = lv_color_hex(color);
    d.width = width;
    lv_point_t a = {(lv_coord_t)lroundf(120 + ax), (lv_coord_t)lroundf(120 + ay)};
    lv_point_t b = {(lv_coord_t)lroundf(120 + bx), (lv_coord_t)lroundf(120 + by)};
    lv_draw_line(ctx, &d, &a, &b);
}

static void draw(lv_event_t *event)
{
    lv_draw_ctx_t *ctx = lv_event_get_draw_ctx(event);
    /* Deterministic stars; rendering never mutates simulation or RNG. */
    for (int i = 0; i < 24; ++i) {
        float a = i * 2.39996f, r = 25 + (i * 37 % 90);
        float sx = cosf(a) * r, sy = sinf(a) * r;
        line(ctx, sx, sy, sx + 1, sy, 0x294451, 1);
    }
    for (int i = 0; i < ROCKS; ++i) {
        rock_t *r = &rocks[i];
        if (!r->alive) continue;
        for (int j = 0; j < 8; ++j) {
            float a = r->angle + j * PI / 4, b = a + PI / 4;
            float ra = r->radius * (j % 2 ? 0.78f : 1);
            float rb = r->radius * (j % 2 ? 1 : 0.78f);
            line(ctx, r->x + cosf(a) * ra, r->y + sinf(a) * ra,
                 r->x + cosf(b) * rb, r->y + sinf(b) * rb, 0x80ABBD, 2);
        }
    }
    for (int i = 0; i < SHOTS; ++i) if (shots[i].life > 0)
        line(ctx, shots[i].x, shots[i].y, shots[i].x - shots[i].vx * 0.018f,
             shots[i].y - shots[i].vy * 0.018f, 0xDEFFAE, 2);
    for (int i = 0; i < SPARKS; ++i) if (sparks[i].life > 0)
        line(ctx, sparks[i].x, sparks[i].y, sparks[i].x - sparks[i].vx * 0.03f,
             sparks[i].y - sparks[i].vy * 0.03f, 0xFFBE70, 1);
    if (dead) return;
    float c = cosf(heading), s = sinf(heading);
    const float px[] = {9, -6, -3, -6, 9}, py[] = {0, 6, 0, -6, 0};
    uint32_t color = shield > 0 && ((int)(shield * 8) % 2) ? 0x71929F : 0x32E6D0;
    for (int i = 0; i < 4; ++i)
        line(ctx, x + px[i] * c - py[i] * s, y + px[i] * s + py[i] * c,
             x + px[i+1] * c - py[i+1] * s, y + px[i+1] * s + py[i+1] * c, color, 2);
    if (thrust > 0.05f)
        line(ctx, x - 6 * c, y - 6 * s, x - (9 + thrust * 8) * c,
             y - (9 + thrust * 8) * s, 0xFFBE70, 2);
}

static lv_obj_t *label(const char *text, int y_pos, uint32_t color)
{
    lv_obj_t *obj = lv_label_create(lv_scr_act());
    lv_label_set_text(obj, text);
    lv_obj_set_style_text_color(obj, lv_color_hex(color), 0);
    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, y_pos);
    return obj;
}

void microasteroids_start(void)
{
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x07111F), 0);
    field = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(field);
    lv_obj_set_size(field, 240, 240);
    lv_obj_set_pos(field, 0, 0);
    lv_obj_clear_flag(field, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(field, draw, LV_EVENT_DRAW_MAIN, NULL);
    hud = label("", 20, 0xDFF8F5);
    message = label("", 87, 0xDFF8F5);
    lv_obj_set_style_bg_color(message, lv_color_hex(0x07111F), 0);
    lv_obj_set_style_bg_opa(message, LV_OPA_90, 0);
    hint = label("", 198, 0x21D4C2);
    label("HOLD: MENU", 215, 0x75929E);
    previous_button = true;
    press_pending = false;
    last_tick = last_frame = lv_tick_get();
    random_state ^= last_tick;
    reset();
}

void microasteroids_tick(const micro_input_state_t *input)
{
    uint32_t now = lv_tick_get();
    if (input->button_pressed && !previous_button) {
        press_started = now;
        press_pending = true;
        if (!dead) fire();
    }
    if (!input->button_pressed && previous_button && press_pending) {
        if (dead && now - press_started < 1000) reset();
        press_pending = false;
    }
    previous_button = input->button_pressed;
    uint32_t elapsed = now - last_tick;
    last_tick = now;
    accumulator += elapsed > 50 ? 50 : elapsed;
    float rotation = input->axes_valid ? axis(input->x_raw) : 0;
    float power = input->axes_valid ? -axis(input->y_raw) : 0;
    while (accumulator >= STEP_MS) {
        step(rotation, power);
        accumulator -= STEP_MS;
    }
    if (now - last_frame >= 33) {
        last_frame = now;
        lv_obj_invalidate(field);
    }
}

void microasteroids_stop(void)
{
    /* No private tasks, timers or allocations. Host removes screen children. */
    field = hud = message = hint = NULL;
}

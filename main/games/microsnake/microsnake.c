#include "microsnake.h"

#include <stdbool.h>
#include <stdint.h>

#include "input/micro_input.h"
#include "lvgl.h"

#define SNAKE_GRID_COLUMNS 18
#define SNAKE_GRID_ROWS 18
#define SNAKE_CELL_SIZE 12
#define SNAKE_GRID_X 12
#define SNAKE_GRID_Y 12
#define SNAKE_MAX_SEGMENTS 72
#define SNAKE_MOVE_INTERVAL_MS 220
#define SNAKE_AXIS_LOW 1200
#define SNAKE_AXIS_HIGH 2900

typedef enum {
    DIRECTION_UP,
    DIRECTION_RIGHT,
    DIRECTION_DOWN,
    DIRECTION_LEFT,
} snake_direction_t;

typedef struct {
    uint8_t x;
    uint8_t y;
} snake_cell_t;

static snake_cell_t snake[SNAKE_MAX_SEGMENTS];
static uint8_t snake_length;
static snake_cell_t food;
static snake_direction_t direction;
static snake_direction_t next_direction;
static uint16_t score;
static uint32_t random_state = 0x4D534E4BU;
static uint32_t last_move;
static bool game_over;
static bool previous_button;
static lv_obj_t *board;
static lv_obj_t *score_label;
static lv_obj_t *status_label;

static uint32_t next_random(void)
{
    random_state = random_state * 1664525U + 1013904223U;
    return random_state;
}

static bool cells_match(snake_cell_t a, snake_cell_t b)
{
    return a.x == b.x && a.y == b.y;
}

static bool direction_is_opposite(snake_direction_t a, snake_direction_t b)
{
    return (a == DIRECTION_UP && b == DIRECTION_DOWN) ||
           (a == DIRECTION_DOWN && b == DIRECTION_UP) ||
           (a == DIRECTION_LEFT && b == DIRECTION_RIGHT) ||
           (a == DIRECTION_RIGHT && b == DIRECTION_LEFT);
}

static void set_direction(snake_direction_t requested)
{
    if (!direction_is_opposite(direction, requested)) next_direction = requested;
}

static void update_direction(const micro_input_state_t *input)
{
    if (!input->axes_valid) return;
    const int x_distance = input->x_raw - 2048;
    const int y_distance = input->y_raw - 2048;
    if (x_distance > 0 && input->x_raw >= SNAKE_AXIS_HIGH && x_distance >= y_distance && x_distance >= -y_distance) {
        set_direction(DIRECTION_RIGHT);
    } else if (x_distance < 0 && input->x_raw <= SNAKE_AXIS_LOW && -x_distance >= y_distance && -x_distance >= -y_distance) {
        set_direction(DIRECTION_LEFT);
    } else if (y_distance > 0 && input->y_raw >= SNAKE_AXIS_HIGH) {
        set_direction(DIRECTION_DOWN);
    } else if (y_distance < 0 && input->y_raw <= SNAKE_AXIS_LOW) {
        set_direction(DIRECTION_UP);
    }
}

/* Circular arena with small top/bottom HUD caps. Cell corners remain inside
 * the bezel; there is no inset rectangular board. */
static bool playable(snake_cell_t cell)
{
    const int x = SNAKE_GRID_X + cell.x * SNAKE_CELL_SIZE + SNAKE_CELL_SIZE / 2;
    const int y = SNAKE_GRID_Y + cell.y * SNAKE_CELL_SIZE + SNAKE_CELL_SIZE / 2;
    const int dx = x - 120, dy = y - 120;
    return dx * dx + dy * dy <= 104 * 104 && y >= 36 && y <= 204;
}

static void place_food(void)
{
    const unsigned cells = SNAKE_GRID_COLUMNS * SNAKE_GRID_ROWS;
    const unsigned first = next_random() % cells;
    for (unsigned offset = 0; offset < cells; ++offset) {
        const unsigned index = (first + offset) % cells;
        food.x = index % SNAKE_GRID_COLUMNS;
        food.y = index / SNAKE_GRID_COLUMNS;
        if (!playable(food)) continue;
        bool occupied = false;
        for (uint8_t i = 0; i < snake_length; ++i) occupied |= cells_match(food, snake[i]);
        if (!occupied) return;
    }
}

static void draw_cell(snake_cell_t cell, lv_color_t color, uint8_t radius)
{
    lv_obj_t *block = lv_obj_create(board);
    lv_obj_remove_flag(block, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(block, SNAKE_CELL_SIZE - 2, SNAKE_CELL_SIZE - 2);
    lv_obj_set_pos(block, SNAKE_GRID_X + cell.x * SNAKE_CELL_SIZE + 1, SNAKE_GRID_Y + cell.y * SNAKE_CELL_SIZE + 1);
    lv_obj_set_style_radius(block, radius, 0);
    lv_obj_set_style_border_width(block, 0, 0);
    lv_obj_set_style_bg_color(block, color, 0);
    lv_obj_set_style_bg_opa(block, LV_OPA_COVER, 0);
}

static void render_game(void)
{
    lv_obj_clean(board);
    draw_cell(food, lv_color_hex(0xFF5573), 7);
    for (uint8_t i = 0; i < snake_length; ++i) {
        draw_cell(snake[i], i == 0 ? lv_color_hex(0xD7FF5F) : lv_color_hex(0x44D88B), i == 0 ? 4 : 3);
    }
    lv_label_set_text_fmt(score_label, "SCORE %u", score);
}

static void new_game(void)
{
    snake_length = 4;
    snake[0] = (snake_cell_t){9, 9};
    snake[1] = (snake_cell_t){8, 9};
    snake[2] = (snake_cell_t){7, 9};
    snake[3] = (snake_cell_t){6, 9};
    direction = DIRECTION_RIGHT;
    next_direction = DIRECTION_RIGHT;
    score = 0;
    game_over = false;
    place_food();
    lv_label_set_text(status_label, "HOLD: MENU");
    lv_obj_set_style_text_color(status_label, lv_color_hex(0x75A7C7), 0);
    last_move = lv_tick_get();
    render_game();
}

static void step_game(void)
{
    direction = next_direction;
    snake_cell_t head = snake[0];
    if (direction == DIRECTION_UP) head.y = (head.y + SNAKE_GRID_ROWS - 1) % SNAKE_GRID_ROWS;
    if (direction == DIRECTION_RIGHT) head.x = (head.x + 1) % SNAKE_GRID_COLUMNS;
    if (direction == DIRECTION_DOWN) head.y = (head.y + 1) % SNAKE_GRID_ROWS;
    if (direction == DIRECTION_LEFT) head.x = (head.x + SNAKE_GRID_COLUMNS - 1) % SNAKE_GRID_COLUMNS;

    /* Wrap to the opposite end of this circular row/column. */
    while (!playable(head)) {
        if (direction == DIRECTION_UP) head.y = (head.y + SNAKE_GRID_ROWS - 1) % SNAKE_GRID_ROWS;
        if (direction == DIRECTION_RIGHT) head.x = (head.x + 1) % SNAKE_GRID_COLUMNS;
        if (direction == DIRECTION_DOWN) head.y = (head.y + 1) % SNAKE_GRID_ROWS;
        if (direction == DIRECTION_LEFT) head.x = (head.x + SNAKE_GRID_COLUMNS - 1) % SNAKE_GRID_COLUMNS;
    }

    const bool ate_food = cells_match(head, food);
    const uint8_t collision_segments = snake_length - (ate_food ? 0 : 1);
    for (uint8_t i = 0; i < collision_segments; ++i) {
        if (cells_match(head, snake[i])) {
            game_over = true;
            lv_label_set_text(status_label, "PRESS TO RETRY");
            lv_obj_set_style_text_color(status_label, lv_color_hex(0xFF5573), 0);
            return;
        }
    }

    if (ate_food && snake_length < SNAKE_MAX_SEGMENTS) ++snake_length;
    for (int i = snake_length - 1; i > 0; --i) snake[i] = snake[i - 1];
    snake[0] = head;
    if (ate_food) {
        score += 10;
        place_food();
        lv_label_set_text(status_label, "HOLD: MENU");
    }
    render_game();
}

void microsnake_tick(const micro_input_state_t *input)
{
    if (input->axes_valid) update_direction(input);
    const bool pressed = input->button_pressed;
    if (game_over && pressed && !previous_button) new_game();
    previous_button = pressed;
    if (!game_over && lv_tick_elaps(last_move) >= SNAKE_MOVE_INTERVAL_MS) {
        last_move = lv_tick_get();
        step_game();
    }
}
void microsnake_start(void)
{
    previous_button = true;
    lv_obj_clean(lv_screen_active());
    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x07111F), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    score_label = lv_label_create(screen);
    lv_label_set_text(score_label, "SCORE 0");
    lv_obj_set_style_text_font(score_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(score_label, lv_color_hex(0xF4FAFF), 0);
    lv_obj_align(score_label, LV_ALIGN_TOP_MID, 0, 18);

    lv_obj_t *rim = lv_obj_create(screen);
    lv_obj_set_size(rim, 234, 234);
    lv_obj_center(rim);
    lv_obj_remove_flag(rim, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(rim, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(rim, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(rim, 1, 0);
    lv_obj_set_style_border_color(rim, lv_color_hex(0x1E4666), 0);

    board = lv_obj_create(screen);
    lv_obj_remove_style_all(board);
    lv_obj_set_size(board, 240, 240);
    lv_obj_set_pos(board, 0, 0);
    lv_obj_remove_flag(board, LV_OBJ_FLAG_SCROLLABLE);
    status_label = lv_label_create(screen);
    lv_obj_set_width(status_label, 140);
    lv_obj_set_style_text_align(status_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_14, 0);
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -18);
    new_game();

}

void microsnake_stop(void)
{
    /* No private timers or allocations: the host deletes the LVGL children. */
    board = NULL;
    score_label = NULL;
    status_label = NULL;
}

// This file contains a simple hello world app which you can base you own apps on.

#include "main.h"
#include "hardware.h"
#include "pax_gfx.h"
#include "pax_codecs.h"
#include "ili9341.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_timer.h"

#include "pax_shaders.h"

static pax_buf_t buf;
xQueueHandle buttonQueue;

static const char *TAG = "robot";

extern const uint8_t nick_png_start[]      asm("_binary_nick_png_start");
extern const uint8_t nick_png_end[]        asm("_binary_nick_png_end");
extern const uint8_t nick_part_png_start[] asm("_binary_nick_a_png_start");
extern const uint8_t nick_part_png_end[]   asm("_binary_nick_a_png_end");
extern const uint8_t nick_gear_png_start[] asm("_binary_nick_gear_png_start");
extern const uint8_t nick_gear_png_end[]   asm("_binary_nick_gear_png_end");

void disp_flush() {
    ili9341_write(get_ili9341(), buf.buf);
    pax_mark_clean(&buf);
}

void disp_sync() {
    ili9341_write_partial(
        get_ili9341(), buf.buf,
        buf.dirty_x0,
        buf.dirty_y0,
        buf.dirty_x1 - buf.dirty_x0,
        buf.dirty_y1 - buf.dirty_y0
    );
    pax_mark_clean(&buf);
}

static pax_col_t bg = 0xff6942a2;
static pax_col_t fg = 0xffc199f4;

static pax_col_t shader_lerp(pax_col_t ignored, int x, int y, float u, float v, void *args) {
    pax_col_t image = pax_shader_texture_aa(-1, x, y, u, v, args);
    return pax_col_lerp(image&255, bg, fg);
}

void app_main() {
    // Init HW.
    bsp_init();
    bsp_rp2040_init();
    buttonQueue = get_rp2040()->queue;
    
    // Init GFX.
    pax_buf_init(&buf, NULL, 320, 240, PAX_BUF_16_565RGB);
    pax_background(&buf, bg);
    draw_partial();
    pax_mark_dirty0(&buf);
    
    uint64_t start = esp_timer_get_time() / 1000;
    uint64_t time;
    while (1) {
        time = esp_timer_get_time() / 1000;
        float angle = M_PI * time / -5000.0;
        draw_gear(angle);
    }
}

void draw_simple() {
    pax_background(&buf, 0);
    const size_t len = (size_t) nick_png_end - (size_t) nick_png_start;
    pax_insert_png_buf(&buf, nick_png_start, len, 0, 0, 0);
    disp_flush();
}

void draw_partial() {
    static pax_buf_t part;
    static bool part_decoded = false;
    
    if (!part_decoded) {
        const size_t len = (size_t) nick_part_png_end - (size_t) nick_part_png_start;
        part_decoded = pax_decode_png_buf(&part, nick_part_png_start, len, PAX_BUF_8_GREY, CODEC_FLAG_OPTIMAL);
    }
    if (!part_decoded) return;
    
    pax_shader_t shader = {
        .callback          = shader_lerp,
        .callback_args     = &part,
        .alpha_promise_0   = false,
        .alpha_promise_255 = true,
    };
    pax_shade_rect(&buf, -1, &shader, NULL, 0, 0, 320, 240);
}

void draw_gear(float angle) {
    static pax_buf_t gear;
    static bool gear_decoded = false;
    
    if (!gear_decoded) {
        const size_t len = (size_t) nick_gear_png_end - (size_t) nick_gear_png_start;
        gear_decoded = pax_decode_png_buf(&gear, nick_gear_png_start, len, PAX_BUF_8_GREY, CODEC_FLAG_OPTIMAL);
    }
    if (!gear_decoded) return;
    
    // Clip to 220, 110 60x60 because it might INTERSECT.
    pax_clip(&buf, 220, 110, 60, 60);
    
    pax_push_2d(&buf);
    
    // Translate to 250, 140 for rotation center.
    pax_apply_2d(&buf, matrix_2d_translate(250, 140));
    // Rotate.
    pax_apply_2d(&buf, matrix_2d_rotate(angle));
    
    // Draw at -30, -30 to center the gear at the rotation point.
    pax_shader_t shader = {
        .callback          = shader_lerp,
        .callback_args     = &gear,
        .alpha_promise_0   = false,
        .alpha_promise_255 = true,
    };
    pax_shade_rect(&buf, -1, &shader, NULL, -30, -30, 60, 60);
    
    pax_pop_2d(&buf);
    
    // Update the screen.
    pax_noclip(&buf);
    disp_sync();
}

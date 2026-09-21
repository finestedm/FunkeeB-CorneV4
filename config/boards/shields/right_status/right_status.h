#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>

#define RIGHT_STATUS_LOGICAL_WIDTH 68
#define RIGHT_STATUS_LOGICAL_HEIGHT 160
#define RIGHT_STATUS_DISPLAY_WIDTH 160
#define RIGHT_STATUS_DISPLAY_HEIGHT 68
#define RIGHT_STATUS_COLOR_FORMAT LV_COLOR_FORMAT_L8

#define RIGHT_STATUS_DRAW_BUF_SIZE                                                               \
    LV_CANVAS_BUF_SIZE(RIGHT_STATUS_LOGICAL_WIDTH, RIGHT_STATUS_LOGICAL_HEIGHT,                  \
                       LV_COLOR_FORMAT_GET_BPP(RIGHT_STATUS_COLOR_FORMAT),                       \
                       LV_DRAW_BUF_STRIDE_ALIGN)
#define RIGHT_STATUS_DISPLAY_BUF_SIZE                                                            \
    LV_CANVAS_BUF_SIZE(RIGHT_STATUS_DISPLAY_WIDTH, RIGHT_STATUS_DISPLAY_HEIGHT,                  \
                       LV_COLOR_FORMAT_GET_BPP(RIGHT_STATUS_COLOR_FORMAT),                       \
                       LV_DRAW_BUF_STRIDE_ALIGN)

#define RIGHT_STATUS_BRAND_WIDTH 88
#define RIGHT_STATUS_BRAND_HEIGHT 24
#define RIGHT_STATUS_BRAND_ROTATED_WIDTH RIGHT_STATUS_BRAND_HEIGHT
#define RIGHT_STATUS_BRAND_ROTATED_HEIGHT RIGHT_STATUS_BRAND_WIDTH
#define RIGHT_STATUS_BRAND_BUF_SIZE                                                              \
    LV_CANVAS_BUF_SIZE(RIGHT_STATUS_BRAND_WIDTH, RIGHT_STATUS_BRAND_HEIGHT,                       \
                       LV_COLOR_FORMAT_GET_BPP(RIGHT_STATUS_COLOR_FORMAT),                       \
                       LV_DRAW_BUF_STRIDE_ALIGN)
#define RIGHT_STATUS_BRAND_ROTATED_BUF_SIZE                                                      \
    LV_CANVAS_BUF_SIZE(RIGHT_STATUS_BRAND_ROTATED_WIDTH,                                        \
                       RIGHT_STATUS_BRAND_ROTATED_HEIGHT,                                       \
                       LV_COLOR_FORMAT_GET_BPP(RIGHT_STATUS_COLOR_FORMAT),                       \
                       LV_DRAW_BUF_STRIDE_ALIGN)

struct zmk_widget_right_status {
    sys_snode_t node;
    lv_obj_t *obj;
    lv_obj_t *drawing_canvas;
    lv_obj_t *display_canvas;
    lv_obj_t *brand_canvas;
    uint8_t drawing_buf[RIGHT_STATUS_DRAW_BUF_SIZE];
    uint8_t display_buf[RIGHT_STATUS_DISPLAY_BUF_SIZE];
    uint8_t brand_buf[RIGHT_STATUS_BRAND_BUF_SIZE];
    uint8_t brand_rotated_buf[RIGHT_STATUS_BRAND_ROTATED_BUF_SIZE];
    uint8_t battery;
    bool charging;
    bool connected;
};

int zmk_widget_right_status_init(struct zmk_widget_right_status *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_right_status_obj(struct zmk_widget_right_status *widget);

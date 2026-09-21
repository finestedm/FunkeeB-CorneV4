#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>

/* Draw upright in the screen's physical 68x160 orientation, then transform the
 * finished frame into the controller's landscape 160x68 framebuffer. */
#define BONGO_LOGICAL_WIDTH 68
#define BONGO_LOGICAL_HEIGHT 160
#define BONGO_DISPLAY_WIDTH 160
#define BONGO_DISPLAY_HEIGHT 68
#define BONGO_COLOR_FORMAT LV_COLOR_FORMAT_L8
#define BONGO_WPM_HISTORY_SIZE 16

#define BONGO_DRAW_BUF_SIZE                                                                       \
    LV_CANVAS_BUF_SIZE(BONGO_LOGICAL_WIDTH, BONGO_LOGICAL_HEIGHT,                                 \
                       LV_COLOR_FORMAT_GET_BPP(BONGO_COLOR_FORMAT), LV_DRAW_BUF_STRIDE_ALIGN)
#define BONGO_DISPLAY_BUF_SIZE                                                                    \
    LV_CANVAS_BUF_SIZE(BONGO_DISPLAY_WIDTH, BONGO_DISPLAY_HEIGHT,                                 \
                       LV_COLOR_FORMAT_GET_BPP(BONGO_COLOR_FORMAT), LV_DRAW_BUF_STRIDE_ALIGN)

enum bongo_connection_status {
    BONGO_CONNECTION_USB,
    BONGO_CONNECTION_BLE_CONNECTED,
    BONGO_CONNECTION_BLE_DISCONNECTED,
    BONGO_CONNECTION_BLE_OPEN,
};

struct zmk_widget_bongo_status {
    sys_snode_t node;
    lv_obj_t *obj;
    lv_obj_t *drawing_canvas;
    lv_obj_t *display_canvas;
    uint8_t drawing_buf[BONGO_DRAW_BUF_SIZE];
    uint8_t display_buf[BONGO_DISPLAY_BUF_SIZE];
    uint8_t battery;
    uint8_t wpm;
    uint8_t wpm_history[BONGO_WPM_HISTORY_SIZE];
    uint8_t wpm_history_head;
    uint8_t wpm_history_count;
    uint8_t layer_index;
    uint8_t profile_index;
    const char *layer_label;
    enum bongo_connection_status connection;
    uint8_t idle_frame;
    bool alternate_paw;
    bool show_tap_frame;
    bool sleeping;
    uint32_t key_press_count;
};

int zmk_widget_bongo_status_init(struct zmk_widget_bongo_status *widget, lv_obj_t *parent);
lv_obj_t *zmk_widget_bongo_status_obj(struct zmk_widget_bongo_status *widget);

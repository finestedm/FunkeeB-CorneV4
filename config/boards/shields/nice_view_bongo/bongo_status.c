#include "bongo_status.h"
#include "bongo_frames.h"

#include <stdio.h>
#include <string.h>

#include <zephyr/logging/log.h>

#include <zmk/battery.h>
#include <zmk/ble.h>
#include <zmk/display.h>
#include <zmk/endpoints.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/events/wpm_state_changed.h>
#include <zmk/keymap.h>
#include <zmk/wpm.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define BONGO_BACKGROUND lv_color_white()
#define BONGO_FOREGROUND lv_color_black()
#define BONGO_IDLE_TIMEOUT K_MINUTES(1)
#define BONGO_IDLE_FRAME_PERIOD K_MSEC(200)
#define BONGO_TAP_HOLD K_MSEC(500)
#define BONGO_FRAME_X 0
#define BONGO_FRAME_Y (BONGO_LOGICAL_HEIGHT - BONGO_FRAME_HEIGHT)
#define BONGO_WPM_GRAPH_X 2
#define BONGO_WPM_GRAPH_Y 52
#define BONGO_WPM_GRAPH_WIDTH 64
#define BONGO_WPM_GRAPH_HEIGHT 56

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct bongo_key_state {
    uint32_t press_count;
};

struct bongo_battery_state {
    uint8_t level;
};

struct bongo_connection_state {
    enum bongo_connection_status status;
    uint8_t profile_index;
};

struct bongo_layer_state {
    uint8_t index;
    const char *label;
};

struct bongo_wpm_state {
    uint8_t wpm;
};

static void init_rect(lv_draw_rect_dsc_t *dsc, lv_color_t fill, int border_width,
                      int radius) {
    lv_draw_rect_dsc_init(dsc);
    dsc->bg_color = fill;
    dsc->bg_opa = LV_OPA_COVER;
    dsc->border_color = BONGO_FOREGROUND;
    dsc->border_width = border_width;
    dsc->radius = radius;
}

static void init_label(lv_draw_label_dsc_t *dsc, lv_text_align_t align) {
    lv_draw_label_dsc_init(dsc);
    dsc->color = BONGO_FOREGROUND;
    dsc->font = &lv_font_unscii_8;
    dsc->align = align;
}

static void draw_rect(lv_obj_t *canvas, int x, int y, int width, int height,
                      lv_draw_rect_dsc_t *dsc) {
    lv_layer_t layer;
    lv_canvas_init_layer(canvas, &layer);
    lv_area_t coords = {x, y, x + width - 1, y + height - 1};
    lv_draw_rect(&layer, dsc, &coords);
    lv_canvas_finish_layer(canvas, &layer);
}

static void draw_text(lv_obj_t *canvas, int x, int y, int width, lv_draw_label_dsc_t *dsc,
                      const char *text) {
    lv_layer_t layer;
    lv_canvas_init_layer(canvas, &layer);
    dsc->text = text;
    lv_area_t coords = {x, y, x + width - 1, BONGO_LOGICAL_HEIGHT - 1};
    lv_draw_label(&layer, dsc, &coords);
    lv_canvas_finish_layer(canvas, &layer);
}

static void draw_battery(lv_obj_t *canvas, uint8_t level) {
    lv_draw_rect_dsc_t black;
    lv_draw_rect_dsc_t white;

    init_rect(&black, BONGO_FOREGROUND, 0, 0);
    init_rect(&white, BONGO_BACKGROUND, 0, 0);

    draw_rect(canvas, 2, 3, 18, 9, &black);
    draw_rect(canvas, 4, 5, 14, 5, &white);
    draw_rect(canvas, 20, 5, 2, 5, &black);

    int fill_width = (level * 12 + 99) / 100;
    if (fill_width > 0) {
        draw_rect(canvas, 5, 6, fill_width, 3, &black);
    }
}

static void draw_indicators(lv_obj_t *canvas, const struct zmk_widget_bongo_status *widget) {
    lv_draw_label_dsc_t left;
    lv_draw_label_dsc_t right;
    char battery_text[6];
    char connection_text[6];
    char layer_text[9];
    char wpm_text[8];

    init_label(&left, LV_TEXT_ALIGN_LEFT);
    init_label(&right, LV_TEXT_ALIGN_RIGHT);

    snprintf(battery_text, sizeof(battery_text), "%u%%", widget->battery);
    switch (widget->connection) {
    case BONGO_CONNECTION_USB:
        strcpy(connection_text, "USB");
        break;
    case BONGO_CONNECTION_BLE_CONNECTED:
        snprintf(connection_text, sizeof(connection_text), "B%u+", widget->profile_index);
        break;
    case BONGO_CONNECTION_BLE_DISCONNECTED:
        snprintf(connection_text, sizeof(connection_text), "B%u-", widget->profile_index);
        break;
    default:
        snprintf(connection_text, sizeof(connection_text), "B%u?", widget->profile_index);
        break;
    }

    if (widget->layer_label != NULL && strlen(widget->layer_label) > 0) {
        snprintf(layer_text, sizeof(layer_text), "%.8s", widget->layer_label);
    } else {
        snprintf(layer_text, sizeof(layer_text), "LAYER %u", widget->layer_index);
    }
    snprintf(wpm_text, sizeof(wpm_text), "%u WPM", widget->wpm);

    draw_battery(canvas, widget->battery);
    draw_text(canvas, 34, 1, 32, &right, battery_text);
    draw_text(canvas, 2, 14, 64, &left, layer_text);
    draw_text(canvas, 2, 25, 64, &left, wpm_text);
    draw_text(canvas, 2, 36, 64, &left, connection_text);
}

static void draw_wpm_graph(lv_obj_t *canvas,
                           const struct zmk_widget_bongo_status *widget) {
    lv_draw_rect_dsc_t black;
    uint8_t scale = 60;

    init_rect(&black, BONGO_FOREGROUND, 0, 0);

    for (uint8_t i = 0; i < widget->wpm_history_count; i++) {
        const uint8_t index =
            (widget->wpm_history_head + BONGO_WPM_HISTORY_SIZE -
             widget->wpm_history_count + i) %
            BONGO_WPM_HISTORY_SIZE;
        scale = MAX(scale, widget->wpm_history[index]);
    }

    /* Compact axes plus 16 three-pixel bars, oldest on the left. */
    draw_rect(canvas, BONGO_WPM_GRAPH_X, BONGO_WPM_GRAPH_Y, 1,
              BONGO_WPM_GRAPH_HEIGHT, &black);
    draw_rect(canvas, BONGO_WPM_GRAPH_X, BONGO_WPM_GRAPH_Y +
                                             BONGO_WPM_GRAPH_HEIGHT - 1,
              BONGO_WPM_GRAPH_WIDTH, 1, &black);

    for (uint8_t i = 0; i < widget->wpm_history_count; i++) {
        const uint8_t index =
            (widget->wpm_history_head + BONGO_WPM_HISTORY_SIZE -
             widget->wpm_history_count + i) %
            BONGO_WPM_HISTORY_SIZE;
        const uint8_t value = widget->wpm_history[index];
        if (value == 0) {
            continue;
        }

        const uint8_t bar_height =
            MIN(BONGO_WPM_GRAPH_HEIGHT - 2,
                (value * (BONGO_WPM_GRAPH_HEIGHT - 2) + scale - 1) / scale);
        const int x = BONGO_WPM_GRAPH_X + 1 +
                      (BONGO_WPM_HISTORY_SIZE - widget->wpm_history_count + i) * 4;
        const int y = BONGO_WPM_GRAPH_Y + BONGO_WPM_GRAPH_HEIGHT - 1 - bar_height;
        draw_rect(canvas, x, y, 3, bar_height, &black);
    }
}

static void draw_source_bitmap(struct zmk_widget_bongo_status *widget,
                               const uint8_t *bitmap, int x_offset, int y_offset) {
    const uint32_t stride =
        lv_draw_buf_width_to_stride(BONGO_LOGICAL_WIDTH, BONGO_COLOR_FORMAT);

    for (uint32_t y = 0; y < BONGO_FRAME_HEIGHT; y++) {
        for (uint32_t x = 0; x < BONGO_FRAME_WIDTH; x++) {
            const uint8_t packed = bitmap[y * BONGO_FRAME_STRIDE_BYTES + x / 8];
            if ((packed & BIT(7 - (x % 8))) != 0) {
                widget->drawing_buf[(y_offset + y) * stride + x_offset + x] = 0x00;
            }
        }
    }
}

static void draw_sleep_overlay(lv_obj_t *canvas) {
    lv_draw_rect_dsc_t black;
    lv_draw_rect_dsc_t white;
    lv_draw_label_dsc_t sleep_label;

    init_rect(&black, BONGO_FOREGROUND, 0, 0);
    init_rect(&white, BONGO_BACKGROUND, 0, 0);

    /* Keep idle frame 0 and only replace its open eyes with short closed-eye lines. */
    draw_rect(canvas, BONGO_FRAME_X + 21, BONGO_FRAME_Y + 13, 2, 3, &black);
    draw_rect(canvas, BONGO_FRAME_X + 34, BONGO_FRAME_Y + 18, 2, 2, &black);
    draw_rect(canvas, BONGO_FRAME_X + 20, BONGO_FRAME_Y + 14, 4, 1, &white);
    draw_rect(canvas, BONGO_FRAME_X + 33, BONGO_FRAME_Y + 19, 4, 1, &white);

    init_label(&sleep_label, LV_TEXT_ALIGN_RIGHT);
    draw_text(canvas, 36, BONGO_FRAME_Y - 11, 29, &sleep_label, "ZZzz");
}

static void draw_source_bongo_cat(struct zmk_widget_bongo_status *widget) {
    const uint8_t *frame;

    lv_canvas_fill_bg(widget->drawing_canvas, BONGO_BACKGROUND, LV_OPA_COVER);

    if (widget->sleeping) {
        frame = bongo_idle_frames[0];
    } else if (widget->show_tap_frame) {
        frame = bongo_tap_frames[widget->alternate_paw ? 1 : 0];
    } else {
        frame = bongo_idle_frames[widget->idle_frame % BONGO_IDLE_FRAME_COUNT];
    }

    draw_source_bitmap(widget, frame, BONGO_FRAME_X, BONGO_FRAME_Y);
    if (widget->sleeping) {
        draw_sleep_overlay(widget->drawing_canvas);
    }
}

static void rotate_for_mounting(struct zmk_widget_bongo_status *widget) {
    const uint32_t source_stride =
        lv_draw_buf_width_to_stride(BONGO_LOGICAL_WIDTH, BONGO_COLOR_FORMAT);
    const uint32_t display_stride =
        lv_draw_buf_width_to_stride(BONGO_DISPLAY_WIDTH, BONGO_COLOR_FORMAT);

    lv_draw_sw_rotate(widget->drawing_buf, widget->display_buf, BONGO_LOGICAL_WIDTH,
                      BONGO_LOGICAL_HEIGHT, source_stride, display_stride,
                      LV_DISPLAY_ROTATION_270, BONGO_COLOR_FORMAT);
    lv_obj_invalidate(widget->display_canvas);
}

static void draw_frame(struct zmk_widget_bongo_status *widget) {
    draw_source_bongo_cat(widget);
    draw_indicators(widget->drawing_canvas, widget);
    draw_wpm_graph(widget->drawing_canvas, widget);
    rotate_for_mounting(widget);
}

static void animation_work_cb(struct k_work *work) {
    bool keep_animating = false;
    struct zmk_widget_bongo_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        if (widget->sleeping) {
            continue;
        }

        widget->show_tap_frame = false;
        widget->idle_frame = (widget->idle_frame + 1) % BONGO_IDLE_FRAME_COUNT;
        draw_frame(widget);
        keep_animating = true;
    }

    if (keep_animating) {
        k_work_reschedule_for_queue(zmk_display_work_q(),
                                    CONTAINER_OF(work, struct k_work_delayable, work),
                                    BONGO_IDLE_FRAME_PERIOD);
    }
}

K_WORK_DELAYABLE_DEFINE(bongo_animation_work, animation_work_cb);

static void wpm_graph_work_cb(struct k_work *work) {
    bool keep_sampling = false;
    struct zmk_widget_bongo_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        if (widget->sleeping) {
            continue;
        }

        widget->wpm_history[widget->wpm_history_head] = widget->wpm;
        widget->wpm_history_head =
            (widget->wpm_history_head + 1) % BONGO_WPM_HISTORY_SIZE;
        widget->wpm_history_count =
            MIN(widget->wpm_history_count + 1, BONGO_WPM_HISTORY_SIZE);
        draw_frame(widget);
        keep_sampling = true;
    }

    if (keep_sampling) {
        k_work_reschedule_for_queue(zmk_display_work_q(),
                                    CONTAINER_OF(work, struct k_work_delayable, work),
                                    K_SECONDS(1));
    }
}

K_WORK_DELAYABLE_DEFINE(bongo_wpm_graph_work, wpm_graph_work_cb);

static void idle_work_cb(struct k_work *work) {
    ARG_UNUSED(work);

    struct zmk_widget_bongo_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        widget->sleeping = true;
        widget->show_tap_frame = false;
        draw_frame(widget);
    }
}

K_WORK_DELAYABLE_DEFINE(bongo_idle_work, idle_work_cb);

static void restart_idle_timer(void) {
    k_work_reschedule_for_queue(zmk_display_work_q(), &bongo_idle_work, BONGO_IDLE_TIMEOUT);
}

static void battery_update_cb(struct bongo_battery_state state) {
    struct zmk_widget_bongo_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        widget->battery = state.level;
        draw_frame(widget);
    }
}

static struct bongo_battery_state battery_get_state(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *event = as_zmk_battery_state_changed(eh);
    return (struct bongo_battery_state){
        .level = event != NULL ? event->state_of_charge : zmk_battery_state_of_charge(),
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(bongo_battery_listener, struct bongo_battery_state,
                            battery_update_cb, battery_get_state)
ZMK_SUBSCRIPTION(bongo_battery_listener, zmk_battery_state_changed);

static void connection_update_cb(struct bongo_connection_state state) {
    struct zmk_widget_bongo_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        widget->connection = state.status;
        widget->profile_index = state.profile_index;
        draw_frame(widget);
    }
}

static struct bongo_connection_state connection_get_state(const zmk_event_t *eh) {
    const struct zmk_endpoint_instance selected = zmk_endpoint_get_selected();
    struct bongo_connection_state state = {
        .status = BONGO_CONNECTION_USB,
        .profile_index = zmk_ble_active_profile_index() + 1,
    };

    if (selected.transport == ZMK_TRANSPORT_BLE) {
        if (zmk_ble_active_profile_is_connected()) {
            state.status = BONGO_CONNECTION_BLE_CONNECTED;
        } else if (zmk_ble_active_profile_is_open()) {
            state.status = BONGO_CONNECTION_BLE_OPEN;
        } else {
            state.status = BONGO_CONNECTION_BLE_DISCONNECTED;
        }
    }

    return state;
}

ZMK_DISPLAY_WIDGET_LISTENER(bongo_connection_listener, struct bongo_connection_state,
                            connection_update_cb, connection_get_state)
ZMK_SUBSCRIPTION(bongo_connection_listener, zmk_endpoint_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(bongo_connection_listener, zmk_usb_conn_state_changed);
#endif
#if IS_ENABLED(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(bongo_connection_listener, zmk_ble_active_profile_changed);
#endif

static void layer_update_cb(struct bongo_layer_state state) {
    struct zmk_widget_bongo_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        widget->layer_index = state.index;
        widget->layer_label = state.label;
        draw_frame(widget);
    }
}

static struct bongo_layer_state layer_get_state(const zmk_event_t *eh) {
    const zmk_keymap_layer_index_t index = zmk_keymap_highest_layer_active();
    return (struct bongo_layer_state){
        .index = index,
        .label = zmk_keymap_layer_name(zmk_keymap_layer_index_to_id(index)),
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(bongo_layer_listener, struct bongo_layer_state, layer_update_cb,
                            layer_get_state)
ZMK_SUBSCRIPTION(bongo_layer_listener, zmk_layer_state_changed);

static void wpm_update_cb(struct bongo_wpm_state state) {
    struct zmk_widget_bongo_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        widget->wpm = state.wpm;
        draw_frame(widget);
    }
}

static struct bongo_wpm_state wpm_get_state(const zmk_event_t *eh) {
    return (struct bongo_wpm_state){.wpm = zmk_wpm_get_state()};
}

ZMK_DISPLAY_WIDGET_LISTENER(bongo_wpm_listener, struct bongo_wpm_state, wpm_update_cb,
                            wpm_get_state)
ZMK_SUBSCRIPTION(bongo_wpm_listener, zmk_wpm_state_changed);

static void key_update_cb(struct bongo_key_state state) {
    bool handled_press = false;
    bool woke_from_sleep = false;
    struct zmk_widget_bongo_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        if (widget->key_press_count == state.press_count) {
            continue;
        }

        const uint32_t press_delta = state.press_count - widget->key_press_count;
        widget->key_press_count = state.press_count;
        handled_press = true;
        woke_from_sleep = woke_from_sleep || widget->sleeping;
        widget->sleeping = false;
        widget->show_tap_frame = true;
        if ((press_delta & 1U) != 0U) {
            widget->alternate_paw = !widget->alternate_paw;
        }
        draw_frame(widget);
    }

    if (!handled_press) {
        return;
    }

    k_work_reschedule_for_queue(zmk_display_work_q(), &bongo_animation_work,
                                BONGO_TAP_HOLD);
    if (woke_from_sleep) {
        k_work_reschedule_for_queue(zmk_display_work_q(), &bongo_wpm_graph_work,
                                    K_SECONDS(1));
    }
    restart_idle_timer();
}

static struct bongo_key_state key_get_state(const zmk_event_t *eh) {
    static uint32_t press_count;
    const struct zmk_keycode_state_changed *event = as_zmk_keycode_state_changed(eh);
    if (event != NULL && event->state) {
        press_count++;
    }

    return (struct bongo_key_state){.press_count = press_count};
}

ZMK_DISPLAY_WIDGET_LISTENER(bongo_key_listener, struct bongo_key_state, key_update_cb,
                            key_get_state)
ZMK_SUBSCRIPTION(bongo_key_listener, zmk_keycode_state_changed);

int zmk_widget_bongo_status_init(struct zmk_widget_bongo_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, BONGO_DISPLAY_WIDTH, BONGO_DISPLAY_HEIGHT);

    widget->drawing_canvas = lv_canvas_create(widget->obj);
    lv_canvas_set_buffer(widget->drawing_canvas, widget->drawing_buf, BONGO_LOGICAL_WIDTH,
                         BONGO_LOGICAL_HEIGHT, BONGO_COLOR_FORMAT);
    lv_obj_add_flag(widget->drawing_canvas, LV_OBJ_FLAG_HIDDEN);

    widget->display_canvas = lv_canvas_create(widget->obj);
    lv_obj_align(widget->display_canvas, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_canvas_set_buffer(widget->display_canvas, widget->display_buf, BONGO_DISPLAY_WIDTH,
                         BONGO_DISPLAY_HEIGHT, BONGO_COLOR_FORMAT);

    sys_slist_append(&widgets, &widget->node);
    draw_frame(widget);
    bongo_battery_listener_init();
    bongo_connection_listener_init();
    bongo_layer_listener_init();
    bongo_wpm_listener_init();
    bongo_key_listener_init();
    k_work_reschedule_for_queue(zmk_display_work_q(), &bongo_animation_work,
                                BONGO_IDLE_FRAME_PERIOD);
    k_work_reschedule_for_queue(zmk_display_work_q(), &bongo_wpm_graph_work,
                                K_SECONDS(1));
    restart_idle_timer();

    return 0;
}

lv_obj_t *zmk_widget_bongo_status_obj(struct zmk_widget_bongo_status *widget) {
    return widget->obj;
}

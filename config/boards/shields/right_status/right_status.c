#include "right_status.h"

#include <stdio.h>

#include <zephyr/logging/log.h>

#include <zmk/battery.h>
#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/split_peripheral_status_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/split/bluetooth/peripheral.h>
#include <zmk/usb.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define RIGHT_STATUS_BACKGROUND lv_color_white()
#define RIGHT_STATUS_FOREGROUND lv_color_black()
#define RIGHT_STATUS_BRAND_X 22
#define RIGHT_STATUS_BRAND_Y 69

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct right_battery_state {
    uint8_t level;
    bool charging;
};

struct right_connection_state {
    bool connected;
};

static void init_rect(lv_draw_rect_dsc_t *dsc, lv_color_t fill) {
    lv_draw_rect_dsc_init(dsc);
    dsc->bg_color = fill;
    dsc->bg_opa = LV_OPA_COVER;
    dsc->border_width = 0;
    dsc->radius = 0;
}

static void init_label(lv_draw_label_dsc_t *dsc, lv_text_align_t align) {
    lv_draw_label_dsc_init(dsc);
    dsc->color = RIGHT_STATUS_FOREGROUND;
    dsc->font = &lv_font_montserrat_18;
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

static void draw_text(lv_obj_t *canvas, int x, int y, int width, int height,
                      lv_draw_label_dsc_t *dsc, const char *text) {
    lv_layer_t layer;
    lv_canvas_init_layer(canvas, &layer);
    dsc->text = text;
    lv_area_t coords = {x, y, x + width - 1, y + height - 1};
    lv_draw_label(&layer, dsc, &coords);
    lv_canvas_finish_layer(canvas, &layer);
}

static void draw_battery(lv_obj_t *canvas, const struct zmk_widget_right_status *widget) {
    lv_draw_rect_dsc_t black;
    lv_draw_rect_dsc_t white;
    lv_draw_label_dsc_t centered;
    char percentage[7];

    init_rect(&black, RIGHT_STATUS_FOREGROUND);
    init_rect(&white, RIGHT_STATUS_BACKGROUND);

    draw_rect(canvas, 4, 3, 52, 20, &black);
    draw_rect(canvas, 6, 5, 48, 16, &white);
    draw_rect(canvas, 56, 9, 5, 8, &black);

    const int fill_width = (widget->battery * 46 + 99) / 100;
    if (fill_width > 0) {
        draw_rect(canvas, 7, 6, fill_width, 14, &black);
    }

    snprintf(percentage, sizeof(percentage), widget->charging ? "%u%%+" : "%u%%",
             widget->battery);
    init_label(&centered, LV_TEXT_ALIGN_CENTER);
    draw_text(canvas, 0, 25, RIGHT_STATUS_LOGICAL_WIDTH, 22, &centered, percentage);
}

static void draw_connection(lv_obj_t *canvas,
                            const struct zmk_widget_right_status *widget) {
    lv_draw_rect_dsc_t black;
    lv_draw_label_dsc_t centered;

    init_label(&centered, LV_TEXT_ALIGN_CENTER);
    draw_text(canvas, 0, 45, RIGHT_STATUS_LOGICAL_WIDTH, 22, &centered,
              widget->connected ? "BT+" : "BT-");

    init_rect(&black, RIGHT_STATUS_FOREGROUND);
    draw_rect(canvas, 4, 66, 60, 1, &black);
}

static void draw_brand(struct zmk_widget_right_status *widget) {
    lv_draw_label_dsc_t centered;

    lv_canvas_fill_bg(widget->brand_canvas, RIGHT_STATUS_BACKGROUND, LV_OPA_COVER);
    init_label(&centered, LV_TEXT_ALIGN_CENTER);
    draw_text(widget->brand_canvas, 0, 1, RIGHT_STATUS_BRAND_WIDTH,
              RIGHT_STATUS_BRAND_HEIGHT, &centered, "FunkeeB");

    const uint32_t source_stride =
        lv_draw_buf_width_to_stride(RIGHT_STATUS_BRAND_WIDTH, RIGHT_STATUS_COLOR_FORMAT);
    const uint32_t rotated_stride = lv_draw_buf_width_to_stride(
        RIGHT_STATUS_BRAND_ROTATED_WIDTH, RIGHT_STATUS_COLOR_FORMAT);
    const uint32_t drawing_stride =
        lv_draw_buf_width_to_stride(RIGHT_STATUS_LOGICAL_WIDTH, RIGHT_STATUS_COLOR_FORMAT);

    lv_draw_sw_rotate(widget->brand_buf, widget->brand_rotated_buf,
                      RIGHT_STATUS_BRAND_WIDTH, RIGHT_STATUS_BRAND_HEIGHT, source_stride,
                      rotated_stride, LV_DISPLAY_ROTATION_90, RIGHT_STATUS_COLOR_FORMAT);

    for (uint32_t y = 0; y < RIGHT_STATUS_BRAND_ROTATED_HEIGHT; y++) {
        for (uint32_t x = 0; x < RIGHT_STATUS_BRAND_ROTATED_WIDTH; x++) {
            widget->drawing_buf[(RIGHT_STATUS_BRAND_Y + y) * drawing_stride +
                                RIGHT_STATUS_BRAND_X + x] =
                widget->brand_rotated_buf[y * rotated_stride + x];
        }
    }
}

static void rotate_for_mounting(struct zmk_widget_right_status *widget) {
    const uint32_t source_stride =
        lv_draw_buf_width_to_stride(RIGHT_STATUS_LOGICAL_WIDTH, RIGHT_STATUS_COLOR_FORMAT);
    const uint32_t display_stride =
        lv_draw_buf_width_to_stride(RIGHT_STATUS_DISPLAY_WIDTH, RIGHT_STATUS_COLOR_FORMAT);

    lv_draw_sw_rotate(widget->drawing_buf, widget->display_buf, RIGHT_STATUS_LOGICAL_WIDTH,
                      RIGHT_STATUS_LOGICAL_HEIGHT, source_stride, display_stride,
                      LV_DISPLAY_ROTATION_270, RIGHT_STATUS_COLOR_FORMAT);
    lv_obj_invalidate(widget->display_canvas);
}

static void draw_frame(struct zmk_widget_right_status *widget) {
    lv_canvas_fill_bg(widget->drawing_canvas, RIGHT_STATUS_BACKGROUND, LV_OPA_COVER);
    draw_battery(widget->drawing_canvas, widget);
    draw_connection(widget->drawing_canvas, widget);
    draw_brand(widget);
    rotate_for_mounting(widget);
}

static void battery_update_cb(struct right_battery_state state) {
    struct zmk_widget_right_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        widget->battery = state.level;
        widget->charging = state.charging;
        draw_frame(widget);
    }
}

static struct right_battery_state battery_get_state(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *event = as_zmk_battery_state_changed(eh);
    return (struct right_battery_state){
        .level = event != NULL ? event->state_of_charge : zmk_battery_state_of_charge(),
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
        .charging = zmk_usb_is_powered(),
#else
        .charging = false,
#endif
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(right_battery_listener, struct right_battery_state,
                            battery_update_cb, battery_get_state)
ZMK_SUBSCRIPTION(right_battery_listener, zmk_battery_state_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(right_battery_listener, zmk_usb_conn_state_changed);
#endif

static void connection_update_cb(struct right_connection_state state) {
    struct zmk_widget_right_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        widget->connected = state.connected;
        draw_frame(widget);
    }
}

static struct right_connection_state connection_get_state(const zmk_event_t *eh) {
    ARG_UNUSED(eh);
    return (struct right_connection_state){
        .connected = zmk_split_bt_peripheral_is_connected(),
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(right_connection_listener, struct right_connection_state,
                            connection_update_cb, connection_get_state)
ZMK_SUBSCRIPTION(right_connection_listener, zmk_split_peripheral_status_changed);

int zmk_widget_right_status_init(struct zmk_widget_right_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, RIGHT_STATUS_DISPLAY_WIDTH, RIGHT_STATUS_DISPLAY_HEIGHT);

    widget->drawing_canvas = lv_canvas_create(widget->obj);
    lv_canvas_set_buffer(widget->drawing_canvas, widget->drawing_buf,
                         RIGHT_STATUS_LOGICAL_WIDTH, RIGHT_STATUS_LOGICAL_HEIGHT,
                         RIGHT_STATUS_COLOR_FORMAT);
    lv_obj_add_flag(widget->drawing_canvas, LV_OBJ_FLAG_HIDDEN);

    widget->brand_canvas = lv_canvas_create(widget->obj);
    lv_canvas_set_buffer(widget->brand_canvas, widget->brand_buf, RIGHT_STATUS_BRAND_WIDTH,
                         RIGHT_STATUS_BRAND_HEIGHT, RIGHT_STATUS_COLOR_FORMAT);
    lv_obj_add_flag(widget->brand_canvas, LV_OBJ_FLAG_HIDDEN);

    widget->display_canvas = lv_canvas_create(widget->obj);
    lv_obj_align(widget->display_canvas, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_canvas_set_buffer(widget->display_canvas, widget->display_buf,
                         RIGHT_STATUS_DISPLAY_WIDTH, RIGHT_STATUS_DISPLAY_HEIGHT,
                         RIGHT_STATUS_COLOR_FORMAT);

    sys_slist_append(&widgets, &widget->node);
    draw_frame(widget);
    right_battery_listener_init();
    right_connection_listener_init();

    return 0;
}

lv_obj_t *zmk_widget_right_status_obj(struct zmk_widget_right_status *widget) {
    return widget->obj;
}

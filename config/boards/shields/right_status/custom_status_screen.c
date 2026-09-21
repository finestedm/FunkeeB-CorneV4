#include "right_status.h"

static struct zmk_widget_right_status right_status_widget;

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    zmk_widget_right_status_init(&right_status_widget, screen);
    lv_obj_align(zmk_widget_right_status_obj(&right_status_widget), LV_ALIGN_TOP_LEFT, 0, 0);

    return screen;
}

#include <string.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"

#include <string.h>

objects_t objects;
lv_obj_t *tick_value_change_obj;
uint32_t active_theme_index = 0;

void create_screen_main() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.main = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 320, 240);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.obj0 = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 320, 240);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // song_header
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.song_header = obj;
            lv_obj_set_pos(obj, 140, 72);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "Song:");
        }
        {
            // song_title
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.song_title = obj;
            lv_obj_set_pos(obj, 107, 99);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "song_name");
        }
        {
            // play_pause_butt
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.play_pause_butt = obj;
            lv_obj_set_pos(obj, 117, 163);
            lv_obj_set_size(obj, 86, 50);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // pp_butt_label
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.pp_butt_label = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_add_event_cb(obj, action_play_pause_song, LV_EVENT_PRESSED, (void *)0);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Play/Pause");
                }
            }
        }
        {
            // next_butt
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.next_butt = obj;
            lv_obj_set_pos(obj, 239, 172);
            lv_obj_set_size(obj, 33, 33);
            lv_obj_add_event_cb(obj, action_next_song, LV_EVENT_PRESSED, (void *)0);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // next_butt_label
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.next_butt_label = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, ">");
                }
            }
        }
        {
            // prev_butt
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.prev_butt = obj;
            lv_obj_set_pos(obj, 48, 172);
            lv_obj_set_size(obj, 33, 33);
            lv_obj_add_event_cb(obj, action_prev_song, LV_EVENT_PRESSED, (void *)0);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // prev_butt_label
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.prev_butt_label = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "<");
                }
            }
        }
        {
            // volume_label
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.volume_label = obj;
            lv_obj_set_pos(obj, 8, 12);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "Volume:");
        }
        {
            // volume_up_butt
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.volume_up_butt = obj;
            lv_obj_set_pos(obj, 64, 9);
            lv_obj_set_size(obj, 22, 22);
            lv_obj_add_event_cb(obj, action_volume_up, LV_EVENT_PRESSED, (void *)0);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // vup_label
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.vup_label = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "+");
                }
            }
        }
        {
            // volume_down_butt
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.volume_down_butt = obj;
            lv_obj_set_pos(obj, 92, 9);
            lv_obj_set_size(obj, 22, 22);
            lv_obj_add_event_cb(obj, action_volume_down, LV_EVENT_PRESSED, (void *)0);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // vdown_label
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.vdown_label = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "-");
                }
            }
        }
    }
    
    tick_screen_main();
}

void tick_screen_main() {
}



typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_main,
};
void tick_screen(int screen_index) {
    tick_screen_funcs[screen_index]();
}
void tick_screen_by_id(enum ScreensEnum screenId) {
    tick_screen_funcs[screenId - 1]();
}

void create_screens() {
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), false, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    
    create_screen_main();
}

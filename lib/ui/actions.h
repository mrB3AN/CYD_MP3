#ifndef EEZ_LVGL_UI_EVENTS_H
#define EEZ_LVGL_UI_EVENTS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void action_next_song(lv_event_t * e);
extern void action_play_pause_song(lv_event_t * e);
extern void action_prev_song(lv_event_t * e);
extern void action_volume_down(lv_event_t * e);
extern void action_volume_up(lv_event_t * e);


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_EVENTS_H*/
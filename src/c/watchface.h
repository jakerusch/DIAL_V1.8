#include <pebble.h>
#pragma once

///////////////////////
// weather variables //
///////////////////////
#define KEY_TEMP
#define KEY_ICON

////////////////////
// font variables //
////////////////////
#define WORD_FONT RESOURCE_ID_ULTRALIGHT_14
#define NUMBER_FONT RESOURCE_ID_ARCON_FONT_14

#define SETTINGS_KEY 1

///////////////////
// Clay settings //
///////////////////
typedef struct ClaySettings {
	GColor BackgroundColor;
  GColor ForegroundColor;
  bool InvertColors;
} ClaySettings; // Define our settings struct

static void config_default();
static void config_load();
static void setColors();
static void config_save();
static void dial_update_proc(Layer *layer, GContext *ctx);
static void temp_update_proc(Layer *layer, GContext *ctx);
static void battery_update_proc(Layer *layer, GContext *ctx);
static void health_update_proc(Layer *layer, GContext *ctx);
static void ticks_update_proc(Layer *layer, GContext *ctx);
static void main_window_load(Window *window);
static void update_time();
static void tick_handler(struct tm *tick_time, TimeUnits units_changed);
static void battery_handler(BatteryChargeState charge_state);
static void bluetooth_callback(bool connected);
static void health_handler(HealthEventType event, void *context);
static void main_window_unload(Window *window);
static void load_icons();
static void inbox_received_callback(DictionaryIterator *iterator, void *context);
static void inbox_dropped_callback(AppMessageResult reason, void *context);
static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context);
static void outbox_sent_callback(DictionaryIterator *iterator, void *context);
static void init();
static void deinit();
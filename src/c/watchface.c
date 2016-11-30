// Written by Jacob Rusch
// 10/3/2016
// code for analog watch dial
// Supports APLITE, BASALT, CHALK, DIORITE
//
//
// *****
//
// 1.6
// Switched from OpenWeatherMap.org to DarkSky.net.
// Weather data in San Antonio, TX seems to be more accurate
// from DarkSky.
//
//
// *****
//
//
// 1.7
// Switched back to OpenWeatherMap.org because I hit the limit of DarkSky.net in a day.  DOH!
//
//
// *****
//
//
// 1.8
// added support for round (chalk)
//
//
// *****


#include <pebble.h>
#include "watchface.h"


static Window *s_main_window;
static Layer *s_dial_layer, *s_hands_layer, *s_temp_circle, *s_battery_circle, *s_health_circle;
static TextLayer *s_temp_layer, *s_health_layer, *s_day_text_layer, *s_date_text_layer;
static GBitmap *s_weather_bitmap, *s_health_bitmap, *s_bluetooth_bitmap, *s_charging_bitmap, *s_bluetooth_bitmap;
static BitmapLayer *s_weather_bitmap_layer, *s_health_bitmap_layer, *s_bluetooth_bitmap_layer, *s_charging_bitmap_layer, *s_bluetooth_bitmap_layer;
static int buf=PBL_IF_ROUND_ELSE(0, 24), battery_percent, step_goal=10000;
static GFont s_word_font, s_number_font;
static char icon_buf[64];
static double step_count;
static char *char_current_steps;
static bool charging;


static ClaySettings settings; // An instance of the struct


///////////////////////////////
// set default Clay settings //
///////////////////////////////
static void config_default() {
	settings.BackgroundColor = GColorBlack;
  settings.ForegroundColor = GColorWhite;
  settings.InvertColors = false;
}


/////////////////////////////////////
// load default settings from Clay //
/////////////////////////////////////
static void config_load() {
	config_default(); // Load the default settings
	persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));	// Read settings from persistent storage, if they exist
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "config_load");
}


///////////////////////
// sets watch colors //
///////////////////////
static void setColors() {
  
  // set background color
  window_set_background_color(s_main_window, settings.BackgroundColor);
  
  // set text color for TextLayers
  text_layer_set_text_color(s_temp_layer, settings.ForegroundColor);
  text_layer_set_text_color(s_health_layer, settings.ForegroundColor);
  text_layer_set_text_color(s_day_text_layer, settings.ForegroundColor);
  text_layer_set_text_color(s_date_text_layer, settings.ForegroundColor);
  
  // draw hands
  layer_mark_dirty(s_hands_layer); 
  
  // load appropriate icon
  load_icons(); 
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "setColors");
}


/////////////////////////
// saves user settings //
/////////////////////////
static void config_save() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "config_save");
}


/////////////////////////
// draws dial on watch //
/////////////////////////
static void dial_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds); 
  
  // draw dial
  graphics_context_set_fill_color(ctx, settings.BackgroundColor);
  graphics_fill_circle(ctx, center, (bounds.size.w+buf)/2);
  
  // set number of tickmarks
  int tick_marks_number = 60;

  // tick mark lengths
  int tick_length_end = (bounds.size.w+buf)/2; 
  int tick_length_start;
  
  // set colors
  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_stroke_color(ctx, settings.ForegroundColor);
  graphics_context_set_stroke_width(ctx, 6);
  
  // draw marks
  for(int i=0; i<tick_marks_number; i++) {
    // if number is divisible by 5, make large mark
    if(i%5==0) {
      graphics_context_set_stroke_width(ctx, 4);
      tick_length_start = tick_length_end-9;
    } else {
      graphics_context_set_stroke_width(ctx, 1);
      tick_length_start = tick_length_end-4;
    }

    int angle = TRIG_MAX_ANGLE * i/tick_marks_number;

    GPoint tick_mark_start = {
      .x = (int)(sin_lookup(angle) * (int)tick_length_start / TRIG_MAX_RATIO) + center.x,
      .y = (int)(-cos_lookup(angle) * (int)tick_length_start / TRIG_MAX_RATIO) + center.y,
    };
    
    GPoint tick_mark_end = {
      .x = (int)(sin_lookup(angle) * (int)tick_length_end / TRIG_MAX_RATIO) + center.x,
      .y = (int)(-cos_lookup(angle) * (int)tick_length_end / TRIG_MAX_RATIO) + center.y,
    };      
    
    graphics_draw_line(ctx, tick_mark_end, tick_mark_start);  
  } // end of loop 
  
  // draw box for day and date on right of watch
  GRect temp_rect = GRect(PBL_IF_ROUND_ELSE(113, 89), PBL_IF_ROUND_ELSE(82, 76), 53, 16);
//   GRect temp_rect = GRect(89, 76, 53, 16);
  graphics_draw_round_rect(ctx, temp_rect, 3);
  
  // dividing line in date round rectange
  GPoint start_temp_line = GPoint(PBL_IF_ROUND_ELSE(147, 123), PBL_IF_ROUND_ELSE(83, 77));
  GPoint end_temp_line = GPoint(PBL_IF_ROUND_ELSE(147, 123), PBL_IF_ROUND_ELSE(97, 91));
  graphics_draw_line(ctx, start_temp_line, end_temp_line);    
}


/////////////////////////////
// draw temperature circle //
/////////////////////////////
static void temp_update_proc(Layer *layer, GContext *ctx) {
  graphics_context_set_stroke_color(ctx, settings.ForegroundColor);
  GPoint center = GPoint(PBL_IF_ROUND_ELSE(180/2, 144/2), 36);
  graphics_context_set_fill_color(ctx, settings.ForegroundColor);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_circle(ctx, center, 40/2);
}


///////////////////////////
// update battery status //
///////////////////////////
static void battery_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = GRect(PBL_IF_ROUND_ELSE(16, 4), PBL_IF_ROUND_ELSE(70, 64), 40, 40);
  graphics_context_set_fill_color(ctx, settings.ForegroundColor);
  graphics_fill_radial(ctx, bounds, GOvalScaleModeFitCircle, 2, DEG_TO_TRIGANGLE(360-(battery_percent*3.6)), DEG_TO_TRIGANGLE(360));
  
  // draw vertical battery
  graphics_context_set_stroke_color(ctx, settings.ForegroundColor);
  graphics_draw_round_rect(ctx, GRect(PBL_IF_ROUND_ELSE(33, 21), PBL_IF_ROUND_ELSE(83, 77), 7, 14), 1);
  int batt = battery_percent/10;
  graphics_fill_rect(ctx, GRect(PBL_IF_ROUND_ELSE(35, 23), PBL_IF_ROUND_ELSE(95-batt, 89-batt), 3, batt), 1, GCornerNone);
  graphics_fill_rect(ctx, GRect(PBL_IF_ROUND_ELSE(35, 23), PBL_IF_ROUND_ELSE(82, 76), 3, 1), 0, GCornerNone);  
  
  // set visibility of charging icon
  layer_set_hidden(bitmap_layer_get_layer(s_charging_bitmap_layer), !charging);
}


//////////////////////////
// update health status //
//////////////////////////
static void health_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = GRect(PBL_IF_ROUND_ELSE(70, 52), PBL_IF_ROUND_ELSE(122, 110), 40, 40);
  graphics_context_set_fill_color(ctx, settings.ForegroundColor);
  graphics_fill_radial(ctx, bounds, GOvalScaleModeFitCircle, 2, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE((step_count/step_goal)*360));
}


/////////////////////////////////
// draw hands and update ticks //
/////////////////////////////////
static void ticks_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds); 
    
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  
  int hand_point_end = ((bounds.size.h)/2)-10;
  int hand_point_start = hand_point_end - 60;
  
  int filler_point_end = 40;
  int filler_point_start = filler_point_end-15;
  
  float minute_angle = TRIG_MAX_ANGLE * t->tm_min / 60;
  GPoint minute_hand_start = {
    .x = (int)(sin_lookup(minute_angle) * (int)hand_point_start / TRIG_MAX_RATIO) + center.x,
    .y = (int)(-cos_lookup(minute_angle) * (int)hand_point_start / TRIG_MAX_RATIO) + center.y,
  };
  
  GPoint minute_hand_end = {
    .x = (int)(sin_lookup(minute_angle) * (int)hand_point_end / TRIG_MAX_RATIO) + center.x,
    .y = (int)(-cos_lookup(minute_angle) * (int)hand_point_end / TRIG_MAX_RATIO) + center.y,
  };    
  
  float hour_angle = TRIG_MAX_ANGLE * ((((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6);
  GPoint hour_hand_start = {
    .x = (int)(sin_lookup(hour_angle) * (int)hand_point_start / TRIG_MAX_RATIO) + center.x,
    .y = (int)(-cos_lookup(hour_angle) * (int)hand_point_start / TRIG_MAX_RATIO) + center.y,
  };
  
  GPoint hour_hand_end = {
    .x = (int)(sin_lookup(hour_angle) * (int)(hand_point_end-25) / TRIG_MAX_RATIO) + center.x,
    .y = (int)(-cos_lookup(hour_angle) * (int)(hand_point_end-25) / TRIG_MAX_RATIO) + center.y,
  };   
  
  GPoint filler_start = {
    .x = (int)(sin_lookup(hour_angle) * (int)filler_point_start / TRIG_MAX_RATIO) + center.x,
    .y = (int)(-cos_lookup(hour_angle) * (int)filler_point_start / TRIG_MAX_RATIO) + center.y,
  };
  
  GPoint filler_end = {
    .x = (int)(sin_lookup(hour_angle) * (int)filler_point_end / TRIG_MAX_RATIO) + center.x,
    .y = (int)(-cos_lookup(hour_angle) * (int)filler_point_end / TRIG_MAX_RATIO) + center.y,
  }; 
  
  // set colors
  graphics_context_set_antialiased(ctx, true);
   
  // draw wide part of minute hand in background color for shadow
  graphics_context_set_stroke_color(ctx, settings.BackgroundColor);  
  graphics_context_set_stroke_width(ctx, 8);  
  graphics_draw_line(ctx, minute_hand_start, minute_hand_end);  
  
  // draw minute line
  graphics_context_set_stroke_color(ctx, settings.ForegroundColor);  
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_line(ctx, center, minute_hand_start);  
  
  // draw wide part of minute hand
   graphics_context_set_stroke_color(ctx, settings.ForegroundColor);
  graphics_context_set_stroke_width(ctx, 6);  
  graphics_draw_line(ctx, minute_hand_start, minute_hand_end);   
  
  // draw wide part of hour hand in background color for shadow
  graphics_context_set_stroke_color(ctx, settings.BackgroundColor);  
  graphics_context_set_stroke_width(ctx, 8);
  graphics_draw_line(ctx, hour_hand_start, hour_hand_end);  
  
  // draw small hour line
  graphics_context_set_stroke_color(ctx, settings.ForegroundColor); 
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_line(ctx, center, hour_hand_start);   
  
  // draw wide part of hour hand
  graphics_context_set_stroke_color(ctx, settings.ForegroundColor);  
  graphics_context_set_stroke_width(ctx, 6);
  graphics_draw_line(ctx, hour_hand_start, hour_hand_end);
  
  // draw inner hour line
  graphics_context_set_stroke_color(ctx, settings.BackgroundColor);  
  graphics_context_set_stroke_width(ctx, 2);
  graphics_draw_line(ctx, filler_start, filler_end);    
  
  // circle overlay
  // draw circle in middle 
  graphics_context_set_fill_color(ctx, settings.ForegroundColor);  
  graphics_fill_circle(ctx, center, 3);
  
  // dot in the middle
  graphics_context_set_fill_color(ctx, settings.BackgroundColor);
  graphics_fill_circle(ctx, center, 1);    
}


//////////////////////
// load main window //
//////////////////////
static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // fonts
  s_word_font = fonts_load_custom_font(resource_get_handle(WORD_FONT));
  s_number_font = fonts_load_custom_font(resource_get_handle(NUMBER_FONT));

  // create canvas layer for dial
  s_dial_layer = layer_create(bounds);
  layer_set_update_proc(s_dial_layer, dial_update_proc);
  layer_add_child(window_layer, s_dial_layer);  
  
  // create temp circle
  s_temp_circle = layer_create(bounds);
  layer_set_update_proc(s_temp_circle, temp_update_proc);
  layer_add_child(s_dial_layer, s_temp_circle);
  
  // create temp text
  s_temp_layer = text_layer_create(GRect(PBL_IF_ROUND_ELSE(78, 60), 19, 24, 16));
  text_layer_set_background_color(s_temp_layer, GColorClear);
  text_layer_set_text_alignment(s_temp_layer, GTextAlignmentCenter);
  text_layer_set_font(s_temp_layer, s_number_font);
  layer_add_child(s_dial_layer, text_layer_get_layer(s_temp_layer));
  
  // create battery layer
  s_battery_circle = layer_create(bounds);
  layer_set_update_proc(s_battery_circle, battery_update_proc);
  layer_add_child(s_dial_layer, s_battery_circle);
  
  // charging icon
  s_charging_bitmap = gbitmap_create_with_resource(RESOURCE_ID_LIGHTENING_WHITE_ICON);
  s_charging_bitmap_layer = bitmap_layer_create(GRect(PBL_IF_ROUND_ELSE(38, 26), PBL_IF_ROUND_ELSE(82, 76), 14, 14));
  bitmap_layer_set_compositing_mode(s_charging_bitmap_layer, GCompOpSet);
  bitmap_layer_set_bitmap(s_charging_bitmap_layer, s_charging_bitmap); 
  layer_add_child(s_dial_layer, bitmap_layer_get_layer(s_charging_bitmap_layer));    
  
  // bluetooth disconnected icon
  s_bluetooth_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BLUETOOTH_DISCONNECTED_WHITE_ICON);
  s_bluetooth_bitmap_layer = bitmap_layer_create(GRect(PBL_IF_ROUND_ELSE(20, 8), PBL_IF_ROUND_ELSE(82, 76), 14, 14));
  bitmap_layer_set_compositing_mode(s_bluetooth_bitmap_layer, GCompOpSet);
  bitmap_layer_set_bitmap(s_bluetooth_bitmap_layer, s_bluetooth_bitmap); 
  layer_add_child(s_dial_layer, bitmap_layer_get_layer(s_bluetooth_bitmap_layer));       
  
  // create health layer text
  s_health_layer = text_layer_create(GRect(PBL_IF_ROUND_ELSE(72, 54), PBL_IF_ROUND_ELSE(127, 115), 36, 16));
  text_layer_set_background_color(s_health_layer, GColorClear);
  text_layer_set_text_alignment(s_health_layer, GTextAlignmentCenter);
  text_layer_set_font(s_health_layer, s_number_font);
  layer_add_child(s_dial_layer, text_layer_get_layer(s_health_layer));  
  
  // create health layer circle
  s_health_circle = layer_create(bounds);
  layer_set_update_proc(s_health_circle, health_update_proc);
  layer_add_child(s_dial_layer, s_health_circle);
    
  // create shoe icon
  s_health_bitmap = gbitmap_create_with_resource(RESOURCE_ID_SHOE_WHITE_ICON);
  s_health_bitmap_layer = bitmap_layer_create(GRect(PBL_IF_ROUND_ELSE(78, 60), PBL_IF_ROUND_ELSE(143, 131), 24, 16));
  bitmap_layer_set_compositing_mode(s_health_bitmap_layer, GCompOpSet);
  bitmap_layer_set_bitmap(s_health_bitmap_layer, s_health_bitmap); 
  layer_add_child(s_dial_layer, bitmap_layer_get_layer(s_health_bitmap_layer));
  
  // Day Text
  s_day_text_layer = text_layer_create(GRect(PBL_IF_ROUND_ELSE(114, 90), PBL_IF_ROUND_ELSE(81, 75), 34, 14));
  text_layer_set_background_color(s_day_text_layer, GColorClear);
  text_layer_set_text_alignment(s_day_text_layer, GTextAlignmentCenter);
  text_layer_set_font(s_day_text_layer, s_word_font);
  layer_add_child(s_dial_layer, text_layer_get_layer(s_day_text_layer));
  
  // Date text
  s_date_text_layer = text_layer_create(GRect(PBL_IF_ROUND_ELSE(148, 124), PBL_IF_ROUND_ELSE(81, 75), 17, 14));
  text_layer_set_background_color(s_date_text_layer, GColorClear);
  text_layer_set_text_alignment(s_date_text_layer, GTextAlignmentCenter);
  text_layer_set_font(s_date_text_layer, s_number_font);
  layer_add_child(s_dial_layer, text_layer_get_layer(s_date_text_layer));  
  
  // create canvas layer for hands
  s_hands_layer = layer_create(bounds);
  layer_set_update_proc(s_hands_layer, ticks_update_proc);
  layer_add_child(window_layer, s_hands_layer);

	setColors();	
	config_save(); 
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "main_window_load");
}


///////////////////////
// update clock time //
///////////////////////
static void update_time() {
  // get a tm strucutre
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  // write date to buffer
  static char date_buffer[32];
  strftime(date_buffer, sizeof(date_buffer), "%e", tick_time);
  
  // write day to buffer
  static char day_buffer[32];
  char *weekday = day_buffer;
  strftime(day_buffer, sizeof(day_buffer), "%w", tick_time);
  
  // make date all caps
  if(strcmp(day_buffer, "0")==0) {
    weekday = "SUN";
  } else if(strcmp(day_buffer, "1")==0) {
    weekday = "MON";
  } else if(strcmp(day_buffer, "2")==0) {
    weekday = "TUE";
  } else if(strcmp(day_buffer, "3")==0) {
    weekday = "WED";
  } else if(strcmp(day_buffer, "4")==0) {
    weekday = "THU";
  } else if(strcmp(day_buffer, "5")==0) {
    weekday = "FRI";
  } else if(strcmp(day_buffer, "6")==0) {
    weekday = "SAT";
  } else {
    weekday = "";
  }
  
  // display this time on the text layer
  text_layer_set_text(s_date_text_layer, date_buffer+(('0' == date_buffer[0]?1:0))); // remove padding
  text_layer_set_text(s_day_text_layer, weekday); 
}


//////////////////
// handle ticks //
//////////////////
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(s_hands_layer);
  update_time();
  
  // Get weather update every 30 minutes
  if(tick_time->tm_min % 30 == 0) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
  
    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);
  
    // Send the message!
    app_message_outbox_send();
  }  
}


/////////////////////////////////////
// registers battery update events //
/////////////////////////////////////
static void battery_handler(BatteryChargeState charge_state) {
  battery_percent = charge_state.charge_percent;
  if(charge_state.is_charging || charge_state.is_plugged) {
    charging = true;
  } else {
    charging = false;
  }
  // force update to circle
  layer_mark_dirty(s_battery_circle);
}


/////////////////////////////
// manage bluetooth status //
/////////////////////////////
static void bluetooth_callback(bool connected) {
  layer_set_hidden(bitmap_layer_get_layer(s_bluetooth_bitmap_layer), connected);
  if(!connected) {
    vibes_double_pulse();
  }
}


// registers health update events
static void health_handler(HealthEventType event, void *context) {
  if(event==HealthEventMovementUpdate) {
    step_count = (double)health_service_sum_today(HealthMetricStepCount);
    // write to char_current_steps variable
    static char health_buf[32];
    if(step_count>=1000) {
      double s_c = step_count/1000;
      snprintf(health_buf, sizeof(health_buf), "%dk", (int)s_c);
    } else {
      snprintf(health_buf, sizeof(health_buf), "%d", (int)step_count);
    }
    
    char_current_steps = health_buf;
    text_layer_set_text(s_health_layer, char_current_steps);
    
    // force update to circle
    layer_mark_dirty(s_health_circle);
    
    APP_LOG(APP_LOG_LEVEL_INFO, "health_handler completed");
  }
}


///////////////////
// unload window //
///////////////////
static void main_window_unload(Window *window) {
  layer_destroy(s_dial_layer);
  layer_destroy(s_hands_layer);
  layer_destroy(s_temp_circle);
  layer_destroy(s_battery_circle);
  layer_destroy(s_health_circle);
  
  text_layer_destroy(s_temp_layer);
  text_layer_destroy(s_health_layer);
  text_layer_destroy(s_day_text_layer);
  text_layer_destroy(s_date_text_layer);  
  
  gbitmap_destroy(s_weather_bitmap);
  gbitmap_destroy(s_health_bitmap);
  gbitmap_destroy(s_bluetooth_bitmap);
  gbitmap_destroy(s_charging_bitmap);
  
  bitmap_layer_destroy(s_weather_bitmap_layer);
  bitmap_layer_destroy(s_health_bitmap_layer);
  bitmap_layer_destroy(s_bluetooth_bitmap_layer);
  bitmap_layer_destroy(s_charging_bitmap_layer);
}


//////////////////////////////////////////////////////
// display appropriate weather icon                 //
// works with DarkSky.net and OpenWeatherMap.org    //
// https://darksky.net/dev/docs/response#data-point //
// https://openweathermap.org/weather-conditions    //
//////////////////////////////////////////////////////
static void load_icons() {
  
  // if inverted
  if(settings.InvertColors) {
    // populate icon variable
    
    // DS clear-day
    // OW 01d (clear sky, day)
    
    if(strcmp(icon_buf, "clear-day")==0 || 
       strcmp(icon_buf, "01d")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CLEAR_SKY_DAY_BLACK_ICON);  
      
    // DS clear-night
    // OW 01n (clear sky, night)
      
    } else if(strcmp(icon_buf, "clear-night")==0 || 
              strcmp(icon_buf, "01n")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CLEAR_SKY_NIGHT_BLACK_ICON);
      
    // DS rain
    // OW 09d (shower rain, day)
    // OW 09n (shower rain, night)
    // OW 10d (rain, day)
    // OW 10n (rain, night)
    // OW 11d (thunderstorm, day)
    // OW 11n (thunderstorm, night)
      
    } else if(strcmp(icon_buf, "rain")==0 ||
             strcmp(icon_buf, "09d")==0 || 
             strcmp(icon_buf, "09n")==0 || 
             strcmp(icon_buf, "10d")==0 || 
             strcmp(icon_buf, "10n")==0 || 
             strcmp(icon_buf, "11d")==0 || 
             strcmp(icon_buf, "11n")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_RAIN_BLACK_ICON);
      
    // OW 50d (mist, day)
      
    } else if(strcmp(icon_buf, "50d")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_MIST_DAY_BLACK_ICON);
      
    // OW 50n (mist, night)
      
    } else if(strcmp(icon_buf, "50n")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_MIST_NIGHT_BLACK_ICON);
      
    // DS snow
    // OW 13d (snow, day)
    // OW 13n (snow, night)
      
    } else if(strcmp(icon_buf, "snow")==0 || 
              strcmp(icon_buf, "13d")==0 || 
              strcmp(icon_buf, "13n")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_SNOW_BLACK_ICON);
      
    // DS sleet
      
    } else if(strcmp(icon_buf, "sleet")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_SLEET_BLACK_ICON);
      
    // DS wind
      
    } else if(strcmp(icon_buf, "wind")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_WIND_BLACK_ICON);
      
    // DS fog
      
    } else if(strcmp(icon_buf, "fog")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_FOG_BLACK_ICON);
      
    // DS cloudy
      
    } else if(strcmp(icon_buf, "cloudy")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CLOUDY_BLACK_ICON);
      
    // DS partly-cloudy-day
    // OW 02d (few clouds, day)
    // OW 03d (scattered clouds, day)
    // OW 04d (broken clouds, day)
      
    } else if(strcmp(icon_buf, "partly-cloudy-day")==0 || 
              strcmp(icon_buf, "02d")==0 || 
              strcmp(icon_buf, "03d")==0 || 
              strcmp(icon_buf, "04d")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_PARTLY_CLOUDY_DAY_BLACK_ICON);
      
    // DS partly-cloudy-night
    // OW 02d (few clouds, night)
    // OW 03d (scattered clouds, night)
    // OW 04d (broken clouds, night)      
      
    } else if(strcmp(icon_buf, "partly-cloudy-night")==0 || 
              strcmp(icon_buf, "02n")==0 || 
              strcmp(icon_buf, "03n")==0 || 
              strcmp(icon_buf, "04n")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_PARTLY_CLOUDY_NIGHT_BLACK_ICON);
    } 
    
  } else {
  // not inverted
  // populate icon variable
    
    // DS clear-day
    // OW 01d (clear sky, day)    
    
    if(strcmp(icon_buf, "clear-day")==0 || 
       strcmp(icon_buf, "01d")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CLEAR_SKY_DAY_WHITE_ICON);  
      
    // DS clear-night
    // OW 01n (clear sky, night)
      
    } else if(strcmp(icon_buf, "clear-night")==0 || 
              strcmp(icon_buf, "01n")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CLEAR_SKY_NIGHT_WHITE_ICON);
      
    // DS rain
    // OW 09d (shower rain, day)
    // OW 09n (shower rain, night)
    // OW 10d (rain, day)
    // OW 10n (rain, night)
    // OW 11d (thunderstorm, day)
    // OW 11n (thunderstorm, night)
      
    } else if(strcmp(icon_buf, "rain")==0 ||
             strcmp(icon_buf, "09d")==0 || 
             strcmp(icon_buf, "09n")==0 || 
             strcmp(icon_buf, "10d")==0 || 
             strcmp(icon_buf, "10n")==0 || 
             strcmp(icon_buf, "11d")==0 || 
             strcmp(icon_buf, "11n")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_RAIN_WHITE_ICON);
      
    // OW 50d (mist, day)
      
    } else if(strcmp(icon_buf, "50d")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_MIST_DAY_WHITE_ICON);
      
    // OW 50n (mist, night)
      
    } else if(strcmp(icon_buf, "50n")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_MIST_NIGHT_WHITE_ICON);      
      
    // DS snow
    // OW 13d (snow, day)
    // OW 13n (snow, night)
      
    } else if(strcmp(icon_buf, "snow")==0 || 
              strcmp(icon_buf, "13d")==0 || 
              strcmp(icon_buf, "13n")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_SNOW_WHITE_ICON);
      
    // DS sleet
      
    } else if(strcmp(icon_buf, "sleet")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_SLEET_WHITE_ICON);
      
    // DS wind
      
    } else if(strcmp(icon_buf, "wind")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_WIND_WHITE_ICON);
      
    // DS fog
      
    } else if(strcmp(icon_buf, "fog")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_FOG_WHITE_ICON);
      
    // DS cloudy
      
    } else if(strcmp(icon_buf, "cloudy")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CLOUDY_WHITE_ICON);
      
    // DS partly-cloudy-day
    // OW 02d (few clouds, day)
    // OW 03d (scattered clouds, day)
    // OW 04d (broken clouds, day)
      
    } else if(strcmp(icon_buf, "partly-cloudy-day")==0 || 
              strcmp(icon_buf, "02d")==0 || 
              strcmp(icon_buf, "03d")==0 || 
              strcmp(icon_buf, "04d")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_PARTLY_CLOUDY_DAY_WHITE_ICON);
      
    // DS partly-cloudy-night
    // OW 02d (few clouds, night)
    // OW 03d (scattered clouds, night)
    // OW 04d (broken clouds, night)
      
    } else if(strcmp(icon_buf, "partly-cloudy-night")==0 || 
              strcmp(icon_buf, "02n")==0 || 
              strcmp(icon_buf, "03n")==0 || 
              strcmp(icon_buf, "04n")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_PARTLY_CLOUDY_NIGHT_WHITE_ICON);
    }   
  }
  
  // populate weather icon
  if(s_weather_bitmap_layer) {
    bitmap_layer_destroy(s_weather_bitmap_layer);
  }
  s_weather_bitmap_layer = bitmap_layer_create(GRect(PBL_IF_ROUND_ELSE(78, 60), 35, 24, 16));
  bitmap_layer_set_compositing_mode(s_weather_bitmap_layer, GCompOpSet);  
  bitmap_layer_set_bitmap(s_weather_bitmap_layer, s_weather_bitmap); 
  layer_add_child(s_dial_layer, bitmap_layer_get_layer(s_weather_bitmap_layer)); 
  
  // populate health icon
  if(s_health_bitmap_layer) {
    bitmap_layer_destroy(s_health_bitmap_layer);
  }
  if(settings.InvertColors) {
    s_health_bitmap = gbitmap_create_with_resource(RESOURCE_ID_SHOE_BLACK_ICON);
  } else {
    s_health_bitmap = gbitmap_create_with_resource(RESOURCE_ID_SHOE_WHITE_ICON);
  }
  s_health_bitmap_layer = bitmap_layer_create(GRect(PBL_IF_ROUND_ELSE(78, 60), PBL_IF_ROUND_ELSE(143, 131), 24, 16));
  bitmap_layer_set_compositing_mode(s_health_bitmap_layer, GCompOpSet);
  bitmap_layer_set_bitmap(s_health_bitmap_layer, s_health_bitmap); 
  layer_add_child(s_dial_layer, bitmap_layer_get_layer(s_health_bitmap_layer));  
}


////////////////////////////
// weather and Clay calls //
////////////////////////////
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Store incoming information
  static char temp_buf[32];

  // Read tuples for data
  Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_KEY_TEMP);
  Tuple *icon_tuple = dict_find(iterator, MESSAGE_KEY_KEY_ICON);  
  
  // If all data is available, use it
  if(temp_tuple && icon_tuple) {
    
    // temp
    snprintf(temp_buf, sizeof(temp_buf), "%d°", (int)temp_tuple->value->int32);  
    text_layer_set_text(s_temp_layer, temp_buf);

    // icon
    snprintf(icon_buf, sizeof(icon_buf), "%s", icon_tuple->value->cstring);
  }  
  
  // load weather icons
  load_icons();
  
  // determine if user inverted colors
  Tuple *invert_colors_t = dict_find(iterator, MESSAGE_KEY_KEY_INVERT_COLORS);
  if(invert_colors_t) { settings.InvertColors = invert_colors_t->value->int32 == 1; }
  
  if(settings.InvertColors==1) {
    settings.BackgroundColor = GColorWhite;
    settings.ForegroundColor = GColorBlack;
  } else {
    settings.BackgroundColor = GColorBlack;
    settings.ForegroundColor = GColorWhite;
  }
  
	setColors();	
	config_save();
  
  APP_LOG(APP_LOG_LEVEL_INFO, "inbox_received_callback");
}


static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}


static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}


static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}


////////////////////
// initialize app //
////////////////////
static void init() {
  config_load();
  
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  // show window on the watch with animated=true
  window_stack_push(s_main_window, true);
  
  // subscribe to time events
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Make sure the time is displayed from the start
  update_time();
    
  // subscribe to health events 
  health_service_events_subscribe(health_handler, NULL); 
  // force initial update
  health_handler(HealthEventMovementUpdate, NULL);   
    
  // register with Battery State Service
  battery_state_service_subscribe(battery_handler);
  // force initial update
  battery_handler(battery_state_service_peek());      
  
  // register with bluetooth state service
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bluetooth_callback
  });
  bluetooth_callback(connection_service_peek_pebble_app_connection());  
    
  // Register weather callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);  
  app_message_open(128, 128);  
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "init");  
}


///////////////////////
// de-initialize app //
///////////////////////
static void deinit() {
  window_destroy(s_main_window);
}


/////////////
// run app //
/////////////
int main(void) {
  init();
  app_event_loop();
  deinit();
}
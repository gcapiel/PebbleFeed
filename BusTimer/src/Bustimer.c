// Copyright (c) 2013 Douwe Maan <http://www.douwemaan.com/>
// The above copyright notice shall be included in all copies or substantial portions of the program.

// Envisioned as a watchface by Jean-Noël Mattern
// Based on the display of the Freebox Revolution, which was designed by Philippe Starck.

#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include <string.h>

#define MY_UUID { 0xAD, 0x5F, 0x60, 0xD1, 0xEB, 0x8E, 0x43, 0xF7, 0x88, 0x7F, 0xE4, 0x2D, 0xD4, 0x5A, 0x22, 0x26 }
PBL_APP_INFO(MY_UUID,
             "BusTimer", "Gerardo Capiel",
             1, 3, /* App version */
             RESOURCE_ID_IMAGE_MENU_ICON,
             APP_INFO_WATCH_FACE);


// Settings
#define COUNTDOWN_SLOT_ANIMATION_DURATION  500

// Magic numbers
#define SCREEN_WIDTH        144
#define SCREEN_HEIGHT       168

#define COUNTDOWN_IMAGE_WIDTH    70
#define COUNTDOWN_IMAGE_HEIGHT   70

#define TIME_IMAGE_WIDTH    20
#define TIME_IMAGE_HEIGHT   20

#define DAY_IMAGE_WIDTH     20
#define DAY_IMAGE_HEIGHT    10

#define MARGIN              1
#define COUNTDOWN_SLOT_SPACE     2
#define TIME_PART_SPACE     4

#define NUMBER_CAL_EVENTS 2

// Images
#define NUMBER_OF_COUNTDOWN_IMAGES 10
const int COUNTDOWN_IMAGE_RESOURCE_IDS[NUMBER_OF_COUNTDOWN_IMAGES] = {
  RESOURCE_ID_IMAGE_COUNTDOWN_0, 
  RESOURCE_ID_IMAGE_COUNTDOWN_1, RESOURCE_ID_IMAGE_COUNTDOWN_2, RESOURCE_ID_IMAGE_COUNTDOWN_3, 
  RESOURCE_ID_IMAGE_COUNTDOWN_4, RESOURCE_ID_IMAGE_COUNTDOWN_5, RESOURCE_ID_IMAGE_COUNTDOWN_6, 
  RESOURCE_ID_IMAGE_COUNTDOWN_7, RESOURCE_ID_IMAGE_COUNTDOWN_8, RESOURCE_ID_IMAGE_COUNTDOWN_9
};

#define NUMBER_OF_TIME_IMAGES 10
const int TIME_IMAGE_RESOURCE_IDS[NUMBER_OF_TIME_IMAGES] = {
  RESOURCE_ID_IMAGE_TIME_0, 
  RESOURCE_ID_IMAGE_TIME_1, RESOURCE_ID_IMAGE_TIME_2, RESOURCE_ID_IMAGE_TIME_3, 
  RESOURCE_ID_IMAGE_TIME_4, RESOURCE_ID_IMAGE_TIME_5, RESOURCE_ID_IMAGE_TIME_6, 
  RESOURCE_ID_IMAGE_TIME_7, RESOURCE_ID_IMAGE_TIME_8, RESOURCE_ID_IMAGE_TIME_9
};

#define NUMBER_OF_DAY_IMAGES 7
const int DAY_IMAGE_RESOURCE_IDS[NUMBER_OF_DAY_IMAGES] = {
  RESOURCE_ID_IMAGE_DAY_0, RESOURCE_ID_IMAGE_DAY_1, RESOURCE_ID_IMAGE_DAY_2, 
  RESOURCE_ID_IMAGE_DAY_3, RESOURCE_ID_IMAGE_DAY_4, RESOURCE_ID_IMAGE_DAY_5, 
  RESOURCE_ID_IMAGE_DAY_6
};


// Main
Window window;
Layer time_container_layer;
TextLayer event_layer;

int buzzOne = 0;
int buzzTwo = 0;

typedef struct calEvent {
  int           buzzed;
  char			*eventTitle;
  int           eventTimeMin;
} calEvent;

calEvent cal_events[NUMBER_CAL_EVENTS];

#define EMPTY_SLOT -1

typedef struct Slot {
  int           number;
  BmpContainer  image_container;
  int           state;
} Slot;

// Time
typedef struct TimeSlot {
  Slot              slot;
  int               new_state;
  PropertyAnimation slide_out_animation;
  PropertyAnimation slide_in_animation;
  bool              animating;
} TimeSlot;

#define NUMBER_OF_COUNTDOWN_SLOTS 4
Layer countdown_layer;
TimeSlot countdown_slots[NUMBER_OF_COUNTDOWN_SLOTS];

// Date
#define NUMBER_OF_TIME_SLOTS 4
Layer time_layer;
Slot time_slots[NUMBER_OF_TIME_SLOTS];

// Day
typedef struct DayItem {
  BmpContainer  image_container;
  Layer         layer;
  bool          loaded;
} DayItem;
DayItem day_item;


// General
BmpContainer *load_digit_image_into_slot(Slot *slot, int digit_value, Layer *parent_layer, GRect frame, const int *digit_resource_ids);
void unload_digit_image_from_slot(Slot *slot);

// Display
void display_countdown(PblTm *tick_time);
void display_day(PblTm *tick_time);

// Time
void display_countdown_value(int value, int row_number);
void uptime_countdown_slot(TimeSlot *countdown_slot, int digit_value);
GRect frame_for_countdown_slot(TimeSlot *countdown_slot);
void slide_in_digit_image_into_countdown_slot(TimeSlot *countdown_slot, int digit_value);
void countdown_slot_slide_in_animation_stopped(Animation *slide_in_animation, bool finished, void *context);
void slide_out_digit_image_from_countdown_slot(TimeSlot *countdown_slot);
void countdown_slot_slide_out_animation_stopped(Animation *slide_out_animation, bool finished, void *context);

// Date
void display_time_value(int value, int part_number);
void uptime_time_slot(Slot *time_slot, int digit_value);

// Handlers
void pbl_main(void *params);
void handle_init(AppContextRef ctx);
void handle_second_tick(AppContextRef ctx, PebbleTickEvent *event);
void handle_deinit(AppContextRef ctx);


// General
BmpContainer *load_digit_image_into_slot(Slot *slot, int digit_value, Layer *parent_layer, GRect frame, const int *digit_resource_ids) {
  if (digit_value < 0 || digit_value > 9) {
    return NULL;
  }

  if (slot->state != EMPTY_SLOT) {
    return NULL;
  }

  slot->state = digit_value;

  BmpContainer *image_container = &slot->image_container;

  bmp_init_container(digit_resource_ids[digit_value], image_container);
  layer_set_frame(&image_container->layer.layer, frame);
  layer_add_child(parent_layer, &image_container->layer.layer);

  return image_container;
}

void unload_digit_image_from_slot(Slot *slot) {
  if (slot->state == EMPTY_SLOT) {
    return;
  }

  BmpContainer *image_container = &slot->image_container;

  layer_remove_from_parent(&image_container->layer.layer);
  bmp_deinit_container(image_container);

  slot->state = EMPTY_SLOT;
}

// Display
void display_countdown(PblTm *tick_time) {

 int diffMinFirst = 0;
 int diffMinSecond = 0;
 int i;
 int hour = tick_time->tm_hour;
 int currentMin = tick_time->tm_year * (365*24*60) + (tick_time->tm_mon + 1) * (31*24*60) + tick_time->tm_mday * (24*60) + tick_time->tm_hour * 60 + tick_time->tm_min;
  
  if ((tick_time->tm_year + 1900) == 2000 && (tick_time->tm_mon + 1) == 1 && tick_time->tm_mday == 1) { 
	  diffMinFirst = (0 * 60 + 0) - (tick_time->tm_hour * 60 + tick_time->tm_min);
	  diffMinSecond = (0 * 60 + 0) - (tick_time->tm_hour * 60 + tick_time->tm_min);
  }
  
  if (diffMinFirst == 5 && buzzOne == 0) {
  	vibes_short_pulse();
  	buzzOne = 1;
  }

  if (diffMinSecond == 5 && buzzTwo == 0) {
  	vibes_short_pulse();
  	buzzTwo = 1;
  }

  if (diffMinFirst >= 0 && diffMinFirst < 100) {
    display_countdown_value(diffMinFirst, 0);
  }

  if (diffMinSecond >= 0 && diffMinSecond < 100) {
    display_countdown_value(diffMinSecond, 1);
  }
  
  /* TODO: calculate right array size or resize it automatically */
  static char messageStr[200];
  strcpy(messageStr, "");
  
  for (i=0; i < NUMBER_CAL_EVENTS; i++) {
	calEvent *cal_event = &cal_events[i];
	if (cal_event->eventTimeMin >= currentMin && (strlen(messageStr) + strlen(cal_event->eventTitle)) < 200) {
		strcat(messageStr, cal_event->eventTitle);
	}
	if ((cal_event->eventTimeMin - currentMin) <= 5 && cal_event->buzzed == 0) {
		vibes_short_pulse();
		cal_event->buzzed = 1;
	}
  }

  if (diffMinFirst < 1 && diffMinSecond < 1) {
	  layer_remove_from_parent(&countdown_layer);
	  text_layer_set_text(&event_layer, messageStr);
  }

  if (!clock_is_24h_style()) {
    hour = hour % 12;
    if (hour == 0) {
      hour = 12;
    }
  }

  display_time_value(hour, 0);
  display_time_value(tick_time->tm_min,   1);

}

void display_day(PblTm *tick_time) {
  BmpContainer *image_container = &day_item.image_container;

  if (day_item.loaded) {
    layer_remove_from_parent(&image_container->layer.layer);
    bmp_deinit_container(image_container);
  }

  bmp_init_container(DAY_IMAGE_RESOURCE_IDS[tick_time->tm_wday], image_container);
  layer_add_child(&day_item.layer, &image_container->layer.layer);

  day_item.loaded = true;
}

// Time
void display_countdown_value(int value, int row_number) {
  value = value % 100; // Maximum of two digits per row.

  for (int column_number = 1; column_number >= 0; column_number--) {
    int countdown_slot_number = (row_number * 2) + column_number;

    TimeSlot *countdown_slot = &countdown_slots[countdown_slot_number];

    uptime_countdown_slot(countdown_slot, value % 10);

    value = value / 10;
  }
}

void uptime_countdown_slot(TimeSlot *countdown_slot, int digit_value) {
  if (countdown_slot->slot.state == digit_value) {
    return;
  }

  if (countdown_slot->animating) {
    // Otherwise we'll crash when the animation is replaced by a new animation before we're finished.
    return;
  }

  countdown_slot->animating = true;

  PropertyAnimation *animation;
  if (countdown_slot->slot.state == EMPTY_SLOT) {
    slide_in_digit_image_into_countdown_slot(countdown_slot, digit_value);
    animation = &countdown_slot->slide_in_animation;
  }
  else {
    countdown_slot->new_state = digit_value;

    slide_out_digit_image_from_countdown_slot(countdown_slot);
    animation = &countdown_slot->slide_out_animation;

    animation_set_handlers(&animation->animation, (AnimationHandlers){
      .stopped = (AnimationStoppedHandler)countdown_slot_slide_out_animation_stopped
    }, (void *)countdown_slot);
  }

  animation_schedule(&animation->animation);
}

GRect frame_for_countdown_slot(TimeSlot *countdown_slot) {
  int x = MARGIN + (countdown_slot->slot.number % 2) * (COUNTDOWN_IMAGE_WIDTH + COUNTDOWN_SLOT_SPACE);
  int y = MARGIN + (countdown_slot->slot.number / 2) * (COUNTDOWN_IMAGE_HEIGHT + COUNTDOWN_SLOT_SPACE);

  return GRect(x, y, COUNTDOWN_IMAGE_WIDTH, COUNTDOWN_IMAGE_HEIGHT);
}

void slide_in_digit_image_into_countdown_slot(TimeSlot *countdown_slot, int digit_value) {
  GRect to_frame = frame_for_countdown_slot(countdown_slot);

  int from_x = to_frame.origin.x;
  int from_y = to_frame.origin.y;
  switch (countdown_slot->slot.number) {
    case 0:
      from_x -= COUNTDOWN_IMAGE_WIDTH + MARGIN;
      break;
    case 1:
      from_y -= COUNTDOWN_IMAGE_HEIGHT + MARGIN;
      break;
    case 2:
      from_y += COUNTDOWN_IMAGE_HEIGHT + MARGIN;
      break;
    case 3:
      from_x += COUNTDOWN_IMAGE_WIDTH + MARGIN;
      break;
  }
  GRect from_frame = GRect(from_x, from_y, COUNTDOWN_IMAGE_WIDTH, COUNTDOWN_IMAGE_HEIGHT);

  BmpContainer *image_container = load_digit_image_into_slot(&countdown_slot->slot, digit_value, &countdown_layer, from_frame, COUNTDOWN_IMAGE_RESOURCE_IDS);

  PropertyAnimation *animation = &countdown_slot->slide_in_animation;
  property_animation_init_layer_frame(animation, &image_container->layer.layer, &from_frame, &to_frame);
  animation_set_duration( &animation->animation, COUNTDOWN_SLOT_ANIMATION_DURATION);
  animation_set_curve(    &animation->animation, AnimationCurveLinear);
  animation_set_handlers( &animation->animation, (AnimationHandlers){
    .stopped = (AnimationStoppedHandler)countdown_slot_slide_in_animation_stopped
  }, (void *)countdown_slot);
}

void countdown_slot_slide_in_animation_stopped(Animation *slide_in_animation, bool finished, void *context) {
  TimeSlot *countdown_slot = (TimeSlot *)context;

  countdown_slot->animating = false;
}

void slide_out_digit_image_from_countdown_slot(TimeSlot *countdown_slot) {
  GRect from_frame = frame_for_countdown_slot(countdown_slot);

  int to_x = from_frame.origin.x;
  int to_y = from_frame.origin.y;
  switch (countdown_slot->slot.number) {
    case 0:
      to_y -= COUNTDOWN_IMAGE_HEIGHT + MARGIN;
      break;
    case 1:
      to_x += COUNTDOWN_IMAGE_WIDTH + MARGIN;
      break;
    case 2:
      to_x -= COUNTDOWN_IMAGE_WIDTH + MARGIN;
      break;
    case 3:
      to_y += COUNTDOWN_IMAGE_HEIGHT + MARGIN;
      break;
  }
  GRect to_frame = GRect(to_x, to_y, COUNTDOWN_IMAGE_WIDTH, COUNTDOWN_IMAGE_HEIGHT);

  BmpContainer *image_container = &countdown_slot->slot.image_container;

  PropertyAnimation *animation = &countdown_slot->slide_out_animation;
  property_animation_init_layer_frame(animation, &image_container->layer.layer, &from_frame, &to_frame);
  animation_set_duration( &animation->animation, COUNTDOWN_SLOT_ANIMATION_DURATION);
  animation_set_curve(    &animation->animation, AnimationCurveLinear);

  // Make sure to unload the digit image from the slot when the animation has finished!
}

void countdown_slot_slide_out_animation_stopped(Animation *slide_out_animation, bool finished, void *context) {
  TimeSlot *countdown_slot = (TimeSlot *)context;

  unload_digit_image_from_slot(&countdown_slot->slot);

  slide_in_digit_image_into_countdown_slot(countdown_slot, countdown_slot->new_state);
  animation_schedule(&countdown_slot->slide_in_animation.animation);

  countdown_slot->new_state = EMPTY_SLOT;
}

// Date
void display_time_value(int value, int part_number) {
  value = value % 100; // Maximum of two digits per row.

  for (int column_number = 1; column_number >= 0; column_number--) {
    int time_slot_number = (part_number * 2) + column_number;

    Slot *time_slot = &time_slots[time_slot_number];

    uptime_time_slot(time_slot, value % 10);

    value = value / 10;
  }
}

void uptime_time_slot(Slot *time_slot, int digit_value) {
  if (time_slot->state == digit_value) {
    return;
  }

  int x = time_slot->number * (TIME_IMAGE_WIDTH + MARGIN);
  if (time_slot->number >= 2) {
    x += 3; // 3 extra pixels of space between the day and month
  }
  GRect frame =  GRect(x, 0, TIME_IMAGE_WIDTH, TIME_IMAGE_HEIGHT);

  unload_digit_image_from_slot(time_slot);
  load_digit_image_into_slot(time_slot, digit_value, &time_layer, frame, TIME_IMAGE_RESOURCE_IDS);
}

// Handlers
void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler   = &handle_init,
    .deinit_handler = &handle_deinit,

    .tick_info = {
      .tick_handler = &handle_second_tick,
      .tick_units   = SECOND_UNIT
    }
  };

  app_event_loop(params, &handlers);
}

void handle_init(AppContextRef ctx) {
  window_init(&window, "Revolution");
  window_stack_push(&window, true /* Animated */);
  window_set_background_color(&window, GColorBlack);

  resource_init_current_app(&APP_RESOURCES);

  text_layer_init(&event_layer, window.layer.frame);
  text_layer_set_text_color(&event_layer, GColorWhite);
  text_layer_set_background_color(&event_layer, GColorClear);
  layer_set_frame(&event_layer.layer, GRect(0, 0, SCREEN_WIDTH, SCREEN_WIDTH));
  layer_add_child(&window.layer, &event_layer.layer);
  //text_layer_set_text(&event_layer, "hi");

  // Time slots
  for (int i = 0; i < NUMBER_OF_COUNTDOWN_SLOTS; i++) {
    TimeSlot *countdown_slot = &countdown_slots[i];
    countdown_slot->slot.number  = i;
    countdown_slot->slot.state   = EMPTY_SLOT;
    countdown_slot->new_state    = EMPTY_SLOT;
    countdown_slot->animating    = false;
  }

  // Date slots
  for (int i = 0; i < NUMBER_OF_TIME_SLOTS; i++) {
    Slot *time_slot = &time_slots[i];
    time_slot->number = i;
    time_slot->state  = EMPTY_SLOT;
  }

  // Day slot
  day_item.loaded = false;


  // Root layer
  Layer *root_layer = window_get_root_layer(&window);

  // Time
  layer_init(&countdown_layer, GRect(0, 0, SCREEN_WIDTH, SCREEN_WIDTH));
  layer_set_clips(&countdown_layer, true);
  layer_add_child(root_layer, &countdown_layer);

  // Date container
  int time_container_height = SCREEN_HEIGHT - SCREEN_WIDTH;

  layer_init(&time_container_layer, GRect(0, SCREEN_WIDTH, SCREEN_WIDTH, time_container_height));
  layer_add_child(root_layer, &time_container_layer);

  // Day
  GRect day_layer_frame = GRect(
    MARGIN, 
    time_container_height - DAY_IMAGE_HEIGHT - MARGIN, 
    DAY_IMAGE_WIDTH, 
    DAY_IMAGE_HEIGHT
  );
  layer_init(&day_item.layer, day_layer_frame);
  layer_add_child(&time_container_layer, &day_item.layer);

  // Date
  GRect time_layer_frame = GRectZero;
  time_layer_frame.size.w   = TIME_IMAGE_WIDTH + MARGIN + TIME_IMAGE_WIDTH + TIME_PART_SPACE + TIME_IMAGE_WIDTH + MARGIN + TIME_IMAGE_WIDTH;
  time_layer_frame.size.h   = TIME_IMAGE_HEIGHT;
  time_layer_frame.origin.x = (SCREEN_WIDTH - time_layer_frame.size.w) / 2;
  time_layer_frame.origin.y = time_container_height - TIME_IMAGE_HEIGHT - MARGIN;

  layer_init(&time_layer, time_layer_frame);
  layer_add_child(&time_container_layer, &time_layer);
  
  calEvent *cal_event;
  
  cal_event = &cal_events[0];
cal_event->eventTitle = "0:00 Trace Update Due to Jim on the 10th\n";
cal_event->eventTimeMin = 59673600;
cal_event->buzzed = 0;
cal_event = &cal_events[1];
cal_event->eventTitle = "21:00 Test dd\n";
cal_event->eventTimeMin = 59674860;
cal_event->buzzed = 0;


  // Display
  PblTm tick_time;
  get_time(&tick_time);

  display_countdown(&tick_time);
  display_day(&tick_time);
}

void handle_second_tick(AppContextRef ctx, PebbleTickEvent *event) {

  if ((event->units_changed & MINUTE_UNIT) == MINUTE_UNIT) {
    display_countdown(event->tick_time);
  }

  if ((event->units_changed & DAY_UNIT) == DAY_UNIT) {
    display_day(event->tick_time);
  }
}

void handle_deinit(AppContextRef ctx) {
  for (int i = 0; i < NUMBER_OF_COUNTDOWN_SLOTS; i++) {
    unload_digit_image_from_slot(&countdown_slots[i].slot);
  }
  for (int i = 0; i < NUMBER_OF_TIME_SLOTS; i++) {
    unload_digit_image_from_slot(&time_slots[i]);
  }

  if (day_item.loaded) {
    bmp_deinit_container(&day_item.image_container);
  }
}

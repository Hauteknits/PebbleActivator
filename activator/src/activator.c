#include <pebble.h>
#include <stdint.h>
#include <string.h>
#include "../../common.h"

#define BITMAP_BUFFER_BYTES 1024

static Window *window;
static TextLayer *text_layer_top;
static TextLayer *text_layer_middle;
static TextLayer *text_layer_bottom;

static void send_cmd(uint8_t cmd, int integer) {
	Tuplet value = TupletInteger(cmd, integer);

	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);

	if (iter == NULL)
		return;

	dict_write_tuplet(iter, &value);
	dict_write_end(iter);
	
	app_message_outbox_send();
}

static void up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
	send_cmd(WATCH_KEY_PRESSED, WATCH_KEY_PRESSED_UP);
}

static void up_long_click_handler(ClickRecognizerRef recognizer, void *context) {
	send_cmd(WATCH_KEY_PRESSED, WATCH_KEY_HELD_UP);
}

static void select_single_click_handler(ClickRecognizerRef recognizer, void *context) {
	send_cmd(WATCH_KEY_PRESSED, WATCH_KEY_PRESSED_SELECT);
}

static void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
	send_cmd(WATCH_KEY_PRESSED, WATCH_KEY_HELD_SELECT);
}

static void down_single_click_handler(ClickRecognizerRef recognizer, void *context) {
	send_cmd(WATCH_KEY_PRESSED, WATCH_KEY_PRESSED_DOWN);
}

static void down_long_click_handler(ClickRecognizerRef recognizer, void *context) {
	send_cmd(WATCH_KEY_PRESSED, WATCH_KEY_HELD_DOWN);
}

static void click_config_provider(void *context) {
	window_single_click_subscribe(BUTTON_ID_UP, up_single_click_handler);
	window_long_click_subscribe(BUTTON_ID_UP, 700, up_long_click_handler, NULL);

	window_single_click_subscribe(BUTTON_ID_SELECT, select_single_click_handler);
	window_long_click_subscribe(BUTTON_ID_SELECT, 700, select_long_click_handler, NULL);

	window_single_click_subscribe(BUTTON_ID_DOWN, down_single_click_handler);
	window_long_click_subscribe(BUTTON_ID_DOWN, 700, down_long_click_handler, NULL);
}

static void in_received_handler(DictionaryIterator *iter, void *context) {
	Tuple *tuple = dict_read_first(iter);
	do {
		switch (tuple->key) {
			case ACTIVATOR_REQUEST_VERSION:
				send_cmd(WATCH_RETURN_VERSION, WATCH_VERSION_CURRENT);
				break;
			case ACTIVATOR_SET_TEXT_TOP:
				text_layer_set_text(text_layer_top, tuple->value->cstring);
				break;
			case ACTIVATOR_SET_TEXT_MIDDLE:
				text_layer_set_text(text_layer_middle, tuple->value->cstring);
				break;
			case ACTIVATOR_SET_TEXT_BOTTOM:
				text_layer_set_text(text_layer_bottom, tuple->value->cstring);
				break;
		}
	} while((tuple = dict_read_next(iter)));
}

static void in_dropped_handler(AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Incoming AppMessage from Pebble dropped, %d", reason);
}

static void out_sent_handler(DictionaryIterator *sent, void *context) {
	// outgoing message was delivered
}

static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Failed to send AppMessage to Pebble");
	text_layer_set_text(text_layer_middle, "Failed to send!");
}

static void window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);

	GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);

	text_layer_top = text_layer_create(GRect(0, 0, 144, 68));
	text_layer_set_text_color(text_layer_top, GColorWhite);
	text_layer_set_background_color(text_layer_top, GColorClear);
	text_layer_set_font(text_layer_top, font);
	text_layer_set_text_alignment(text_layer_top, GTextAlignmentCenter);
	text_layer_set_text(text_layer_top, "");
	text_layer_set_overflow_mode(text_layer_top, GTextOverflowModeTrailingEllipsis);

	text_layer_middle = text_layer_create(GRect(0, 55, 144, 68));
	text_layer_set_text_color(text_layer_middle, GColorWhite);
	text_layer_set_background_color(text_layer_middle, GColorClear);
	text_layer_set_font(text_layer_middle, font);
	text_layer_set_text_alignment(text_layer_middle, GTextAlignmentCenter);
	text_layer_set_text(text_layer_middle, "Loading...");
	text_layer_set_overflow_mode(text_layer_middle, GTextOverflowModeTrailingEllipsis);

	text_layer_bottom = text_layer_create(GRect(0, 110, 144, 68));
	text_layer_set_text_color(text_layer_bottom, GColorWhite);
	text_layer_set_background_color(text_layer_bottom, GColorClear);
	text_layer_set_font(text_layer_bottom, font);
	text_layer_set_text_alignment(text_layer_bottom, GTextAlignmentCenter);
	text_layer_set_text(text_layer_bottom, "");
	text_layer_set_overflow_mode(text_layer_bottom, GTextOverflowModeTrailingEllipsis);

	layer_add_child(window_layer, text_layer_get_layer(text_layer_top));
	layer_add_child(window_layer, text_layer_get_layer(text_layer_middle));
	layer_add_child(window_layer, text_layer_get_layer(text_layer_bottom));
}

static void window_unload(Window *window) {
	text_layer_destroy(text_layer_top);
	text_layer_destroy(text_layer_middle);
	text_layer_destroy(text_layer_bottom);
}

static void app_message_init(void) {
	app_message_register_inbox_received(in_received_handler);
	app_message_register_inbox_dropped(in_dropped_handler);
	app_message_register_outbox_sent(out_sent_handler);
	app_message_register_outbox_failed(out_failed_handler);
	app_message_open(512 /* inbound_size */, 16 /* outbound_size */);
}

static void init(void) {
	app_message_init();

	window = window_create();
	window_set_click_config_provider(window, click_config_provider);
	window_set_window_handlers(window, (WindowHandlers) {
		.load = window_load,
		.unload = window_unload,
	});
	window_set_background_color(window, GColorBlack);
	window_stack_push(window, true /* animated */);

	send_cmd(WATCH_REQUEST_TEXT, 0);
}

static void deinit(void) {
	window_destroy(window);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}

#include <pebble.h>

#define VIGOR_TIME 34

//Here be the variables.
Window *window;
Layer *bg_layer;
TextLayer *text_date_layer;
TextLayer *text_time_layer;
TextLayer *text_battery_layer;
TextLayer *text_accl_layer;
Layer *hr_layer;

static void handle_battery(BatteryChargeState charge_state) {
	static char battery_text[] = "100% +";

	if (charge_state.is_charging) {
		snprintf(battery_text, sizeof(battery_text), "%d%% +", charge_state.charge_percent);
	} else {
		snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);
	}

	text_layer_set_text(text_battery_layer, battery_text);
}

static void bg_layer_update_callback(Layer *layer, GContext* ctx) {
	graphics_context_set_fill_color(ctx, GColorBlack);
	graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

static void accl_handlr(uint16_t type, AppWorkerMessage* data) {
	static char buff[40];
	snprintf(buff, sizeof(buff), "t=%d, x=%d", (*data).data0, (int) ((*data).data1));
	text_layer_set_text(text_accl_layer, buff);
}

static void hr_layer_update_callback(Layer *layer, GContext* ctx) {
	graphics_context_set_fill_color(ctx, GColorWhite);
	graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

//Here be teh tikz.
static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
	// Need to be static because they're used by the system later.
	static char time_text[] = "00:00";
	static char date_text[] = "Xxx 00";

	char *time_format;

	if (!tick_time) {
		time_t now = time(NULL);
		tick_time = localtime(&now);
	}

	// TODO: Only update the date when it's changed.
	strftime(date_text, sizeof(date_text), "%a %e", tick_time);
	text_layer_set_text(text_date_layer, date_text);


	if (clock_is_24h_style()) {
		time_format = "%H:%M";
	} else {
		time_format = "%I:%M";
	}

	strftime(time_text, sizeof(time_text), time_format, tick_time);

	// Kludge to handle lack of non-padded hour format string
	// for twelve hour clock.
	if (!clock_is_24h_style() && (time_text[0] == '0')) {
		memmove(time_text, &time_text[1], sizeof(time_text) - 1);
	}

	text_layer_set_text(text_time_layer, time_text);

	handle_battery(battery_state_service_peek());
}

// Kill teh shiz
static void handle_deinit(void) {
	battery_state_service_unsubscribe();
	tick_timer_service_unsubscribe();
	text_layer_destroy(text_time_layer);
	text_layer_destroy(text_date_layer);
	text_layer_destroy(text_battery_layer);
	layer_destroy(bg_layer);
	text_layer_destroy(text_accl_layer);
	layer_destroy(hr_layer);
	window_destroy(window);
}

// Start teh shiz.
static void handle_init(void) {
	window = window_create();
	window_stack_push(window, true /* Animated */);
	window_set_background_color(window, GColorBlack);

	Layer *window_layer = window_get_root_layer(window);

	// battery_init
	text_battery_layer = text_layer_create(GRect(1, 0, 71, 24));
	text_layer_set_text_color(text_battery_layer, GColorWhite);
	text_layer_set_background_color(text_battery_layer, GColorClear);
	text_layer_set_font(text_battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
	//Does the lefting thing
	text_layer_set_text_alignment(text_battery_layer, GTextAlignmentLeft);
	text_layer_set_text(text_battery_layer, "100% charged");

	//date_init
	text_date_layer = text_layer_create(GRect(72, 0, 71, 24));
	text_layer_set_text_color(text_date_layer, GColorWhite);
	text_layer_set_background_color(text_date_layer, GColorClear);
	text_layer_set_font(text_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
	// Does the righting thing
	text_layer_set_text_alignment(text_date_layer, GTextAlignmentRight);

	//bg_init
	bg_layer = layer_create(GRect(0,0,144,24));
	layer_set_update_proc(bg_layer, bg_layer_update_callback);

	//time_init
	text_time_layer = text_layer_create(GRect(0, 40, 144, 42));
	text_layer_set_text_color(text_time_layer, GColorWhite);
	text_layer_set_background_color(text_time_layer, GColorClear);
	text_layer_set_font(text_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
	//Does the centering thing
	text_layer_set_text_alignment(text_time_layer, GTextAlignmentCenter);

	//accl_init
	text_accl_layer = text_layer_create(GRect(0, 100, 144, 24));
	text_layer_set_text_color(text_accl_layer, GColorWhite);
	text_layer_set_background_color(text_accl_layer, GColorClear);
	text_layer_set_font(text_accl_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	//Does the centering thing
	text_layer_set_text_alignment(text_accl_layer, GTextAlignmentCenter);

	//hr_init
	hr_layer = layer_create(GRect(21, 94, 104, 2));
	layer_set_update_proc(hr_layer, hr_layer_update_callback);

	if (!app_worker_is_running())
		app_worker_launch();
	app_worker_message_subscribe(&accl_handlr);

	// Draw teh stuffz
	layer_add_child(window_layer, bg_layer);
	layer_add_child(window_layer, text_layer_get_layer(text_battery_layer));
	layer_add_child(window_layer, text_layer_get_layer(text_date_layer));
	layer_add_child(window_layer, text_layer_get_layer(text_time_layer));
	layer_add_child(window_layer, text_layer_get_layer(text_accl_layer));
	layer_add_child(window_layer, hr_layer);


	tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
	battery_state_service_subscribe(&handle_battery);
	handle_minute_tick(NULL, MINUTE_UNIT);
}


// Teh MAINZ
int main(void) {
	handle_init();

	app_event_loop();

	handle_deinit();
}

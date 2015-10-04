#include <pebble_worker.h>

#define VIGOR_TIME 34

static int32_t active_minutes = 0;

static int32_t average_accl;

static void tick_handler(struct tm *tick_timer, TimeUnits units_changed) {
	if (average_accl > 12 || average_accl < 9) {
		// Update value
		active_minutes++;
		average_accl = 10;
	} else {
		active_minutes = 0;
	}
}

static void accl_handler(AccelData *data, uint32_t num_samples) {
	// Construct a data packet
	AppWorkerMessage msg_data = {
		.data0 = active_minutes,
		.data1 = average_accl
	};

	average_accl += ((data[0].x * data[0].x) + (data[0].y * data[0].y) + (data[0].z * data[0].z)) / 100000;
	average_accl /= 2;

	// Send the data to the foreground app
	app_worker_send_message(VIGOR_TIME, &msg_data);
}

static void worker_init() {
	accel_service_set_samples_per_update(1);

	// Use the TickTimer Service as a data source
	tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
	accel_data_service_subscribe(1, &accl_handler);
	accel_service_set_sampling_rate(ACCEL_SAMPLING_10HZ);
	average_accl = 10;
}

static void worker_deinit() {
	// Stop using the TickTimerService
	tick_timer_service_unsubscribe();
	accel_data_service_unsubscribe();
}

int main(void) {
	worker_init();
	worker_event_loop();
	worker_deinit();
}

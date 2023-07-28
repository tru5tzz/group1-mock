#include "StatusIndicator.h"

#include "sl_simple_led.h"
#include "sl_simple_led_instances.h"

#include "sl_sleeptimer.h"

#include "sl_btmesh_api.h"
#include "sl_bt_api.h"

#include "app_log.h"

sl_sleeptimer_timer_handle_t led_timer;
sl_sleeptimer_timer_handle_t led_off_timer;

void led_timer_on_timeout(sl_sleeptimer_timer_handle_t *handle, void *data) {
	(void)data;
	(void)handle;

	sl_simple_led_toggle(sl_led_led0.context);
}

void led_off_timer_on_timeout(sl_sleeptimer_timer_handle_t *handle, void *data) {
	(void)handle;
	(void)data;

	app_log("Turn off LED indicator\n");

	sl_simple_led_turn_off(sl_led_led0.context);
	sl_sleeptimer_stop_timer(&led_off_timer);
	sl_sleeptimer_stop_timer(&led_timer);
}

void status_indicator_on_provisioning() {
	app_log("LED pattern: Start provisioning\n");
	sl_sleeptimer_stop_timer(&led_timer);
	sl_simple_led_turn_off(sl_led_led0.context);
	sl_sleeptimer_start_periodic_timer_ms(&led_timer,
																		500,
																		led_timer_on_timeout,
																		NULL,
																		0,
																		SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG);
}

void status_indicator_on_provisioning_success() {
	app_log("LED pattern: Prov successfully\n");
	sl_sleeptimer_stop_timer(&led_timer);
	sl_simple_led_turn_on(sl_led_led0.context);
	sl_sleeptimer_start_timer_ms(&led_timer,
															3000,
															led_timer_on_timeout,
															NULL,
															0,
															SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG);
}

void status_indicator_on_provisioning_failed() {
	app_log("LED pattern: Prov failed\n");
	sl_sleeptimer_stop_timer(&led_timer);
	sl_simple_led_turn_off(sl_led_led0.context);
	sl_sleeptimer_start_periodic_timer_ms(&led_timer,
																		100,
																		led_timer_on_timeout,
																		NULL,
																		0,
																		SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG);
	sl_sleeptimer_start_timer_ms(&led_off_timer,
														3000,
														led_off_timer_on_timeout,
														NULL,
														0,
														SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG);
}

void status_indicator_on_btmesh_event(sl_btmesh_msg_t *evt) {
	switch (SL_BT_MSG_ID(evt->header)) {
		case sl_btmesh_evt_prov_provisioning_failed_id:
			status_indicator_on_provisioning_failed();
			break;
		case sl_btmesh_evt_prov_device_provisioned_id:
			status_indicator_on_provisioning_success();
			break;
		default:
			break;
	}
}
/***************************************************************************//**
 * @file
 * @brief Core application logic.
 *******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/
#include "em_common.h"
#include "app_assert.h"
#include "sl_status.h"
#include "app.h"
#include "stdio.h"
#include "app_log.h"

#include "gatt_db.h"
#include "sl_bluetooth.h"
#include "sl_bt_api.h"
#include "sl_bt_api.h"

#include "sl_btmesh.h"
#include "sl_btmesh_api.h"
#include "sl_btmesh_api.h"
#include "sl_btmesh_factory_reset.h"
#include "sl_btmesh_provisioning_decorator.h"
#include "sl_btmesh_lighting_server_config.h"

#include "sl_simple_button_instances.h"
#include "sl_simple_timer.h"
#include "app_button_press.h"
#include"sl_pwm_instances.h"
#include "sl_btmesh_sensor_client.h"

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/
/// Length of the display name buffer
#define NAME_BUF_LEN                                     20
/// Timout for Blinking LED during provisioning
#define APP_LED_BLINKING_TIMEOUT                        250
/// Callback has not parameters
#define NO_CALLBACK_DATA                                (void *)NULL
/// Used button indexes
#define BUTTON_PRESS_BUTTON_0                           0
/// periodic timer handle
static sl_simple_timer_t app_led_blinking_timer;
// maximum duticycle
#define PWM_MAX_DUTY_CYCLE                              100
// length of unicast address
#define LENGTH_UNICAST_ADDRESS                          2
/// Timout for Blinking LED during provisioning
#define APP_LED_TURN_OFF_TIMEOUT                        6000


/*******************************************************************************
 ***************************  LOCAL VARIABLES   ********************************
 ******************************************************************************/
/// confirm provision done
static bool init_provision_done = false;
/// periodic timer handle
static sl_simple_timer_t app_led_turn_off_timer;
/// confirm led is turn off
static bool init_turn_off_done = true;

/*******************************************************************************
 ***************************  PROTOTYPE FUNCTION  ******************************
 ******************************************************************************/
static void device_init_change_uuid();
/// Handles button press and does a factory reset
static bool handle_reset_conditions(void);
/// Handles timer call back to turn off led
static void app_led_turn_off_timer_cb(sl_simple_timer_t *handle, void *data);

/*******************************************************************************
 ***************************  GLOBAL FUNCTION  ********************************
 ******************************************************************************/

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////
  app_log("Demo Light Server"APP_LOG_NL);
  // Enable button presses
  sl_pwm_start(&sl_pwm_led0);
  app_button_press_enable();
  handle_reset_conditions();
}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
SL_WEAK void app_process_action(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application code here!                              //
  // This is called infinitely.                                              //
  // Do not call blocking functions from here!                               //
  /////////////////////////////////////////////////////////////////////////////
}

/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event(struct sl_bt_msg *evt)
{
//  static uint8_t address_light[LENGTH_UNICAST_ADDRESS];
//  static uint8_t *num_th_light[2];

  switch (SL_BT_MSG_ID(evt->header)) {
    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////
   case sl_bt_evt_system_boot_id:
     if(!handle_reset_conditions())
       {
         device_init_change_uuid();
       }
     break;
   case sl_bt_evt_connection_opened_id:
          app_log("Connected" APP_LOG_NL);
          break;
   case sl_bt_evt_connection_closed_id:
         app_log("Disconnected" APP_LOG_NL);
         break;
    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

/**************************************************************************//**
 * Bluetooth Mesh stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth Mesh stack.
 *****************************************************************************/
void sl_btmesh_on_event(sl_btmesh_msg_t *evt)
{
  switch (SL_BT_MSG_ID(evt->header)) {
    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////
    // -------------------------------
    // Default event handler.
    case sl_btmesh_evt_sensor_client_status_id:
		//start timer to turn off led when sensor server send signal
        if (init_turn_off_done)
        {
            sl_simple_led_turn_on(sl_led_led0.context);
            sl_status_t sc = sl_simple_timer_start(&app_led_turn_off_timer,
                                                   APP_LED_TURN_OFF_TIMEOUT,
                                                   app_led_turn_off_timer_cb,
                                                   NO_CALLBACK_DATA,
                                                   true);
            init_turn_off_done = false;
            app_assert_status_f(sc, "Failed to stop periodic timer");
        }
        break;
    default:
      break;
  }
}

/***************************************************************************//**
 * Handles button press and does a factory reset
 *
 * @return true if there is no button press
 ******************************************************************************/
static bool handle_reset_conditions(void)
{
  bool flag = true;
  // If PB0 is held down then do full factory reset
  if (sl_simple_button_get_state(&sl_button_btn0)
      == SL_SIMPLE_BUTTON_PRESSED) {
    // Full factory reset
    sl_btmesh_initiate_full_reset();
    flag = false;
  }
  return flag;
}
/*******************************************************************************
 * Timer Callbacks
 ******************************************************************************/
/***************************************************************************//**
 * periodic timer callback
 * @param[in] handle Timer descriptor handle
 * @param[in] data Callback input arguments
 ******************************************************************************/
static void app_led_blinking_timer_cb(sl_simple_timer_t *handle, void *data)
{
  (void)data;
  (void)handle;
}

/***************************************************************************//**
* periodic timer callback turn off led when sensor sever send signal
* @param[in] handle Timer descriptor handle
* @param[in] data Callback input arguments
******************************************************************************/
static void app_led_turn_off_timer_cb(sl_simple_timer_t *handle, void *data)
{
    (void)data;
    (void)handle;
    if (!init_turn_off_done)
    {
		// turn off led and reset timer
        sl_simple_led_turn_off(sl_led_led0.context);
        init_turn_off_done = true;
    }
}

/***************************************************************************//**
* Set device name in the GATT database. A unique name is generated using
* the two last bytes from the UUID of this device.
*
* @param[in] uuid  Pointer to device UUID.
******************************************************************************/
static void set_device_name(uuid_128 *uuid)
{
    char name[NAME_BUF_LEN];
    sl_status_t result;
    // Create unique device name using the last two bytes of the device UUID
    snprintf(name,
             NAME_BUF_LEN,
             "light node %02x%02x",
             uuid->data[14],
             uuid->data[15]);
    app_log("Device name: '%s'" APP_LOG_NL, name);
    result = sl_bt_gatt_server_write_attribute_value(gattdb_device_name,
                                                     0,
                                                     strlen(name),
                                                     (uint8_t *)name);
    app_log_status_error_f(result,
                           "sl_bt_gatt_server_write_attribute_value() failed" APP_LOG_NL);
}

/***************************************************************************//**
* Callbacks
* ******************************************************************************/

/***************************************************************************//**
* Handling of provisionee init result
******************************************************************************/
void sl_btmesh_provisionee_on_init(sl_status_t result)
{
   uuid_128 uuid;
   if (SL_STATUS_OK != result) {
      app_log("Init failed (0x%lx)"APP_LOG_NL,result);
   } else {
      sl_status_t sc = sl_btmesh_node_get_uuid(&uuid);
      app_assert_status_f(sc, "Failed to get UUID");
      set_device_name(&uuid);
   }
}

/*******************************************************************************
 * Provisioning Decorator Callbacks
 ******************************************************************************/

/*******************************************************************************
 * Called when the Provisioning starts
 * @param[in] result Result code. 0: success, non-zero: error
 ******************************************************************************/
void sl_btmesh_on_node_provisioning_started(uint16_t result)
{
  app_log("Start provisioning" APP_LOG_NL);
  sl_status_t sc = sl_simple_timer_start(&app_led_blinking_timer,
                                         APP_LED_BLINKING_TIMEOUT,
                                         app_led_blinking_timer_cb,
                                         NO_CALLBACK_DATA,
                                         true);
  app_assert_status_f(sc, "Failed to start periodic timer");
  app_show_btmesh_node_provisioning_started(result);
}


/************************************************
 * Control light using PWM, generic on off lightness
 ************************************************/
void app_led_set_level(uint16_t level)
{
  //convert the lightness to pwm duty cycle
  uint16_t pwm_duty_cycle = (uint16_t)((uint32_t)level * PWM_MAX_DUTY_CYCLE
                            / SL_BTMESH_LIGHTING_SERVER_PWM_MAXIMUM_BRIGHTNESS_CFG_VAL);
  // check the duty cycle value (0->100).
  if (pwm_duty_cycle > PWM_MAX_DUTY_CYCLE) {
    pwm_duty_cycle = PWM_MAX_DUTY_CYCLE;
  }
  // set the lightness through PWM
  sl_pwm_set_duty_cycle(&sl_pwm_led0, pwm_duty_cycle);
}
void sl_btmesh_lighting_level_pwm_cb(uint16_t level)
{
  app_led_set_level(level);
}
/***************************************************************************//**
 * Button press Callbacks
 ******************************************************************************/
void app_button_press_cb(uint8_t button, uint8_t duration)
{
  (void)button;
  (void)duration;
}
static void device_init_change_uuid() {
  uuid_128 temp;
  sl_status_t retval;
  retval = sl_btmesh_node_get_uuid(&temp);
  app_assert_status_f(retval, "Getting UUID failed:\n");
  temp.data[0] = 0x02;
  temp.data[1] = 0xFF;
  temp.data[2] = 0x00;
  temp.data[3] = 0x01;

  retval = sl_btmesh_node_set_uuid(temp);
  app_assert_status_f(retval, "Setting UUID failed\n");
}

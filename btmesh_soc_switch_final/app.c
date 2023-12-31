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
#include <stdio.h>

#include "em_common.h"
#include "app_assert.h"
#include "sl_status.h"
#include "app.h"

#ifdef SL_COMPONENT_CATALOG_PRESENT
#include "sl_component_catalog.h"
#endif // SL_COMPONENT_CATALOG_PRESENT

#include "sl_btmesh_api.h"
#include "sl_bt_api.h"
#include "app_btmesh_util.h"

#include "gatt_db.h"


#ifdef SL_CATALOG_APP_LOG_PRESENT
#include "app_log.h"
#endif // SL_CATALOG_APP_LOG_PRESENT

#ifdef SL_CATALOG_BTMESH_FACTORY_RESET_PRESENT
#include "sl_btmesh_factory_reset.h"
#endif // SL_CATALOG_BTMESH_FACTORY_RESET_PRESENT

#ifdef SL_CATALOG_BTMESH_PROVISIONING_DECORATOR_PRESENT
#include "sl_btmesh_provisioning_decorator.h"
#endif // SL_CATALOG_BTMESH_PROVISIONING_DECORATOR_PRESENT

#include "sl_simple_button.h"
#include "sl_simple_button_instances.h"

#include "app_button_press.h"


#include "sl_btmesh_lighting_client.h"


#include "custom.h"

/*******************************************************************************
 * DEFINE
 ******************************************************************************/

#define NAME_BUF_LEN 30

#define BOOT_ERR_MSG_BUF_LEN 30

#define BUTTON_PRESS_BUTTON_0 0

#define MAX_PERCENT         100
/// Increase step of physical values (lightness, color temperature)
#define INCREASE             10
/// Decrease step of physical values (lightness, color temperature)
#define DECREASE           (-10)


/*******************************************************************************
 * Global variables
 ******************************************************************************/
/// number of active Bluetooth connections
static uint8_t num_connections = 0;



/*******************************************************************************
 * Global variables
 ******************************************************************************/

static uint8_t lightness_percent = 0;


/*******************************************************************************
 * Local Prototype
 ******************************************************************************/

// Set device name in the GATT database
static void set_device_name(uuid_128 *uuid);

// Handles button press and does a factory reset
static bool handle_reset_conditions(void);

static uint8_t wrap_add(int8_t a, int8_t b, uint8_t max);

// change uuid to provisioned
static void device_init_change_uuid();


/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////

  sl_simple_button_enable(&sl_button_btn0);
  sl_sleeptimer_delay_millisecond(1);
  app_button_press_enable();
  app_log("BT mesh Switch initialized" APP_LOG_NL);

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
  switch (SL_BT_MSG_ID(evt->header)) {
    case sl_bt_evt_system_boot_id:
      if(!handle_reset_conditions())
        {
          device_init_change_uuid();
        }
      break;
      case sl_bt_evt_connection_opened_id:
        num_connections++;
        //lcd_print("connected", SL_BTMESH_WSTK_LCD_ROW_CONNECTION_CFG_VAL);
        app_log("BT Connected (%d)" APP_LOG_NL, num_connections);
        break;

      case sl_bt_evt_connection_closed_id:
        if (num_connections > 0) {
          if (--num_connections == 0) {
            //lcd_print("", SL_BTMESH_WSTK_LCD_ROW_CONNECTION_CFG_VAL);
            app_log("BT Disconnected (%d)" APP_LOG_NL, num_connections);
          }
        }
        break;

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
    case sl_btmesh_evt_node_heartbeat_id:

      app_log("Heartbeat message from light node (address %d ) to address %d (%d hops)" NL, evt->data.evt_node_heartbeat.src_addr,
                                                              evt->data.evt_node_heartbeat.dst_addr,
                                                              evt->data.evt_node_heartbeat.hops);

      break;

    case sl_btmesh_evt_node_heartbeat_start_id:
      app_log("Start send Heartbeat message from light node (address %d ) to address %d (period %d s)" NL, evt->data.evt_node_heartbeat_start.src_addr,
                                                                    evt->data.evt_node_heartbeat_start.dst_addr,
                                                                    evt->data.evt_node_heartbeat_start.period_sec);

      break;
    case sl_btmesh_evt_node_heartbeat_stop_id:

      app_log("Stop send Heartbeat message from light node (address %d ) to address %d" NL, evt->data.evt_node_heartbeat_stop.src_addr,
                                                                          evt->data.evt_node_heartbeat_stop.dst_addr);
      break;

    case sl_btmesh_evt_generic_client_server_status_id:
    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}


/*******************************************************************************
 * Local Function
 ******************************************************************************/

static void set_device_name(uuid_128 *uuid) {

  char name[NAME_BUF_LEN];
  sl_status_t result;

   // Create unique device name using the last two bytes of the device UUID
   snprintf(name, NAME_BUF_LEN, "Bac's switch node %02x%02x",
            uuid->data[14], uuid->data[15]);

   app_log("Device name: '%s'" APP_LOG_NL, name);

   result = sl_bt_gatt_server_write_attribute_value(gattdb_device_name,
                                                      0,
                                                      strlen(name),
                                                      (uint8_t *)name);
   app_log_status_error_f(result,
                            "sl_bt_gatt_server_write_attribute_value() failed" APP_LOG_NL);

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


/***************************************************************************//**
 * Wrap the sum of two signed 8-bit integers to fit within a given maximum value
 *
 * @param a The first signed 8-bit integer
 * @param b The second signed 8-bit integer
 * @param max The maximum value that the sum can take
 *
 * @return The sum of `a` and `b`, wrapped around `max`
 ******************************************************************************/
static uint8_t wrap_add(int8_t a, int8_t b, uint8_t max)
{
  int16_t sum = a + b;
  while (sum > max) {
    sum -= max;
  }
  while (sum < 0 ) {
    sum += max;
  }
  return sum;
}


/***************************************************************************//**
 * Handles button press and does a factory reset
 *
 * @return true if there is no button press
 ******************************************************************************/
static bool handle_reset_conditions(void)
{
#ifdef SL_CATALOG_BTMESH_FACTORY_RESET_PRESENT
  // If PB0 is held down then do full factory reset
  if (sl_simple_button_get_state(&sl_button_btn0)
      == SL_SIMPLE_BUTTON_PRESSED) {
    // Disable button presses
    app_button_press_disable();
    // Full factory reset
    sl_btmesh_initiate_full_reset();
    return false;
  }

#endif // SL_CATALOG_BTMESH_FACTORY_RESET_PRESENT
  return true;
}


/*******************************************************************************
 * Callbacks
 ******************************************************************************/

void sl_btmesh_provisionee_on_init(sl_status_t result)
{
  if ( SL_STATUS_OK != result ) {
      char buf[BOOT_ERR_MSG_BUF_LEN];
      snprintf(buf, sizeof(buf), "init failed (0x%lx)", result);

  } else {

      uuid_128 uuid;
      sl_status_t sc = sl_btmesh_node_get_uuid(&uuid);
      app_assert_status_f(sc, "Failed to get UUID");
      set_device_name(&uuid);
  }
}



/***************************************************************************//**
 * Button press Callbacks
 ******************************************************************************/
void app_button_press_cb(uint8_t button, uint8_t duration)
{
#if SL_SIMPLE_BUTTON_COUNT == 1
  if (button == BUTTON_PRESS_BUTTON_0) {
//    sl_btmesh_change_switch_position(SL_BTMESH_LIGHTING_CLIENT_OFF);
    switch (duration) {
      // Handling of button press less than 0.25s
      case APP_BUTTON_PRESS_DURATION_SHORT: {
        lightness_percent = wrap_add(lightness_percent, DECREASE, MAX_PERCENT);
        //app_log("Decrease brightness 10%%" APP_LOG_NL);
        sl_btmesh_set_lightness(lightness_percent);
      } break;
      // Handling of button press greater than 0.25s and less than 1s
      case APP_BUTTON_PRESS_DURATION_MEDIUM: {
        lightness_percent = wrap_add(lightness_percent, INCREASE, MAX_PERCENT);
        //app_log("Increase brightness 10%%" APP_LOG_NL);
        sl_btmesh_set_lightness(lightness_percent);
      } break;
      // Handling of button press greater than 1s and less than 5s
      case APP_BUTTON_PRESS_DURATION_LONG: {
        app_log("Toggle Led" APP_LOG_NL);
        if ( lightness_percent != 0 ) lightness_percent = 0;
        else lightness_percent = 100;
        sl_btmesh_set_lightness(lightness_percent);
      } break;
      // Handling of button press greater than 5s
//      case APP_BUTTON_PRESS_DURATION_VERYLONG: {
//        sl_btmesh_select_scene(1);
//      } break;
      default:
        break;
    }
  }
#endif
}


/*******************************************************************************
 * Provisioning Decorator Callbacks
 ******************************************************************************/
// Called when the Provisioning starts
void sl_btmesh_on_node_provisioning_started(uint16_t result)
{

#if defined(SL_CATALOG_BTMESH_WSTK_LCD_PRESENT) || defined(SL_CATALOG_APP_LOG_PRESENT)
  app_show_btmesh_node_provisioning_started(result);
#else
  (void)result;
#endif // defined(SL_CATALOG_BTMESH_WSTK_LCD_PRESENT) || defined(SL_CATALOG_APP_LOG_PRESENT)
}

// Called when the Provisioning finishes successfully
void sl_btmesh_on_node_provisioned(uint16_t address,
                                   uint32_t iv_index)
{

#if defined(SL_CATALOG_BTMESH_WSTK_LCD_PRESENT) || defined(SL_CATALOG_APP_LOG_PRESENT)
  app_show_btmesh_node_provisioned(address, iv_index);
#else
  (void)address;
  (void)iv_index;
#endif // defined(SL_CATALOG_BTMESH_WSTK_LCD_PRESENT) || defined(SL_CATALOG_APP_LOG_PRESENT)
}

///***************************************************************************//**
// * Timer Callbacks
// ******************************************************************************/
//static void app_led_blinking_timer_cb(app_timer_t *handle, void *data)
//{
//  (void)data;
//  (void)handle;
//}

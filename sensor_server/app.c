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
#include "sl_btmesh.h"
#include "sl_bluetooth.h"
#include "stdio.h"
#include "stdbool.h"

#include "gatt_db.h"

#include "sl_btmesh_api.h"
#include "sl_bt_api.h"

#ifdef SL_CATALOG_APP_LOG_PRESENT
#include "app_log.h"
#endif // SL_CATALOG_APP_LOG_PRESENT

#include "sl_simple_button_instances.h"
#include "sl_simple_led_instances.h"
#include "sl_simple_timer.h"
#include "sl_btmesh_factory_reset.h"
#include "sl_btmesh_provisioning_decorator.h"
#include "app_button_press.h"
#include "sl_btmesh_sensor_people_count.h"
#include "sl_btmesh_sensor_server.h"
#include "sl_btmesh_lpn.h"

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/
/// Length of the display name buffer
#define NAME_BUF_LEN                            20
/// Timout for Blinking LED during provisioning
#define APP_LED_BLINKING_TIMEOUT                250
/// Callback has not parameters
#define NO_CALLBACK_DATA                        (void *)NULL
/// Used button indexes
#define BUTTON_PRESS_BUTTON_0                   0
/// timeout for periodic people count
#define PEOPLE_COUNT_TIMEOUT                    500

/*******************************************************************************
 ***************************  LOCAL VARIABLES   ********************************
 ******************************************************************************/
/// periodic timer handle
static sl_simple_timer_t app_led_blinking_timer;
/// confirm provision done
static bool init_provision_done = false;

/// Handles button press and does a factory reset
static bool handle_reset_conditions(void);
/// periodic timer handle
static sl_simple_timer_t app_people_count_timer;
/// check motion appear
static bool motion_appear = true;
/// event public
static sl_btmesh_msg_t evt_motion_appear;

/*******************************************************************************
***************************  GLOBAL FUNCTION   ********************************
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

    // Enable button presses
    app_button_press_enable();
    handle_reset_conditions();
    sl_btmesh_node_set_proxy_service_adv_interval(160, 200);
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
    switch (SL_BT_MSG_ID(evt->header))
    {
        ///////////////////////////////////////////////////////////////////////////
        // Add additional event handlers here as your application requires!      //
        ///////////////////////////////////////////////////////////////////////////

    case sl_bt_evt_connection_opened_id:
        break;

    case sl_bt_evt_connection_closed_id:
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
    sl_btmesh_msg_t evt_enable_lpn;

    switch (SL_BT_MSG_ID(evt->header))
    {
    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////
    ///
        case sl_btmesh_evt_node_config_set_id:
            // enable lpn to find node friend
            evt_enable_lpn.header= sl_btmesh_evt_proxy_disconnected_id;
            sl_btmesh_lpn_on_event(&evt_enable_lpn);
            break;
    // -------------------------------
    // Default event handler.
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
    if (sl_simple_button_get_state(&sl_button_btn0) == SL_SIMPLE_BUTTON_PRESSED)
    {
        // Full factory reset
        sl_btmesh_initiate_full_reset();
        flag = false;
    }

    return flag;
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
             "sensor client %02x%02x",
             uuid->data[14],
             uuid->data[15]);

    result = sl_bt_gatt_server_write_attribute_value(gattdb_device_name,
                                                     0,
                                                     strlen(name),
                                                     (uint8_t *)name);
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
    if (!init_provision_done)
    {
        // Toggle LEDs
        sl_simple_led_toggle(sl_led_led0.context);
    }
}

/***************************************************************************//**
*
* Turn off signal demo motion sensor
* @param[in] handle Timer descriptor handle
* @param[in] data Callback input arguments
******************************************************************************/

static void app_people_count_timer_cb(sl_simple_timer_t *handle,
                                      void *data)
{
    (void)data;
    (void)handle;
    sl_status_t sc;
    // reset people count and stop timer
    sl_btmesh_people_count_decrease();
    motion_appear = true;
    sc = sl_simple_timer_stop(&app_people_count_timer);
    app_assert_status_f(sc, "Failed to stop periodic timer");
}

/*******************************************************************************
 * Callback for button press
 * @param button button ID. Either of
 *                   - BUTTON_PRESS_BUTTON_0
 *                   - BUTTON_PRESS_BUTTON_1
 * @param duration duration of button press. Either of
 *                   - APP_BUTTON_PRESS_NONE
 *                   - APP_BUTTON_PRESS_DURATION_SHORT
 *                   - APP_BUTTON_PRESS_DURATION_MEDIUM
 *                   - APP_BUTTON_PRESS_DURATION_LONG
 *                   - APP_BUTTON_PRESS_DURATION_VERYLONG
 ******************************************************************************/
void app_button_press_cb(uint8_t button, uint8_t duration)
{
    sl_status_t sc;
    evt_motion_appear.header = sl_btmesh_evt_sensor_server_publish_id;
    (void)duration;
    // button pressed
    if (duration == APP_BUTTON_PRESS_DURATION_SHORT)
    {
        if (button == BUTTON_PRESS_BUTTON_0)
        {
            // check if sensor signal reset
            if (motion_appear)
            {
                motion_appear = false;
                // increase people count
                sl_btmesh_people_count_increase();
                // start timer
                sc = sl_simple_timer_start(&app_people_count_timer,
                                           PEOPLE_COUNT_TIMEOUT,
                                           app_people_count_timer_cb,
                                           NO_CALLBACK_DATA,
                                           true);

                app_assert_status_f(sc, "Failed to start periodic timer");
            }
            // public signal to group
            sl_btmesh_handle_sensor_server_events(&evt_motion_appear);
        }
    }
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

    if (SL_STATUS_OK != result)
    {
    }
    else
    {
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
    // Change buttons to LEDs in case of shared pin
    sl_simple_led_turn_on(sl_led_led0.context);

    sl_status_t sc = sl_simple_timer_start(&app_led_blinking_timer,
                                           APP_LED_BLINKING_TIMEOUT,
                                           app_led_blinking_timer_cb,
                                           NO_CALLBACK_DATA,
                                           true);

    app_assert_status_f(sc, "Failed to start periodic timer");

}

/*******************************************************************************
 * Called when the Provisioning finishes successfully
 * @param[in] address      Unicast address of the primary element of the node.
 *                         Ignored if unprovisioned.
 * @param[in] iv_index     IV index for the first network of the node
 *                         Ignored if unprovisioned.
 ******************************************************************************/
void sl_btmesh_on_node_provisioned(uint16_t address, uint32_t iv_index)
{
    sl_status_t sc = sl_simple_timer_stop(&app_led_blinking_timer);
    app_assert_status_f(sc, "Failed to stop periodic timer");
    // Turn off LED
    init_provision_done = true;
    sl_simple_led_turn_off(sl_led_led0.context);
}

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

#include "gatt_db.h"

#include "sl_btmesh_api.h"
#include "sl_bt_api.h"
#include "app_log.h"
#include "sl_simple_button_instances.h"
#include "sl_simple_led_instances.h"
#include "sl_simple_timer.h"
#include "app_button_press.h"
#include "sl_btmesh_factory_reset.h"
#include "sl_btmesh_provisioning_decorator.h"

#include "sl_btmesh_lighting_client.h"
#include "sl_btmesh_model_specification_defs.h"
#include "sl_btmesh_dcd.h"
#include "sl_btmesh_generic_model_capi_types.h"
#include "sl_btmesh_lib.h"

#include "app_out_log.h"
#include "gateway_define.h"
#include "sl_btmesh_set_uuid.h"

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

/*******************************************************************************
 ***************************  LOCAL VARIABLES   ********************************
 ******************************************************************************/

/// periodic timer handle
static sl_simple_timer_t app_led_blinking_timer;
static sl_simple_timer_t app_update_devices_timer;
/// confirm provision done
static bool init_provision_done = false;
/// array capacity address device in mesh
static uint16_t buf_unicast_light_group[NUM_MAX_DEVICE];
static uint16_t buf_unicast_switch_group[NUM_MAX_DEVICE];
static uint16_t buf_unicast_sensor_group[NUM_MAX_DEVICE];
/// count num device found
static uint8_t count_unicast_light;
static uint8_t count_unicast_switch;
static uint8_t count_unicast_sensor;
/// app key index
static uint8_t app_key_index;
///enable print console menu
static bool enable_console_menu;
/// save uuid device
uint16_t address_node_device;

/*******************************************************************************
 **************************  PROTOTYPE FUNCTIONS   *****************************
 ******************************************************************************/
/// Handles button press and does a factory reset
static bool handle_reset_conditions(void);
/// show console menu
void app_usart_menu_light(void);

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
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

    app_log("--------- GATE WAY ---------" APP_LOG_NL);
    // Enable button presses
    app_button_press_enable();
    handle_reset_conditions();
    enable_console_menu = false;
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
    static bool check_charater = false;
    static uint8_t cmd_usart_light;

    // Show menu
    if (enable_console_menu)
    {
        app_log("\n\n\n--------------Console menu GATEWAY-----------------\n");
        app_log("-press p: print list unicast address of light,"
            "switch, sensor in mesh network\n");
        app_log("-press c: choose led to control\n");
        // print one
        enable_console_menu = false;
        check_charater = true;
        // wrtie attribute value to GATT
        sl_bt_gatt_server_write_attribute_value(gattdb_number_light,
                                                0,
                                                sizeof(count_unicast_light),
                                                (uint8_t *)&count_unicast_light);
    }

    // check cmd to control menu
    if ((cmd_usart_light > 0) && (check_charater == true) && (cmd_usart_light != '\n') && (cmd_usart_light != '\r'))
    {
        switch (cmd_usart_light)
        {
        case 'c':
        case 'C':
            app_log("Choose light according number th in list. Press 'e' to exit\n");
            app_usart_menu_light();
            enable_console_menu = true;
            break;
        case 'p':
        case 'P':
            app_log("Unicast address devices light in mesh: \n");
            app_print_device(buf_unicast_light_group);
            app_log("Unicast address devices switch in mesh: \n");
            app_print_device(buf_unicast_switch_group);
            app_log("Unicast address devices sensor in mesh: \n");
            app_print_device(buf_unicast_sensor_group);
            enable_console_menu = true;
            break;
        case USART_GET_CHAR_NULL:
            break;
        default:
            app_log("Please press correct character\n");
            break;
        }
    }
    /// get character enter from console
    cmd_usart_light = getchar();
}

/***************************************************************************//**
* BLE function control light
* @param[in] data pointer point to array
*            data[1]: address led
*            data[2]: state of light 
******************************************************************************/
void app_ble_control_light(uint8_t *data)
{
    static uint8_t number_th_led;
    static uint8_t set_state_led;
    static sl_status_t sc;
    struct mesh_generic_request req;

    number_th_led = data[0];
    set_state_led = data[1];
    app_log("Led choose have uinicast adress 0x%x.\n",
            buf_unicast_light_group[number_th_led - 1]);
    // check state light enter from console
    if (set_state_led != '\r' && set_state_led != '\n' && set_state_led != USART_GET_CHAR_NULL)
    {
        switch (set_state_led)
        {
        case 1:
            app_log("Turn on led\n");
            req.kind = mesh_generic_request_on_off;
            req.on_off = MESH_GENERIC_ON_POWER_UP_STATE_ON;
            /// set state light by address
            sc = mesh_lib_generic_client_set(MESH_GENERIC_ON_OFF_CLIENT_MODEL_ID,
                                             ELEMENT_INDEX_ROOT,
                                             buf_unicast_light_group[number_th_led - 1],
                                             app_key_index,
                                             0,
                                             &req,
                                             0,
                                             0,
                                             0);
            app_assert_status_f(sc, "Failed to set light");
            break;
        case 0:
            app_log("Turn off led\n");
            req.kind = mesh_generic_request_on_off;
            req.on_off = MESH_GENERIC_ON_POWER_UP_STATE_OFF;
            /// set state light by address
            sc = mesh_lib_generic_client_set(MESH_GENERIC_ON_OFF_CLIENT_MODEL_ID,
                                            ELEMENT_INDEX_ROOT,
                                            buf_unicast_light_group[number_th_led - 1],
                                            app_key_index,
                                            0,
                                            &req,
                                            0,
                                            0,
                                            0);
            app_assert_status_f(sc, "Failed to set light");
            break;
        default:
            app_log("Press incorrect character\n");
            app_log("Press 1: turn on led. Press 0: turn off led.Press 'e' to exit\n");
            break;
        }
    }
}

/***************************************************************************//**
* USART function control state light
* @param[in] character position address light in list
******************************************************************************/
void app_usart_control_light(uint8_t character)
{
    uint8_t number_th_led;
    uint8_t set_state_led;
    sl_status_t sc;
    struct mesh_generic_request req;

    //transfer character to digit
    number_th_led = character - CHAR_TO_DIGIT;
    app_log("Led choose have uinicast adress 0x%x.\n",
            buf_unicast_light_group[number_th_led - 1]);
    app_log("Press 1: turn on led. Press 0: turn off led.Press 'e' to exit\n");
    do
    {
        // take charater enter from console 
        set_state_led = getchar();
        // check charater
        if (set_state_led != '\r' && set_state_led != '\n' && set_state_led != USART_GET_CHAR_NULL)
        {
            switch (set_state_led)
            {
            case '1':
                app_log("Turn on led\n");
                req.kind = mesh_generic_request_on_off;
                req.on_off = MESH_GENERIC_ON_POWER_UP_STATE_ON;
                sc = mesh_lib_generic_client_set(MESH_GENERIC_ON_OFF_CLIENT_MODEL_ID,
                                                ELEMENT_INDEX_ROOT,
                                                buf_unicast_light_group[number_th_led - 1],
                                                app_key_index,
                                                0,
                                                &req,
                                                0,
                                                0,
                                                0);
                if (sc == SL_STATUS_OK)
                {
                    set_state_led = 'e';
                }
                else
                {
                    app_assert_status_f(sc, "Failed to set light");
                }
                break;
            case '0':
                app_log("Turn off led\n");
                req.kind = mesh_generic_request_on_off;
                req.on_off = MESH_GENERIC_ON_POWER_UP_STATE_OFF;
                   sc = mesh_lib_generic_client_set(MESH_GENERIC_ON_OFF_CLIENT_MODEL_ID,
                                                    ELEMENT_INDEX_ROOT,
                                                    buf_unicast_light_group[number_th_led - 1],
                                                    app_key_index,
                                                    0,
                                                    &req,
                                                    0,
                                                    0,
                                                    0);
                if (sc == SL_STATUS_OK)
                {
                    set_state_led = 'e';
                }
                else
                {
                    app_assert_status_f(sc, "Failed to set light");
                }
                break;
            case 'e':
                break;
            default:
                app_log("Press incorrect character\n");
                app_log("Press 1: turn on led. Press 0: turn off led.Press 'e' to exit\n");
                break;
            }
        }
    } while (set_state_led != 'e');

}

/***************************************************************************//**
* USART function choose light
******************************************************************************/
void app_usart_menu_light(void)
{
    uint8_t number_th_led;
    uint8_t set_light;

    do
    {
        // take charater enter from console
        set_light = getchar();
        if (set_light > 0)
        {
            // check charater enter from console
            if (set_light != '\r' && set_light != '\n' && set_light != USART_GET_CHAR_NULL && set_light != 'e')
            {
                // tranfer char to digit
                number_th_led = set_light - CHAR_TO_DIGIT;
                if ((number_th_led > 0) && (number_th_led <= NUM_MAX_DEVICE))
                {
                    if (buf_unicast_light_group[number_th_led - 1] != 0)
                    {
                        app_usart_control_light(set_light);
                        set_light = 'e';
                    }
                    else
                    {
                        app_log("Number greater than devices exist in list\n");
                    }
                }
                else
                {
                    app_log("Enter correct numbers. Number in around 1 to 10\n");
                }
            }
        }
    } while (set_light != 'e');
}

/***************************************************************************//**
* Update adress devices after the specified timeout
* @param[in] handle Timer descriptor handle
* @param[in] data Callback input arguments
******************************************************************************/
static void app_update_devices_timer_cb(sl_simple_timer_t *handle,
                                        void *data)
{
    (void)data;
    (void)handle;
    sl_status_t sc;
    static uint16_t tick = 0;

    if(tick <=TIMEOUT_SCAN_DEVICE){
        // find light 
        sc = mesh_lib_generic_client_get(MESH_GENERIC_ON_OFF_CLIENT_MODEL_ID,
                                        ELEMENT_INDEX_ROOT,
                                        0xFFFF,
                                        app_key_index,
                                        mesh_generic_state_on_off);
        if (sc != SL_STATUS_OK)
        {
            /* Something went wrong */
            app_log("sl_btmesh_generic_client_get: failed 0x%.2lx\r\n", sc);
        }
        else
        {
            app_log("Success,sl_btmesh_generic_client_get\n");
        }
    }
    // increase tick and check timeout to stop find address
    tick = tick + DEVICE_REGISTER_SHORT_TIMEOUT;
    if ((count_unicast_light == NUM_MAX_DEVICE) || (tick == TIMEOUT_SCAN_DEVICE))
    {
        tick = 0;
        sl_simple_timer_stop(&app_update_devices_timer);
        app_log("Stop gateway scanning \n");
        enable_console_menu = true;
    }
}

/***************************************************************************//**
* Button press Callbacks
* @param[in] button     button is pressed
* @param[in] duration   time press button
******************************************************************************/
void app_button_press_cb(uint8_t button, uint8_t duration)
{
    (void)button;
    uint8_t index;

    // Selecting action by duration
    switch (duration)
    {
    case APP_BUTTON_PRESS_DURATION_SHORT:
        enable_console_menu = false;
        app_log("Start find unicast address light,switch,sensor" APP_LOG_NL);
        // reset list address
        count_unicast_light = 0;
        count_unicast_sensor=0;
        count_unicast_switch=0;
        for (index = 0; index < NUM_MAX_DEVICE; index++)
        {
            buf_unicast_light_group[index] = 0;
            buf_unicast_switch_group[index] = 0;
            buf_unicast_sensor_group[index] = 0;
        }
        // start timer to scan address
        sl_status_t sc =
            sl_simple_timer_start(&app_update_devices_timer,
                                  DEVICE_REGISTER_SHORT_TIMEOUT,
                                  app_update_devices_timer_cb,
                                  NO_CALLBACK_DATA,
                                  true);
        app_assert_status_f(sc, "Failed to start timer");
        break;
    default:
        break;
    }
}

/**************************************************************************//**
* Bluetooth stack event handler.
* This overrides the dummy weak implementation.
*
* @param[in] evt Event coming from the Bluetooth stack.
*****************************************************************************/
void sl_bt_on_event(struct sl_bt_msg *evt)
{
    static uint8_t address_light[LENGTH_UNICAST_ADDRESS];
    static uint8_t *num_th_light = 0;
    static uint8_t *state;
    static uint8_t set_state_light[2];

    switch (SL_BT_MSG_ID(evt->header))
    {
        ///////////////////////////////////////////////////////////////////////////
        // Add additional event handlers here as your application requires!      //
        ///////////////////////////////////////////////////////////////////////////
    case sl_bt_evt_system_boot_id:
        if(!handle_reset_conditions())
        {
            sl_btmesh_set_my_uuid();
        }
        break;
    case sl_bt_evt_connection_opened_id:
        app_log("Connected" APP_LOG_NL);
        break;

    case sl_bt_evt_connection_closed_id:
        app_log("Disconnected" APP_LOG_NL);
        break;
    case sl_bt_evt_gatt_server_attribute_value_id:
        // check attribute
        if (gattdb_connect_light == evt->data.evt_gatt_server_attribute_value.attribute)
        {
            // take position light want to connect
            num_th_light = (uint8_t *)(evt->data.evt_gatt_server_attribute_value.value.data);
            // check position exists
            if ((*num_th_light > 0) && (*num_th_light <= NUM_MAX_DEVICE))
            {
                // check light exist
                if (buf_unicast_light_group[*num_th_light - 1] != 0)
                {
                    set_state_light[0] = *num_th_light;
                    address_light[0] = VALUE_MSB(buf_unicast_light_group[*num_th_light - 1]);
                    address_light[1] = buf_unicast_light_group[*num_th_light - 1];
                    sl_bt_gatt_server_write_attribute_value(gattdb_light_connected,
                                                            0,
                                                            sizeof(address_light),
                                                            (uint8_t *)&address_light);
                    // send notifycation to gatt light connected success
                    sl_bt_gatt_server_notify_all(gattdb_light_connected,
                                                 sizeof(address_light),
                                                 (uint8_t *)&address_light);
                }
                else
                {
                    set_state_light[0] = 0;
                    address_light[0] = 0;
                    address_light[1] = 0;
                    sl_bt_gatt_server_write_attribute_value(gattdb_light_connected,
                                                            0,
                                                            sizeof(address_light),
                                                            (uint8_t *)&address_light);
                    // send notifycation to gatt light connected failed
                    sl_bt_gatt_server_notify_all(gattdb_light_connected,
                                                 sizeof(address_light),
                                                 (uint8_t *)&address_light);
                }
            }
        }
        if (gattdb_set_light == evt->data.evt_gatt_server_attribute_value.attribute)
        {
            // take state set in gatt
            state = (uint8_t *)(evt->data.evt_gatt_server_attribute_value.value.data);

            set_state_light[1] = *state;
            if ((set_state_light[0] > 0) && (set_state_light[0] <= NUM_MAX_DEVICE))
            {
                if (buf_unicast_light_group[set_state_light[0] - 1] != 0)
                {
                    app_ble_control_light(set_state_light);
                }
            }
        }
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
    uint16_t light_address;
    uint16_t switch_address;
    uint16_t sensor_address;
    uint8_t index;
    bool check_is_address_exits = false;

    switch (SL_BT_MSG_ID(evt->header))
    {
    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////
    ///
    case sl_btmesh_evt_node_initialized_id:
        if (evt->data.evt_node_initialized.provisioned)
        {
            app_log("Press PB0 short to start find unicast address light,switch,sensor\n");
        }
        break;
    case sl_btmesh_evt_generic_client_server_status_id:
        // take address light
        light_address = evt->data.evt_generic_client_server_status.server_address;
        // Save to buff light address
        if ((count_unicast_light == 0)&&(light_address!=address_node_device))
        {
            buf_unicast_light_group[0] = light_address;
            app_log("Found an unicast address of server light: 0x%x\n", light_address);
            count_unicast_light++;
        }
        else
        {
            // check address is exist in list
            for (index = 0; index < NUM_MAX_DEVICE; index++)
            {
                if (light_address == buf_unicast_light_group[index])
                {
                    check_is_address_exits = true;
                    break;
                }
            }
            if (!check_is_address_exits)
            {
                for (index = 1; index < NUM_MAX_DEVICE; index++)
                {
                    if ((buf_unicast_light_group[index] == 0)&&
                        (light_address!=address_node_device))
                    {
                        buf_unicast_light_group[index] = light_address;
                        app_log("Found an address of server light: 0x%x\n",light_address);
                        count_unicast_light++;
                        check_is_address_exits = false;
                        break;
                    }
                }
            }
        }
        break;
    case sl_btmesh_evt_node_model_config_changed_id:
        app_log("Press PB0 short to start find unicast address light,switch,sensor\n");
        break;
    case sl_btmesh_evt_generic_server_client_request_id:

        app_log("\n-------------------------------------\n");
        app_log("Switch pressed in group %x\n",
                evt->data.evt_generic_server_client_request.server_address);
        
        // take address switch
        switch_address = evt->data.evt_generic_server_client_request.client_address;
        // save address to buff switch
        if (count_unicast_switch == 0)
        {
            buf_unicast_switch_group[0] = switch_address;
            app_log("Found an unicast address of switch: 0x%x\n", switch_address);
            count_unicast_switch++;
        }
        else
        {
            // check address is exist in list
            for (index = 0; index < NUM_MAX_DEVICE; index++)
            {
                if (switch_address == buf_unicast_switch_group[index])
                {
                    check_is_address_exits = true;
                    break;
                }
            }
            if (!check_is_address_exits)
            {
                for (index = 1; index < NUM_MAX_DEVICE; index++)
                {
                    if (buf_unicast_switch_group[index] == 0)
                    {
                        buf_unicast_switch_group[index] = switch_address;
                        app_log("Found an unicast address of switch: 0x%x\n", switch_address);
                        count_unicast_switch++;
                        check_is_address_exits = false;
                        break;
                    }
                }
            }
        }
        app_log("\n-------------------------------------\n");
        break;
    case sl_btmesh_evt_sensor_client_status_id:
        app_log("\n-------------------------------------\n");
        app_log("Sensor in group %x have signal\n",
                evt->data.evt_sensor_client_status.client_address);
        
        // take address switch
        sensor_address = evt->data.evt_generic_server_client_request.client_address;
        // save address to buff switch
        if (count_unicast_sensor == 0)
        {
            buf_unicast_sensor_group[0] = sensor_address;
            app_log("Found an unicast address of sensor: 0x%x\n", sensor_address);
            count_unicast_sensor++;
        }
        else
        {
            // check address is exist in list
            for (index = 0; index < NUM_MAX_DEVICE; index++)
            {
                if (sensor_address == buf_unicast_sensor_group[index])
                {
                    check_is_address_exits = true;
                    break;
                }
            }
            if (!check_is_address_exits)
            {
                for (index = 1; index < NUM_MAX_DEVICE; index++)
                {
                    if (buf_unicast_sensor_group[index] == 0)
                    {
                        buf_unicast_sensor_group[index] = sensor_address;
                        app_log("Found an unicast address of sensor: 0x%x\n", sensor_address);
                        count_unicast_sensor++;
                        check_is_address_exits = false;
                        break;
                    }
                }
            }
        }
        app_log("\n-------------------------------------\n");
        break;
    case sl_btmesh_evt_node_key_added_id:
        // save app_key index
        app_key_index = evt->data.evt_node_key_added.index;
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
             "Gate way %02x%02x",
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

    if (SL_STATUS_OK != result)
    {
        app_log("Init failed (0x%lx)" APP_LOG_NL, result);
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
    app_log("Start provisioning" APP_LOG_NL);
    sl_simple_led_turn_on(sl_led_led0.context);

    sl_status_t sc = sl_simple_timer_start(&app_led_blinking_timer,
                                           APP_LED_BLINKING_TIMEOUT,
                                           app_led_blinking_timer_cb,
                                           NO_CALLBACK_DATA,
                                           true);

    app_assert_status_f(sc, "Failed to start periodic timer");

    app_show_btmesh_node_provisioning_started(result);
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
    // save address node
    app_show_btmesh_node_provisioned(address, iv_index);
    sl_simple_led_turn_off(sl_led_led0.context);
}

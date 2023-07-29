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
#include "sl_btmesh_factory_reset.h"
#include "sl_btmesh_provisioning_decorator.h"
#include "app_button_press.h"

#include "sl_btmesh_lighting_client.h"
#include "sl_btmesh_model_specification_defs.h"
#include "sl_btmesh_dcd.h"
#include "sl_btmesh_generic_model_capi_types.h"
#include "sl_btmesh_lib.h"

/// timeout for registering new devices after startup
#define DEVICE_REGISTER_SHORT_TIMEOUT  500
/// timeout for registering new devices after startup
#define DEVICE_REGISTER_LONG_TIMEOUT   10000
/// Length of the display name buffer
#define NAME_BUF_LEN                   20
/// Timout for Blinking LED during provisioning
#define APP_LED_BLINKING_TIMEOUT       250
/// Callback has not parameters
#define NO_CALLBACK_DATA               (void *)NULL
/// timeout for periodic sensor data update
#define SENSOR_DATA_TIMEOUT            2000
/// Used button indexes
#define BUTTON_PRESS_BUTTON_0          0

#define NUM_LIGHTNESS_SERVER           10
#define TIMEOUT_SCAN_LIGHT_SERVER      10000
#define USART_GET_CHAR_NULL            255
#define CHAR_TO_DIGIT                  48
#define LENGTH_UNICAST_ADDRESS         2
#define LENGTH_SET_STATE_LIGHT         2
#define VALUE_MSB(x)                   (x>>8)
#define ADDRESS_GENERAL                0xFFFF
#define ELEMENT_INDEX                  0

/// periodic timer handle
static sl_simple_timer_t app_led_blinking_timer;
static sl_simple_timer_t app_update_devices_timer;

/// confirm provision done
static bool init_provision_done = false;

/// Handles button press and does a factory reset
static bool handle_reset_conditions(void);

static uint16_t buf_unicast_light_client_group[NUM_LIGHTNESS_SERVER];
static uint16_t buf_unicast_switch_group[NUM_LIGHTNESS_SERVER];
static uint16_t buf_unicast_sensor_group[NUM_LIGHTNESS_SERVER];

static uint8_t count_unicast_light_client;
static uint8_t count_unicast_switch;
static uint8_t count_unicast_sensor;

static bool enable_console_menu;
static uint8_t cmd_usart_light= '0';
static uint8_t app_key_index;

/* Input buffer */
static uint8_t set_light;

void app_usart_menu_light(void);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////

  app_log("--------- GATE WAY ---------"APP_LOG_NL);
  // Enable button presses
  app_button_press_enable();
  handle_reset_conditions();
  sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM2);
  enable_console_menu=false;

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
  uint8_t index;
  static bool check_charater=false;

  if(enable_console_menu){
      app_log("\n\n\n--------------Console menu GATEWAY-----------------\n");
      app_log("-press p: print list unicast address of light in mesh network\n");
      app_log("-press c: choose led to control\n");
      enable_console_menu=false;
      check_charater = true;

      sl_bt_gatt_server_write_attribute_value(gattdb_number_light,
                                                     0,
                                                     sizeof(count_unicast_light_client),
                                                     (uint8_t*)&count_unicast_light_client);
  }
  if ((cmd_usart_light > 0) && (check_charater==true)
      &&(cmd_usart_light !='\n') &&(cmd_usart_light!='\r')){
      switch(cmd_usart_light){
        case 'c':
        case 'C':
          app_log("Choose light according number th in list. Press 'e' to exit\n");
          app_usart_menu_light();
          enable_console_menu=true;
          break;
        case 'p':
        case 'P':
          app_log("Unicast address devices light in mesh: \n");
          for(index=0;index<NUM_LIGHTNESS_SERVER;index++){
              if(buf_unicast_light_client_group[index]!=0){
                  app_log("%d: 0x%x\n",index+1,buf_unicast_light_client_group[index]);
              }
          }
          app_log(APP_LOG_NL);
          cmd_usart_light='0';
          enable_console_menu=true;
          break;
        case '0':
          break;
        case USART_GET_CHAR_NULL:
          break;
        default:
          app_log("Please press correct character\n");
          break;
      }
  }
  cmd_usart_light=getchar();
}

/***************************************************************************//**
 * Example BLE function.
 ******************************************************************************/
void app_ble_control_light(uint8_t* data){
  static uint8_t number_th_led;
  static uint8_t set_state_led;
  static sl_status_t sc;
  struct mesh_generic_request req;

  number_th_led=data[0];
  set_state_led=data[1];
  app_log("Led choose have uinicast adress 0x%x.\n",
          buf_unicast_light_client_group[number_th_led-1]);

  if (set_state_led != '\r' && set_state_led != '\n'
      && set_state_led!= USART_GET_CHAR_NULL){
      switch (set_state_led){
        case 1:
          app_log("Turn on led\n");
          req.kind =mesh_generic_request_on_off;
          req.on_off =MESH_GENERIC_ON_POWER_UP_STATE_ON;
          sc = mesh_lib_generic_client_set(MESH_GENERIC_ON_OFF_CLIENT_MODEL_ID,
                                           0,
                                           buf_unicast_light_client_group[number_th_led-1],
                                           0,
                                           0,
                                           &req,
                                           0,
                                           0,
                                           0) ;
              app_assert_status_f(sc, "Failed to set light");
              break;
        case 0:
            app_log("Turn off led\n");
              req.kind =mesh_generic_request_on_off;
              req.on_off =MESH_GENERIC_ON_POWER_UP_STATE_OFF;
              sc = mesh_lib_generic_client_set(MESH_GENERIC_ON_OFF_CLIENT_MODEL_ID,
                                               0,
                                               buf_unicast_light_client_group[number_th_led-1],
                                               0,
                                               0,
                                               &req,
                                               0,
                                               0,
                                               0) ;
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
 * Example USART function.
 ******************************************************************************/
void app_usart_control_light(uint8_t character){
  uint8_t number_th_led;
  uint8_t set_state_led;
  sl_status_t sc;
  struct mesh_generic_request req;

  number_th_led=character-CHAR_TO_DIGIT;
  app_log("Led choose have uinicast adress 0x%x.\n",
          buf_unicast_light_client_group[number_th_led-1]);
  app_log("Press 1: turn on led. Press 0: turn off led.Press 'e' to exit\n");
  do{
      set_state_led=getchar();
      if (set_state_led != '\r' && set_state_led != '\n'
          && set_state_led!= USART_GET_CHAR_NULL){
          switch (set_state_led){
            case '1':
              app_log("Turn on led\n");
              req.kind =mesh_generic_request_on_off;
              req.on_off =MESH_GENERIC_ON_POWER_UP_STATE_ON;
              sc = mesh_lib_generic_client_set(MESH_GENERIC_ON_OFF_CLIENT_MODEL_ID,
                                               0,
                                               buf_unicast_light_client_group[number_th_led-1],
                                               0,
                                               0,
                                               &req,
                                               0,
                                               0,
                                               0) ;
              app_assert_status_f(sc, "Failed to set light");
              if(sc==SL_STATUS_OK){
                  set_state_led='e';
              }
              break;
            case '0':
              app_log("Turn off led\n");
              req.kind =mesh_generic_request_on_off;
              req.on_off =MESH_GENERIC_ON_POWER_UP_STATE_OFF;
              sc = mesh_lib_generic_client_set(MESH_GENERIC_ON_OFF_CLIENT_MODEL_ID,
                                               0,
                                               buf_unicast_light_client_group[number_th_led-1],
                                               0,
                                               0,
                                               &req,
                                               0,
                                               0,
                                               0) ;
              app_assert_status_f(sc, "Failed to set light");
              if(sc==SL_STATUS_OK){
                  set_state_led='e';
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
  }while(set_state_led!='e');
}

void app_usart_menu_light(void)
{
  uint8_t number_th_led;
  /* Retrieve characters, print local echo and full line back */
  do{
      set_light = getchar();
      if (set_light > 0) {
          if (set_light != '\r' && set_light != '\n'
              && set_light!=USART_GET_CHAR_NULL && set_light!='e') {
              number_th_led=set_light-48;
              if((number_th_led > 0) && (number_th_led<=NUM_LIGHTNESS_SERVER)){
                  if(buf_unicast_light_client_group[number_th_led-1]!=0){
                      app_usart_control_light(set_light);
                      set_light='e';
                  }
                  else{
                      app_log("Number greater than devices exist in list\n");
                  }
              }
              else{
                  app_log("Enter correct numbers. Number in around 1 to 10\n");
              }
          }
      }
  }while(set_light!='e');
}

/***************************************************************************//**
 * Update registered devices after the specified timeout
 *
 * @param[in] timeout_ms Timer timeout, in milliseconds.
 ******************************************************************************/
static void  app_update_devices_timer_cb(sl_simple_timer_t *handle,
                                     void *data)
{
  (void)data;
  (void)handle;
  sl_status_t sc;
  struct mesh_generic_request req;
  static uint16_t tick=0;

  req.kind =mesh_generic_request_on_off;
  req.on_off =MESH_GENERIC_ON_OFF_STATE_OFF;

  sc = mesh_lib_generic_client_publish(MESH_GENERIC_ON_OFF_CLIENT_MODEL_ID,
                                    BTMESH_LIGHTING_CLIENT_MAIN,
                                    0,
                                    &req,
                                    0, // transition time in ms
                                    0,
                                    3   // flags
                                    );



  sc =mesh_lib_generic_client_get(MESH_GENERIC_ON_OFF_CLIENT_MODEL_ID,
                                  ELEMENT_INDEX,
                                  ADDRESS_GENERAL,
                                  app_key_index,
                                  mesh_generic_state_on_off);



  sc =mesh_lib_generic_server_publish(MESH_GENERIC_ON_OFF_SERVER_MODEL_ID,
                                      ELEMENT_INDEX,
                                      mesh_generic_state_on_off);


  sc = sl_btmesh_sensor_client_get(0,
                                   ELEMENT_INDEX,
                                   app_key_index,
                                   0,
                                   0);

  if(sc != SL_STATUS_OK) {
            /* Something went wrong */
            app_log("sl_btmesh_node_set_uuid: failed 0x%.2lx\r\n", sc);
        } else {
            app_log("Success,sl_btmesh_node_set_uuid\n");
        }

  tick = tick + DEVICE_REGISTER_SHORT_TIMEOUT;
  if((count_unicast_light_client==NUM_LIGHTNESS_SERVER)||(tick==TIMEOUT_SCAN_LIGHT_SERVER)){
      tick=0;
      sl_simple_timer_stop(& app_update_devices_timer);
      app_log("Stop gateway scanning \n");
      enable_console_menu=true;
  }

}

/***************************************************************************//**
 * Button press Callbacks
 ******************************************************************************/
void app_button_press_cb(uint8_t button, uint8_t duration)
{
  uint8_t index;

  (void)button;
  // Selecting action by duration
  switch (duration) {
      case APP_BUTTON_PRESS_DURATION_SHORT:
          enable_console_menu = false;
          app_log("Start find unicast address light client"APP_LOG_NL);
          count_unicast_light_client=0;
          for(index=0;index<NUM_LIGHTNESS_SERVER;index++){
              buf_unicast_light_client_group[index]=0;
          }
          sl_status_t sc =
             sl_simple_timer_start(& app_update_devices_timer,
                                   DEVICE_REGISTER_SHORT_TIMEOUT,
                                   app_update_devices_timer_cb,
                                   NO_CALLBACK_DATA,
                                   true);
          app_assert_status_f(sc, "Failed to start timer");
          break;
    case APP_BUTTON_PRESS_DURATION_MEDIUM:
        app_log("PB0 press medium\n");
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
  static uint8_t *num_th_light=0;
  static uint8_t *state;
  static uint8_t set_state_light[2];

  switch (SL_BT_MSG_ID(evt->header)) {
    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////

    case sl_bt_evt_connection_opened_id:
          app_log("Connected" APP_LOG_NL);
          break;

   case sl_bt_evt_connection_closed_id:
         app_log("Disconnected" APP_LOG_NL);
         break;
   case sl_bt_evt_gatt_server_attribute_value_id:
     if(gattdb_connect_light == evt->data.evt_gatt_server_attribute_value.attribute){
         num_th_light=(uint8_t *)(evt->data.evt_gatt_server_attribute_value.value.data);
         if((*num_th_light > 0) && (*num_th_light<=NUM_LIGHTNESS_SERVER)){
             set_state_light[0]=*num_th_light;
             if(buf_unicast_light_client_group[*num_th_light-1]!=0){
                 address_light[0]=VALUE_MSB(buf_unicast_light_client_group[*num_th_light-1]);
                 address_light[1]=buf_unicast_light_client_group[*num_th_light-1];
                 sl_bt_gatt_server_write_attribute_value(gattdb_light_connected  ,
                                                         0,
                                                         sizeof(address_light),
                                                         (uint8_t*)&address_light);
                 // send notifycation
                 sl_bt_gatt_server_notify_all(gattdb_light_connected  ,
                                              sizeof(address_light),
                                              (uint8_t*)&address_light);
             }
             else{
                 set_state_light[0]=0;
                 address_light[0]=0;
                 address_light[1]=0;
                 sl_bt_gatt_server_write_attribute_value(gattdb_light_connected,
                                                         0,
                                                         sizeof(address_light),
                                                         (uint8_t*)&address_light);
                 // send notifycation
                 sl_bt_gatt_server_notify_all(gattdb_light_connected,
                                              sizeof(address_light),
                                              (uint8_t*)&address_light);
             }
         }
     }
     if(gattdb_set_light == evt->data.evt_gatt_server_attribute_value.attribute){
         state=(uint8_t *)(evt->data.evt_gatt_server_attribute_value.value.data);
         set_state_light[1]=*state;
         if((set_state_light[0] > 0) && (set_state_light[0]<=NUM_LIGHTNESS_SERVER)){
             if(buf_unicast_light_client_group[set_state_light[0]-1]!=0){
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
  uint16_t server_address_light;
  uint16_t switch_address;
  uint8_t index;
  bool check_is_address_exits = false;

  app_log("evt_btmesh: %lx\n",evt->header);

  switch (SL_BT_MSG_ID(evt->header)) {
    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////
    ///
    case sl_btmesh_evt_node_initialized_id:
        if (evt->data.evt_node_initialized.provisioned) {
            app_log("Press PB0 short to start find unicast address\n");
        }
        break;
    case sl_btmesh_evt_generic_client_server_status_id:
        server_address_light=evt->data.evt_generic_client_server_status.server_address;
        if(count_unicast_light_client==0){
            buf_unicast_light_client_group[0]=server_address_light;
            app_log("Found an unicast address of server light: 0x%x\n",server_address_light);
            count_unicast_light_client++;
        }
        else{
            for(index=0;index<NUM_LIGHTNESS_SERVER;index++){
                if(server_address_light==buf_unicast_light_client_group[index]){
                    check_is_address_exits=true;
                    break;
                }
            }
            if(!check_is_address_exits){
                for(index=1;index<NUM_LIGHTNESS_SERVER;index++){
                    if(buf_unicast_light_client_group[index]==0){
                        buf_unicast_light_client_group[index]=server_address_light;
                        app_log("Found an address of server light: 0x%x\n",server_address_light);
                        count_unicast_light_client++;
                        check_is_address_exits=false;
                        break;
                    }
                }
            }
        }
        break;
    case sl_btmesh_evt_node_model_config_changed_id:
        app_log("Press PB0 short to start find unicast address\n");
        break;
    case sl_btmesh_evt_generic_server_client_request_id:
        app_log("\n-------------------------------------\n");
        app_log("Switch pressed in group %x\n",
                  evt->data.evt_generic_server_client_request.server_address);
        switch_address=evt->data.evt_generic_server_client_request.client_address;
        if(count_unicast_switch==0){
            buf_unicast_switch_group[0]=switch_address;
            app_log("Found an unicast address of switch: 0x%x\n",switch_address);
            count_unicast_switch++;
        }
        else{
            for(index=0;index<NUM_LIGHTNESS_SERVER;index++){
                if(switch_address==buf_unicast_switch_group[index]){
                    check_is_address_exits=true;
                    break;
                }
            }
            if(!check_is_address_exits){
                for(index=1;index<NUM_LIGHTNESS_SERVER;index++){
                    if(buf_unicast_switch_group[index]==0){
                        buf_unicast_switch_group[index]=switch_address;
                        app_log("Found an unicast address of switch: 0x%x\n",switch_address);
                        count_unicast_switch++;
                        check_is_address_exits=false;
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
      app_log("\n-------------------------------------\n");
      break;
    case sl_btmesh_evt_node_key_added_id:
      app_key_index= evt->data.evt_node_key_added.index;
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
  if (!init_provision_done) {
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

  app_show_btmesh_node_provisioned(address, iv_index);
  sl_simple_led_turn_off(sl_led_led0.context);
}

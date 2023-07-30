/***************************************************************************//**
 * @file  sl_btmesh_set_uuid.c
 * @brief function set uuid for device.
 ******************************************************************************/
#include "em_common.h"
#include "app_assert.h"
#include "sl_status.h"
#include "sl_btmesh.h"
#include "sl_bluetooth.h"
#include "stdio.h"
#include "sl_btmesh_api.h"
#include "app_log.h"

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/
#define LENGTH_COMPANY_ID       6
#define LENGTH_UUID             16
#define CHAR_MIN                0
#define CHAR_MAX                255

/*******************************************************************************
 *******************************   LOCAL VARIABLES   ***************************
 ******************************************************************************/
static  uuid_128 my_uuid_device;
static  uint8_t company_uuid[LENGTH_COMPANY_ID]={0,0,2,255,0,2};

/*******************************************************************************
 *******************************   GLOBAL FUNCTION   ***************************
 ******************************************************************************/


/**************************************************************************//**
 *Function set uuid for device
 *
 * @param[in] evt Event coming from the Bluetooth Mesh stack.
 *****************************************************************************/
void sl_btmesh_set_my_uuid(sl_bt_msg_t *evt)
{
  sl_status_t sc;
  uint8_t index;

  switch (SL_BT_MSG_ID(evt->header)) {
      case sl_bt_evt_system_boot_id:
        // get uuid device
        sc = sl_btmesh_node_get_uuid(&my_uuid_device);
        if(sc != SL_STATUS_OK)
        {
            /* Something went wrong */
            app_log("sl_btmesh_node_get_uuid: failed 0x%.2lx\r\n", sc);
        }
        else
        {
            // set uuid for device
            app_log("Success,sl_btmesh_node_get_uuid\n");
            for(index=0;index<LENGTH_UUID;index++)
            {
                if(index<LENGTH_COMPANY_ID)
                {
                    my_uuid_device.data[index]=company_uuid[index];
                }
                else
                {
                    my_uuid_device.data[index]= CHAR_MIN + rand() % (CHAR_MAX-CHAR_MIN+1);
                }

            }
        }
        sc = sl_btmesh_node_set_uuid(my_uuid_device);
        if(sc != SL_STATUS_OK) {
            /* Something went wrong */
            app_log("sl_btmesh_node_set_uuid: failed 0x%.2lx\r\n", sc);
        }
        else
        {
            app_log("Success,sl_btmesh_node_set_uuid\n");
        }
        break;
      default:
        break;
  }
}

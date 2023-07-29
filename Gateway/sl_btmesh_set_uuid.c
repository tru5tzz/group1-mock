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
#include "sl_btmesh.h"
#include "sl_bluetooth.h"
#include "stdio.h"
#include "sl_btmesh_api.h"
#include "app_log.h"

#define LENGTH_COMPANY_ID       6
#define LENGTH_UUID             16
#define CHAR_MIN                0
#define CHAR_MAX                255

static  uuid_128 my_uuid_device;
static  uint8_t company_uuid[LENGTH_COMPANY_ID]={0,0,2,255,0,1};

void sl_btmesh_set_my_uuid(sl_bt_msg_t *evt)
{
  sl_status_t sc;
  uint8_t index;

  switch (SL_BT_MSG_ID(evt->header)) {
      case sl_bt_evt_system_boot_id:
        sc = sl_btmesh_node_get_uuid(&my_uuid_device);
        if(sc != SL_STATUS_OK) {
            /* Something went wrong */
            app_log("sl_btmesh_node_get_uuid: failed 0x%.2lx\r\n", sc);
        } else {
            app_log("Success,sl_btmesh_node_get_uuid\n");
            for(index=0;index<LENGTH_UUID;index++){
                if(index<LENGTH_COMPANY_ID){
                    my_uuid_device.data[index]=company_uuid[index];
                }
                else{
                    my_uuid_device.data[index]= CHAR_MIN + rand() % (CHAR_MAX-CHAR_MIN+1);
                }

            }
        }
        sc = sl_btmesh_node_set_uuid(my_uuid_device);
        if(sc != SL_STATUS_OK) {
            /* Something went wrong */
            app_log("sl_btmesh_node_set_uuid: failed 0x%.2lx\r\n", sc);
        } else {
            app_log("Success,sl_btmesh_node_set_uuid\n");
        }
        break;
      default:
        break;
  }
}

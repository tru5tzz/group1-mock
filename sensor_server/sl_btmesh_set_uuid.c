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

/*******************************************************************************
 *******************************   LOCAL VARIABLES   ***************************
 ******************************************************************************/

/*******************************************************************************
 *******************************   GLOBAL FUNCTION   ***************************
 ******************************************************************************/


/**************************************************************************//**
 *Function set uuid for device
 *
 * @param[in] evt Event coming from the Bluetooth Mesh stack.
 *****************************************************************************/
void sl_btmesh_set_my_uuid(void)
{
    sl_status_t sc;
    static  uuid_128 my_uuid_device;

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
        my_uuid_device.data[0] = 0x02;
        my_uuid_device.data[1] = 0xFF;
        my_uuid_device.data[2] = 0x00;
        my_uuid_device.data[3] = 0x01;
    }
    sc = sl_btmesh_node_set_uuid(my_uuid_device);
    if(sc != SL_STATUS_OK)
    {
        /* Something went wrong */
        app_log("sl_btmesh_node_set_uuid: failed 0x%.2lx\r\n", sc);
    }
    else
    {
        app_log("Success,sl_btmesh_node_set_uuid\n");
    }
}

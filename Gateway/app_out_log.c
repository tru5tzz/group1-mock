#include <stdbool.h>
#include "em_common.h"
#include "sl_status.h"

#include "app.h"
#include "app_log.h"

#include "sl_btmesh_api.h"
#include "sl_btmesh_factory_reset.h"
#include "sl_simple_led_instances.h"
#include "sl_simple_timer.h"
#include "app_assert.h"
#include "gateway_define.h"

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

// -----------------------------------------------------------------------------
// Console menu control light
/*******************************************************************************
 * Print address device in list
 *
 * @param[in] buff_dev pointer point to array capacity address device  
 *
 ******************************************************************************/
void app_print_device(uint16_t *buff_dev)
{
    uint8_t index;

    for (index = 0; index <NUM_MAX_DEVICE; index++)
    {
        if (buff_dev[index] != 0)
        {
            app_log("%d: 0x%x\n", index + 1,buff_dev[index]);
        }
    }
    app_log(APP_LOG_NL);

}

// -----------------------------------------------------------------------------
// Factory Reset Callbacks

/*******************************************************************************
 * Called when full reset is established, before system reset
 ******************************************************************************/
void sl_btmesh_factory_reset_on_full_reset(void)
{
    app_log("Factory reset" APP_LOG_NL);
}

// -----------------------------------------------------------------------------
// Provisioning Decorator Callbacks

/*******************************************************************************
 *  Called from sl_btmesh_on_node_provisioning_started callback in app.c
 ******************************************************************************/
void app_show_btmesh_node_provisioning_started(uint16_t result)
{
    app_log("BT mesh node provisioning is started (result: 0x%04x)" APP_LOG_NL,
            result);
    (void)result;
}

/*******************************************************************************
 *  Called from sl_btmesh_on_node_provisioned callback in app.c
 ******************************************************************************/
void app_show_btmesh_node_provisioned(uint16_t address,
                                      uint32_t iv_index)
{
    app_log("BT mesh node is provisioned (address: 0x%04x, iv_index: 0x%lx)" APP_LOG_NL,
            address,
            iv_index);
    (void)address;
    (void)iv_index;
}

/*******************************************************************************
 *  Called at node initialization time to provide provisioning information
 ******************************************************************************/
void sl_btmesh_on_provision_init_status(bool provisioned,
                                        uint16_t address,
                                        uint32_t iv_index)
{
    if (provisioned)
    {
        app_show_btmesh_node_provisioned(address, iv_index);
    }
    else
    {
        app_log("BT mesh node is unprovisioned, started unprovisioned beaconing..." APP_LOG_NL);
    }
}

/*******************************************************************************
 * Called when the Provisioning fails
 *
 ******************************************************************************/
void sl_btmesh_on_node_provisioning_failed(uint16_t result)
{
    app_log("BT mesh node provisioning failed (result: 0x%04x)" APP_LOG_NL, result);
    (void)result;
}

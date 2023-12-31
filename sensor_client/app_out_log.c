#include <stdbool.h>
#include "em_common.h"
#include "sl_status.h"

#include "app.h"
#include "app_log.h"

#include "sl_btmesh_api.h"

#include "sl_btmesh_factory_reset.h"
#include "sl_btmesh_sensor_client.h"
#include "sl_simple_led_instances.h"
#include "sl_simple_timer.h"
#include "app_assert.h"

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

/*******************************************************************************
 ***************************  GLOBAL FUNCTION  ********************************
 ******************************************************************************/
// -----------------------------------------------------------------------------
// BT mesh Friend Node Callbacks

/*******************************************************************************
 * Called when the Friend Node establishes friendship with another node.
 *
 * @param[in] netkey_index Index of the network key used in friendship
 * @param[in] lpn_address Low Power Node address
 *
 ******************************************************************************/
void sl_btmesh_friend_on_friendship_established(uint16_t netkey_index,
                                                uint16_t lpn_address)
{
    app_log("BT mesh Friendship established with LPN "
            "(netkey idx: %u, lpn addr: 0x%04x)" APP_LOG_NL,
            netkey_index,
            lpn_address);
    (void)netkey_index;
    (void)lpn_address;
}

/*******************************************************************************
 * Called when the friendship that was successfully established with a Low Power
 * Node has been terminated.
 *
 * @param[in] netkey_index Index of the network key used in friendship
 * @param[in] lpn_address Low Power Node address
 * @param[in] reason Reason for friendship termination
 *
 ******************************************************************************/
void sl_btmesh_friend_on_friendship_terminated(uint16_t netkey_index,
                                               uint16_t lpn_address,
                                               uint16_t reason)
{
    app_log("BT mesh Friendship terminated with LPN "
            "(netkey idx: %d, lpn addr: 0x%04x, reason: 0x%04x)" APP_LOG_NL,
            netkey_index,
            lpn_address,
            reason);
    (void)netkey_index;
    (void)lpn_address;
    (void)reason;
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

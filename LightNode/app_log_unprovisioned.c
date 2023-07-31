/*
 * app_log_unprovisioned.c
 *
 *  Created on: Jul 28, 2023
 *      Author: Duke  Chan
 */


#include <stdbool.h>
#include "em_common.h"
#include "sl_status.h"
#include "app.h"
#include "app_log.h"
#include "sl_btmesh_api.h"
#include "sl_btmesh_factory_reset.h"
#include "sl_simple_timer.h"
#include "app_assert.h"
/// Integer part of temperature
#define INT_TEMP(x) (x / 2)
/// Fractional part of temperature
#define FRAC_TEMP(x) ((x * 5) % 10)
/// Timout for Blinking LED during provisioning
#define APP_LED_TURN_OFF_TIMEOUT        10000
/// Callback has not parameters
#define NO_CALLBACK_DATA               (void *)NULL

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
  if (provisioned) {
    app_show_btmesh_node_provisioned(address, iv_index);
  } else {
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

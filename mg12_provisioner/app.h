#ifndef APP_H
#define APP_H

/* System header */
#include <stdio.h>

/***************************************************************************//**
 * Application Init.
 ******************************************************************************/
void app_init(void);

/***************************************************************************//**
 * Application Process Action.
 ******************************************************************************/
void app_process_action(void);

/***************************************************************************//**
 * Shows the provisioning start information
 ******************************************************************************/
void app_show_btmesh_node_provisioning_started(uint16_t result);

/***************************************************************************//**
 * Shows the provisioning completed information
 ******************************************************************************/
void app_show_btmesh_node_provisioned(uint16_t address,
                                      uint32_t iv_index);

/* Initialize the stack */
void initBLEMeshStack_app(void);

/* BLE stack device state machine */
void bkgndBLEMeshStack_app(void);

/* provision list */
void provisionBLEMeshStack_app();

#endif // APP_H
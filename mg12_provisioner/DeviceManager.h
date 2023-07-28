#ifndef __DEVICE_MAN__
#define __DEVICE_MAN__

#include "sl_btmesh_api.h"

#define DEVICE_MANAGER_SUCCESS 0
#define DEVICE_MANAGER_ERROR 1
#define DEVICE_MANAGER_TABLE_FLOW 2
#define DEVICE_MANAGER_DEVICE_NOT_FOUND 3
#define DEVICE_MANAGER_DEVICE_PRESENTED 4

/**
 * @brief Init the device manager instance to default value
 *
 */
void device_manager_init();

/**
 * @brief Add new device to the current device table
 *
 * @param devUUID The 128-bit UUDI of the device
 * @param devAddr The 6-byte bluetooth address of the device
 * @return int Status code defined above
 */
uint8_t device_manager_add_device(uuid_128 *devUUID, bd_addr *devAddr);

/**
 * @brief Remove a device from the table.
 * @details This actually just set the entry invalid so that
 *          it could be overwritten by new entry in the future
 *
 * @param devAddr The 128-bit UUID of the device
 * @return int Status code defined above
 */
uint8_t device_manager_remove_device(const bd_addr *devAddr);

/**
 * @brief Get the next item in the device table
 *
 * @param [out] id Buffer to hold the UUID of the next device
 * @param [out] add Buffer to hold the BLE address of the next device
 * @return uint8_t Status code defined above
 */
uint8_t device_manager_get_next_device(uuid_128 *id, bd_addr *add);

/**
 * @brief Get the number of current device in the list
 *
 * @param [out] count Buffer to hold the value
 * @return uint8_t Status code defined above
 */
uint8_t device_manager_get_device_count(uint8_t *count);

/**
 * @brief Print out the list of the beaconing device within the same family
 * 
 */
void device_manager_print_list(void);
#endif  // __DEVICE_MAN__
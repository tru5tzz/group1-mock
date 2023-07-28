#include "DeviceManager.h"

#include <string.h>

#include "app_log.h"
#include "sl_btmesh_api.h"

#define MAX_PRESENT_DEVICE_NUMBER 24

#define DEVICE_FAMILY_CODE 0x02FF

typedef struct device_entry {
  uuid_128 uuid;
  bd_addr add;
  int lifetime;
  uint8_t same_family;
} device_entry_t;

typedef struct device_manager {
  uint8_t present_devices;
  device_entry_t device_table[MAX_PRESENT_DEVICE_NUMBER];
} device_manager_t;

static device_manager_t manager_instance;

void device_manager_init() {
  manager_instance.present_devices = 0;
  memset(&manager_instance.device_table[0], 0,
         sizeof(device_entry_t) * MAX_PRESENT_DEVICE_NUMBER);
}

/**
 * @brief Query if the device is already present in the list or not.
 *
 * @param devAddr
 *
 * @return If the device is present, return its current index in the table. If
 * not, return 0
 */
uint8_t __device_present(const bd_addr *devAddr) {
  for (int i = 0; i < MAX_PRESENT_DEVICE_NUMBER; i++) {
    if ((devAddr->addr[0] == manager_instance.device_table[i].add.addr[0]) &&
        (devAddr->addr[1] == manager_instance.device_table[i].add.addr[1]) &&
        (devAddr->addr[2] == manager_instance.device_table[i].add.addr[2]) &&
        (devAddr->addr[3] == manager_instance.device_table[i].add.addr[3]) &&
        (devAddr->addr[4] == manager_instance.device_table[i].add.addr[4]) &&
        (devAddr->addr[5] == manager_instance.device_table[i].add.addr[5]) &&
        (manager_instance.device_table[i].lifetime > 0)) {
      return i + 1;
    }
  }

  return 0;
}

uint8_t device_manager_add_device(uuid_128 *devUUID, bd_addr *devAddr) {
  uint8_t retval = __device_present(devAddr);
  if (retval > 0) {
    return DEVICE_MANAGER_DEVICE_PRESENTED;
  }

  for (int i = 0; i < MAX_PRESENT_DEVICE_NUMBER; i++) {
    if (manager_instance.device_table[i].lifetime < 1) {
      memcpy(&manager_instance.device_table[i].uuid, devUUID, sizeof(uuid_128));
      memcpy(&manager_instance.device_table[i].add, devAddr, sizeof(bd_addr));
      manager_instance.present_devices++;
      manager_instance.device_table[i].lifetime = 1;

      if (devUUID->data[0] == 0x02 && devUUID->data[1] == 0xFF) {
        manager_instance.device_table[i].same_family = 1;
      }
      app_log("Add sucessfully, ble address of %x:%x:%x:%x:%x:%x\n",
              manager_instance.device_table[i].add.addr[5],
              manager_instance.device_table[i].add.addr[4],
              manager_instance.device_table[i].add.addr[3],
              manager_instance.device_table[i].add.addr[2],
              manager_instance.device_table[i].add.addr[1],
              manager_instance.device_table[i].add.addr[0]);
      return DEVICE_MANAGER_SUCCESS;
    }
  }

  return DEVICE_MANAGER_TABLE_FLOW;
}

uint8_t device_manager_remove_device(const bd_addr *devAddr) {
  int device_index = __device_present(devAddr);
  if (device_index > 0) {
    device_index -= 1;
    manager_instance.device_table[device_index].lifetime = 0;
    manager_instance.present_devices--;
    return DEVICE_MANAGER_SUCCESS;
  } else {
    return DEVICE_MANAGER_DEVICE_NOT_FOUND;
  }
}

uint8_t device_manager_get_next_device(uuid_128 *id, bd_addr *add) {
  for (uint8_t i = 0; i < MAX_PRESENT_DEVICE_NUMBER; i++) {
    if (manager_instance.device_table[i].lifetime > 0 &&
        manager_instance.device_table[i].same_family == 1) {
      *id = manager_instance.device_table[i].uuid;
      *add = manager_instance.device_table[i].add;
      return DEVICE_MANAGER_SUCCESS;
    }
  }

  return DEVICE_MANAGER_DEVICE_NOT_FOUND;
}

uint8_t device_manager_get_device_count(uint8_t *count) {
  *count = manager_instance.present_devices;

  return DEVICE_MANAGER_SUCCESS;
}

void device_manager_print_list(void) {
  uint8_t present_devices = manager_instance.present_devices;
  app_log("Devices available for provisioning:\n");
  for (uint8_t i = 0; i < MAX_PRESENT_DEVICE_NUMBER; i++) {
    if (manager_instance.device_table[i].lifetime > 0 &&
        manager_instance.device_table[i].same_family > 0) {
      app_log("BLE Address: %x:%x:%x\n",
              manager_instance.device_table[i].add.addr[5],
              manager_instance.device_table[i].add.addr[4],
              manager_instance.device_table[i].add.addr[3]);
    }
  }
}
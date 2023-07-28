#ifndef CONFIG_H
#define CONFIG_H

#include "sl_btmesh_api.h"

// #include "bg_types.h"

// max number of SIG models in the DCD
#define MAX_SIG_MODELS 25

// max number of vendor models in the DCD
#define MAX_VENDOR_MODELS 4

// The max number of elements each device should support
#define MAX_ELEMS_PER_DEV 3

#define TARGET_DEVICE_TYPE_NODE 0x01
#define TARGET_DEVICE_TYPE_GATEWAY 0x02

typedef struct {
  uint16_t model_id;
  uint16_t vendor_id;
} tsModel;

/* struct for storing the content of one element in the DCD */
typedef struct {
  uint16_t SIG_models[MAX_SIG_MODELS];
  uint8_t numSIGModels;

  tsModel vendor_models[MAX_VENDOR_MODELS];
  uint8_t numVendorModels;
} tsDCD_ElemContent;

/* this struct is used to help decoding the raw DCD data */
typedef struct {
  uint16_t companyID;
  uint16_t productID;
  uint16_t version;
  uint16_t replayCap;
  uint16_t featureBitmask;
  uint8_t payload[1];
} tsDCD_Header;

/* this struct is used to help decoding the raw DCD data */
typedef struct {
  uint16_t location;
  uint8_t numSIGModels;
  uint8_t numVendorModels;
  uint8_t payload[1];
} tsDCD_Elem;

void DCD_decode(void);

void DCD_decode_element(tsDCD_Elem *pElem, tsDCD_ElemContent *pDest);

uint8_t device_configuration_config_session(uint16_t target,
                                            uint16_t target_group,
                                            uint8_t device_type);

void device_config_handle_mesh_evt(sl_btmesh_msg_t *evt);

/**
 * @brief This is the prototype for the callback fucntion
 * User should self-define it.
 * This function will be called when the configuration press complete
 * successfully
 *
 */
void device_config_configuration_on_success_callback();

// Defines to avoid old code confict with elements table change.
#define _sDCD_Prim _sDCD_Table[0]
#define _sDCD_2nd _sDCD_Table[1]

#endif

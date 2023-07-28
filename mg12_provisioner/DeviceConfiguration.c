/* System header */
#include <stdio.h>
#include <string.h>

#include "app_log.h"

/* This will be the model agregator: config and load model */
#include "DeviceConfiguration.h"
#include "NetworkConfiguration.h"
#include "sl_bluetooth.h"
#include "sl_bt_api.h"
#include "sl_btmesh.h"
#include "sl_btmesh_api.h"

// Hold to maximum of 5 elements
tsDCD_ElemContent _sDCD_Table[MAX_ELEMS_PER_DEV] = {0};

// DCD content of the last provisioned device. (the example code decodes up to
// two elements, but only the primary element is used in the configuration to
// simplify the code) tsDCD_ElemContent _sDCD_Prim; tsDCD_ElemContent _sDCD_2nd;
// /* second DCD element is decoded if present, but not used for anything (just
// informative) */

uint8_t _dcd_raw[256];  // raw content of the DCD received from remote node
uint8_t _dcd_raw_len;

/**
 * NOTE This function currently can only decode DCD data of maximun 2 elements.
 * If there are more than 2, it will discard all the exceedings.
 *
 */
void DCD_decode(void) {
  tsDCD_Header *pHeader;
  tsDCD_Elem *pElem;
  uint8_t byte_offset;

  pHeader = (tsDCD_Header *)&_dcd_raw;

  app_log("DCD: company ID %4.4x, Product ID %4.4x\r\n", pHeader->companyID,
          pHeader->productID);

  pElem = (tsDCD_Elem *)pHeader->payload;

  // decode primary element:
  DCD_decode_element(pElem, &_sDCD_Prim);

  // check if DCD has more than one element by calculating where we are
  // currently at the raw DCD array and compare against the total size of the
  // raw DCD:
  byte_offset = 10 + 4 + pElem->numSIGModels * 2 +
                pElem->numVendorModels *
                    4;  // +10 for DCD header, +4 for header in the DCD element

  // TODO: Modify this code to make this function works even when there are more
  // than 2 elements
  if (byte_offset < _dcd_raw_len) {
    // set elem pointer to the beginning of 2nd element:
    pElem = (tsDCD_Elem *)&(_dcd_raw[byte_offset]);

    app_log(
        "Decoding 2nd element (just informative, not used for anything)\r\n");
    DCD_decode_element(pElem, &_sDCD_2nd);
  }
}

/* function for decoding one element inside the DCD. Parameters:
 *  pElem: pointer to the beginning of element in the raw DCD data
 *  pDest: pointer to a struct where the decoded values are written
 * */
void DCD_decode_element(tsDCD_Elem *pElem, tsDCD_ElemContent *pDest) {
  uint16_t *pu16;
  int i;

  memset(pDest, 0, sizeof(*pDest));

  pDest->numSIGModels = pElem->numSIGModels;
  pDest->numVendorModels = pElem->numVendorModels;

  app_log("Num sig models: %d\r\n", pDest->numSIGModels);
  app_log("Num vendor models: %d\r\n", pDest->numVendorModels);

  if (pDest->numSIGModels > MAX_SIG_MODELS) {
    app_log(
        "ERROR: number of SIG models in DCD exceeds MAX_SIG_MODELS (%u) "
        "limit!\r\n",
        MAX_SIG_MODELS);
    return;
  }
  if (pDest->numVendorModels > MAX_VENDOR_MODELS) {
    app_log(
        "ERROR: number of VENDOR models in DCD exceeds MAX_VENDOR_MODELS (%u) "
        "limit!\r\n",
        MAX_VENDOR_MODELS);
    return;
  }

  // set pointer to the first model:
  pu16 = (uint16_t *)pElem->payload;

  // grab the SIG models from the DCD data
  for (i = 0; i < pDest->numSIGModels; i++) {
    pDest->SIG_models[i] = *pu16;
    pu16++;
    app_log("model ID: %4.4x\r\n", pDest->SIG_models[i]);
  }

  // grab the vendor models from the DCD data
  for (i = 0; i < pDest->numVendorModels; i++) {
    pDest->vendor_models[i].vendor_id = *pu16;
    pu16++;
    pDest->vendor_models[i].model_id = *pu16;
    pu16++;

    app_log("vendor ID: %4.4x, model ID: %4.4x\r\n",
            pDest->vendor_models[i].vendor_id,
            pDest->vendor_models[i].model_id);
  }
}

// ANCHOR - Configuration section

/**
 * NOTE What this module needs when configure model in the target device?
 * Network ID (get from the network header)
 * Appkey Index (get from the network header)
 * Target Address (set by provisioner in the provisioning process) (send from
 * the main program) DCD of the target (this module will get this later) Pub &
 * sub address for each model (get from the network header)
 */
static uint16_t target_device_address = 0;
static uint16_t target_group_address = 0;
static uint8_t target_device_type = TARGET_DEVICE_TYPE_NODE;

// Always start with element 0
static uint8_t element_index = 0;

/**
 * @brief This function will initialize the variable needed for configuring the
 * target device then start it
 *
 * @param [in] target The network address of the target device
 * @return uint8_t
 */
uint8_t device_configuration_config_session(uint16_t target_device,
                                            uint16_t target_group,
                                            uint8_t device_type) {
  target_device_address = target_device;
  target_group_address = target_group;
  target_device_type = device_type;
  app_log("The target address is %2x\n", target_device_address);

  return sl_btmesh_config_client_get_dcd(NETWORK_ID, target_device_address, 0,
                                         NULL);
}

/**
 * @brief This struct hold the config data for the last provisioned device
 *
 */
typedef struct {
  // model bindings to be done. for simplicity, all models are bound to same
  // appkey in this example (assuming there is exactly one appkey used and the
  // same appkey is used for all model bindings)
  tsModel bind_model[MAX_ELEMS_PER_DEV * 3];
  uint8_t num_bind;
  uint8_t num_bind_done;

  // publish addresses for up to 4 models
  tsModel pub_model[MAX_ELEMS_PER_DEV * 3];
  uint16_t pub_address[MAX_ELEMS_PER_DEV * 3];
  uint8_t num_pub;
  uint8_t num_pub_done;

  // subscription addresses for up to 4 models
  tsModel sub_model[MAX_ELEMS_PER_DEV * 3];
  uint16_t sub_address[MAX_ELEMS_PER_DEV * 3];
  uint8_t num_sub;
  uint8_t num_sub_done;

} tsConfig;

// config data to be sent to last provisioned node:
tsConfig _sConfig;

/*
 * Add one publication setting to the list of configurations to be done
 * */
static void config_pub_add(uint16_t model_id, uint16_t vendor_id,
                           uint16_t address) {
  _sConfig.pub_model[_sConfig.num_pub].model_id = model_id;
  _sConfig.pub_model[_sConfig.num_pub].vendor_id = vendor_id;
  _sConfig.pub_address[_sConfig.num_pub] = address;
  _sConfig.num_pub++;
}

/*
 * Add one subscription setting to the list of configurations to be done
 * */
static void config_sub_add(uint16_t model_id, uint16_t vendor_id,
                           uint16_t address) {
  _sConfig.sub_model[_sConfig.num_sub].model_id = model_id;
  _sConfig.sub_model[_sConfig.num_sub].vendor_id = vendor_id;
  _sConfig.sub_address[_sConfig.num_sub] = address;
  _sConfig.num_sub++;
}

/*
 * Add one appkey/model bind setting to the list of configurations to be done
 * */
static void config_bind_add(uint16_t model_id, uint16_t vendor_id) {
  _sConfig.bind_model[_sConfig.num_bind].model_id = model_id;
  _sConfig.bind_model[_sConfig.num_bind].vendor_id = vendor_id;
  _sConfig.num_bind++;
}

// TODO Make this fucntion more flexible in stead of hard-coding

static void config_check() {
  uint8_t number_of_elem = 0;
  for (uint8_t i = 0; i < MAX_ELEMS_PER_DEV; i++) {
    if ((_sDCD_Table[i].numSIGModels > 0) ||
        (_sDCD_Table[i].numVendorModels > 0)) {
      number_of_elem++;
    }
  }

  memset(&_sConfig, 0, sizeof(_sConfig));
  // scan the SIG models in the DCD data
  if (target_device_type == TARGET_DEVICE_TYPE_NODE) {
    for (int i = 0; i < number_of_elem; i++) {
      for (int j = 0; j < _sDCD_Table[i].numSIGModels; j++) {
        if (_sDCD_Table[i].SIG_models[j] != 0x0000) {
          config_bind_add(_sDCD_Table[i].SIG_models[j], 0xFFFF);
          config_pub_add(_sDCD_Table[i].SIG_models[j], 0xFFFF,
                         target_group_address);
          config_sub_add(_sDCD_Table[i].SIG_models[j], 0xFFFF,
                         target_group_address);
        }
      }
    }
  } else if (target_device_type == TARGET_DEVICE_TYPE_GATEWAY) {
    target_device_address = LIGHT_GROUP_1;
    for (int i = 0; i < number_of_elem; i++) {
      for (int j = 0; j < _sDCD_Table[i].numSIGModels; j++) {
        if (_sDCD_Table[i].SIG_models[j] != 0x0000) {
          config_bind_add(_sDCD_Table[i].SIG_models[j], 0xFFFF);
          config_pub_add(_sDCD_Table[i].SIG_models[j], 0xFFFF,
                         target_group_address);
          config_sub_add(_sDCD_Table[i].SIG_models[j], 0xFFFF,
                         target_group_address);
        }
      }
    }
    target_device_address = LIGHT_GROUP_2;
    for (int i = 0; i < number_of_elem; i++) {
      for (int j = 0; j < _sDCD_Table[i].numSIGModels; j++) {
        if (_sDCD_Table[i].SIG_models[j] != 0x0000) {
          config_bind_add(_sDCD_Table[i].SIG_models[j], 0xFFFF);
          config_pub_add(_sDCD_Table[i].SIG_models[j], 0xFFFF,
                         target_group_address);
          config_sub_add(_sDCD_Table[i].SIG_models[j], 0xFFFF,
                         target_group_address);
        }
      }
    }
  }
}

void device_config_handle_mesh_evt(sl_btmesh_msg_t *evt) {
  sl_btmesh_evt_config_client_dcd_data_t *pDCD;
  uint16_t result;
  sl_status_t retval;
  uint16_t vendor_id, model_id, pub_address, sub_address;
  switch (SL_BT_MSG_ID(evt->header)) {
    case sl_btmesh_evt_config_client_dcd_data_id:
      pDCD = &evt->data.evt_config_client_dcd_data;
      app_log("DCD data event, received %u bytes\r\n", pDCD->data.len);

      // copy the data into one large array. the data may come in multiple
      // smaller pieces. the data is not decoded until all DCD events have been
      // received (see below)
      if ((_dcd_raw_len + pDCD->data.len) <= 256) {
        memcpy(&(_dcd_raw[_dcd_raw_len]), pDCD->data.data, pDCD->data.len);
        _dcd_raw_len += pDCD->data.len;
      }

      break;
    case sl_btmesh_evt_config_client_dcd_data_end_id:
      app_log("DCD data end event. Decoding the data.\r\n");
      // decode the DCD content
      DCD_decode();

      // check the desired configuration settings depending on what's in the DCD
      config_check();

      model_id = _sConfig.bind_model[_sConfig.num_bind_done].model_id;
      vendor_id = _sConfig.bind_model[_sConfig.num_bind_done].vendor_id;

      app_log("Binding appkey index %x to model ID %x", APPKEY_INDEX, model_id);
      retval = sl_btmesh_config_client_bind_model(
          NETWORK_ID, target_device_address, element_index, vendor_id, model_id,
          APPKEY_INDEX, NULL);

      break;
    case sl_btmesh_evt_config_client_binding_status_id:
      result = evt->data.evt_config_client_appkey_status.result;
      if (result == SL_STATUS_OK) {
        app_log(" bind complete\r\n");
        _sConfig.num_bind_done++;

        if (_sConfig.num_bind_done < _sConfig.num_bind) {
          // take the next model from the list of models to be bound with
          // application key. for simplicity, the same appkey is used for all
          // models but it is possible to also use several appkeys
          model_id = _sConfig.bind_model[_sConfig.num_bind_done].model_id;
          vendor_id = _sConfig.bind_model[_sConfig.num_bind_done].vendor_id;

          app_log("APP BIND, config %d/%d:: model %4.4x key index %x\r\n",
                  _sConfig.num_bind_done + 1, _sConfig.num_bind, model_id,
                  APPKEY_INDEX);

          retval = sl_btmesh_config_client_bind_model(
              NETWORK_ID, target_device_address, 0, vendor_id, model_id,
              APPKEY_INDEX, NULL);
          if (retval == SL_STATUS_OK) {
            app_log("Binding model 0x%4.4x\r\n", model_id);
          } else {
            app_log("Binding model %x, error: %lx\r\n", model_id, retval);
          }
        } else {
          // get the next model/address pair from the configuration list:
          model_id = _sConfig.pub_model[_sConfig.num_pub_done].model_id;
          vendor_id = _sConfig.pub_model[_sConfig.num_pub_done].vendor_id;
          pub_address = _sConfig.pub_address[_sConfig.num_pub_done];

          app_log("PUB SET, config %d/%d: model %4.4x -> address %4.4x\r\n",
                  _sConfig.num_pub_done + 1, _sConfig.num_pub, model_id,
                  pub_address);

          retval = sl_btmesh_config_client_set_model_pub(
              NETWORK_ID, target_device_address, 0, /* element index */
              vendor_id, model_id, pub_address, APPKEY_INDEX,
              0,  /* friendship credential flag */
              3,  /* Publication time-to-live value */
              0,  /* period = NONE */
              0,  /* Publication retransmission count */
              50, /* Publication retransmission interval */
              NULL);

          if (retval == SL_STATUS_OK) {
            app_log(" waiting pub ack\r\n");
          }
        }
      } else {
        app_log(" appkey bind failed with code %x\r\n", result);
      }
      break;
    case sl_btmesh_evt_config_client_model_pub_status_id:
      result = evt->data.evt_config_client_model_pub_status.result;
      if (result == SL_STATUS_OK) {
        app_log(" pub set OK\r\n");
        _sConfig.num_pub_done++;

        if (_sConfig.num_pub_done < _sConfig.num_pub) {
          /* more publication settings to be done
          ** get the next model/address pair from the configuration list: */
          model_id = _sConfig.pub_model[_sConfig.num_pub_done].model_id;
          vendor_id = _sConfig.pub_model[_sConfig.num_pub_done].vendor_id;
          pub_address = _sConfig.pub_address[_sConfig.num_pub_done];

          app_log("PUB SET, config %d/%d: model %4.4x -> address %4.4x\r\n",
                  _sConfig.num_pub_done + 1, _sConfig.num_pub, model_id,
                  pub_address);

          retval = sl_btmesh_config_client_set_model_pub(
              NETWORK_ID, target_device_address, 0, /* element index */
              vendor_id, model_id, pub_address, APPKEY_INDEX,
              0,  /* friendship credential flag */
              3,  /* Publication time-to-live value */
              0,  /* period = NONE */
              0,  /* Publication retransmission count */
              50, /* Publication retransmission interval */
              NULL);
        } else {
          // move to next step which is configuring subscription settings
          // get the next model/address pair from the configuration list:
          model_id = _sConfig.sub_model[_sConfig.num_sub_done].model_id;
          vendor_id = _sConfig.sub_model[_sConfig.num_sub_done].vendor_id;
          sub_address = _sConfig.sub_address[_sConfig.num_sub_done];

          app_log("SUB ADD, config %d/%d: model %4.4x -> address %4.4x\r\n",
                  _sConfig.num_sub_done + 1, _sConfig.num_sub, model_id,
                  sub_address);

          retval = sl_btmesh_config_client_add_model_sub(
              NETWORK_ID, target_device_address, 0, vendor_id, model_id,
              sub_address, NULL);

          if (retval == SL_STATUS_OK) {
            app_log(" waiting sub ack\r\n");
          }
        }
      } else {
        app_log(" pub set failed with code %x\r\n", result);
      }
      break;
    case sl_btmesh_evt_config_client_model_sub_status_id:
      result = evt->data.evt_config_client_model_sub_status.result;
      if (result == SL_STATUS_OK) {
        app_log(" sub add OK\r\n");
        _sConfig.num_sub_done++;
        if (_sConfig.num_sub_done < _sConfig.num_sub) {
          // move to next step which is configuring subscription settings
          // get the next model/address pair from the configuration list:
          model_id = _sConfig.sub_model[_sConfig.num_sub_done].model_id;
          vendor_id = _sConfig.sub_model[_sConfig.num_sub_done].vendor_id;
          sub_address = _sConfig.sub_address[_sConfig.num_sub_done];

          app_log("SUB ADD, config %d/%d: model %4.4x -> address %4.4x\r\n",
                  _sConfig.num_sub_done + 1, _sConfig.num_sub, model_id,
                  sub_address);

          retval = sl_btmesh_config_client_add_model_sub(
              NETWORK_ID, target_device_address, 0, vendor_id, model_id,
              sub_address, NULL);

          if (retval == SL_STATUS_OK) {
            app_log(" waiting sub ack\r\n");
          }
        } else {
          app_log("***\r\nconfiguration complete\r\n***\r\n");
          _dcd_raw_len = 0;
          device_config_configuration_on_success_callback();
        }
      } else {
        app_log(" sub add failed with code %x\r\n", result);
      }

      break;
  }
}

// NOTE I am considering adding one callback function
// below to let the main program be able to do after the configuration process
// complete Like proivsioning next device for example
SL_WEAK void device_config_configuration_on_success_callback() {}
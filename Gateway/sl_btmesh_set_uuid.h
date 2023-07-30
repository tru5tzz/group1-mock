/***************************************************************************//**
 * @file  sl_btmesh_set_uuid.h
 * @brief prototype function set uuid for device.
 ******************************************************************************/

#ifndef SL_BTMESH_SET_UUID_H_
#define SL_BTMESH_SET_UUID_H_

/**************************************************************************//**
 *Function set uuid for device
 *
 * @param[in] evt Event coming from the Bluetooth Mesh stack.
 *****************************************************************************/
void sl_btmesh_set_my_uuid(sl_bt_msg_t *evt);

#endif /* SL_BTMESH_SET_UUID_H_ */

/***************************************************************************//**
 * @file
 * @brief define value 
 ******************************************************************************/
#ifndef GATEWAY_DEFINE_H
#define GATEWAY_DEFINE_H

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/
/// timeout for find new devices after startup
#define DEVICE_REGISTER_SHORT_TIMEOUT                   500
/// Length of the display name buffer
#define NAME_BUF_LEN                                    20
/// Timout for Blinking LED during provisioning
#define APP_LED_BLINKING_TIMEOUT                        250
/// Callback has not parameters
#define NO_CALLBACK_DATA                                (void *)NULL
/// Used button indexes
#define BUTTON_PRESS_BUTTON_0                           0
/// number max of devices in mesh
#define NUM_MAX_DEVICE                                  10
/// Get charater null 
#define USART_GET_CHAR_NULL                             255
/// transfer from char to digit
#define CHAR_TO_DIGIT                                   48
/// Length unicast and state of light
#define LENGTH_UNICAST_ADDRESS                          2
#define LENGTH_SET_STATE_LIGHT                          2
/// address public all
#define ADDRESS_GENERAL                                 0xFFFF
/// element index main
#define ELEMENT_INDEX_ROOT                              0
/// take value MSB
#define VALUE_MSB(x)                                    (x >> 8)
/// timout for find device 
#define TIMEOUT_SCAN_DEVICE                             10000
#define TIMEOUT_SCAN_LIGHT                              5000
#define TIMEOUT_SCAN_SWITCH                             TIMEOUT_SCAN_LIGHT +2500

#endif // GATEWAY_DEFINE_H

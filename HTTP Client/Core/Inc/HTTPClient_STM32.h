/*
 * HTTPClient_STM32.h
 *
 *  Created on: Feb 18, 2021
 *      Author: Ngoc Hang
 */

#ifndef INC_HTTPCLIENT_STM32_H_
#define INC_HTTPCLIENT_STM32_H_

#include "SIM_STM32.h"

extern STATUS ret;
extern uint8_t Sim_Rxdata[MAX_RECVBUF_LEN];

STATUS HTTP_configParams(void);
STATUS HTTP_sendGETRequest(char* url);
STATUS HTTP_readGETResponse(char* datatoget);

#endif /* INC_HTTPCLIENT_STM32_H_ */

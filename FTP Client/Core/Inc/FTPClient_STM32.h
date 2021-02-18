/*
 * FTPClient_STM32.h
 *
 *  Created on: Feb 18, 2021
 *      Author: ngochang
 */

#ifndef INC_FTPCLIENT_STM32_H_
#define INC_FTPCLIENT_STM32_H_

#include "SIM_STM32.h"

extern STATUS ret;
extern uint8_t Sim_Rxdata[MAX_RECVBUF_LEN];

STATUS FTP_configParams(void);
STATUS FTP_setUserAndPass(char* user, char* pass);
STATUS FTP_setFileType(uint8_t type);
STATUS FTP_setTransmode(uint8_t mode);
STATUS FTP_setRspTimeout(uint8_t timeout);
STATUS FTP_loginServer(char* server, uint16_t port);
STATUS FTP_setPath(char* path);
STATUS FTP_listFileNames(char* datatoget);
uint32_t FTP_getFileSize(char* filename);
STATUS FTP_downloadFile(char* filename, int32_t startpos, int32_t downloadlen, int32_t* firstbyte_pos, int32_t* lastbyte_pos);
STATUS FTP_uploadFile(char* filename, uint32_t startpos, uint32_t uploadlen, int8_t beof, char* datatoput);

#endif /* INC_FTPCLIENT_STM32_H_ */

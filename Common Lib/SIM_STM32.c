/* --COPYRIGHT--,
 * Copyright (c)2020, TAPIT Co.,Ltd.
 * https://tapit.vn
 *
 **************************_TAPIT_EC15_EC21_STM32_************************
 *  Description:	Using UARTx to communicate with ModuleSIM EC15/EC21.
 *					USART2 in default,
 *					EC15/EC21 pin	- STM pin in default:
 *						TX_Pin 		- 	A3
 *  					RX_Pin 		-	A2
 *  Version:  		1.0
 *  Author: 		Hang Tran
 *  Release: 		May 8, 2020
 *  Built with STM32CubeIDE Version: 1.3.1
 *************************************************************************
 */

#include "SIM_STM32.h"

uint8_t Sim_Rxbyte[1] = {0};
uint8_t Sim_Rxdata[MAX_RECVBUF_LEN] = {0};
volatile uint16_t Sim_Count = 0;
volatile bool isSimResponse = false;
volatile uint32_t Sim_UartTime = 0;

uint8_t HTTP_DataToGet[MAX_RECVBUF_LEN] = {0};
uint8_t FTP_DataToSend[MAX_RECVBUF_LEN/2] = "";
uint8_t FTP_DataToGet[FTP_MAX_DOWNLOADLEN+1] = {0};
uint16_t FTP_DataLen = 0;

STATUS ret;

uint8_t check = 0;

/**
  * @brief  Rx Transfer completed callbacks. USART2 is UART Module Sim used for
  * 		communicating with MCU. Change if use another UART.
  * @param  huart  Pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart->Instance == USART3)
	{
		if(Sim_Count == MAX_RECVBUF_LEN)
		{
			Sim_Count = 0;
		}
		Sim_Rxdata[Sim_Count++] = Sim_Rxbyte[0];
		HAL_UART_Receive_IT(&SIM_UART, Sim_Rxbyte, 1);
		Sim_UartTime = HAL_GetTick();
		isSimResponse = true;
	}
}

/**
  * @brief  Delete all data in buffer
  * @param  buf which buffer needs to be deleted
  * @param  len which length of buffer to be deleted.
  * @retval None
  */
void deleteBuffer(char* buf, uint32_t len)
{
	for(uint32_t i = 0; i < len; i++)
	{
		buf[i] = '\0';
	}
}

/**
  * @brief  Find the first occurrence of the substring userdata in the string Sim_Rxdata.
  * 		The terminating '\0' characters are compared.
  * @param  userdata which buffer needs to be searched in Sim_Rxdata
  * @param  endpos which is end position of Sim_Rxdata.
  * 		This function will search from beginning to end position of Sim_Rxdata to find userdata sequence
  * @retval The array index to the first occurrence in Sim_Rxdata of the entire sequence of characters specified in userdata,
  * 		or 0 if the sequence is not present in Sim_Rxdata.
  */
static uint32_t strstrFromStart(char* userdata, uint32_t endpos)
{
	uint8_t len = strlen(userdata);
	for(uint32_t i = 0; i < endpos; i++)
	{
		if(Sim_Rxdata[i] == userdata[0])
		{
			for(uint8_t j = 1; j < len; j++)
			{
				if(Sim_Rxdata[++i] == userdata[j])
				{
					if(j == (len-1)) return(i-len+1); //return first occurrence in Sim_Rxdata of userdata
				}
				else
				{
					i--;
					break;
				}
			}
			if(ret != 0) break;
		}
	}
	return 0;
}

/**
  * @brief  Find the first occurrence of the substring userdata in the string Sim_Rxdata.
  * 		The terminating '\0' characters are compared.
  * @param  userdata which buffer needs to be searched in Sim_Rxdata
  * @param  startpos which is start position of Sim_Rxdata.
  * 		This function will search from end position to start position of Sim_Rxdata to find userdata sequence
  * @retval The array index to the first occurrence in Sim_Rxdata of the entire sequence of characters specified in userdata,
  * 		or 0 if the sequence is not present in Sim_Rxdata.
  */
static uint32_t strstrFromEnd(char* userdata, uint32_t startpos)
{
	uint32_t len = strlen(userdata);
	for(uint32_t i = startpos; i > 0; i--)
	{
		if(Sim_Rxdata[i] == userdata[0])
		{
			uint32_t ind = i;
			for(uint8_t j = 1; j < len; j++)
			{
				if(Sim_Rxdata[++ind] == userdata[j])
				{
					if(j == (len-1)) return(ind-len+1);
				}
				else
				{
					i--;
					break;
				}
			}
			if(ret != 0) break;
		}
	}
	return 0;
}

/**
  * @brief  Get a substring of a String
  * @param  maindata where the content is to be cut.
  * @param  subdata where the content after cut is stored.
  * @param  startpos is the index to start the substring at.
  * @param  endpos is the index to end the substring before.
  * @retval None
  */
static void subString(char* maindata, char* subdata, uint32_t startpos, uint32_t endpos)
{
	if(endpos < strlen(maindata))
	{
		for(uint32_t i = startpos; i < endpos; i++)
		{
			subdata[i - startpos] = maindata[i];
		}
	}
}

/**
  * @brief  Send AT commands or data to ModuleSim
  * @note	SIM_UART is defined at file SimLib.h. Change if use another UART.
  * @param  command which is AT commands or data.
  * @param  len which is length of AT commands or data.
  * @retval None
  */
STATUS Sim_send(char *command, uint16_t len)
{
	deleteBuffer((char*)Sim_Rxdata, strlen((char*)Sim_Rxdata));
	Sim_Count = 0;
	if(HAL_UART_Transmit(&SIM_UART, (uint8_t*) command, len, MAX_SEND_TIME) == HAL_OK)
	{
		return RET_OK;
	}
	return RET_TIMEOUT;
}

/**
  * @brief  Receive response data from ModuleSim
  * @param  timeout which is maximum waiting time for receiving data.
  * @retval RET_TIMEOUT if no response data.
  * 		RET_OK if received a string of data.
  */
STATUS Sim_recv(uint32_t timeout)
{
	uint32_t time = HAL_GetTick();
	while(isSimResponse == false)
	{
		if(HAL_GetTick() - time > timeout)
		{
			return RET_TIMEOUT;
		}
	}
	HAL_Delay(5);
	uint32_t time1 = 0;
	while(1)
	{
		time1 = HAL_GetTick();
		if((time1 - Sim_UartTime) > 100)
		{
			__NOP();
			break;
		}
		HAL_Delay(1);
	}
	isSimResponse = false;
	return RET_OK;
}

/**
  * @brief  Wait and check if the data series returned by ModuleSim include userdata.
  * 		If it is not correct, continue to wait for data from the ModuleSim,
  * 		process up to maxretry times.
  * @param  userdata which is User input data for comparison.
  * @param	maxretry which the maximum number of times the process of waiting and
  * 		checking data is performed.
  * @param	timeout is maximum waiting time of a data
  * @retval RET_FAIL if no userdata data in Sim_Rxdata.
  * 		RET_OK if otherwise.
  */
STATUS Sim_checkResponseWith(char* userdata, uint8_t maxretry, uint32_t timeout)
{
	uint8_t i;
	for(i = 0; i < maxretry; i++)
	{
		if(Sim_recv(timeout) == RET_OK)
		{
			if(strstr((char*)Sim_Rxdata,userdata) != NULL)
			{
				__NOP();
				#if (SIM_DEBUG == 1)
					printf("%s\r\n",(char*)Sim_Rxdata);
				#endif
				return RET_OK;
			}
		}
	}
	#if (SIM_DEBUG == 1)
		printf("-----Error-----\r\n");
		printf("Max retry = %d\r\n",i);
		printf("%s",(char*)Sim_Rxdata);
		printf("---------------\r\n");
	#endif
	__NOP();
	return RET_FAIL;
}

/**
  * @brief  Similar to the Sim_checkResponseWith function.
  * 		However, the data from the ModuleSim contains NULL characters.
  * @param  userdata which is User input data for comparison.
  * @param	maxretry which the maximum number of times the process of waiting and
  * 		checking data is performed.
  * @param	timeout is maximum waiting time of a data
  * @retval RET_FAIL if no userdata data in Sim_Rxdata.
  * 		RET_OK if otherwise.
  */
STATUS Sim_checkRespIncludingNULL(char* userdata, uint8_t maxretry, uint32_t timeout)
{
	uint8_t i;
	for(i = 0; i < maxretry; i++)
	{
		if(Sim_recv(timeout) == RET_OK)
		{
			if(strstrFromStart(userdata, MAX_RECVBUF_LEN) > 0)
			{
				__NOP();
				return RET_OK;
			}
			else
			{
				isSimResponse = false;
			}
		}
		__NOP();
	}
	__NOP();
	return RET_FAIL;
}

/**
  * @brief  Check ModuleSim response by using the "AT" command.
  * @param  None.
  * @retval RET_FAIL if no response or incorrect data.
  * 		RET_OK if response correct data.
  */
STATUS Sim_checkOK(void)
{
	Sim_send("AT\r",3);
	ret = Sim_checkResponseWith("OK", 1, 1000);
	__NOP();
	return ret;
}

/**
  * @brief  Get the RSSI value of the SIM.
  * @param  None.
  * @retval RET_FAIL if 0 < RSSI < 32.
  * 		RET_OK if otherwise.
  */
uint8_t Sim_getSignalQuality(void)
{
	uint8_t rssi = 0;
	Sim_send("AT+CSQ\r",7);
	ret = Sim_checkResponseWith("OK", 2, 3000);
	__NOP();
	if(ret == RET_OK)
	{
		char* tok = strtok((char*)Sim_Rxdata,":");
		tok = strtok(NULL,",");
		rssi = atoi(tok);
		deleteBuffer((char*)Sim_Rxdata, 100);
		if(rssi < 32)
		{
			return rssi;
		}
	}
	return 99;
}

/**
  * @brief  Check SIM card pin state
  * @param  None.
  * @retval RET_FAIL if SIM not inserted
  * 		RET_OK if SIM inserted.
  */
STATUS Sim_queryCardStatus(void)
{
	Sim_send("AT+CPIN?\r",9);
	ret = Sim_checkResponseWith("READY", 2, 3000);
	__NOP();
	return ret;
}

/**
  * @brief  Configure APN, username, password of the network that the SIM card is using.
  * @param  apn is the access point name.
  * @param	user is the username.
  * @param	pass is the password.
  * @retval RET_FAIL if configuration failed.
  * 		RET_OK if configuration successful.
  */
STATUS Sim_configInternet(char* apn, char* user, char* pass)
{
	char cmd[50] = {0};
	sprintf(cmd,"%s%s%s%s%s%s%s%s%s%s%s","AT+QICSGP=1,1,","\"",apn,
					"\",","\"",user,"\",","\"",pass,"\",","1\r");
	for(uint8_t i = 0; i < 2; i++)
	{
		Sim_send(cmd,strlen(cmd));
		ret = Sim_checkResponseWith("OK", 2, 5000);
		__NOP();
		if(ret  == RET_OK)
		{
			break;
		}
		else
		{
			Sim_disconnectInternet();
			HAL_Delay(1000);
		}
	}
	return ret;
}

/**
  * @brief  Active PDP.
  * @param  None.
  * @retval RET_FAIL if connection failed.
  * 		RET_OK if connection successful.
  */
STATUS Sim_connectInternet(void)
{
	uint8_t failtime;
	for(failtime = 0; failtime < 2; failtime++)
	{
		Sim_send("AT+QIACT=1\r",11);
		ret = Sim_checkResponseWith("OK", 2, 5000);
		__NOP();
		if(ret  == RET_OK)
		{
			break;
		}
		else
		{
			#if (SIM_DEBUG == 1)
				printf("Active PDP failed. Deactive the %d time!\r\n",failtime+1);
			#endif
			if(Sim_disconnectInternet() == RET_OK)
			{
				Sim_configInternet(APN, APN_USER, APN_PASS);
			}
			else
			{
				if(failtime == 2)
				{
					//Sim_rebootModule();
					#if (SIM_DEBUG == 1)
						printf("Deactive failed 2 times. Reboot module!\r\n");
					#endif
				}
			}
		}
	}
	return ret;
}

/**
  * @brief  Deactive PDP.
  * @param  None.
  * @retval RET_FAIL if disconnection failed.
  * 		RET_OK if disconnection successful.
  */
STATUS Sim_disconnectInternet(void)
{
	Sim_send("AT+QIDEACT=1\r",13);
	ret = Sim_checkResponseWith("OK", 2, 5000);
	__NOP();
	return ret;
}

/**
  * @brief  Get HTTP, FTP error code from SIM response.
  * @param  response is buffer stored SIM response.
  * @param	delim is the delimiter.
  * @retval Error code of HTTP or FTP
  * 		0 if no error.
  */
uint16_t Sim_getErrorCode(char* response, char* delim)
{
	char errorcode[5] = {0};
	char* ptr = NULL;
	ptr = strstr((char*)Sim_Rxdata,response);
	if(ptr != NULL)
	{
		uint16_t startpos = ptr - (char*)Sim_Rxdata + strlen(response);
		uint16_t endpos = 0;
		uint16_t len = strlen((char*)Sim_Rxdata);
		if(delim != NULL)
		{
			for(uint16_t i = startpos; i < len; i++)
			{
				if(Sim_Rxdata[i] == delim[0])
				{
					if(i > startpos)
					{
						endpos = i;
						break;
					}
				}
			}
		}
		else
		{
			endpos = startpos + 4; //4 digits after delim
		}
		subString((char*)Sim_Rxdata, errorcode, startpos, endpos);
		return(atoi(errorcode));
	}
	return 0;
}

/**
  * @brief  Configure contextID and responseheader parameters for HTTP
  * @param  None.
  * @retval RET_FAIL if configuration failed.
  * 		RET_OK if configuration successful.
  */
STATUS HTTP_configParams(void)
{
	Sim_send("AT+QHTTPCFG=\"contextid\",1\r",26);
	ret = Sim_checkResponseWith("OK", 2, 5000);
	__NOP();
	if(ret != RET_OK)
	{
		#if (SIM_DEBUG == 1)
				printf("Error: HTTP contextid failed - %d\r\n",Sim_getErrorCode("+CME ERROR:", NULL));
		#endif
		return ret;
	}
	Sim_send("AT+QHTTPCFG=\"responseheader\",1\r",32);
	ret = Sim_checkResponseWith("OK", 2, 5000);
	__NOP();
	if(ret != RET_OK)
	{
		#if (SIM_DEBUG == 1)
				printf("Error: HTTP responseheader failed - %d\r\n",Sim_getErrorCode("+CME ERROR:", NULL));
		#endif
	}
	return ret;
}

/**
  * @brief  Send HTTP request using GET method.
  * @param  url is URL to access.
  * @retval RET_OK if Server responds to code 200.
  * 		RET_FAIL if otherwise.
  */
STATUS HTTP_sendGETRequest(char* url)
{
	char cmd[100] = {0};
	sprintf(cmd,"%s%d%s","AT+QHTTPURL=",strlen(url),",30\r");
	Sim_send(cmd,strlen(cmd));
	ret = Sim_checkResponseWith("CONNECT", 2, 30000);
	__NOP();
	if(ret == RET_OK)
	{
		Sim_send(url,strlen(url));
		ret = Sim_checkResponseWith("OK", 2, 30000);
		__NOP();
		if(ret == RET_OK)
		{
			Sim_send("AT+QHTTPGET=60\r",15);
			ret = Sim_checkResponseWith("+QHTTPGET: ", 3, 60000);
			__NOP();
			if(ret == RET_OK)
			{
				//Get HTTP response code
				char* ptr = strstr((char*)Sim_Rxdata,"+QHTTPGET: ");
				char* tok = NULL;
				tok = strtok(ptr,":");
				tok = strtok(NULL,",");
				uint16_t code = atoi(tok);
				tok = strtok(NULL,",");
				uint16_t httpcode = atoi(tok);
				if(code != 0)
				{
					ret = RET_FAIL;
					#if (SIM_DEBUG == 1)
						printf("Error: HTTP GET failed - %d\r\n",code);
					#endif
				}
				else
				{
					if(httpcode == 200)
					{
						ret = RET_OK;
					}
					else
					{
						ret = RET_FAIL;
						#if (SIM_DEBUG == 1)
							printf("Error: HTTP failed - %d (protocol code)\r\n",httpcode);
						#endif
					}
				}
				deleteBuffer((char*)Sim_Rxdata, 100);
			}
			else //AT+QHTTPGET +CME ERROR
			{
				#if (SIM_DEBUG == 1)
					printf("Error: HTTP GET failed - %d (+CME ERROR)\r\n",Sim_getErrorCode("+CME ERROR:", NULL));
				#endif
			}
		}
	}
	else //AT+QHTTPURL +CME ERROR
	{
		#if (SIM_DEBUG == 1)
			printf("Error: HTTP URL failed - %d\r\n",Sim_getErrorCode("+CME ERROR:", NULL));
		#endif
	}
	return ret;
}

/**
  * @brief  Read the HTTP response from the server after sending the request.
  * @param  datatoget is used to store response data from the server.
  * @retval RET_OK if read success.
  * 		RET_FAIL if read fail.
  */
STATUS HTTP_readGETResponse(char* datatoget)
{
	deleteBuffer(datatoget, strlen(datatoget));
	Sim_send("AT+QHTTPREAD=60\r",16);
	ret = Sim_checkResponseWith("+QHTTPREAD:", 4, 60000);
	__NOP();
	if(ret == RET_OK)
	{
		uint16_t code = Sim_getErrorCode("+QHTTPREAD:", NULL);
		if(code == 0)
		{
			char* ptr = NULL;
			ptr = strstr((char*)Sim_Rxdata,"HTTP/");
			if(ptr != NULL)
			{
				char* ptr1 = NULL;
				ptr1 = strstr((char*)Sim_Rxdata,"\r\nOK");
				uint16_t startpos = ptr - (char*)Sim_Rxdata;
				uint16_t endpos = ptr1 - (char*)Sim_Rxdata;
				subString((char*)Sim_Rxdata, datatoget, startpos, endpos);
			}
		}
		else
		{
			ret = RET_FAIL;
			#if (SIM_DEBUG == 1)
				printf("Error: HTTP failed - %d\r\n", code);
			#endif
		}
	}
	else //AT+QHTTPREAD +CME ERROR:
	{
		#if (SIM_DEBUG == 1)
			printf("Error: HTTP READ GET failed - %d\r\n",Sim_getErrorCode("+CME ERROR:", NULL));
		#endif
	}
	return ret;
}

/**
  * @brief  Configure context ID parameter for FTP.
  * @param  None.
  * @retval RET_FAIL if configuration failed.
  * 		RET_OK if configuration successful.
  */
STATUS FTP_configParams(void)
{
	Sim_send("AT+QFTPCFG=\"contextid\",1\r",25);
	ret = Sim_checkResponseWith("OK", 2, 5000);
	__NOP();
	if(ret != RET_OK)
	{
		#if (SIM_DEBUG == 1)
			printf("Error: FTP contextid failed - %d\r\n",Sim_getErrorCode("+CME ERROR:", NULL));
		#endif
	}
	return ret;
}

/**
  * @brief  Enter the username and password to access the FTP Server.
  * @param  user is the username.
  * @param  pass is the password.
  * @retval RET_FAIL if configuration failed.
  * 		RET_OK if configuration successful.
  */
STATUS FTP_setUserAndPass(char* user, char* pass)
{
	char cmd[50] = {0};
	sprintf(cmd,"%s%s%s%s%s","AT+QFTPCFG=\"account\",\"",user,"\",\"",pass,"\"\r");
	Sim_send(cmd,strlen(cmd));
	ret = Sim_checkResponseWith("OK", 2, 5000);
	__NOP();
	if(ret != RET_OK)
	{
		#if (SIM_DEBUG == 1)
			printf("Error: FTP set User, Pass failed - %d\r\n",Sim_getErrorCode("+CME ERROR:", NULL));
		#endif
	}
	return ret;
}

/**
  * @brief  Configure the data type of the transfer file between the ModuleSIM and the FTP Server.
  * @param  type is "binary" or "ascii".
  * @retval RET_FAIL if configuration failed.
  * 		RET_OK if configuration successful.
  */
STATUS FTP_setFileType(uint8_t type)
{
	char cmd[50] = {0};
	sprintf(cmd,"%s%d%s","AT+QFTPCFG=\"filetype\",",type,"\r");
	Sim_send(cmd,strlen(cmd));
	ret = Sim_checkResponseWith("OK", 2, 5000);
	__NOP();
	if(ret != RET_OK)
	{
		#if (SIM_DEBUG == 1)
			printf("Error: FTP set File Type failed - %d\r\n",Sim_getErrorCode("+CME ERROR:", NULL));
		#endif
	}
	return ret;
}

/**
  * @brief  Configure the data transmission mode in the data connection.
  * @param  mode is "passive" or "active".
  * @retval RET_FAIL if configuration failed.
  * 		RET_OK if configuration successful.
  */
STATUS FTP_setTransmode(uint8_t mode)
{
	char cmd[50] = {0};
	sprintf(cmd,"%s%d%s","AT+QFTPCFG=\"transmode\",",mode,"\r");
	Sim_send(cmd,strlen(cmd));
	ret = Sim_checkResponseWith("OK", 2, 5000);
	__NOP();
	if(ret != RET_OK)
	{
		#if (SIM_DEBUG == 1)
			printf("Error: FTP set Transmode failed - %d\r\n",Sim_getErrorCode("+CME ERROR:", NULL));
		#endif
	}
	return ret;
}

/**
  * @brief  Configure the maximum response time from the FTP Server.
  * @param  timeout from 20 – 180. Unit: second.
  * @retval RET_FAIL if configuration failed.
  * 		RET_OK if configuration successful.
  */
STATUS FTP_setRspTimeout(uint8_t timeout)
{
	char cmd[50] = {0};
	sprintf(cmd,"%s%d%s","AT+QFTPCFG=\"rsptimeout\",",timeout,"\r");
	Sim_send(cmd,strlen(cmd));
	ret = Sim_checkResponseWith("OK", 2, 5000);
	__NOP();
	if(ret != RET_OK)
	{
		#if (SIM_DEBUG == 1)
			printf("Error: FTP set RspTimeout failed - %d\r\n",Sim_getErrorCode("+CME ERROR:", NULL));
		#endif
	}
	return ret;
}

/**
  * @brief  Log in to the FTP server.
  * @param  server is domain name of FTP server
  * @param  port is port of FTP server
  * @retval RET_FAIL if configuration failed.
  * 		RET_OK if configuration successful.
  */
STATUS FTP_loginServer(char* server, uint16_t port)
{
	char cmd[50] = {0};
	sprintf(cmd,"%s%s%s%d%s","AT+QFTPOPEN=\"",server,"\",",port,"\r");
	Sim_send(cmd,strlen(cmd));
	ret = Sim_checkResponseWith("+QFTPOPEN: ", 2, 30000);
	__NOP();
	if(ret != RET_OK)
	{
		#if (SIM_DEBUG == 1)
			printf("Error: FTP OPEN failed - %d (+CME ERROR)\r\n",Sim_getErrorCode("+CME ERROR:", NULL));
		#endif
	}
	else
	{
		uint16_t code = Sim_getErrorCode("+QFTPOPEN:", ",");
		if(code != 0)
		{
			ret = RET_FAIL;
			#if (SIM_DEBUG == 1)
				printf("Error: FTP OPEN failed - %d. ",Sim_getErrorCode(",", NULL));
				printf("Error Codes - %d\r\n",code);
			#endif
		}
	}
	return ret;
}

/**
  * @brief  Configure file path for uploading or downloading process.
  * @param  path is directory path.
  * @retval RET_FAIL if configuration failed.
  * 		RET_OK if configuration successful.
  */
STATUS FTP_setPath(char* path)
{
	char cmd[50] = {0};
	sprintf(cmd,"%s%s%s","AT+QFTPCWD=\"",path,"\"\r");
	Sim_send(cmd,strlen(cmd));
	ret = Sim_checkResponseWith("+QFTPCWD:", 3, 10000);
	__NOP();
	if(ret != RET_OK)
	{
		#if (SIM_DEBUG == 1)
				printf("Error: FTP set Path failed - %d (+CME ERROR)\r\n",Sim_getErrorCode("+CME ERROR:", NULL));
		#endif
	}
	else
	{
		uint16_t code = Sim_getErrorCode("+QFTPCWD:", ",");
		if(code != 0)
		{
			ret = RET_FAIL;
			#if (SIM_DEBUG == 1)
				printf("Error: FTP set Path failed - %d. ",Sim_getErrorCode(",", NULL));
				printf("Error Codes - %d\r\n",code);
			#endif
		}
	}
	return ret;
}

/**
  * @brief  Lists all file names in the current directory.
  * @note	Must be call after FTP_setPath function
  * @param  datatoget is used to store all file names.
  * @retval RET_FAIL if list failed.
  * 		RET_OK if list successful.
  */
STATUS FTP_listFileNames(char* datatoget)
{
	char* ptr = NULL;
	char* ptr1 = NULL;
	uint32_t pos_start, pos_end = 0;
	Sim_send("AT+QFTPNLST=\".\"\r",16);
	ret = Sim_checkResponseWith("+QFTPNLST:", 5, 30000);
	__NOP();
	if(ret == RET_OK)
	{
		uint16_t code = Sim_getErrorCode("+QFTPNLST:", ",");
		if(code != 0)
		{
			ret = RET_FAIL;
			#if (SIM_DEBUG == 1)
				printf("Error: FTP list FileNames failed - %d. ",Sim_getErrorCode(",", NULL));
				printf("Error Codes - %d\r\n",code);
			#endif
		}
		else
		{
			ptr = strstr((char*)Sim_Rxdata,"CONNECT");
			if(ptr != NULL)
			{
				ptr1 = strstr((char*)Sim_Rxdata,"\r\nOK");
				pos_start = ptr - (char*)Sim_Rxdata;
				pos_end = ptr1 - (char*)Sim_Rxdata;
				deleteBuffer(datatoget, strlen(datatoget));
				uint32_t cnt = 0;
				for(uint32_t i = (pos_start + 9); i < pos_end; i++)
				{
					datatoget[cnt++] = Sim_Rxdata[i];
				}
				__NOP();
			}
			else
			{
				ret = RET_FAIL;
			}
		}
	}
	else
	{
		#if (SIM_DEBUG == 1)
			printf("Error: FTP list FileNames failed - %d (+CME ERROR)\r\n",Sim_getErrorCode("+CME ERROR:", NULL));
		#endif
	}
	return ret;
}

/**
  * @brief  Get file size of a specified file.
  * @param  filename is name of file to get size.
  * @retval 0 if get failed.
  * 		Size of file if get successful.
  */
uint32_t FTP_getFileSize(char* filename)
{
	char cmd[50] = {0};
	sprintf(cmd,"%s%s%s","AT+QFTPSIZE=\"",filename,"\"\r");
	Sim_send(cmd,strlen(cmd));
	ret = Sim_checkResponseWith("+QFTPSIZE:", 2, 20000);
	__NOP();
	if(ret == RET_OK)
	{
		uint16_t code = Sim_getErrorCode("+QFTPSIZE:", ",");
		if(code != 0)
		{
			ret = RET_FAIL;
			#if (SIM_DEBUG == 1)
				printf("Error: QFTPSIZE failed - %d. ",Sim_getErrorCode(",", NULL));
				printf("Error Codes - %d\r\n",code);
			#endif
		}
		else
		{
			char* tok = strtok((char*)Sim_Rxdata,":");
			tok = strtok(NULL, ",");
			tok = strtok(NULL, ",");
			uint32_t size = atoi(tok);
			#if (SIM_DEBUG == 1)
				printf("Size of file = %ld\r\n",size);
			#endif
			deleteBuffer((char*)Sim_Rxdata, 100);
			return size;
		}
	}
	return 0;
}

/**
  * @brief  Download the file from the FTP Server. Data will be exported via COM port.
  * @param  filename is name of file to download.
  * @param  startpos. Start downloading from the startpos position.
  * 				  Input -1 if you want to download the entire file.
  * @param  downloadlen is size to download. Input -1 if you want to download the entire file.
  * @param  firstbyte_pos is the first byte's position of the downloaded data in Sim_Rxdata
  * @param  lastbyte_pos is the last byte's position of the downloaded data in Sim_Rxdata
  * @retval RET_FAIL if download failed.
  * 		RET_OK if download successful.
  */
STATUS FTP_downloadFile(char* filename, int32_t startpos, int32_t downloadlen, int32_t* firstbyte_pos, int32_t* lastbyte_pos)
{
	uint32_t pos_start, pos_end = 0;
	char cmd[50] = {0};
	sprintf(cmd,"%s%s%s","AT+QFTPGET=\"",filename,"\",\"COM:\"");
	if(startpos != -1) //= -1: download full file content
	{
		sprintf(cmd,"%s,%ld",cmd,startpos);
	}
	if(downloadlen != -1) //== -1: download full file content
	{
		sprintf(cmd,"%s,%ld",cmd,downloadlen);
	}
	strcat(cmd,"\r");
	Sim_send(cmd,strlen(cmd));
	char resp[20] = {0};
	sprintf(resp,"%s%ld","+QFTPGET: 0,",downloadlen);
	ret = Sim_checkRespIncludingNULL(resp, 3, 30000);
	__NOP();
	if(ret == RET_OK)
	{
		uint16_t code = Sim_getErrorCode("+QFTPGET:", ",");
		if(code != 0)
		{
			ret = RET_FAIL;
			#if (SIM_DEBUG == 1)
				printf("Error: FTP GET failed - %d. ",Sim_getErrorCode(",", NULL));
				printf("Error Codes - %d\r\n",code);
			#endif
		}
		else
		{
			pos_start = strstrFromStart("CONNECT\r\n", MAX_RECVBUF_LEN) + 9; //remove "CONNECT\r\n"
			if(pos_start > 0)
			{
				pos_end = strstrFromEnd("OK\r\n\r\n", MAX_RECVBUF_LEN) - 3; //remove 1 "\r","\n" before OK
				if(pos_end > pos_start)
				{
					*firstbyte_pos = pos_start;
					*lastbyte_pos = pos_end;
				}
				else
				{
					*firstbyte_pos = *lastbyte_pos = -1; //error
				}
				__NOP();
			}
			else
			{
				ret = RET_FAIL;
			}
		}
	}
	else
	{
		#if (SIM_DEBUG == 1)
			printf("Error: FTP GET failed- %d (+CME ERROR)\r\n",Sim_getErrorCode("+CME ERROR:", NULL));
		#endif
	}
	return ret;
}

/**
  * @brief  Upload the file to FTP Server. Data will be exported via COM port.
  * @param  filename is name of file to download.
  * @param  startpos. Start uploading from the startpos position.
  * @param  uploadlen is size to upload. Input 0 if you want to upload the entire file.
  * @param  beof. Enter 0 if the content has not been uploaded completely yet.
  * 			  Enter 1 if this is the last upload. When startpos = 0, this parameter will be ignored.
  * @param  datatoput is file upload content.
  * @retval RET_FAIL if upload failed.
  * 		RET_OK if upload successful.
  */
STATUS FTP_uploadFile(char* filename, uint32_t startpos, uint32_t uploadlen, int8_t beof, char* datatoput)
{
	char cmd[50] = {0};
	sprintf(cmd,"%s%s%s%ld","AT+QFTPPUT=\"",filename,"\",\"COM:\",",startpos);
	if(uploadlen > 0) //= 0: upload all data of file
	{
		sprintf(cmd,"%s,%ld",cmd,uploadlen);
		if((beof == 0) || (beof == 1))
		{
			sprintf(cmd,"%s,%d",cmd,beof);
		}
	}
	strcat(cmd,"\r");
	Sim_send(cmd,strlen(cmd));
	ret = Sim_checkResponseWith("CONNECT", 2, 30000);
	__NOP();
	if(ret == RET_OK)
	{
		Sim_send(datatoput,strlen(datatoput));
		if((beof != 0) && (beof != 1))
		{
			Sim_send("+++",strlen("+++")); //terminal process
		}
		ret = Sim_checkResponseWith("+QFTPPUT: ", 2, 30000);
		__NOP();
		if(ret == RET_OK)
		{
			uint16_t code = Sim_getErrorCode("+QFTPPUT:", ",");
			if(code != 0)
			{
				ret = RET_FAIL;
				#if (SIM_DEBUG == 1)
					printf("Error: FTP UPLOAD failed - %d. ",Sim_getErrorCode(",", NULL));
					printf("Error Codes - %d\r\n",code);
				#endif
			}
		}
		else
		{
			#if (SIM_DEBUG == 1)
				printf("Error: FTP UPLOAD failed- %d (+CME ERROR)\r\n",Sim_getErrorCode("+CME ERROR:", NULL));
			#endif
		}
	}
	return ret;
}

/**
  * @brief  Send SMS.
  * @param  phonenumb is recipient's phone number
  * @param  datatosend is message content.
  * @retval RET_FAIL if send failed.
  * 		RET_OK if send successful.
  */
STATUS SMS_sendMessage(char* phonenumb, char* datatosend)
{
	char cmd[50] = {0};
	sprintf(cmd,"%s%s%s","AT+CMGS=\"",phonenumb,"\"\r");
	Sim_send(phonenumb, strlen(cmd));
	ret = Sim_checkResponseWith(">", 2, 5000);
	__NOP();
	if(ret == RET_OK)
	{
		Sim_send(datatosend, strlen(datatosend));
		Sim_send("\x1a", 1);
		ret = Sim_checkResponseWith("+CMGS: ", 2, 5000);
		__NOP();
		if(ret != RET_OK)
		{
			#if (SIM_DEBUG == 1)
				printf("Error: SMS send failed - %d \r\n",Sim_getErrorCode("+CMS ERROR:", NULL));
			#endif
		}
	}
	return ret;
}

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

STATUS ret;

/**
  * @brief  Rx Transfer completed callbacks.
  * @param  huart  Pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart->Instance == SIM_USART)
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
uint32_t strstrFromStart(char* userdata, uint32_t endpos)
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
uint32_t strstrFromEnd(char* userdata, uint32_t startpos)
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
void subString(char* maindata, char* subdata, uint32_t startpos, uint32_t endpos)
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

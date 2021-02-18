#include "HTTPClient_STM32.h"

uint8_t HTTP_DataToGet[MAX_RECVBUF_LEN] = {0};
uint8_t FTP_DataToSend[MAX_RECVBUF_LEN/2] = "";


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

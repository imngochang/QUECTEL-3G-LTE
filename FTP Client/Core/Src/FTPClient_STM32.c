/*
 * FTPClient_STM32.c
 *
 *  Created on: Feb 18, 2021
 *      Author: ngochang
 */

#include "FTPClient_STM32.h"

uint8_t FTP_DataToGet[FTP_MAX_DOWNLOADLEN+1] = {0};

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



#include <stdio.h> 
#include <ctype.h> 
#include <string.h>
#include <limits.h>
#include <stdarg.h>
#include "stdio_private.h"
#include "socket.h"
#include "ftpc.h"
#include "mmcHandler.h"

/* FTP Responses */
#define R_150	150		/* File status ok; opening data conn */
#define R_200	200		/* 'Generic' command ok */
#define R_220	220		/* Service ready for new user. */
#define R_226	226		/* Closing data connection.  File transfer/abort successful */
#define R_227	227		/* Entering passive mode (h1,h2,h3,h4,p1,p2) */
#define R_230	230		/* User logged in, proceed */
#define R_331	331		/* User name okay, need password. */

#define TransferAscii		0x41
#define TransferBinary		0x49
#define ConnectActive		1
#define ConnectPassive		0

un_l2cval remote_ip;
uint16_t  remote_port;
un_l2cval local_ip;
uint16_t  local_port;
uint8_t connect_state_control = 0;
uint8_t connect_state_data = 0;
uint8_t gModeActivePassiveflag = 0;
struct ftpd ftp;

enum CommandFirst {
	f_nocmd,
	f_dir,
	f_put,
	f_get,
};
enum CommandSecond {
	s_nocmd,
	s_dir,
	s_put,
	s_get,
};
struct Command {
	enum CommandFirst First;
	enum CommandSecond Second;
};

struct Command Command;

int current_year = 2014;
int current_month = 12;
int current_day = 31;
int current_hour = 10;
int current_min = 10;
int current_sec = 30;

uint8_t gMenuStart = 0;
uint8_t gDataSockReady = 0;
uint8_t gDataPutGetStart = 0;
uint8_t gModeAsciiBinary = 0;
uint8_t gModeActivePassive = ConnectPassive;

static uint8_t gMsgBuf[20]={0,};

// For FTP client examples; destination network info
uint8_t FTP_destip[4] = {192, 168, 0, 230};
uint16_t FTP_destport = 21;
extern int Board_UARTGetCharBlocking(void);

uint8_t* User_Keyboard_MSG()
{
	uint8_t i=0;
	do{
		gMsgBuf[i] = Board_UARTGetCharBlocking();
		i++;
	}while(gMsgBuf[i-1]!=0x0d);
	gMsgBuf[i-1]=0;
	return gMsgBuf;
}

int fsprintf(uint8_t s, const char *format, ...)
{
	int i;
/*
	char buf[LINELEN];
	FILE f;
	va_list ap;

	f.flags = __SWR | __SSTR;
	f.buf = buf;
	f.size = INT_MAX;
	va_start(ap, format);
	i = vfprintf(&f, format, ap);
	va_end(ap);
	buf[f.len] = 0;

	send(s, (uint8_t *)buf, strlen(buf));
*/
	return i;
}

void ftpd_init(uint8_t * src_ip)
{
	ftp.state = FTPS_NOT_LOGIN;
	ftp.dsock_mode = ACTIVE_MODE;
	local_ip.cVal[0] = src_ip[0];
	local_ip.cVal[1] = src_ip[1];
	local_ip.cVal[2] = src_ip[2];
	local_ip.cVal[3] = src_ip[3];
	local_port = 35000;
	strcpy(ftp.workingdir, "/");
	socket(CTRL_SOCK, Sn_MR_TCP, IPPORT_FTP, 0x0);
}

uint8_t ftpd_run(uint8_t * dbuf)
{
	uint16_t size = 0;
	long ret = 0;
	uint32_t send_byte, recv_byte;
	uint32_t blocklen;
	uint32_t remain_filesize;
	uint32_t remain_datasize;
	uint8_t msg_c;
	uint8_t dat[50]={0,};
	uint32_t totalSize = 0, availableSize = 0;

    switch(getSn_SR(CTRL_SOCK))
    {
    	case SOCK_ESTABLISHED :
    		if(!connect_state_control){
    			printf("%d:FTP Connected\r\n", CTRL_SOCK);
    			strcpy(ftp.workingdir, "/");
    			connect_state_control = 1;
    		}
    		if(gMenuStart){
				gMenuStart = 0;
				printf("\r\n----------------------------------------\r\n");
				printf("Press menu key\r\n");
				printf("----------------------------------------\r\n");
				printf("1> View FTP Server Directory\r\n");
				printf("2> View My Directory\r\n");
				printf("3> Sets the type of file to be transferred. Current state : %s\r\n", (gModeAsciiBinary==TransferAscii)?"Ascii":"Binary");
				printf("4> Sets Data Connection. Current state : %s\r\n", (gModeActivePassive==ConnectActive)?"Active":"Passive");
				printf("5> Put File to Server\r\n");
				printf("6> Get File from Server\r\n");
				printf("7> Delete My File\r\n");
				printf("----------------------------------------\r\n");
				while(1){
					msg_c=Board_UARTGetCharBlocking();
					if(msg_c=='1'){
						if(gModeActivePassive==ConnectPassive){
							sprintf(dat,"PASV\r\n");
							send(CTRL_SOCK, (uint8_t *)dat, strlen(dat));
							Command.First = f_dir;
							break;
						}
						else{
							wiz_NetInfo gWIZNETINFO;
							ctlnetwork(CN_GET_NETINFO, (void*) &gWIZNETINFO);
							sprintf(dat,"PORT %d,%d,%d,%d,%d,%d\r\n", gWIZNETINFO.ip[0], gWIZNETINFO.ip[1], gWIZNETINFO.ip[2], gWIZNETINFO.ip[3], (uint8_t)(local_port>>8), (uint8_t)(local_port&0x00ff));
							send(CTRL_SOCK, (uint8_t *)dat, strlen(dat));
							Command.First = f_dir;

							gModeActivePassiveflag = 1;
							break;
						}
						break;
					}
					else if(msg_c=='5'){
						if(gModeActivePassive==ConnectPassive){
							sprintf(dat,"PASV\r\n");
							send(CTRL_SOCK, (uint8_t *)dat, strlen(dat));
							Command.First = f_put;
							break;
						}
						else{
							wiz_NetInfo gWIZNETINFO;
							ctlnetwork(CN_GET_NETINFO, (void*) &gWIZNETINFO);
							sprintf(dat,"PORT %d,%d,%d,%d,%d,%d\r\n", gWIZNETINFO.ip[0], gWIZNETINFO.ip[1], gWIZNETINFO.ip[2], gWIZNETINFO.ip[3], (uint8_t)(local_port>>8), (uint8_t)(local_port&0x00ff));
							send(CTRL_SOCK, (uint8_t *)dat, strlen(dat));
							Command.First = f_put;

							gModeActivePassiveflag = 1;
							break;
						}
					}
					else if(msg_c=='6'){
						if(gModeActivePassive==ConnectPassive){
							sprintf(dat,"PASV\r\n");
							send(CTRL_SOCK, (uint8_t *)dat, strlen(dat));
							Command.First = f_get;
							break;
						}
						else{
							wiz_NetInfo gWIZNETINFO;
							ctlnetwork(CN_GET_NETINFO, (void*) &gWIZNETINFO);
							sprintf(dat,"PORT %d,%d,%d,%d,%d,%d\r\n", gWIZNETINFO.ip[0], gWIZNETINFO.ip[1], gWIZNETINFO.ip[2], gWIZNETINFO.ip[3], (uint8_t)(local_port>>8), (uint8_t)(local_port&0x00ff));
							send(CTRL_SOCK, (uint8_t *)dat, strlen(dat));
							Command.First = f_get;

							gModeActivePassiveflag = 1;
							break;
						}
					}
					else if(msg_c=='2'){
						scan_files(ftp.workingdir, dbuf, (int *)&size);
						printf("\r\n%s\r\n", dbuf);
						getMountedMemorySize(SPI_FLASHM, &totalSize, &availableSize);
						printf(" - Available Memory Size : %ld kB / %ld kB ( %ld kB is used )\r\n", availableSize, totalSize, (totalSize - availableSize));
						gMenuStart = 1;
						break;
					}
					else if(msg_c=='3'){
						printf("1> ASCII\r\n");
						printf("2> BINARY\r\n");
						while(1){
							msg_c=Board_UARTGetCharBlocking();
							if(msg_c=='1'){
								sprintf(dat,"TYPE %c\r\n", TransferAscii);
								gModeAsciiBinary = TransferAscii;
								send(CTRL_SOCK, (uint8_t *)dat, strlen(dat));
								break;
							}
							else if(msg_c=='2'){
								sprintf(dat,"TYPE %c\r\n", TransferBinary);
								gModeAsciiBinary = TransferBinary;
								send(CTRL_SOCK, (uint8_t *)dat, strlen(dat));
								break;
							}
							else{
								printf("\r\nRetry...\r\n");
							}
						}
						break;
					}
					else if(msg_c=='4'){
						printf("1> ACTIVE\r\n");
						printf("2> PASSIVE\r\n");
						while(1){
							msg_c=Board_UARTGetCharBlocking();
							if(msg_c=='1'){
								gModeActivePassive = ConnectActive;
								break;
							}
							else if(msg_c=='2'){
								gModeActivePassive = ConnectPassive;
								break;
							}
							else{
								printf("\r\nRetry...\r\n");
							}
						}
						gMenuStart = 1;
						break;
					}
					else if(msg_c=='7'){
						printf(">del filename?");
						sprintf(ftp.filename, "/%s\r\n", User_Keyboard_MSG());
						if (f_unlink((const char *)ftp.filename) != 0){
							printf("\r\nCould not delete.\r\n");
						}
						else{
							printf("\r\nDeleted.\r\n");
						}
						gMenuStart = 1;
						break;
					}
					else{
						printf("\r\nRetry...\r\n");
					}
				}
			}
			if(gDataSockReady){
				gDataSockReady = 0;
				switch(Command.First){
					case f_dir:
						sprintf(dat,"LIST\r\n");
						send(CTRL_SOCK, (uint8_t *)dat, strlen(dat));
						break;
					case f_put:
						printf(">put filename?");
						sprintf(dat,"STOR %s\r\n", User_Keyboard_MSG());
						send(CTRL_SOCK, (uint8_t *)dat, strlen(dat));
						break;
					case f_get:
						printf(">get filename?");
						sprintf(dat,"RETR %s\r\n", User_Keyboard_MSG());
						send(CTRL_SOCK, (uint8_t *)dat, strlen(dat));
						break;
					default:
						printf("shit!!!! Command.First = default\r\n");
						break;
				}
			}
    		if((size = getSn_RX_RSR(CTRL_SOCK)) > 0){ // Don't need to check SOCKERR_BUSY because it doesn't not occur.
    			memset(dbuf, 0, _MAX_SS);
    			if(size > _MAX_SS) size = _MAX_SS - 1;
    			ret = recv(CTRL_SOCK,dbuf,size);
    			dbuf[ret] = '\0';
    			if(ret != size)
    			{
    				if(ret==SOCK_BUSY) return 0;
    				if(ret < 0){
    					printf("%d:recv() error:%ld\r\n",CTRL_SOCK,ret);
    					close(CTRL_SOCK);
    					return ret;
    				}
    			}
    			printf("Rcvd Command: %s\r\n", dbuf);
    			proc_ftpd((char *)dbuf);
    		}
    		break;
    	case SOCK_CLOSE_WAIT :
    		printf("%d:CloseWait\r\n",CTRL_SOCK);
    		if((ret=disconnect(CTRL_SOCK)) != SOCK_OK) return ret;
    		printf("%d:Closed\r\n",CTRL_SOCK);
    		break;
    	case SOCK_CLOSED :
    		printf("%d:FTPStart\r\n",CTRL_SOCK);
    		if((ret=socket(CTRL_SOCK, Sn_MR_TCP, IPPORT_FTP, 0x0)) != CTRL_SOCK){
    			printf("%d:socket() error:%ld\r\n", CTRL_SOCK, ret);
    			close(CTRL_SOCK);
    			return ret;
    		}
    		break;
    	case SOCK_INIT :
    		printf("%d:Opened\r\n",CTRL_SOCK);
			if((ret = connect(CTRL_SOCK, FTP_destip, FTP_destport)) != SOCK_OK){
				printf("%d:Connect error\r\n",CTRL_SOCK);
				return ret;
			}
			connect_state_control = 0;
			printf("%d:Connectting...\r\n",CTRL_SOCK);
			break;
    	default :
    		break;
    }

    switch(getSn_SR(DATA_SOCK)){
    	case SOCK_ESTABLISHED :
    		if(!connect_state_data){
    			printf("%d:FTP Data socket Connected\r\n", DATA_SOCK);
    			connect_state_data = 1;
    			//gDataSockReady = 1;
    		}
			if(gDataPutGetStart){
				switch(Command.Second){
				case s_dir:
					printf("waitng...\r\n");
					if((size = getSn_RX_RSR(DATA_SOCK)) > 0){ // Don't need to check SOCKERR_BUSY because it doesn't not occur.
						printf("ok\r\n");
						memset(dbuf, 0, _MAX_SS);
						if(size > _MAX_SS) size = _MAX_SS - 1;
						ret = recv(DATA_SOCK,dbuf,size);
						dbuf[ret] = '\0';
						if(ret != size){
							if(ret==SOCK_BUSY) return 0;
							if(ret < 0){
								printf("%d:recv() error:%ld\r\n",CTRL_SOCK,ret);
								close(DATA_SOCK);
								return ret;
							}
						}
						printf("Rcvd Data:\n\r%s\n\r", dbuf);
						gDataPutGetStart = 0;
					}
					break;
				case s_put:
					printf("waitng...\r\n");
					if(strlen(ftp.workingdir) == 1)
						sprintf(ftp.filename, "/%s", (uint8_t *)gMsgBuf);
					else
						sprintf(ftp.filename, "%s/%s", ftp.workingdir, (uint8_t *)gMsgBuf);
					ftp.fr = f_open(&(ftp.fil), (const char *)ftp.filename, FA_READ);
					if(ftp.fr == FR_OK){
						remain_filesize = ftp.fil.fsize;
						printf("f_open return FR_OK\r\n");
						do{
							memset(dbuf, 0, _MAX_SS);
							if(remain_filesize > _MAX_SS)
								send_byte = _MAX_SS;
							else
								send_byte = remain_filesize;
							ftp.fr = f_read(&(ftp.fil), (void *)dbuf, send_byte , (UINT *)&blocklen);
							if(ftp.fr != FR_OK){
								break;
							}
							printf("#");
							send(DATA_SOCK, dbuf, blocklen);
							remain_filesize -= blocklen;
						}while(remain_filesize != 0);
						printf("\r\nFile read finished\r\n");
						ftp.fr = f_close(&(ftp.fil));
						gDataPutGetStart = 0;
						disconnect(DATA_SOCK);
					}
					else{
						printf("File Open Error: %d\r\n", ftp.fr);
						ftp.fr = f_close(&(ftp.fil));
						gDataPutGetStart = 0;
						disconnect(DATA_SOCK);
					}
					break;
				case s_get:
					printf("waitng...\r\n");
					if(strlen(ftp.workingdir) == 1)
						sprintf(ftp.filename, "/%s", (uint8_t *)gMsgBuf);
					else
						sprintf(ftp.filename, "%s/%s", ftp.workingdir, (uint8_t *)gMsgBuf);
					ftp.fr = f_open(&(ftp.fil), (const char *)ftp.filename, FA_CREATE_ALWAYS | FA_WRITE);
					if(ftp.fr == FR_OK){
						printf("f_open return FR_OK\r\n");
						while(1){
							if((remain_datasize = getSn_RX_RSR(DATA_SOCK)) > 0){
								while(1){
									memset(dbuf, 0, _MAX_SS);
									if(remain_datasize > _MAX_SS)	recv_byte = _MAX_SS;
									else	recv_byte = remain_datasize;
									ret = recv(DATA_SOCK, dbuf, recv_byte);
									ftp.fr = f_write(&(ftp.fil), (const void *)dbuf, (UINT)ret, (UINT *)&blocklen);
									remain_datasize -= blocklen;
									if(ftp.fr != FR_OK){
										printf("f_write failed\r\n");
										break;
									}
									if(remain_datasize <= 0)	break;
								}
								if(ftp.fr != FR_OK){
									printf("f_write failed\r\n");
									break;
								}
								printf("#");
							}
							else{
								if(getSn_SR(DATA_SOCK) != SOCK_ESTABLISHED)	break;
							}
						}
						printf("\r\nFile write finished\r\n");
						ftp.fr = f_close(&(ftp.fil));
						gDataPutGetStart = 0;
					}else{
						printf("File Open Error: %d\r\n", ftp.fr);
					}
					break;
				default:
					printf("shit!!!! Command.Second = default\r\n");
					break;
				}
			}
    		break;
   		case SOCK_CLOSE_WAIT :
   			printf("%d:CloseWait\r\n",DATA_SOCK);
   			if((ret=disconnect(DATA_SOCK)) != SOCK_OK) return ret;
   			printf("%d:Closed\r\n",DATA_SOCK);
   			break;
   		case SOCK_CLOSED :
   			if(ftp.dsock_state == DATASOCK_READY){
   				if(ftp.dsock_mode == PASSIVE_MODE){
   					printf("%d:FTPDataStart, port : %d\r\n",DATA_SOCK, local_port);
   					if((ret=socket(DATA_SOCK, Sn_MR_TCP, local_port, 0x0)) != DATA_SOCK){
   						printf("%d:socket() error:%ld\r\n", DATA_SOCK, ret);
   						close(DATA_SOCK);
   						return ret;
   					}
   					local_port++;
   					if(local_port > 50000)
   						local_port = 35000;
   				}else{
   					printf("%d:FTPDataStart, port : %d\r\n",DATA_SOCK, local_port);
   					if((ret=socket(DATA_SOCK, Sn_MR_TCP, local_port, 0x0)) != DATA_SOCK){
   						printf("%d:socket() error:%ld\r\n", DATA_SOCK, ret);
   						close(DATA_SOCK);
   						return ret;
   					}
   					local_port++;
   					if(local_port > 50000)
   						local_port = 35000;
   				}
   				ftp.dsock_state = DATASOCK_START;
   			}
   			break;

   		case SOCK_INIT :
   			printf("%d:Opened\r\n",DATA_SOCK);
   			if(ftp.dsock_mode == ACTIVE_MODE){
   				if( (ret = listen(DATA_SOCK)) != SOCK_OK){
   					printf("%d:Listen error\r\n",DATA_SOCK);
   					return ret;
   				}
   				gDataSockReady = 1;
   				printf("%d:Listen ok\r\n",DATA_SOCK);
   			}else{
   				if((ret = connect(DATA_SOCK, remote_ip.cVal, remote_port)) != SOCK_OK){
   					printf("%d:Connect error\r\n", DATA_SOCK);
   					return ret;
   				}
   				gDataSockReady = 1;
   			}
   			connect_state_data = 0;
   			break;
   		default :
   			break;
    }
    return 0;
}

char proc_ftpd(char * buf)
{
	uint16_t Responses;
	uint8_t dat[30]={0,};

	Responses =(buf[0]-'0')*100+(buf[1]-'0')*10+(buf[2]-'0');

	switch(Responses){
		case R_220:
			printf("\r\nInput your User ID > ");
			sprintf(dat,"USER %s\r\n", User_Keyboard_MSG());
			printf("\r\n");
			send(CTRL_SOCK, (uint8_t *)dat, strlen(dat));
			break;
		case R_331:
			printf("\r\nInput your Password > ");
			sprintf(dat,"PASS %s\r\n", User_Keyboard_MSG());
			printf("\r\n");
			send(CTRL_SOCK, (uint8_t *)dat, strlen(dat));
			break;
		case R_230:
			printf("\r\nUser logged in, proceed\r\n");
			sprintf(dat,"TYPE %c\r\n", TransferAscii);
			gModeAsciiBinary = TransferAscii;
			send(CTRL_SOCK, (uint8_t *)dat, strlen(dat));
			break;
		case R_200:
			if((gModeActivePassive==ConnectActive)&&gModeActivePassiveflag){
				ftp.dsock_mode = ACTIVE_MODE;
				ftp.dsock_state = DATASOCK_READY;
				gModeActivePassiveflag = 0;
			}
			else{
				gMenuStart = 1;
			}

			break;
		case R_150:
			switch(Command.First){
			case f_dir:
				Command.Second = s_dir;
				gDataPutGetStart = 1;
				break;
			case f_get:
				Command.Second = s_get;
				gDataPutGetStart = 1;
				break;
			case f_put:
				Command.Second = s_put;
				gDataPutGetStart = 1;
				break;
			default :
				printf("shit!!!! Command.First = default\r\n");
				break;
			}
			break;
		case R_226:					/* Closing data connection.  File transfer/abort successful */
			gMenuStart = 1;
			break;
		case R_227:
			if (pport(buf) == -1){
				printf("Bad port syntax\r\n");
			}
			else{
				printf("Go Open Data Sock...\r\n ");
				ftp.dsock_mode = PASSIVE_MODE;
				ftp.dsock_state = DATASOCK_READY;
			}
			break;
		default:
			printf("\r\nDefault Status = %d\r\n",(uint16_t)Responses);
			gDataSockReady = 1;
			break;
		}
	return 1;
}
int pport(char * arg)
{
	int i;
	char* tok=0;
	strtok(arg,"(");
	for (i = 0; i < 4; i++)
	{
		if(i==0) tok = strtok(NULL,",\r\n");
		else	 tok = strtok(NULL,",");
		remote_ip.cVal[i] = (uint8_t)atoi(tok, 10);
		if (!tok){
			printf("bad pport : %s\r\n", arg);
			return -1;
		}
	}
	remote_port = 0;
	for (i = 0; i < 2; i++){
		tok = strtok(NULL,",\r\n");
		remote_port <<= 8;
		remote_port += atoi(tok, 10);
		if (!tok){
			printf("bad pport : %s\r\n", arg);
			return -1;
		}
	}
	printf("ip : %d.%d.%d.%d, port : %d\r\n", remote_ip.cVal[0], remote_ip.cVal[1], remote_ip.cVal[2], remote_ip.cVal[3], remote_port);
	return 0;
}

#if defined(F_FILESYSTEM)
void print_filedsc(FIL *fil)
{
#if defined(_FTP_DEBUG_)
	printf("File System pointer : %08X\r\n", fil->fs);
	printf("File System mount ID : %d\r\n", fil->id);
	printf("File status flag : %08X\r\n", fil->flag);
	printf("File System pads : %08X\r\n", fil->err);
	printf("File read write pointer : %08X\r\n", fil->fptr);
	printf("File size : %08X\r\n", fil->fsize);
	printf("File start cluster : %08X\r\n", fil->sclust);
	printf("current cluster : %08X\r\n", fil->clust);
	printf("current data sector : %08X\r\n", fil->dsect);
	printf("dir entry sector : %08X\r\n", fil->dir_sect);
	printf("dir entry pointer : %08X\r\n", fil->dir_ptr);
#endif
}
#endif

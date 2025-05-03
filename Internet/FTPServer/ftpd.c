/*
* Wiznet.
* (c) Copyright 2002, Wiznet.
*
* Filename	: ftpd.c
* Version	: 1.0
* Programmer(s)	: 
* Created	: 2003/01/28
* Description   : FTP daemon. (AVR-GCC Compiler)
*/


#include <stdio.h> 
#include <ctype.h> 
#include <string.h>
#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include "socket.h"
#include "ftpd.h"

/* If you need this header, use it. */
//#include "stdio_private.h"

/* Command table */
static char *commands[] = {
	"user",
	"acct",
	"pass",
	"type",
	"list",
	"cwd",
	"dele",
	"name",
	"quit",
	"retr",
	"stor",
	"port",
	"nlst",
	"pwd",
	"xpwd",
	"mkd",
	"xmkd",
	"xrmd",
	"rmd ",
	"stru",
	"mode",
	"syst",
	"xmd5",
	"xcwd",
	"feat",
	"pasv",
	"size",
	"mlsd",
	"appe",
	NULL
};

#if 0
/* Response messages */
char banner[] = "220 %s FTP version %s ready.\r\n";
char badcmd[] = "500 Unknown command '%s'\r\n";
char binwarn[] = "100 Warning: type is ASCII and %s appears to be binary\r\n";
char unsupp[] = "500 Unsupported command or option\r\n";
char givepass[] = "331 Enter PASS command\r\n";
char logged[] = "230 Logged in\r\n";
char typeok[] = "200 Type %s OK\r\n";
char only8[] = "501 Only logical bytesize 8 supported\r\n";
char deleok[] = "250 File deleted\r\n";
char mkdok[] = "200 MKD ok\r\n";
char delefail[] = "550 Delete failed: %s\r\n";
char pwdmsg[] = "257 \"%s\" is current directory\r\n";
char badtype[] = "501 Unknown type \"%s\"\r\n";
char badport[] = "501 Bad port syntax\r\n";
char unimp[] = "502 Command does not implemented yet.\r\n";
char bye[] = "221 Goodbye!\r\n";
char nodir[] = "553 Can't read directory \"%s\": %s\r\n";
char cantopen[] = "550 Can't read file \"%s\": %s\r\n";
char sending[] = "150 Opening data connection for %s (%d.%d.%d.%d,%d)\r\n";
char cantmake[] = "553 Can't create \"%s\": %s\r\n";
char writerr[] = "552 Write error: %s\r\n";
char portok[] = "200 PORT command successful.\r\n";
char rxok[] = "226 Transfer complete.\r\n";
char txok[] = "226 Transfer complete.\r\n";
char noperm[] = "550 Permission denied\r\n";
char noconn[] = "425 Data connection reset\r\n";
char lowmem[] = "421 System overloaded, try again later\r\n";
char notlog[] = "530 Please log in with USER and PASS\r\n";
char userfirst[] = "503 Login with USER first.\r\n";
char okay[] = "200 Ok\r\n";
char syst[] = "215 %s Type: L%d Version: %s\r\n";
char sizefail[] = "550 File not found\r\n";
#endif
#define remain_time 400000
#define connect_timeout_en 1
#define ftp_ID 		"wiznet"
#define ftp_PW 		"wiznet54321"

#if 1
// 20231017 taylor
un_l2cval ftp_remote_ip;
uint16_t  ftp_remote_port;
#else
un_l2cval remote_ip;
uint16_t  remote_port;
#endif
un_l2cval local_ip;
uint16_t  local_port;
uint8_t connect_state_control = 0;
uint8_t connect_state_control1 = 0;
uint8_t connect_state_data = 0;
uint8_t connect_count = 0;
uint32_t con_remain_cnt1 = 0;
uint32_t con_remain_cnt2 = 0;
uint8_t cur_sn = CTRL_SOCK;
struct ftpd ftp;

int current_year = 2014;
int current_month = 12;
int current_day = 31;
int current_hour = 10;
int current_min = 10;
int current_sec = 30;

#if defined(F_FILESYSTEM)
/* List contents of a directory
 * Taken from: http://elm-chan.org/fsw/ff/doc/readdir.html*/
FRESULT scan_files (char* path, char* dbuf, int* size)
{
    FRESULT res;
    DIR dir;
    UINT i;
    static FILINFO fno;
    unsigned int year = 0;
	char month, day = 0b00000000;
    char hour, min, second = 0b00000000;

    res = f_opendir(&dir, path); /* Open the directory */
    if(res != FR_OK)
    {
    	printf("Failed to open \"%s\". (%u)\n", path, res);
    	return res;
    }
    if (res == FR_OK)
    {
        for (;;)
        {
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
            if (fno.fattrib & AM_DIR)					   /* It is a directory*/
            {
            	i = strlen((char*)dbuf);
            	year = (fno.fdate >> 9) + 1980;			   /*Year origin from 1980 (0..127)*/
            	month = (fno.fdate >> 5) & 15;			   /*Month (1..12)*/
            	day = fno.fdate & 31;					   /*Day (1..31)*/
            	/*==================*/
            	hour = (fno.ftime >> 11) & 31;			   /*Hour (0..23)*/
            	min = (fno.ftime >> 5) & 63;			   /*Minute (0..59)*/
            	second = fno.ftime & 31;				   /*Second / 2 (0..29)*/
            	*size = sprintf(&dbuf[i], "type=dir;modify=%u%02d%02d%02d%02d%02d; %s/%s\r\n", year, month, day, hour, min, second, path, fno.fname);
            }
            else 										   /* It is a file. */
            {
                i = strlen((char*)dbuf);
                year = (fno.fdate >> 9) + 1980;
                month = (fno.fdate >> 5) & 15;
                day = fno.fdate & 31;
                //==================
                hour = (fno.ftime >> 11) & 31;
                min = (fno.ftime >> 5) & 63;
                second = fno.ftime & 31;
                *size = sprintf(&dbuf[i], "type=file;size=%lu;modify=%u%02d%02d%02d%02d%02d; %s/%s\r\n", fno.fsize, year, month, day, hour, min, second, path, fno.fname);
            }
        }
        f_closedir(&dir);
    }
    return res;
}


/* Return Filesize in unit BYTES, if it's DIRECTORY return 0 */
int get_filesize(char* path)
{
	FIL file_s;
	FSIZE_t	o_size;
	FRESULT res;

	res = f_open(&file_s, path, FA_READ);
	if(res != FR_OK) 					 /*File not found*/
	{
	    DIR dir;
	    res = f_opendir(&dir, path);     /* Is it Directory? */
	        if(res != FR_OK)
	        {
	        	printf("Failed to Open DIR \"%s\". (%u)\n\r", path, res);
	        	return 0;                /*It's not directory as well, return 0 for SIZE_CMD error handler*/
	        }
	        if (res == FR_OK)
	        {

				printf("Open as DIR \"%s\". (%u)\n\r", path, res);
				f_closedir(&dir);
		    	return 0;				 /*return 0 for SIZE_CMD error handler*/
	        }
	}
	o_size = file_s.obj.objsize;
	if(f_close(&file_s) != FR_OK)
	{
				printf("Failed to close \"%s\". (%u)\n\r", path, res);
				return 0;				/*return 0 for SIZE_CMD error handler*/
	}
	return o_size;
}
#endif

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
	ftp.current_cmd = NO_CMD;
	ftp.dsock_mode = ACTIVE_MODE;

	ftp.ID_Enable = STATUS_USED;
	ftp.PW_Enable = STATUS_USED;

	if(ftp.ID_Enable == STATUS_USED)
	{
		strcpy(ftp.username, ftp_ID);
		printf(" FTP ID[%d]:%s \r\n", strlen(ftp.username), ftp.username);
	}
	if(ftp.PW_Enable == STATUS_USED)
	{
		strcpy(ftp.userpassword, ftp_PW);
		printf(" FTP PW[%d]:%s \r\n", strlen(ftp.userpassword), ftp.userpassword);
	}
	


	local_ip.cVal[0] = src_ip[0];
	local_ip.cVal[1] = src_ip[1];
	local_ip.cVal[2] = src_ip[2];
	local_ip.cVal[3] = src_ip[3];
	local_port = 35000;
	
	strcpy(ftp.workingdir, "/");

	socket(CTRL_SOCK, Sn_MR_TCP, IPPORT_FTP, 0x0);
    socket(CTRL_SOCK1, Sn_MR_TCP, IPPORT_FTP, 0x0);
}

uint8_t ftpd_run(uint8_t * dbuf)
{
	uint16_t size = 0, i;
	long ret = 0;
	uint32_t blocklen, send_byte, recv_byte;
	uint32_t remain_filesize;
	int32_t remain_datasize;
	//static uint32_t con_remain_cnt1 = 0;
	//static uint32_t con_remain_cnt2 = 0;
#if defined(F_FILESYSTEM)
	//FILINFO fno;
#endif

	//memset(dbuf, 0, sizeof(_MAX_SS));
	////////////////////////FTP Control 1
	#if 1
    switch(getSn_SR(CTRL_SOCK))
    {
    	case SOCK_ESTABLISHED :
    		if(!connect_state_control)
    		{
#if defined(_FTP_DEBUG_)
    			printf("%d:FTP Connected\r\n", CTRL_SOCK);
#endif
    			//fsprintf(CTRL_SOCK, banner, HOSTNAME, VERSION);
    			strcpy(ftp.workingdir, "/");
    			sprintf((char *)dbuf, "220 %s FTP version %s ready.\r\n", HOSTNAME, VERSION);
    			ret = send(CTRL_SOCK, (uint8_t *)dbuf, strlen((const char *)dbuf));
                
#if defined(_FTP_DEBUG_)
                printf("%d:send() [%s]\r\n",CTRL_SOCK,dbuf);
#endif
    			if(ret < 0)
    			{
#if defined(_FTP_DEBUG_)
    				printf("%d:send() error:%ld\r\n",CTRL_SOCK,ret);
#endif
    				close(CTRL_SOCK);
    				return ret;
    			}
    			connect_state_control = 1;
    		}
			#if connect_timeout_en
			else
			{
				if(con_remain_cnt1 > remain_time)
				{
					if((ret=disconnect(CTRL_SOCK)) != SOCK_OK) return ret;
			#if defined(_FTP_DEBUG_)
						printf("%d:Timeout Closed\r\n",CTRL_SOCK);
			#endif
				}
				#if defined(_FTP_DEBUG_)
				else if(((con_remain_cnt1 % 10000) == 0) && (con_remain_cnt1 != 0))
				{
					printf("%d:Timeout Count:%ld\r\n", CTRL_SOCK, con_remain_cnt1);
				}
				#endif
				con_remain_cnt1++;
			}
			#endif
	
#if defined(_FTP_DEBUG_)
    		//printf("ftp socket %d\r\n", CTRL_SOCK);
#endif
			 
    		if((size = getSn_RX_RSR(CTRL_SOCK)) > 0) // Don't need to check SOCKERR_BUSY because it doesn't not occur.
    		{
#if defined(_FTP_DEBUG_)
    			printf("%d:size: %d\r\n", CTRL_SOCK, size);
#endif

    			memset(dbuf, 0, _MAX_SS);

    			if(size > _MAX_SS) size = _MAX_SS - 1;

    			ret = recv(CTRL_SOCK,dbuf,size);
    			dbuf[ret] = '\0';
    			if(ret != size)
    			{
    				if(ret==SOCK_BUSY) return 0;
    				if(ret < 0)
    				{
#if defined(_FTP_DEBUG_)
    					printf("%d:recv() error:%ld\r\n",CTRL_SOCK,ret);
#endif
    					close(CTRL_SOCK);
    					return ret;
    				}
    			}
#if defined(_FTP_DEBUG_)
    			printf("%d:Rcvd Command: %s", CTRL_SOCK, dbuf);
#endif
    			proc_ftpd(CTRL_SOCK, (char *)dbuf);
				con_remain_cnt1 = 0;
    		}
    		break;

    	case SOCK_CLOSE_WAIT :
#if defined(_FTP_DEBUG_)
    		printf("%d:CloseWait\r\n",CTRL_SOCK);
#endif
    		if((ret=disconnect(CTRL_SOCK)) != SOCK_OK) return ret;
#if defined(_FTP_DEBUG_)
    		printf("%d:Closed\r\n",CTRL_SOCK);
#endif
    		break;

    	case SOCK_CLOSED :
#if defined(_FTP_DEBUG_)
    		printf("%d:FTPStart\r\n",CTRL_SOCK);
#endif
    		if((ret=socket(CTRL_SOCK, Sn_MR_TCP, IPPORT_FTP, 0x0)) != CTRL_SOCK)
    		{
#if defined(_FTP_DEBUG_)
    			printf("%d:socket() error:%ld\r\n", CTRL_SOCK, ret);
#endif
    			close(CTRL_SOCK);
    			return ret;
    		}
    		break;

    	case SOCK_INIT :
#if defined(_FTP_DEBUG_)
    		printf("%d:Opened\r\n",CTRL_SOCK);
#endif
    		//strcpy(ftp.workingdir, "/");
    		if( (ret = listen(CTRL_SOCK)) != SOCK_OK)
    		{
#if defined(_FTP_DEBUG_)
    			printf("%d:Listen error\r\n",CTRL_SOCK);
#endif
    			return ret;
    		}
			connect_state_control = 0;
			con_remain_cnt1 = 0;

#if defined(_FTP_DEBUG_)
			printf("%d:Listen ok\r\n",CTRL_SOCK);
#endif
			break;

    	default :
    		break;
    }
	////////////////////////FTP Control 2
	
	switch(getSn_SR(CTRL_SOCK1))
	{
		case SOCK_ESTABLISHED :
			if(!connect_state_control1)
			{
#if defined(_FTP_DEBUG_)
				printf("%d:FTP Connected\r\n", CTRL_SOCK1);
#endif
				strcpy(ftp.workingdir, "/");
				sprintf((char *)dbuf, "220 %s FTP version %s ready.\r\n", HOSTNAME, VERSION);
				ret = send(CTRL_SOCK1, (uint8_t *)dbuf, strlen((const char *)dbuf));
				
#if defined(_FTP_DEBUG_)
				printf("%d:send() [%s]\r\n",CTRL_SOCK1,dbuf);
#endif
				if(ret < 0)
				{
#if defined(_FTP_DEBUG_)
					printf("%d:send() error:%ld\r\n",CTRL_SOCK1,ret);
#endif
					close(CTRL_SOCK1);
					return ret;
				}
				connect_state_control1 = 1;
			}
			#if connect_timeout_en
			else
			{
				if(con_remain_cnt2 > remain_time)
				{
					if((ret=disconnect(CTRL_SOCK1)) != SOCK_OK) return ret;
			#if defined(_FTP_DEBUG_)
						printf("%d:Timeout Closed\r\n",CTRL_SOCK1);
			#endif
				}
				#if defined(_FTP_DEBUG_)
				else if(((con_remain_cnt2 % 10000) == 0) && (con_remain_cnt2 != 0))
				{
					printf("%d:Timeout Count:%ld\r\n", CTRL_SOCK1, con_remain_cnt2);
				}
				#endif
				con_remain_cnt2++;
			}
			#endif
	
#if defined(_FTP_DEBUG_)
			//printf("ftp socket %d\r\n", CTRL_SOCK);
#endif
			
			if((size = getSn_RX_RSR(CTRL_SOCK1)) > 0) // Don't need to check SOCKERR_BUSY because it doesn't not occur.
			{
#if defined(_FTP_DEBUG_)
				printf("%d: size: %d\r\n", CTRL_SOCK1, size);
#endif

				memset(dbuf, 0, _MAX_SS);

				if(size > _MAX_SS) size = _MAX_SS - 1;

				ret = recv(CTRL_SOCK1,dbuf,size);
				dbuf[ret] = '\0';
				if(ret != size)
				{
					if(ret==SOCK_BUSY) return 0;
					if(ret < 0)
					{
#if defined(_FTP_DEBUG_)
						printf("%d:recv() error:%ld\r\n",CTRL_SOCK1,ret);
#endif
						close(CTRL_SOCK1);
						return ret;
					}
				}
#if defined(_FTP_DEBUG_)
				printf("%d: Rcvd Command: %s", CTRL_SOCK1, dbuf);
#endif
				proc_ftpd(CTRL_SOCK1, (char *)dbuf);
				con_remain_cnt2 = 0;
			}
			break;

		case SOCK_CLOSE_WAIT :
#if defined(_FTP_DEBUG_)
			printf("%d:CloseWait\r\n",CTRL_SOCK1);
#endif
			if((ret=disconnect(CTRL_SOCK1)) != SOCK_OK) return ret;
			connect_count--;
#if defined(_FTP_DEBUG_)
			printf("%d:Closed\r\n",CTRL_SOCK1);
#endif
			break;

		case SOCK_CLOSED :
#if defined(_FTP_DEBUG_)
			printf("%d:FTPStart\r\n",CTRL_SOCK1);
#endif
			if((ret=socket(CTRL_SOCK1, Sn_MR_TCP, IPPORT_FTP, 0x0)) != CTRL_SOCK1)
			{
#if defined(_FTP_DEBUG_)
				printf("%d:socket() error:%ld\r\n", CTRL_SOCK1, ret);
#endif
				close(CTRL_SOCK1);
				return ret;
			}
			break;

		case SOCK_INIT :
#if defined(_FTP_DEBUG_)
			printf("%d:Opened\r\n",CTRL_SOCK1);
#endif
			//strcpy(ftp.workingdir, "/");
			if( (ret = listen(CTRL_SOCK1)) != SOCK_OK)
			{
#if defined(_FTP_DEBUG_)
				printf("%d:Listen error\r\n",CTRL_SOCK1);
#endif
				return ret;
			}
			connect_state_control1 = 0;
			con_remain_cnt2 = 0;
#if defined(_FTP_DEBUG_)
			printf("%d:Listen ok\r\n",CTRL_SOCK1);
#endif
			break;

		default :
			break;
	}
    
	#endif
/////////////////////////////////// ftp data part
#if 1
    switch(getSn_SR(DATA_SOCK))
    {
    	case SOCK_ESTABLISHED :
    		if(!connect_state_data)
    		{
#if defined(_FTP_DEBUG_)
    			printf("%d:FTP Data socket Connected\r\n", DATA_SOCK);
#endif
    			connect_state_data = 1;
    		}
	
    		switch(ftp.current_cmd)
    		{
    			case LIST_CMD:
    			case MLSD_CMD:
#if defined(_FTP_DEBUG_)
    				printf("previous size: %d\r\n", size);
#endif
#if defined(F_FILESYSTEM)
    				scan_files(ftp.workingdir, dbuf, (int *)&size);
#endif
#if defined(_FTP_DEBUG_)
    				printf("returned size: %d\r\n", size);
    				printf("%s\r\n", dbuf);
#endif
#if !defined(F_FILESYSTEM)
    				if (strncmp(ftp.workingdir, "/$Recycle.Bin", sizeof("/$Recycle.Bin")) != 0)
    					size = sprintf(dbuf, "drwxr-xr-x 1 ftp ftp 0 Dec 31 2014 $Recycle.Bin\r\n-rwxr-xr-x 1 ftp ftp 512 Dec 31 2014 test.txt\r\n");
#endif
    				size = strlen(dbuf);
    				send(DATA_SOCK, dbuf, size);
    				ftp.current_cmd = NO_CMD;
    				disconnect(DATA_SOCK);
    				size = sprintf(dbuf, "226 Successfully transferred \"%s\"\r\n", ftp.workingdir);
					send(cur_sn, dbuf, size);
    				break;

    			case RETR_CMD:
#if defined(_FTP_DEBUG_)
    				printf("filename to retrieve : %s %d\r\n", ftp.filename, strlen(ftp.filename));
#endif
#if defined(F_FILESYSTEM)
    				ftp.fr = f_open(&(ftp.fil), (const char *)ftp.filename, FA_READ);
    				//print_filedsc(&(ftp.fil));
    				if(ftp.fr == FR_OK){
    					remain_filesize = ftp.fil.obj.objsize;
#if defined(_FTP_DEBUG_)
    					printf("f_open return FR_OK\r\n");
#endif
    					do{
#if defined(_FTP_DEBUG_)
    						//printf("remained file size: %d\r\n", ftp.fil.fsize);
#endif
    						memset(dbuf, 0, _MAX_SS);

    						if(remain_filesize > _MAX_SS)
    							send_byte = _MAX_SS;
    						else
    							send_byte = remain_filesize;

    						ftp.fr = f_read(&(ftp.fil), dbuf, send_byte , &blocklen);
    						if(ftp.fr != FR_OK)
    							break;
#if defined(_FTP_DEBUG_)
    						printf("#");
    						//printf("----->fsize:%d recv:%d len:%d \r\n", remain_filesize, send_byte, blocklen);
    						//printf("----->fn:%s data:%s \r\n", ftp.filename, dbuf);
#endif
    						send(DATA_SOCK, dbuf, blocklen);
    						remain_filesize -= blocklen;
    					}while(remain_filesize != 0);
#if defined(_FTP_DEBUG_)
    					printf("\r\nFile read finished\r\n");
#endif
    					ftp.fr = f_close(&(ftp.fil));
    				}else{
#if defined(_FTP_DEBUG_)
    					printf("File Open Error: %d\r\n", ftp.fr);
#endif
    				}
#else
					remain_filesize = strlen(ftp.filename);
					#if defined(_FTP_DEBUG_)
                    printf("<<<<<1recv data[%d]\r\n", remain_datasize);
					#endif
					do{
						memset(dbuf, 0, _MAX_SS);

						blocklen = sprintf(dbuf, "%s", ftp.filename);

						printf("####1[%d] dbuf:%s\r\n",remain_filesize ,dbuf);

						send(DATA_SOCK, dbuf, blocklen);
						remain_filesize -= blocklen;
					}while(remain_filesize != 0);

#endif
    				ftp.current_cmd = NO_CMD;
    				disconnect(DATA_SOCK);
    				size = sprintf(dbuf, "226 Successfully transferred \"%s\"\r\n", ftp.filename);
					send(cur_sn, dbuf, size);
    				break;

    			case STOR_CMD:
#if defined(_FTP_DEBUG_)
    				printf("filename to store : %s %d\r\n", ftp.filename, strlen(ftp.filename));
#endif
#if defined(F_FILESYSTEM)
    				ftp.fr = f_open(&(ftp.fil), (const char *)ftp.filename, FA_CREATE_ALWAYS | FA_WRITE);
    				//print_filedsc(&(ftp.fil));
    				if(ftp.fr == FR_OK){
#if defined(_FTP_DEBUG_)
    					printf("f_open return FR_OK\r\n");
#endif
    					while(1){
    						if((remain_datasize = getSn_RX_RSR(DATA_SOCK)) > 0){
    							while(1){
    								memset(dbuf, 0, _MAX_SS);

    								if(remain_datasize > _MAX_SS)
    									recv_byte = _MAX_SS;
    								else
    									recv_byte = remain_datasize;

    								ret = recv(DATA_SOCK, dbuf, recv_byte);
#if defined(_FTP_DEBUG_)
    								//printf("----->fn:%s data:%s \r\n", ftp.filename, dbuf);
#endif

    								ftp.fr = f_write(&(ftp.fil), dbuf, (UINT)ret, &blocklen);
#if defined(_FTP_DEBUG_)
    								//printf("----->dsize:%d recv:%d len:%d \r\n", remain_datasize, ret, blocklen);
#endif
    								remain_datasize -= blocklen;

    								if(ftp.fr != FR_OK){
#if defined(_FTP_DEBUG_)
    									printf("f_write failed\r\n");
#endif
    									break;
    								}

    								if(remain_datasize <= 0)
    									break;
    							}

    							if(ftp.fr != FR_OK){
#if defined(_FTP_DEBUG_)
    								printf("f_write failed\r\n");
#endif
    								break;
    							}

#if defined(_FTP_DEBUG_)
    							printf("#");
#endif
    						}else{
    							if(getSn_SR(DATA_SOCK) != SOCK_ESTABLISHED)
    								break;
    						}
    					}
#if defined(_FTP_DEBUG_)
    					printf("\r\nFile write finished\r\n");
#endif
    					ftp.fr = f_close(&(ftp.fil));
    				}else{
#if defined(_FTP_DEBUG_)
    					printf("File Open Error: %d\r\n", ftp.fr);
#endif
    				}

    				//fno.fdate = (WORD)(((current_year - 1980) << 9) | (current_month << 5) | current_day);
    				//fno.ftime = (WORD)((current_hour << 11) | (current_min << 5) | (current_sec >> 1));
    				//f_utime((const char *)ftp.filename, &fno);
#else
					while(1){
						if((remain_datasize = getSn_RX_RSR(DATA_SOCK)) > 0){
							#if defined(_FTP_DEBUG_)
                            printf("<<<<<2recv data[%ld]\r\n", remain_datasize);
							#endif
							while(1){
								memset(dbuf, 0, _MAX_SS);

								if(remain_datasize > _MAX_SS)
									recv_byte = _MAX_SS;
								else
									recv_byte = remain_datasize;

								ret = recv(DATA_SOCK, dbuf, recv_byte);
								if(ret < 0)
                                {
									#if defined(_FTP_DEBUG_)
                                    printf("finish ret[%d\r\n", ret);
									#endif
                                    break;
                                }

								#if defined(_FTP_DEBUG_)
								printf("#####2[%ld] dbuf:%s\r\n", remain_datasize, dbuf);
								#endif

								remain_datasize -= ret;
								#if defined(_FTP_DEBUG_)
                                printf("###ret:%ld,remain:%d\r\n",ret, remain_datasize);
								#endif

								if(remain_datasize <= 0)
									break;
							}
						}else{
							if(getSn_SR(DATA_SOCK) != SOCK_ESTABLISHED)
								break;
						}
					}
#endif
    				ftp.current_cmd = NO_CMD;
    				disconnect(DATA_SOCK);
    				size = sprintf(dbuf, "226 Successfully transferred \"%s\"\r\n", ftp.filename);
					send(cur_sn, dbuf, size);
    				break;

    			case NO_CMD:
    			default:
    				break;
    		}
    		break;

   		case SOCK_CLOSE_WAIT :
#if defined(_FTP_DEBUG_)
   			printf("%d:CloseWait\r\n",DATA_SOCK);
#endif
   			if((ret=disconnect(DATA_SOCK)) != SOCK_OK) return ret;
#if defined(_FTP_DEBUG_)
   			printf("%d:Closed\r\n",DATA_SOCK);
#endif
   			break;

   		case SOCK_CLOSED :
   			if(ftp.dsock_state == DATASOCK_READY)
   			{
   				if(ftp.dsock_mode == PASSIVE_MODE){
#if defined(_FTP_DEBUG_)
   					printf("%d:FTPDataStart, port : %d\r\n",DATA_SOCK, local_port);
#endif
   					if((ret=socket(DATA_SOCK, Sn_MR_TCP, local_port, 0x0)) != DATA_SOCK)
   					{
#if defined(_FTP_DEBUG_)
   						printf("%d:socket() error:%ld\r\n", DATA_SOCK, ret);
#endif
   						close(DATA_SOCK);
   						return ret;
   					}

   					local_port++;
   					if(local_port > 50000)
   						local_port = 35000;
   				}else{
#if defined(_FTP_DEBUG_)
   					printf("%d:FTPDataStart, port : %d\r\n",DATA_SOCK, IPPORT_FTPD);
#endif
   					if((ret=socket(DATA_SOCK, Sn_MR_TCP, IPPORT_FTPD, 0x0)) != DATA_SOCK)
   					{
#if defined(_FTP_DEBUG_)
   						printf("%d:socket() error:%ld\r\n", DATA_SOCK, ret);
#endif
   						close(DATA_SOCK);
   						return ret;
   					}
   				}

   				ftp.dsock_state = DATASOCK_START;
   			}
   			break;

   		case SOCK_INIT :
#if defined(_FTP_DEBUG_)
   			printf("%d:Opened\r\n",DATA_SOCK);
#endif
   			if(ftp.dsock_mode == PASSIVE_MODE){
   				if( (ret = listen(DATA_SOCK)) != SOCK_OK)
   				{
#if defined(_FTP_DEBUG_)
   					printf("%d:Listen error\r\n",DATA_SOCK);
#endif
   					return ret;
   				}

#if defined(_FTP_DEBUG_)
   				printf("%d:Listen ok\r\n",DATA_SOCK);
#endif
   			}else{
#if 1
   				// 20231016 taylor//teddy 240122
#if ((_WIZCHIP_ == 6100) || (_WIZCHIP_ == 6300))
   				if((ret = connect(DATA_SOCK, ftp_remote_ip.cVal, ftp_remote_port, 4)) != SOCK_OK){
#else
   				if((ret = connect(DATA_SOCK, ftp_remote_ip.cVal, ftp_remote_port)) != SOCK_OK){
#endif
#else
   				if((ret = connect(DATA_SOCK, remote_ip.cVal, remote_port)) != SOCK_OK){
#endif
#if defined(_FTP_DEBUG_)
   					printf("%d:Connect error\r\n", DATA_SOCK);
#endif
   					return ret;
   				}
   			}
   			connect_state_data = 0;
   			break;

   		default :
   			break;
    }
#endif

    return 0;
}

char proc_ftpd(uint8_t sn, char * buf)
{
	char **cmdp, *cp, *arg, *tmpstr;
	char sendbuf[200];
	int slen;
	long ret;
	

	/* Translate first word to lower case */
	for (cp = buf; *cp != ' ' && *cp != '\0'; cp++)
		*cp = tolower(*cp);

	/* Find command in table; if not present, return syntax error */
	for (cmdp = commands; *cmdp != NULL; cmdp++)
		if (strncmp(*cmdp, buf, strlen(*cmdp)) == 0)
			break;

	if (*cmdp == NULL)
	{
		//fsprintf(CTRL_SOCK, badcmd, buf);
		slen = sprintf(sendbuf, "500 Unknown command '%s'\r\n", buf);
		send(sn, (uint8_t *)sendbuf, slen);
		return 0;
	}
	/* Allow only USER, PASS and QUIT before logging in */
	if (ftp.state == FTPS_NOT_LOGIN)
	{
		switch(cmdp - commands)
		{
			case USER_CMD:
			case PASS_CMD:
			case QUIT_CMD:
				break;
			default:
				//fsprintf(CTRL_SOCK, notlog);
				slen = sprintf(sendbuf, "530 Please log in with USER and PASS\r\n");
				send(sn, (uint8_t *)sendbuf, slen);
				return 0;
		}
	}
	
	arg = &buf[strlen(*cmdp)];
	while(*arg == ' ') arg++;

	/* Execute specific command */
	switch (cmdp - commands)
	{
		case USER_CMD :
#if defined(_FTP_DEBUG_)
			printf("USER_CMD : %s", arg);
#endif
			slen = strlen(arg);
			arg[slen - 1] = 0x00;
			arg[slen - 2] = 0x00;
			if(ftp.ID_Enable == STATUS_USED)
			{
				if(strcmp(ftp.username, arg) != 0)
				{
					slen = sprintf(sendbuf, "430 Invalid username\r\n");
					ret = send(sn, (uint8_t *)sendbuf, slen);
					if(ret < 0)
					{
		#if defined(_FTP_DEBUG_)
						printf("%d:send() error:%ld\r\n",sn,ret);
		#endif
						close(sn);
						return ret;
					}
					break;
				}
			}
			else
			{
				strcpy(ftp.username, arg);
			}
			//fsprintf(CTRL_SOCK, givepass);
			slen = sprintf(sendbuf, "331 Enter PASS command\r\n");
			ret = send(sn, (uint8_t *)sendbuf, slen);
			if(ret < 0)
			{
#if defined(_FTP_DEBUG_)
				printf("%d:send() error:%ld\r\n",sn,ret);
#endif
				close(sn);
				return ret;
			}
			break;

		case PASS_CMD :
#if defined(_FTP_DEBUG_)
			printf("PASS_CMD : %s", arg);
#endif
			slen = strlen(arg);
			arg[slen - 1] = 0x00;
			arg[slen - 2] = 0x00;
			if(ftp.PW_Enable == STATUS_USED)
			{
				if(strcmp(ftp.userpassword, arg) != 0)
				{
					slen = sprintf(sendbuf, "430 Invalid password\r\n");
					ret = send(sn, (uint8_t *)sendbuf, slen);
					if(ret < 0)
					{
		#if defined(_FTP_DEBUG_)
						printf("%d:send() error:%ld\r\n",sn,ret);
		#endif
						close(sn);
						return ret;
					}
					break;
				}
			}
			ftplogin(sn, arg);
			break;

		case TYPE_CMD :
			slen = strlen(arg);
			arg[slen - 1] = 0x00;
			arg[slen - 2] = 0x00;
			switch(arg[0])
			{
				case 'A':
				case 'a':	/* Ascii */
					ftp.type = ASCII_TYPE;
					//fsprintf(CTRL_SOCK, typeok, arg);
					slen = sprintf(sendbuf, "200 Type set to %s\r\n", arg);
					send(sn, (uint8_t *)sendbuf, slen);
					break;

				case 'B':
				case 'b':	/* Binary */
				case 'I':
				case 'i':	/* Image */
					ftp.type = IMAGE_TYPE;
					//fsprintf(CTRL_SOCK, typeok, arg);
					slen = sprintf(sendbuf, "200 Type set to %s\r\n", arg);
					send(sn, (uint8_t *)sendbuf, slen);
					break;

				default:	/* Invalid */
					//fsprintf(CTRL_SOCK, badtype, arg);
					slen = sprintf(sendbuf, "501 Unknown type \"%s\"\r\n", arg);
					send(sn, (uint8_t *)sendbuf, slen);
					break;
			}
			break;

		case FEAT_CMD :
			slen = sprintf(sendbuf, "211-Features:\r\n MDTM\r\n REST STREAM\r\n SIZE\r\n MLST size*;type*;create*;modify*;\r\n MLSD\r\n UTF8\r\n CLNT\r\n MFMT\r\n211 END\r\n");
			send(sn, (uint8_t *)sendbuf, slen);
			break;

		case QUIT_CMD :
#if defined(_FTP_DEBUG_)
			printf("QUIT_CMD\r\n");
#endif
			//fsprintf(CTRL_SOCK, bye);
			slen = sprintf(sendbuf, "221 Goodbye!\r\n");
			send(sn, (uint8_t *)sendbuf, slen);
			disconnect(sn);
			break;

		case RETR_CMD :
			slen = strlen(arg);
			arg[slen - 1] = 0x00;
			arg[slen - 2] = 0x00;
#if defined(_FTP_DEBUG_)
			printf("RETR_CMD\r\n");
#endif
			if(strlen(ftp.workingdir) == 1)
				sprintf(ftp.filename, "/%s", arg);
			else
				sprintf(ftp.filename, "%s/%s", ftp.workingdir, arg);
			slen = sprintf(sendbuf, "150 Opening data channel for file downloand from server of \"%s\"\r\n", ftp.filename);
			send(sn, (uint8_t *)sendbuf, slen);
			ftp.current_cmd = RETR_CMD;
			break;

		case APPE_CMD :
		case STOR_CMD:
			slen = strlen(arg);
			arg[slen - 1] = 0x00;
			arg[slen - 2] = 0x00;
#if defined(_FTP_DEBUG_)
			printf("STOR_CMD\r\n");
#endif
			if(strlen(ftp.workingdir) == 1)
				sprintf(ftp.filename, "/%s", arg);
			else
				sprintf(ftp.filename, "%s/%s", ftp.workingdir, arg);
			slen = sprintf(sendbuf, "150 Opening data channel for file upload to server of \"%s\"\r\n", ftp.filename);
			send(sn, (uint8_t *)sendbuf, slen);
			ftp.current_cmd = STOR_CMD;
			if(ftp.dsock_mode == ACTIVE_MODE)
			{
#if 1
				// 20231016 taylor//teddy 240122
#if ((_WIZCHIP_ == 6100) || (_WIZCHIP_ == 6300))
				if((ret = connect(DATA_SOCK, ftp_remote_ip.cVal, ftp_remote_port, 4)) != SOCK_OK){
#else
				if((ret = connect(DATA_SOCK, ftp_remote_ip.cVal, ftp_remote_port)) != SOCK_OK){
#endif
#else
				if((ret = connect(DATA_SOCK, remote_ip.cVal, remote_port)) != SOCK_OK){
#endif
	#if defined(_FTP_DEBUG_)
					printf("%d:Connect error\r\n", DATA_SOCK);
	#endif
					return ret;
				}
			}
   			connect_state_data = 0;
			break;

		case PORT_CMD:
#if defined(_FTP_DEBUG_)
			printf("PORT_CMD\r\n");
#endif
			if (pport(arg) == -1){
				//fsprintf(CTRL_SOCK, badport);
				slen = sprintf(sendbuf, "501 Bad port syntax\r\n");
				send(sn, (uint8_t *)sendbuf, slen);
			} else{
				//fsprintf(CTRL_SOCK, portok);
				ftp.dsock_mode = ACTIVE_MODE;
				ftp.dsock_state = DATASOCK_READY;
				slen = sprintf(sendbuf, "200 PORT command successful.\r\n");
				send(sn, (uint8_t *)sendbuf, slen);
			}
			break;

		case MLSD_CMD:
#if defined(_FTP_DEBUG_)
			printf("MLSD_CMD\r\n");
#endif
			slen = sprintf(sendbuf, "150 Opening data channel for directory listing of \"%s\"\r\n", ftp.workingdir);
			send(sn, (uint8_t *)sendbuf, slen);
			ftp.current_cmd = MLSD_CMD;
			break;

		case LIST_CMD:
#if defined(_FTP_DEBUG_)
			printf("LIST_CMD\r\n");
#endif
			slen = sprintf(sendbuf, "150 Opening data channel for directory listing of \"%s\"\r\n", ftp.workingdir);
			send(sn, (uint8_t *)sendbuf, slen);
			ftp.current_cmd = LIST_CMD;
			break;

		case NLST_CMD:
#if defined(_FTP_DEBUG_)
			printf("NLST_CMD\r\n");
#endif
			break;

		case SYST_CMD:
			slen = sprintf(sendbuf, "215 UNIX emulated by WIZnet\r\n");
			send(sn, (uint8_t *)sendbuf, slen);
			break;

		case PWD_CMD:
		case XPWD_CMD:
			slen = sprintf(sendbuf, "257 \"%s\" is current directory.\r\n", ftp.workingdir);
			send(sn, (uint8_t *)sendbuf, slen);
			break;

		case PASV_CMD:
			slen = sprintf(sendbuf, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)\r\n", local_ip.cVal[0], local_ip.cVal[1], local_ip.cVal[2], local_ip.cVal[3], local_port >> 8, local_port & 0x00ff);
			send(sn, (uint8_t *)sendbuf, slen);

			if(getSn_SR(DATA_SOCK) == SOCK_ESTABLISHED) 
			{
#if defined(_FTP_DEBUG_)
				printf("data disconnect: %d\r\n", DATA_SOCK);
#endif			
				disconnect(DATA_SOCK);
			}
			ftp.dsock_mode = PASSIVE_MODE;
			ftp.dsock_state = DATASOCK_READY;
			cur_sn = sn;
#if defined(_FTP_DEBUG_)
			printf("PASV port: %d\r\n", local_port);
#endif
		break;

		case SIZE_CMD:
			slen = strlen(arg);
			arg[slen - 1] = 0x00;
			arg[slen - 2] = 0x00;
			if(slen > 3)
			{
				tmpstr = strrchr(arg, '/');
				//*tmpstr = 0;
#if defined(F_FILESYSTEM)
				slen = (int) get_filesize(arg);
#else
				slen = _MAX_SS;
#endif
				if(slen > 0)
					slen = sprintf(sendbuf, "213 %d\r\n", slen);
				else
					slen = sprintf(sendbuf, "550 File not Found\r\n");
			}
			else
			{
				slen = sprintf(sendbuf, "550 File not Found\r\n");
			}
			send(sn, (uint8_t *)sendbuf, slen);
			break;

		case CWD_CMD:
			slen = strlen(arg);
			arg[slen - 1] = 0x00;
			arg[slen - 2] = 0x00;
			if(slen > 3)
			{
				//arg[slen - 3] = 0x00;
				tmpstr = strrchr(arg, '/');
				//*tmpstr = 0;
#if defined(F_FILESYSTEM)
				slen = get_filesize(arg);			/* FATFS Cannot get DIR size; return 0*/
#else
				slen = 0;
#endif
				*tmpstr = '/';
				if(slen == 0){
					slen = sprintf(sendbuf, "213 %d\r\n", slen);
					strcpy(ftp.workingdir, arg);
					slen = sprintf(sendbuf, "250 CWD successful. \"%s\" is current directory.\r\n", ftp.workingdir);
				}
				else
				{
					slen = sprintf(sendbuf, "550 CWD failed. \"%s\"\r\n", arg);
				}
			}
			else
			{
				strcpy(ftp.workingdir, arg);
				slen = sprintf(sendbuf, "250 CWD successful. \"%s\" is current directory.\r\n", ftp.workingdir);
			}
			send(sn, (uint8_t *)sendbuf, slen);
			break;

		case MKD_CMD:
		case XMKD_CMD:
			slen = strlen(arg);
			arg[slen - 1] = 0x00;
			arg[slen - 2] = 0x00;
#if defined(F_FILESYSTEM)
			if (f_mkdir(arg) != 0)
			{
				slen = sprintf(sendbuf, "550 Can't create directory. \"%s\"\r\n", arg);
			}
			else
			{
				slen = sprintf(sendbuf, "257 MKD command successful. \"%s\"\r\n", arg);
				//strcpy(ftp.workingdir, arg);
			}
#else
			slen = sprintf(sendbuf, "550 Can't create directory. Permission denied\r\n");
#endif
			send(sn, (uint8_t *)sendbuf, slen);
			break;

		case DELE_CMD:
			slen = strlen(arg);
			arg[slen - 1] = 0x00;
			arg[slen - 2] = 0x00;
#if defined(F_FILESYSTEM)
			if (f_unlink(arg) != 0)
			{
				slen = sprintf(sendbuf, "550 Could not delete. \"%s\"\r\n", arg);
			}
			else
			{
				slen = sprintf(sendbuf, "250 Deleted. \"%s\"\r\n", arg);
			}
#else
			slen = sprintf(sendbuf, "550 Could not delete. Permission denied\r\n");
#endif
			send(sn, (uint8_t *)sendbuf, slen);
			break;

		case XCWD_CMD:
		case ACCT_CMD:
		case XRMD_CMD:
		case RMD_CMD:
		case STRU_CMD:
		case MODE_CMD:
		case XMD5_CMD:
			//fsprintf(CTRL_SOCK, unimp);
			slen = sprintf(sendbuf, "502 Command does not implemented yet.\r\n");
			send(sn, (uint8_t *)sendbuf, slen);
			break;

		default:	/* Invalid */
			//fsprintf(CTRL_SOCK, badcmd, arg);
			slen = sprintf(sendbuf, "500 Unknown command \'%s\'\r\n", arg);
			send(sn, (uint8_t *)sendbuf, slen);
			break;
	}
	
	return 1;
}


char ftplogin(uint8_t sn, char * pass)
{
	char sendbuf[100];
	int slen = 0;
	
	//memset(sendbuf, 0, DATA_BUF_SIZE);
	
#if defined(_FTP_DEBUG_)
	printf("%s logged in\r\n", ftp.username);
#endif
	//fsprintf(CTRL_SOCK, logged);
	slen = sprintf(sendbuf, "230 Logged on\r\n");
	send(sn, (uint8_t *)sendbuf, slen);
	ftp.state = FTPS_LOGIN;
	
	return 1;
}

int pport(char * arg)
{
	int i;
	char* tok=0;

	for (i = 0; i < 4; i++)
	{
		if(i==0) tok = strtok(arg,",\r\n");
		else	 tok = strtok(NULL,",");
#if 1
		// 20231017 taylor
		ftp_remote_ip.cVal[i] = (uint8_t)atoi(tok);
#else
		remote_ip.cVal[i] = (uint8_t)atoi(tok);
#endif
		if (!tok)
		{
#if defined(_FTP_DEBUG_)
			printf("bad pport : %s\r\n", arg);
#endif
			return -1;
		}
	}
#if 1
	// 20231017 taylor
	ftp_remote_port = 0;
#else
	remote_port = 0;
#endif
	for (i = 0; i < 2; i++)
	{
		tok = strtok(NULL,",\r\n");
#if 1
		// 20231017 taylor
		ftp_remote_port <<= 8;
		ftp_remote_port += atoi(tok);
#else
		remote_port <<= 8;
		remote_port += atoi(tok);
#endif
		if (!tok)
		{
#if defined(_FTP_DEBUG_)
			printf("bad pport : %s\r\n", arg);
#endif
			return -1;
		}
	}
#if defined(_FTP_DEBUG_)
#if 1
	// 20231017 taylor
	printf("ip : %d.%d.%d.%d, port : %d\r\n", ftp_remote_ip.cVal[0], ftp_remote_ip.cVal[1], ftp_remote_ip.cVal[2], ftp_remote_ip.cVal[3], ftp_remote_port);
#else
	printf("ip : %d.%d.%d.%d, port : %d\r\n", remote_ip.cVal[0], remote_ip.cVal[1], remote_ip.cVal[2], remote_ip.cVal[3], remote_port);
#endif
#endif

	return 0;
}

#if defined(F_FILESYSTEM)
void print_filedsc(FIL *fil)
{
#if defined(_FTP_DEBUG_)
	printf("File System pointer : %08X\r\n", fil->obj.fs);
	printf("File System mount ID : %d\r\n", fil->obj.id);
	printf("File status flag : %08X\r\n", fil->flag);
	printf("File System pads : %08X\r\n", fil->err);
	printf("File read write pointer : %08X\r\n", fil->fptr);
	printf("File size : %08X\r\n", fil->obj.objsize);
	printf("File start cluster : %08X\r\n", fil->obj.sclust);
	printf("current cluster : %08X\r\n", fil->clust);
	printf("current data sector : %08X\r\n", fil->sect);
	printf("dir entry sector : %08X\r\n", fil->dir_sect);
	printf("dir entry pointer : %08X\r\n", fil->dir_ptr);
#endif
}
#endif

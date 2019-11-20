#ifndef _FTPD_H_
#define _FTPD_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
* Wiznet.
* (c) Copyright 2002, Wiznet.
*
* Filename	: ftpd.h
* Version	: 1.0
* Programmer(s)	: 
* Created	: 2003/01/28
* Description   : Header file of FTP daemon. (AVR-GCC Compiler)
*/

#include <stdint.h>

//#define F_FILESYSTEM // If your target support a file system, you have to activate this feature and implement.

#if defined(F_FILESYSTEM)
//#include "ff.h"
#endif

#define F_APP_FTP
#define _FTP_DEBUG_


#define _FTP_FILENAME_LEN_		100
//#define DATA_BUF_SIZE	100
#if !defined(F_FILESYSTEM)
#define _MAX_SS 		512
#endif
#define SOCK_MAX_NUM 4

#define	IPPORT_FTPD	20	/* FTP Data port */
#define	IPPORT_FTP	21	/* FTP Control port */

#define HOSTNAME	"iinChip"
#define VERSION		"1.0"

#define FILENAME	"a.txt"

/* FTP commands */
enum ftp_cmd {
	USER_CMD,
	ACCT_CMD,
	PASS_CMD,
	TYPE_CMD,
	LIST_CMD,
	CWD_CMD,
	DELE_CMD,
	NAME_CMD,
	QUIT_CMD,
	RETR_CMD,
	STOR_CMD,
	PORT_CMD,
	NLST_CMD,
	PWD_CMD,
	XPWD_CMD,
	MKD_CMD,
	XMKD_CMD,
	XRMD_CMD,
	RMD_CMD,
	STRU_CMD,
	MODE_CMD,
	SYST_CMD,
	XMD5_CMD,
	XCWD_CMD,
	FEAT_CMD,
	PASV_CMD,
	SIZE_CMD,
	MLSD_CMD,
	APPE_CMD,
	NO_CMD,
};


struct ftpd {
	uint8_t ctrl_sock;			/* Control Socket */
	uint8_t data_sock;			/* Data Socket */
	char user[20];				/* FTP Server user */
	char pass[20];				/* FTP Server password */
	char username[_FTP_FILENAME_LEN_];		/* Arg to USER command */
	char workingdir[_FTP_FILENAME_LEN_];
	char filename[_FTP_FILENAME_LEN_];
	uint8_t     is_login;		/* Login 1: login, 0: Not login */
	uint8_t     is_ascii;       /* Transfer Type 1 : ASCII, 0 : Binary */
	uint8_t     is_active;      /* FTP Mode 1 : Active, 0: Passive */

#if defined(F_FILESYSTEM)
	FIL fil;	// FatFs File objects
	FRESULT fr;	// FatFs function common result code
#endif

};

void ftpd_init(char* user, char*pass);
uint8_t ftpd_run(uint8_t * dbuf);

int sendit(char * command);
int recvit(char * command);

long sendfile(uint8_t s, char * command);
long recvfile(uint8_t s);

#if defined(F_FILESYSTEM)
void print_filedsc(FIL *fil);
#endif

#ifdef __cplusplus
}
#endif

#endif // _FTPD_H_

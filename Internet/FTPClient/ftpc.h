#ifndef _FTPC_H_
#define _FTPC_H_


#include <stdint.h>

#define F_FILESYSTEM // If your target support a file system, you have to activate this feature and implement.

#if defined(F_FILESYSTEM)
#include "ff.h"
#endif

#define F_APP_FTP
#define _FTP_DEBUG_
#define F_APP_FTP_CLIENT

#define LINELEN		100
//#define DATA_BUF_SIZE	100
#if !defined(F_FILESYSTEM)
#define _MAX_SS		512
#endif

#define CTRL_SOCK	2
#define DATA_SOCK	3

#define	IPPORT_FTPD	20	/* FTP Data port */
#define	IPPORT_FTP	21	/* FTP Control port */

#define HOSTNAME	"iinChip"
#define VERSION		"1.0"

#define FILENAME	"a.txt"

enum ftp_type {
	ASCII_TYPE,
	IMAGE_TYPE,
	LOGICAL_TYPE
};

enum ftp_state {
	FTPS_NOT_LOGIN,
	FTPS_LOGIN
};

enum datasock_state{
	DATASOCK_IDLE,
	DATASOCK_READY,
	DATASOCK_START
};

enum datasock_mode{
	PASSIVE_MODE,
	ACTIVE_MODE
};

struct ftpd {
	uint8_t control;			/* Control stream */
	uint8_t data;			/* Data stream */

	enum ftp_type type;		/* Transfer type */
	enum ftp_state state;

	enum datasock_state dsock_state;
	enum datasock_mode dsock_mode;

	char username[LINELEN];		/* Arg to USER command */
	char workingdir[LINELEN];
	char filename[LINELEN];

#if defined(F_FILESYSTEM)
	FIL fil;	// FatFs File objects
	FRESULT fr;	// FatFs function common result code
#endif

};

#ifndef un_I2cval
typedef union _un_l2cval {
	uint32_t	lVal;
	uint8_t		cVal[4];
}un_l2cval;
#endif

void ftpd_init(uint8_t * src_ip);
uint8_t ftpd_run(uint8_t * dbuf);
char proc_ftpd(char * buf);
int pport(char * arg);
uint8_t* User_Keyboard_MSG();

int sendit(char * command);
int recvit(char * command);

long sendfile(uint8_t s, char * command);
long recvfile(uint8_t s);

#if defined(F_FILESYSTEM)
void print_filedsc(FIL *fil);
#endif
#endif // _FTPC_H_

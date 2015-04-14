#ifndef _FTPC_H_
#define _FTPC_H_


#include <stdint.h>

//#define F_FILESYSTEM // If your target support a file system, you have to activate this feature and implement.

#if defined(F_FILESYSTEM)
#include "ff.h"
#endif

#define F_APP_FTP

#define LINELEN		100
#if !defined(F_FILESYSTEM)
#define _MAX_SS		512
#endif

#define CTRL_SOCK	2
#define DATA_SOCK	3

enum ftpc_type {
	ASCII_TYPE,
	IMAGE_TYPE,
	LOGICAL_TYPE
};

enum ftpc_datasock_state{
	DATASOCK_IDLE,
	DATASOCK_READY,
	DATASOCK_START
};

enum ftpc_datasock_mode{
	PASSIVE_MODE,
	ACTIVE_MODE
};
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
struct ftpc {
	uint8_t control;			/* Control stream */
	uint8_t data;				/* Data stream */

	enum ftpc_type type;		/* Transfer type */

	enum ftpc_datasock_state dsock_state;
	enum ftpc_datasock_mode dsock_mode;

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

void ftpc_init(uint8_t * src_ip);
uint8_t ftpc_run(uint8_t * dbuf);
char proc_ftpc(char * buf);
int pportc(char * arg);
uint8_t* User_Keyboard_MSG();

#endif // _FTPC_H_

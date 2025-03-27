#ifndef _LOOPBACK_H_
#define _LOOPBACK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "wizchip_conf.h"

/* Loopback test debug message printout enable */
#if 0
#define	_LOOPBACK_DEBUG_
#endif

/* DATA_BUF_SIZE define for Loopback example */
#ifndef DATA_BUF_SIZE
	#define DATA_BUF_SIZE			2048
#endif

/************************/
/* Select LOOPBACK_MODE */
/************************/
#define LOOPBACK_MAIN_NOBLOCK    0
#define LOOPBACK_MODE   LOOPBACK_MAIN_NOBLOCK
int8_t set_loopback_mode_W6x00 (uint8_t get_loopback_mode ) ;
int8_t check_loopback_mode_W6x00();

int32_t loopback_tcps(uint8_t sn, uint8_t* buf, uint16_t port);

/* TCP client Loopback test example */
int32_t loopback_tcpc(uint8_t sn, uint8_t* buf, uint8_t* destip, uint16_t destport);

/* UDP Loopback test example */
int32_t loopback_udps(uint8_t sn, uint8_t* buf, uint16_t port);
//teddy 240122
int32_t iperf_tcpc(uint8_t sn, uint8_t* buf, uint8_t* destip, uint16_t destport, uint32_t size, uint16_t count);
#ifdef __cplusplus
}
#endif

#endif

/*
 * file: SSLInterface.h
 * description: wiznet network interface for mbedtls
 * author: peter
 * company: wiznet
 * data: 2015.11.26
 */

#ifndef _SSLINTERFACE_H_
#define _SSLINTERFACE_H_

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/compat-1.3.h"

#include <stdlib.h>

#define DEBUG_LEVEL 0

#define DEBUG_BUFFER_SIZE	1024

#define WIZ_RECV_TIMEOUT_VALUE      2000

/*
 * Call Back function registration
 */
#define SSLSendCB 				WIZnetSend
#define SSLRecvCB 				WIZnetRecv
#define SSLRecvTimeOutCB        WIZnetRecvTimeOut

typedef struct{
	mbedtls_ctr_drbg_context* ctr_drbg;
	mbedtls_ssl_context* ssl;
	mbedtls_ssl_config* conf;
	mbedtls_x509_crt* cacert;
}wiz_tls_context;

/*
 * name: WIZnetRecv
 * brief: WIZnet socket(recv) interface function for mbedTLS
 * param ctx: Context for callback(socket handler = w5500 socket number)
 * param buf: buffer
 * param len: number of bytes to read
 */
int WIZnetRecv(void *ctx, unsigned char *buf, unsigned int len );

/*
 * name: WIZnetRecvTimeOut
 * brief: WIZnet socket(recv) interface function for mbedTLS
 * param ctx: Context for callback(socket handler = w5500 socket number)
 * param buf: buffer
 * param len: number of bytes to read
 * param timeout: timeout value in millisecond
 */

//int WIZnetRecvTimeOut(void *ctx, unsigned char *buf, unsigned int len, unsigned int timeout);
int WIZnetRecvTimeOut(void *ctx, unsigned char *buf, size_t len, uint32_t timeout);

/*
 * name: WIZnetSend
 * brief: WIZnet socket(send) interface function for mbedTLS
 * param ctx: Context for callback(socket handler = w5500 socket number)
 * param buf: buffer
 * param len: number of bytes to write
 */
int WIZnetSend(void *ctx, const unsigned char *buf, unsigned int len );


#if defined (MBEDTLS_DEBUG_C)
/*
 * name: WIZnetDebugCB
 * brief: printf callback function for debug
 * param ctx: Context for callback - ignored
 * param level: debug level - 0/1/2/3(0 is no debug)
 * param file: file pointer - ignored
 * param line: - ignored
 * param str: debug message pointer
 */
void WIZnetDebugCB(void *ctx, int level, const char *file, int line, const char *str);
#endif

/*
 * name: wiz_tls_init
 * brief: Initialize SSL/TLS Contexts.
 * param SSL/TLS Context
 * param socket file descriptor ( socket number)
 * param host name
 * param certificate
 */
unsigned int wiz_tls_init(wiz_tls_context* tlsContext, int* socket_fd, const char * host, const char * cert);

void wiz_tls_deinit(wiz_tls_context* tlsContext);

int wiz_tls_socket(wiz_tls_context* tlsContext, unsigned int port);

int wiz_tls_connect(wiz_tls_context* tlsContext, char * addr, unsigned int port);

int wiz_tls_connect_timeout(wiz_tls_context* tlsContext, char * addr, unsigned int port, uint32_t timeout);

int wiz_tls_socket_connect(wiz_tls_context* tlsContext, char * addr, unsigned int port);

int wiz_tls_socket_connect_timeout(wiz_tls_context* tlsContext, char * addr, unsigned int port, uint32_t timeout);

unsigned int wiz_tls_read(wiz_tls_context* SSLContext, unsigned char* readbuf, unsigned int len);

unsigned int wiz_tls_write(wiz_tls_context* SSLContext, unsigned char* writebuf, unsigned int len);

int wiz_tls_disconnect(wiz_tls_context* tlsContext, uint32_t timeout);

unsigned int wiz_tls_x509_verify(wiz_tls_context* SSLContext);

unsigned int wiz_tls_close_notify(wiz_tls_context* SSLContext);

int wiz_tls_session_reset(wiz_tls_context* tlsContext);

#endif


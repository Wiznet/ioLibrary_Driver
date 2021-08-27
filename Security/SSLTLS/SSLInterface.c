/*
 * file: SSLInterface.c
 * description: mbedtls callback functions
 * author: peter
 * company: wiznet
 * data: 2015.11.26
 */
#include <stdio.h>
#include <string.h>
#include "mbedtls/x509_crt.h"
#include "mbedtls/error.h"

#include "SSLInterface.h"
#include "SSL_Random.h"
#include "socket.h"
#include "certificate.h"

unsigned char tempBuf[DEBUG_BUFFER_SIZE] = {0,};

int WIZnetRecvTimeOut(void *ctx, unsigned char *buf, size_t len, uint32_t timeout)
{
    int ret;
    uint32_t start_ms = millis();
    do
    {
        if(getSn_RX_RSR((uint8_t)ctx)){
            return recv((uint8_t)ctx, (uint8_t *)buf, (uint16_t)len);
        }
    }while((millis() - start_ms) < timeout);

    return MBEDTLS_ERR_SSL_TIMEOUT;
}

/*Shell for mbedtls recv function*/
int WIZnetRecv(void *ctx, unsigned char *buf, unsigned int len )
{
    return (recv((uint8_t)ctx, (uint8_t *)buf, (uint16_t)len));
}

/*Shell for mbedtls send function*/
int WIZnetSend(void *ctx, const unsigned char *buf, unsigned int len )
{
    return (send((uint8_t)ctx, (uint8_t *)buf, (uint16_t)len));
}

/*Shell for mbedtls debug function.
 *DEBUG_LEBEL can be changed from 0 to 3*/
#ifdef MBEDTLS_DEBUG_C
void WIZnetDebugCB(void *ctx, int level, const char *file, int line, const char *str)
{
    if(level <= DEBUG_LEVEL)
    {
       printf("%s\r\n",str);
    }
}
#endif


/* SSL context initialization
 * */
unsigned int wiz_tls_init(wiz_tls_context* tlsContext, int* socket_fd, const char * host, const char * cert)
{
	int ret = 1;
#if defined (MBEDTLS_ERROR_C)
	char error_buf[100];
#endif

#if defined (MBEDTLS_DEBUG_C)
	debug_set_threshold(DEBUG_LEVEL);
#endif

	/*
		Initialize session data
	*/
#if defined (MBEDTLS_ENTROPY_C)
	tlsContext->entropy = malloc(sizeof(mbedtls_entropy_context));
	mbedtls_entropy_init( tlsContext->entropy);
#endif

	tlsContext->ctr_drbg = malloc(sizeof(mbedtls_ctr_drbg_context));
	tlsContext->ssl = malloc(sizeof(mbedtls_ssl_context));
	tlsContext->conf = malloc(sizeof(mbedtls_ssl_config));
	tlsContext->cacert = malloc(sizeof(mbedtls_x509_crt));

	mbedtls_ctr_drbg_init(tlsContext->ctr_drbg);
	mbedtls_x509_crt_init(tlsContext->cacert);
	mbedtls_ssl_init(tlsContext->ssl);
	mbedtls_ssl_config_init(tlsContext->conf);
	/*
		Initialize certificates
	*/

#if defined (MBEDTLS_X509_CRT_PARSE_C)

#if defined (MBEDTLS_DEBUG_C)
	printf(" Loading the CA root certificate \r\n");
#endif
	mbedtls_ssl_config_defaults((tlsContext->conf),
								MBEDTLS_SSL_IS_CLIENT,
								MBEDTLS_SSL_TRANSPORT_STREAM,
								MBEDTLS_SSL_PRESET_DEFAULT);
	mbedtls_ssl_setup(tlsContext->ssl, tlsContext->conf);

	mbedtls_ssl_set_hostname(tlsContext->ssl, host);

#if defined (MBEDTLS_CERTS_C)
	ret = mbedtls_x509_crt_parse(tlsContext->cacert, (const char *)root_ca, strlen(root_ca));
#endif

#else
	ret = 1;
#if defined (MBEDTLS_DEBUG_C)
	printf("SSL_CERTS_C not define .\r\n");
#endif
#endif

	if(ret < 0)
	{
#if defined (MBEDTLS_CERTS_C)
		printf("x509_crt_parse failed.%x \r\n",ret);
#endif
		return 0;
	}
	/*
		set ssl session param
	*/
	mbedtls_ssl_conf_ca_chain(tlsContext->conf, tlsContext->cacert, NULL);
	if(host != NULL)
	{
	    mbedtls_ssl_conf_authmode(tlsContext->conf, MBEDTLS_SSL_VERIFY_REQUIRED); // This option is for server certificate verification
	}
	else
	{
	    mbedtls_ssl_conf_authmode(tlsContext->conf, MBEDTLS_SSL_VERIFY_NONE); // This option is for server certificate verification
	}
	mbedtls_ssl_conf_rng(tlsContext->conf, SSLRandomCB,tlsContext->ctr_drbg);
	mbedtls_ssl_conf_read_timeout(tlsContext->conf, WIZ_RECV_TIMEOUT_VALUE);
#if defined (MBEDTLS_DEBUG_C)
	mbedtls_ssl_conf_dbg(tlsContext->conf, WIZnetDebugCB, stdout);
#endif
	mbedtls_ssl_set_bio(tlsContext->ssl, socket_fd, SSLSendCB, SSLRecvCB, SSLRecvTimeOutCB);		 //set client's socket send and receive functions

	return 1;
}

/*Free the memory for ssl context*/
void wiz_tls_deinit(wiz_tls_context* tlsContext)
{
	/*  free SSL context memory  */

#if defined (MBEDTLS_ENTROPY_C)
    mbedtls_entropy_free(tlsContext->entropy);
    free(tlsContext->entropy);
#endif

	mbedtls_ssl_free( tlsContext->ssl );
	mbedtls_ssl_config_free( tlsContext->conf );
#if defined (MBEDTLS_DEBUG_C)
	mbedtls_ctr_drbg_free( tlsContext->ctr_drbg );
#endif
	//mbedtls_entropy_free( tlsContext->entropy );
#if defined(MBEDTLS_X509_CRT_PARSE_C)
	mbedtls_x509_crt_free( tlsContext->cacert );
#endif
	free(tlsContext->ctr_drbg);
	free(tlsContext->ssl);
	free(tlsContext->conf);
	free(tlsContext->cacert);
}

int wiz_tls_socket(wiz_tls_context* tlsContext, unsigned int port)
{
    /*socket open*/
    return socket((uint8_t)(tlsContext->ssl->p_bio), Sn_MR_TCP, (uint16_t)port, (SF_TCP_NODELAY));
}

/* SSL handshake */
int wiz_tls_connect(wiz_tls_context* tlsContext, char * addr, unsigned int port)
{
    int ret;

    /*Connect to the target*/
   ret = connect((uint8_t)(tlsContext->ssl->p_bio), (uint8_t *)addr, (uint16_t)port);
   if(ret != SOCK_OK)
       return ret;

#if defined(MBEDTLS_DEBUG_C)
    printf(" Performing the SSL/TLS handshake...\r\n");
#endif

    while( ( ret = mbedtls_ssl_handshake( tlsContext->ssl ) ) != 0 )
    {
        if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE )
        {
#if defined(MBEDTLS_ERROR_C)
            mbedtls_strerror(ret, (char *) tempBuf, DEBUG_BUFFER_SIZE );
            printf( " failed\n\r  ! mbedtls_ssl_handshake returned %d: %s\n\r", ret, tempBuf );
#endif
            return( -1 );
        }
    }

#if defined(MBEDTLS_DEBUG_C)
    printf( " ok\n\r    [ Ciphersuite is %s ]\n\r",
            mbedtls_ssl_get_ciphersuite( tlsContext->ssl ) );
#endif

    return( 0 );
}

int wiz_tls_connect_timeout(wiz_tls_context* tlsContext, char * addr, unsigned int port, uint32_t timeout)
{
    int ret;
    uint32_t start_ms = millis();

    uint8_t sock = (uint8_t)(tlsContext->ssl->p_bio);

    /*Connect to the target*/
    do {
        ret = connect_nb(sock, (uint8_t *)addr, (uint16_t)port);
        if((ret == SOCK_OK) || (ret == SOCKERR_TIMEOUT)) break;
    } while((millis() - start_ms) < timeout);

    if(ret == SOCK_BUSY) return -1;
    if(ret != SOCK_OK) return ret;

#if defined(MBEDTLS_DEBUG_C)
    printf(" Performing the SSL/TLS handshake...\r\n");
#endif

    while( ( ret = mbedtls_ssl_handshake( tlsContext->ssl ) ) != 0 )
    {
        if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE )
        {
#if defined(MBEDTLS_ERROR_C)
            mbedtls_strerror(ret, (char *) tempBuf, DEBUG_BUFFER_SIZE );
            printf( " failed\n\r  ! mbedtls_ssl_handshake returned %d: %s\n\r", ret, tempBuf );
#endif
            return( -1 );
        }
    }

#if defined(MBEDTLS_DEBUG_C)
    printf( " ok\n\r    [ Ciphersuite is %s ]\n\r",
            mbedtls_ssl_get_ciphersuite( tlsContext->ssl ) );
#endif

    return( 0 );
}



/* SSL handshake */
int wiz_tls_socket_connect(wiz_tls_context* tlsContext, char * addr, unsigned int port)
{
    int ret;
    uint8_t sock = (uint8_t)(tlsContext->ssl->p_bio);

    /*socket open*/
    ret = socket(sock, Sn_MR_TCP, 0, 0x00);
    if(ret != sock)
      return ret;

    /*Connect to the target*/
    ret = connect(sock, addr, port);
    if(ret != SOCK_OK)
        return ret;

#if defined(MBEDTLS_DEBUG_C)
    printf(" Performing the SSL/TLS handshake...\r\n");
#endif

    while( ( ret = mbedtls_ssl_handshake( tlsContext->ssl ) ) != 0 )
    {
        if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE )
        {
#if defined(MBEDTLS_ERROR_C)
            memset(tempBuf, 0, 1024);
            mbedtls_strerror(ret, (char *) tempBuf, DEBUG_BUFFER_SIZE );
            printf( " failed\n\r  ! mbedtls_ssl_handshake returned %d: %s\n\r", ret, tempBuf );
#endif
            return( -1 );
        }
    }

#if defined(MBEDTLS_DEBUG_C)
    printf( " ok\n\r    [ Ciphersuite is %s ]\n\r",
            mbedtls_ssl_get_ciphersuite( tlsContext->ssl ) );
#endif

    return( 0 );
}

int wiz_tls_socket_connect_timeout(wiz_tls_context* tlsContext, char * addr, unsigned int port, uint32_t timeout)
{
    int ret;
    uint32_t start_ms = millis();

    uint8_t sock = (uint8_t)(tlsContext->ssl->p_bio);

    /*socket open*/
    ret = socket(sock, Sn_MR_TCP, 0, 0x00);
    if(ret != sock)
      return ret;

    /*Connect to the target*/
    ret = connect(sock, addr, port);
    if(ret != SOCK_OK)
        return ret;

#if defined(MBEDTLS_DEBUG_C)
    printf(" Performing the SSL/TLS handshake...\r\n");
#endif

    while( ( ret = mbedtls_ssl_handshake( tlsContext->ssl ) ) != 0 )
    {
        if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE )
        {
#if defined(MBEDTLS_ERROR_C)
            memset(tempBuf, 0, 1024);
            mbedtls_strerror(ret, (char *) tempBuf, DEBUG_BUFFER_SIZE );
            printf( " failed\r\n  ! mbedtls_ssl_handshake returned %d: %s\n\r", ret, tempBuf );
#endif
            return( -1 );
        }

        if((millis() - start_ms) > timeout) // timeout
        {
#if defined(MBEDTLS_ERROR_C)
            memset(tempBuf, 0, 1024);
            mbedtls_strerror(ret, (char *) tempBuf, DEBUG_BUFFER_SIZE );
            printf( " timeout\r\n  ! mbedtls_ssl_handshake returned %d: %s\n\r", ret, tempBuf );
#endif
            return( -1 );
        }
    }

#if defined(MBEDTLS_DEBUG_C)
    printf( " ok\n\r    [ Ciphersuite is %s ]\n\r",
            mbedtls_ssl_get_ciphersuite( tlsContext->ssl ) );
#endif

    return( 0 );
}

unsigned int wiz_tls_read(wiz_tls_context* tlsContext, unsigned char* readbuf, unsigned int len)
{
	return mbedtls_ssl_read( tlsContext->ssl, readbuf, len );
}

unsigned int wiz_tls_write(wiz_tls_context* tlsContext, unsigned char* writebuf, unsigned int len)
{
	return mbedtls_ssl_write( tlsContext->ssl, writebuf, len );
}

int wiz_tls_disconnect(wiz_tls_context* tlsContext, uint32_t timeout)
{
    int ret = 0;
    uint8_t sock = (uint8_t)(tlsContext->ssl->p_bio);
    uint32_t tickStart = millis();

    do {
        ret = disconnect(sock);
        if((ret == SOCK_OK) || (ret == SOCKERR_TIMEOUT)) break;
    } while ((millis() - tickStart) < timeout);

    if(ret == SOCK_OK)
        ret = sock; // socket number

    return ret;
}


/* SSL X509 verify */
unsigned int wiz_tls_x509_verify(wiz_tls_context* tlsContext)
{
	uint32_t flags;

	memset(tempBuf,0,1024);

#if defined(MBEDTLS_DEBUG_C)
	printf( "Verifying peer X.509 certificate..." );
#endif
	/* In real life, we probably want to bail out when ret != 0 */
	if( ( flags = mbedtls_ssl_get_verify_result( tlsContext->ssl ) ) != 0 )
	{
		mbedtls_x509_crt_verify_info( tempBuf, DEBUG_BUFFER_SIZE, "  ! ", flags );

#if defined(MBEDTLS_DEBUG_C)
		memset(tempBuf,0,1024);
		printf( "failed.\n\r" );
		printf( "%s\n\r", tempBuf );
#endif
		return flags;
	}
	else
	{
#if defined(MBEDTLS_DEBUG_C)
		printf( "ok\n\r" );
#endif
		return 0;
	}
}


/* ssl Close notify */
unsigned int wiz_tls_close_notify(wiz_tls_context* tlsContext)
{
	uint32_t rc;
	do rc = mbedtls_ssl_close_notify( tlsContext->ssl );
	while( rc == MBEDTLS_ERR_SSL_WANT_WRITE );
	//SSLDeinit(tlsContext);
	return rc;
}


/* ssl session reset */
int wiz_tls_session_reset(wiz_tls_context* tlsContext)
{
    return mbedtls_ssl_session_reset( tlsContext->ssl );
}

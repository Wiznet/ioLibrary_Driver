#ifndef MBEDTLS_CONFIG_H
#define MBEDTLS_CONFIG_H

/************************ macro define ***********************************/
#define MBEDTLS_SSL_CLI_C   //complie SSL cleint protocols


/************************ ssl config *************************************/

/* System supports functions */
//#define MBEDTLS_FS_IO    //

/* ssl features supports functions */
//#define MBEDTLS_PROTO_TLS1    //supports TLS v1.0
//#define MBEDTLS_PROTO_TLS1_1  //supports TLS v1.1
#define MBEDTLS_SSL_PROTO_TLS1_2        //supports TL v1.2
//#define MBEDTLS_PROTO_SSL3         //supports SSL v3.0

#define MBEDTLS_CIPHER_MODE_CBC //Use verify certificate function
#define MBEDTLS_PKCS1_V15       //define public key padding method

//#define MBEDTLS_SRV_SUPPORT_SSLV2_CLIENT_HELLO //define suppots SSL v2

/* ssl function supports */
//#define MBEDTLS_CERTS_C             //define to use certication testing function
#define MBEDTLS_RSA_C               //define public key encryption method (Using RSA) !!!

//#define MBEDTLS_ECDH_C  //LIU
//#define MBEDTLS_ECP_C

//#define #define MBEDTLS_ARC4_C      //define public key encryption method (Using RC4)

#define MBEDTLS_X509_USE_C          //Using X509 frame function
#define MBEDTLS_X509_CRT_PARSE_C    //define supporting ssl x.509 certificate analyze !!!
#define MBEDTLS_PK_C                //Using private key encryption function
#define MBEDTLS_PK_PARSE_C          //Using private key encryption analyze function

#define MBEDTLS_BIGNUM_C            //define using Big number function !!!
#define MBEDTLS_CIPHER_C            //define using certificate function

//#define MBEDTLS_ENTROPY_C           //define using Entropy calculation !!!
//#define MBEDTLS_SHA512_C            //define using SHA512 calculation
//#define MBEDTLS_SHA1_C            //define using SHA128 calculation
#define MBEDTLS_SHA256_C          //define using SHA256 calculation

#define MBEDTLS_MD_C              //define using MD calculation
//#define MBEDTLS_MD5_C             //define using MD5 calculation

#define MBEDTLS_CTR_DRBG_C          //define using random number function !!!

#define MBEDTLS_AES_C               //define using AES function (after handshake - communicate stage)
//#define MBEDTLS_DES_C                     //define using DES function
#define MBEDTLS_ASN1_PARSE_C        //define using ASN analysis function
#define MBEDTLS_ASN1_WRITE_C        //define using ASN write function
#define MBEDTLS_OID_C              //define using OID function

#define MBEDTLS_SSL_TLS_C           //define using TLS support function !!!

#define MBEDTLS_ERROR_C             //define using ERROR function (Showing error message)
#define MBEDTLS_DEBUG_C             //define using debug function (Showing debug message)

/* Testing certificate procedure */
//#define MBEDTLS_CERTS_C        //define using certificate testing function
#define MBEDTLS_PEM_PARSE_C    //define testing certifcate will use ASCII format (Sending PEM file)
#define MBEDTLS_BASE64_C       //define testing certificate is in ASCII format

//define communicate encrytion methods 
//#define MBEDTLS_KEY_EXCHANGE_ECDHE_PSK_ENABLED
//#define MBEDTLS_KEY_EXCHANGE_ECDHE_RSDEBUG_DFL_MODEA_ENABLED

#define MBEDTLS_KEY_EXCHANGE_RSA_ENABLED        //
//#define MBEDTLS_KEY_EXCHANGE_RSA_PSK_ENABLED   //
#define MBEDTLS_GCM_C    //AES in GCM mode��base on parallel design��therefore it can good performance on speed,cost and less delay
//#define MBEDTLS_CCM_C    //

//define encryption and discrete methods
#define MBEDTLS_SSL_CIPHERSUITES	\
        MBEDTLS_TLS_RSA_WITH_AES_128_GCM_SHA256, \
        MBEDTLS_TLS_RSA_WITH_AES_256_CBC_SHA256, \
        MBEDTLS_TLS_RSA_WITH_AES_256_GCM_SHA384

//Save RAM 
#define MBEDTLS_MPI_MAX_SIZE   256
//#define MBEDTLS_MPI_MAX_SIZE   512
#define MBEDTLS_ENTROPY_MAX_SOURCES   10

//#define SSL_MAX_CONTENT_LEN    4096

//According to SSL requested data length to define, if the data is long, it takes a larger RAM space 

#if defined (MBEDTLS_CLIENT_SUPPOR_AILPAY) //defines for alipay
		#define MBEDTLS_SSL_MAX_CONTENT_LEN   5120   //define SSL client for connection to AILPAY
#else
		#define MBEDTLS_SSL_MAX_CONTENT_LEN    4096 //2048
#endif

//#define MBEDTLS_X509_ALLOW_UNSUPPORTED_CRITICAL_EXTENSION

#include "mbedtls/check_config.h"
#endif /* ssl_config */

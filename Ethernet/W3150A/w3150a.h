//*****************************************************************************
//
//! \file w3150a.h
//! \brief W3150A+ HAL Header File.
//! \details W3150A+ is register compatible with W5100 (same common/socket
//!          register map, 4 sockets, 8KB TX + 8KB RX), but it has no internal
//!          PHY (MII to an external PHY) and no version register.
//!          Differences from W5100 handled here:
//!          - MR bit2 is LB (byte order in indirect bus mode).
//!          - Sn_MR bit6 is reserved (no MAC filter), Sn_IMR does not exist.
//!          - SPI mode 0 only (W3150A+/W5100 errata 4).
//!          Reference : W3150A+ Datasheet v2.0.0
//! \version 1.0.0
//
//*****************************************************************************
#ifndef _W3150A_H_
#define _W3150A_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "wizchip_conf.h"

#if (_WIZCHIP_ == W3150A)

#if !(_WIZCHIP_IO_MODE_ & _WIZCHIP_IO_MODE_SPI_)
   #error "W3150A driver supports only the SPI interface mode."
#endif

#define _WIZCHIP_SN_BASE_  (0x0400)
#define _WIZCHIP_SN_SIZE_  (0x0100)
#define _WIZCHIP_IO_TXBUF_ (0x4000) /* Internal Tx buffer address of the iinchip */
#define _WIZCHIP_IO_RXBUF_ (0x6000) /* Internal Rx buffer address of the iinchip */

#define WIZCHIP_CREG_BLOCK          0x00                                            ///< Common register block
#define WIZCHIP_SREG_BLOCK(N)       (_WIZCHIP_SN_BASE_ + _WIZCHIP_SN_SIZE_ * (N))   ///< Socket N register block
#define WIZCHIP_OFFSET_INC(ADDR, N) ((ADDR) + (N))                                  ///< Increase offset address

#define _W3150A_IO_BASE_   0x0000

/* SPI OP-codes (W3150A+ datasheet 6.3.2) */
#define W3150A_SPI_OP_WRITE   0xF0
#define W3150A_SPI_OP_READ    0x0F

///////////////////////////////////////
// Definition For Legacy Chip Driver //
///////////////////////////////////////
#define IINCHIP_READ(ADDR)                WIZCHIP_READ(ADDR)               ///< The defined for legacy chip driver
#define IINCHIP_WRITE(ADDR,VAL)           WIZCHIP_WRITE(ADDR,VAL)          ///< The defined for legacy chip driver
#define IINCHIP_READ_BUF(ADDR,BUF,LEN)    WIZCHIP_READ_BUF(ADDR,BUF,LEN)   ///< The defined for legacy chip driver
#define IINCHIP_WRITE_BUF(ADDR,BUF,LEN)   WIZCHIP_WRITE_BUF(ADDR,BUF,LEN)  ///< The defined for legacy chip driver

//----------------------------- W3150A+ Common Registers -----------------------------
#define MR          (_W3150A_IO_BASE_ + (0x0000))  ///< Mode
#define GAR         (_W3150A_IO_BASE_ + (0x0001))  ///< Gateway IP address (4 bytes)
#define SUBR        (_W3150A_IO_BASE_ + (0x0005))  ///< Subnet mask (4 bytes)
#define SHAR        (_W3150A_IO_BASE_ + (0x0009))  ///< Source hardware address (6 bytes)
#define SIPR        (_W3150A_IO_BASE_ + (0x000F))  ///< Source IP address (4 bytes)
// Reserved         (_W3150A_IO_BASE_ + (0x0013) ~ (0x0014))
#define IR          (_W3150A_IO_BASE_ + (0x0015))  ///< Interrupt
#define _IMR_       (_W3150A_IO_BASE_ + (0x0016))  ///< Interrupt mask
#define _RTR_       (_W3150A_IO_BASE_ + (0x0017))  ///< Retry time (2 bytes, 1 = 100us)
#define _RCR_       (_W3150A_IO_BASE_ + (0x0019))  ///< Retry count
#define RMSR        (_W3150A_IO_BASE_ + (0x001A))  ///< RX memory size
#define TMSR        (_W3150A_IO_BASE_ + (0x001B))  ///< TX memory size
#define PATR        (_W3150A_IO_BASE_ + (0x001C))  ///< Authentication type in PPPoE (2 bytes)
// Reserved         (_W3150A_IO_BASE_ + (0x001E) ~ (0x0027))
#define PTIMER      (_W3150A_IO_BASE_ + (0x0028))  ///< PPP LCP request timer
#define PMAGIC      (_W3150A_IO_BASE_ + (0x0029))  ///< PPP LCP magic number
#define UIPR        (_W3150A_IO_BASE_ + (0x002A))  ///< Unreachable IP address (4 bytes)
#define UPORTR      (_W3150A_IO_BASE_ + (0x002E))  ///< Unreachable port (2 bytes)
// Reserved         (_W3150A_IO_BASE_ + (0x0030) ~ (0x03FF))

//----------------------------- W3150A+ Socket Registers -----------------------------
#define Sn_MR(sn)       (_W3150A_IO_BASE_ + WIZCHIP_SREG_BLOCK(sn) + (0x0000)) ///< Socket mode
#define Sn_CR(sn)       (_W3150A_IO_BASE_ + WIZCHIP_SREG_BLOCK(sn) + (0x0001)) ///< Socket command
#define Sn_IR(sn)       (_W3150A_IO_BASE_ + WIZCHIP_SREG_BLOCK(sn) + (0x0002)) ///< Socket interrupt
#define Sn_SR(sn)       (_W3150A_IO_BASE_ + WIZCHIP_SREG_BLOCK(sn) + (0x0003)) ///< Socket status
#define Sn_PORT(sn)     (_W3150A_IO_BASE_ + WIZCHIP_SREG_BLOCK(sn) + (0x0004)) ///< Source port (2 bytes)
#define Sn_DHAR(sn)     (_W3150A_IO_BASE_ + WIZCHIP_SREG_BLOCK(sn) + (0x0006)) ///< Destination hardware address (6 bytes)
#define Sn_DIPR(sn)     (_W3150A_IO_BASE_ + WIZCHIP_SREG_BLOCK(sn) + (0x000C)) ///< Destination IP address (4 bytes)
#define Sn_DPORT(sn)    (_W3150A_IO_BASE_ + WIZCHIP_SREG_BLOCK(sn) + (0x0010)) ///< Destination port (2 bytes)
#define Sn_MSSR(sn)     (_W3150A_IO_BASE_ + WIZCHIP_SREG_BLOCK(sn) + (0x0012)) ///< Maximum segment size (2 bytes)
#define Sn_PROTO(sn)    (_W3150A_IO_BASE_ + WIZCHIP_SREG_BLOCK(sn) + (0x0014)) ///< IP protocol in IP raw mode
#define Sn_TOS(sn)      (_W3150A_IO_BASE_ + WIZCHIP_SREG_BLOCK(sn) + (0x0015)) ///< IP TOS
#define Sn_TTL(sn)      (_W3150A_IO_BASE_ + WIZCHIP_SREG_BLOCK(sn) + (0x0016)) ///< IP TTL
// Reserved             (_W3150A_IO_BASE_ + WIZCHIP_SREG_BLOCK(sn) + (0x0017) ~ (0x001F))
#define Sn_TX_FSR(sn)   (_W3150A_IO_BASE_ + WIZCHIP_SREG_BLOCK(sn) + (0x0020)) ///< TX free size (2 bytes)
#define Sn_TX_RD(sn)    (_W3150A_IO_BASE_ + WIZCHIP_SREG_BLOCK(sn) + (0x0022)) ///< TX read pointer (2 bytes)
#define Sn_TX_WR(sn)    (_W3150A_IO_BASE_ + WIZCHIP_SREG_BLOCK(sn) + (0x0024)) ///< TX write pointer (2 bytes)
#define Sn_RX_RSR(sn)   (_W3150A_IO_BASE_ + WIZCHIP_SREG_BLOCK(sn) + (0x0026)) ///< RX received size (2 bytes)
#define Sn_RX_RD(sn)    (_W3150A_IO_BASE_ + WIZCHIP_SREG_BLOCK(sn) + (0x0028)) ///< RX read pointer (2 bytes)
/// RX write pointer (2 bytes, read only). Reserved in the datasheet map, but defined
/// as RX_WR_PTR in the WIZnet EVB-B1+ reference driver (w3150a.h).
#define Sn_RX_WR(sn)    (_W3150A_IO_BASE_ + WIZCHIP_SREG_BLOCK(sn) + (0x002A))
// Reserved             (_W3150A_IO_BASE_ + WIZCHIP_SREG_BLOCK(sn) + (0x002C) ~ (0x00FF))

//----------------------------- W3150A+ Register values  -----------------------------
/* MR values */
#define MR_RST          0x80 ///< S/W reset
#define MR_PB           0x10 ///< Ping block
#define MR_PPPOE        0x08 ///< PPPoE mode
#define MR_LB           0x04 ///< Little-endian ordering in indirect bus mode (W3150A+ only)
#define MR_AI           0x02 ///< Address auto-increment in indirect bus mode
#define MR_IND          0x01 ///< Indirect bus mode

/* IR values */
#define IR_CONFLICT     0x80 ///< IP conflict
#define IR_UNREACH      0x40 ///< Destination unreachable
#define IR_PPPoE        0x20 ///< PPPoE connection closed
#define IR_SOCK(sn)     (0x01 << (sn)) ///< Socket interrupt, cleared when Sn_IR is cleared
#define IR_COMMON_MASK  (IR_CONFLICT | IR_UNREACH | IR_PPPoE) ///< Bits cleared by writing '1' to IR

/* Sn_MR values */
#define Sn_MR_CLOSE     0x00 ///< Unused socket
#define Sn_MR_TCP       0x01 ///< TCP
#define Sn_MR_UDP       0x02 ///< UDP
#define Sn_MR_IPRAW     0x03 ///< IP layer raw socket
#define Sn_MR_MACRAW    0x04 ///< MAC layer raw socket (socket 0 only)
#define Sn_MR_PPPoE     0x05 ///< PPPoE (socket 0 only)
#define Sn_MR_ND        0x20 ///< No delayed ACK (TCP)
#define Sn_MR_MC        Sn_MR_ND ///< IGMP version select in UDP multicast
#define Sn_MR_MULTI     0x80 ///< Multicasting (UDP)
// Sn_MR bit6 is reserved on W3150A+ : there is no Sn_MR_MF / Sn_MR_MFEN.

/* Sn_MR alternate values, for Berkeley Socket API (same names as the reference driver and w5500.h) */
#define SOCK_STREAM     Sn_MR_TCP
#define SOCK_DGRAM      Sn_MR_UDP

/* Sn_CR values */
#define Sn_CR_OPEN      0x01 ///< Initialize or open socket
#define Sn_CR_LISTEN    0x02 ///< Wait for a connection request (TCP server)
#define Sn_CR_CONNECT   0x04 ///< Send a connection request (TCP client)
#define Sn_CR_DISCON    0x08 ///< Send a closing request (TCP)
#define Sn_CR_CLOSE     0x10 ///< Close socket
#define Sn_CR_SEND      0x20 ///< Send data in TX memory
#define Sn_CR_SEND_MAC  0x21 ///< Send data with Sn_DHAR, no ARP (UDP)
#define Sn_CR_SEND_KEEP 0x22 ///< Send keep-alive (TCP)
#define Sn_CR_RECV      0x40 ///< Update RX read pointer
#define Sn_CR_PCON      0x23 ///< PPPoE connection
#define Sn_CR_PDISCON   0x24 ///< PPPoE disconnection
#define Sn_CR_PCR       0x25 ///< PPPoE request in each phase
#define Sn_CR_PCN       0x26 ///< PPPoE NAK in each phase
#define Sn_CR_PCJ       0x27 ///< PPPoE REJECT in each phase

/* Sn_IR values */
#define Sn_IR_PRECV     0x80 ///< PPP receive
#define Sn_IR_PFAIL     0x40 ///< PPP fail
#define Sn_IR_PNEXT     0x20 ///< PPP next phase
#define Sn_IR_SENDOK    0x10 ///< Send completed (added in W3150A+)
#define Sn_IR_TIMEOUT   0x08 ///< Timeout
#define Sn_IR_RECV      0x04 ///< Data received
#define Sn_IR_DISCON    0x02 ///< Connection terminated
#define Sn_IR_CON       0x01 ///< Connection established

/* Sn_SR values */
#define SOCK_CLOSED      0x00 ///< Closed
#define SOCK_INIT        0x13 ///< TCP socket opened
#define SOCK_LISTEN      0x14 ///< TCP server waiting for a connection
#define SOCK_SYNSENT     0x15 ///< Connection request sent
#define SOCK_SYNRECV     0x16 ///< Connection request received
#define SOCK_ESTABLISHED 0x17 ///< Connected
#define SOCK_FIN_WAIT    0x18 ///< Closing
#define SOCK_FIN_WAIT2   0x19 ///< Closing (reference driver only, not listed in the datasheet)
#define SOCK_CLOSING     0x1A ///< Closing
#define SOCK_TIME_WAIT   0x1B ///< Closing
#define SOCK_CLOSE_WAIT  0x1C ///< Termination request received from peer
#define SOCK_LAST_ACK    0x1D ///< Closing
#define SOCK_UDP         0x22 ///< UDP socket
#define SOCK_IPRAW       0x32 ///< IP raw socket
#define SOCK_MACRAW      0x42 ///< MAC raw socket
#define SOCK_PPPOE       0x5F ///< PPPoE socket
#define SOCK_ARP_TCP     0x11 ///< ARP request sent for a TCP connect
#define SOCK_ARP_UDP     0x21 ///< ARP request sent for a UDP send
#define SOCK_ARP_ICMP    0x31 ///< ARP request sent for an IP raw send

/* IP protocol */
#define IPPROTO_IP      0   ///< Dummy for IP
#define IPPROTO_ICMP    1   ///< Control message protocol
#define IPPROTO_IGMP    2   ///< Internet group management protocol
#define IPPROTO_GGP     3   ///< Gateway^2 (deprecated)
#define IPPROTO_TCP     6   ///< TCP
#define IPPROTO_PUP     12  ///< PUP
#define IPPROTO_UDP     17  ///< UDP
#define IPPROTO_IDP     22  ///< XNS idp
#define IPPROTO_ND      77  ///< UNOFFICIAL net disk protocol
#define IPPROTO_RAW     255 ///< Raw IP packet

/* Reset values, used to identify the chip (there is no version register) */
#define W3150A_RTR_RESET_VALUE    0x07D0
#define W3150A_RCR_RESET_VALUE    0x08
#define W3150A_MSR_RESET_VALUE    0x55  ///< RMSR and TMSR
#define W3150A_PTIMER_RESET_VALUE 0x28

#define WIZCHIP_CRITICAL_ENTER()    WIZCHIP.CRIS._enter()

#ifdef _exit
#undef _exit
#endif

#define WIZCHIP_CRITICAL_EXIT()     WIZCHIP.CRIS._exit()

////////////////////////
// Basic I/O Function //
////////////////////////
uint8_t  WIZCHIP_READ (uint32_t AddrSel);
void     WIZCHIP_WRITE(uint32_t AddrSel, uint8_t wb );
void     WIZCHIP_READ_BUF (uint32_t AddrSel, uint8_t* pBuf, uint16_t len);
void     WIZCHIP_WRITE_BUF(uint32_t AddrSel, uint8_t* pBuf, uint16_t len);

/////////////////////////////////
// Common Register IO function //
/////////////////////////////////
#define setMR(mr)          WIZCHIP_WRITE(MR, mr)
#define getMR()            WIZCHIP_READ(MR)

#define setGAR(gar)        WIZCHIP_WRITE_BUF(GAR, gar, 4)
#define getGAR(gar)        WIZCHIP_READ_BUF(GAR, gar, 4)

#define setSUBR(subr)      WIZCHIP_WRITE_BUF(SUBR, subr, 4)
#define getSUBR(subr)      WIZCHIP_READ_BUF(SUBR, subr, 4)

#define setSHAR(shar)      WIZCHIP_WRITE_BUF(SHAR, shar, 6)
#define getSHAR(shar)      WIZCHIP_READ_BUF(SHAR, shar, 6)

#define setSIPR(sipr)      WIZCHIP_WRITE_BUF(SIPR, sipr, 4)
#define getSIPR(sipr)      WIZCHIP_READ_BUF(SIPR, sipr, 4)

/**
 * @brief Clear IR.
 * @details IR_CONFLICT, IR_UNREACH and IR_PPPoE are cleared by writing '1'.
 *          The socket bits (IR_SOCK) fall to 0 when Sn_IR is cleared, so for each
 *          socket bit set in @b ir the pending Sn_IR flags are cleared first and
 *          then @b ir is written to IR, as iinchip_irq() of the reference driver does.
 */
void    setIR(uint8_t ir);
/** @brief Get IR, including the socket bits (bit4 is reserved). */
#define getIR()            (WIZCHIP_READ(IR) & (IR_COMMON_MASK | 0x0F))

#define setIMR(imr)        WIZCHIP_WRITE(_IMR_, ((imr) & (IR_COMMON_MASK | 0x0F)))
#define getIMR()           WIZCHIP_READ(_IMR_)

/**
 * @brief Socket interrupt bits of IR / IMR, under the W5500 SIR / SIMR names.
 * @details The W3150A+ has no SIR / SIMR register: the socket interrupts are
 *          IR[3:0] (S0_INT..S3_INT) and IMR[3:0] (IM_IR0..IM_IR3), datasheet 4.1.
 *          As with SIR, a socket bit falls to 0 only when its Sn_IR is cleared.
 *          setSIMR() leaves IMR[7:5] (CONFLICT / UNREACH / PPPoE) as they are.
 */
#define getSIR()           (WIZCHIP_READ(IR) & 0x0F)
void    setSIMR(uint8_t simr);
#define getSIMR()          (WIZCHIP_READ(_IMR_) & 0x0F)

#define setRTR(rtr)   { \
      WIZCHIP_WRITE(_RTR_,   (uint8_t)((rtr) >> 8)); \
      WIZCHIP_WRITE(WIZCHIP_OFFSET_INC(_RTR_,1), (uint8_t)(rtr)); \
   }
#define getRTR() \
      (((uint16_t)WIZCHIP_READ(_RTR_) << 8) + WIZCHIP_READ(WIZCHIP_OFFSET_INC(_RTR_,1)))

#define setRCR(rcr)        WIZCHIP_WRITE(_RCR_, rcr)
#define getRCR()           WIZCHIP_READ(_RCR_)

#define setRMSR(rmsr)      WIZCHIP_WRITE(RMSR, rmsr)
#define getRMSR()          WIZCHIP_READ(RMSR)
#define setTMSR(tmsr)      WIZCHIP_WRITE(TMSR, tmsr)
#define getTMSR()          WIZCHIP_READ(TMSR)

#define getPATR() \
      (((uint16_t)WIZCHIP_READ(PATR) << 8) + WIZCHIP_READ(WIZCHIP_OFFSET_INC(PATR,1)))

#define setPTIMER(ptimer)  WIZCHIP_WRITE(PTIMER, ptimer)
#define getPTIMER()        WIZCHIP_READ(PTIMER)

#define setPMAGIC(pmagic)  WIZCHIP_WRITE(PMAGIC, pmagic)
#define getPMAGIC()        WIZCHIP_READ(PMAGIC)

#define getUIPR(uipr)      WIZCHIP_READ_BUF(UIPR, uipr, 4)
#define getUPORTR() \
      (((uint16_t)WIZCHIP_READ(UPORTR) << 8) + WIZCHIP_READ(WIZCHIP_OFFSET_INC(UPORTR,1)))

///////////////////////////////////
// Socket N register I/O function //
///////////////////////////////////
#define setSn_MR(sn, mr)   WIZCHIP_WRITE(Sn_MR(sn), mr)
#define getSn_MR(sn)       WIZCHIP_READ(Sn_MR(sn))

#define setSn_CR(sn, cr)   WIZCHIP_WRITE(Sn_CR(sn), cr)
#define getSn_CR(sn)       WIZCHIP_READ(Sn_CR(sn))

#define setSn_IR(sn, ir)   WIZCHIP_WRITE(Sn_IR(sn), ir)
#define getSn_IR(sn)       WIZCHIP_READ(Sn_IR(sn))

#define getSn_SR(sn)       WIZCHIP_READ(Sn_SR(sn))

#define setSn_PORT(sn, port)  { \
      WIZCHIP_WRITE(Sn_PORT(sn),   (uint8_t)((port) >> 8)); \
      WIZCHIP_WRITE(WIZCHIP_OFFSET_INC(Sn_PORT(sn),1), (uint8_t)(port)); \
   }
#define setSn_PORTR        setSn_PORT
#define getSn_PORT(sn) \
      (((uint16_t)WIZCHIP_READ(Sn_PORT(sn)) << 8) + WIZCHIP_READ(WIZCHIP_OFFSET_INC(Sn_PORT(sn),1)))
#define getSn_PORTR        getSn_PORT

#define setSn_DHAR(sn, dhar)  WIZCHIP_WRITE_BUF(Sn_DHAR(sn), dhar, 6)
#define getSn_DHAR(sn, dhar)  WIZCHIP_READ_BUF(Sn_DHAR(sn), dhar, 6)

#define setSn_DIPR(sn, dipr)  WIZCHIP_WRITE_BUF(Sn_DIPR(sn), dipr, 4)
#define getSn_DIPR(sn, dipr)  WIZCHIP_READ_BUF(Sn_DIPR(sn), dipr, 4)

#define setSn_DPORT(sn, dport) { \
      WIZCHIP_WRITE(Sn_DPORT(sn),   (uint8_t)((dport) >> 8)); \
      WIZCHIP_WRITE(WIZCHIP_OFFSET_INC(Sn_DPORT(sn),1), (uint8_t)(dport)); \
   }
#define setSn_DPORTR(sn, dport)   setSn_DPORT(sn, dport)
#define getSn_DPORT(sn) \
      (((uint16_t)WIZCHIP_READ(Sn_DPORT(sn)) << 8) + WIZCHIP_READ(WIZCHIP_OFFSET_INC(Sn_DPORT(sn),1)))
#define getSn_DPORTR(sn)          getSn_DPORT(sn)

#define setSn_MSSR(sn, mss) { \
      WIZCHIP_WRITE(Sn_MSSR(sn),   (uint8_t)((mss) >> 8)); \
      WIZCHIP_WRITE(WIZCHIP_OFFSET_INC(Sn_MSSR(sn),1), (uint8_t)(mss)); \
   }
#define getSn_MSSR(sn) \
      (((uint16_t)WIZCHIP_READ(Sn_MSSR(sn)) << 8) + WIZCHIP_READ(WIZCHIP_OFFSET_INC(Sn_MSSR(sn),1)))

#define setSn_PROTO(sn, proto)    WIZCHIP_WRITE(Sn_PROTO(sn), proto)
#define getSn_PROTO(sn)           WIZCHIP_READ(Sn_PROTO(sn))

#define setSn_TOS(sn, tos)        WIZCHIP_WRITE(Sn_TOS(sn), tos)
#define getSn_TOS(sn)             WIZCHIP_READ(Sn_TOS(sn))

#define setSn_TTL(sn, ttl)        WIZCHIP_WRITE(Sn_TTL(sn), ttl)
#define getSn_TTL(sn)             WIZCHIP_READ(Sn_TTL(sn))

/* Socket memory size : 2 bits per socket in RMSR/TMSR, 0=1KB 1=2KB 2=4KB 3=8KB */
#define setSn_RXMEM_SIZE(sn, rxmemsize) \
      WIZCHIP_WRITE(RMSR, (WIZCHIP_READ(RMSR) & ~(0x03 << (2*(sn)))) | ((rxmemsize) << (2*(sn))))
#define setSn_RXBUF_SIZE(sn, rxmemsize) setSn_RXMEM_SIZE(sn, rxmemsize)
#define getSn_RXMEM_SIZE(sn) \
      ((WIZCHIP_READ(RMSR) >> (2*(sn))) & 0x03)
#define getSn_RXBUF_SIZE(sn)            getSn_RXMEM_SIZE(sn)

#define setSn_TXMEM_SIZE(sn, txmemsize) \
      WIZCHIP_WRITE(TMSR, (WIZCHIP_READ(TMSR) & ~(0x03 << (2*(sn)))) | ((txmemsize) << (2*(sn))))
#define setSn_TXBUF_SIZE(sn, txmemsize) setSn_TXMEM_SIZE(sn, txmemsize)
#define getSn_TXMEM_SIZE(sn) \
      ((WIZCHIP_READ(TMSR) >> (2*(sn))) & 0x03)
#define getSn_TXBUF_SIZE(sn)            getSn_TXMEM_SIZE(sn)

uint16_t getSn_TX_FSR(uint8_t sn);

#define getSn_TX_RD(sn) \
      (((uint16_t)WIZCHIP_READ(Sn_TX_RD(sn)) << 8) + WIZCHIP_READ(WIZCHIP_OFFSET_INC(Sn_TX_RD(sn),1)))

#define setSn_TX_WR(sn, txwr) { \
      WIZCHIP_WRITE(Sn_TX_WR(sn),   (uint8_t)((txwr) >> 8)); \
      WIZCHIP_WRITE(WIZCHIP_OFFSET_INC(Sn_TX_WR(sn),1), (uint8_t)(txwr)); \
   }
#define getSn_TX_WR(sn) \
      (((uint16_t)WIZCHIP_READ(Sn_TX_WR(sn)) << 8) + WIZCHIP_READ(WIZCHIP_OFFSET_INC(Sn_TX_WR(sn),1)))

uint16_t getSn_RX_RSR(uint8_t sn);

#define setSn_RX_RD(sn, rxrd) { \
      WIZCHIP_WRITE(Sn_RX_RD(sn),   (uint8_t)((rxrd) >> 8)); \
      WIZCHIP_WRITE(WIZCHIP_OFFSET_INC(Sn_RX_RD(sn),1), (uint8_t)(rxrd)); \
   }
#define getSn_RX_RD(sn) \
      (((uint16_t)WIZCHIP_READ(Sn_RX_RD(sn)) << 8) + WIZCHIP_READ(WIZCHIP_OFFSET_INC(Sn_RX_RD(sn),1)))

#define getSn_RX_WR(sn) \
      (((uint16_t)WIZCHIP_READ(Sn_RX_WR(sn)) << 8) + WIZCHIP_READ(WIZCHIP_OFFSET_INC(Sn_RX_WR(sn),1)))

/////////////////////////////////////
// Sn_TXBUF & Sn_RXBUF IO function //
/////////////////////////////////////
#define getSn_RxMAX(sn)   ((uint16_t)(1 << getSn_RXMEM_SIZE(sn)) << 10)
#define getSn_TxMAX(sn)   ((uint16_t)(1 << getSn_TXMEM_SIZE(sn)) << 10)
#define getSn_RxMASK(sn)  (getSn_RxMAX(sn) - 1)
#define getSn_TxMASK(sn)  (getSn_TxMAX(sn) - 1)

uint32_t getSn_RxBASE(uint8_t sn);
uint32_t getSn_TxBASE(uint8_t sn);

void wiz_send_data(uint8_t sn, uint8_t *wizdata, uint16_t len);
void wiz_recv_data(uint8_t sn, uint8_t *wizdata, uint16_t len);
void wiz_recv_ignore(uint8_t sn, uint16_t len);

#endif /* _WIZCHIP_ == W3150A */

#ifdef __cplusplus
}
#endif

#endif /* _W3150A_H_ */

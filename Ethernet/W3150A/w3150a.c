//*****************************************************************************
//
//! \file w3150a.c
//! \brief W3150A+ HAL Interface.
//! \details In SPI mode the W3150A+ works in units of a 32-bit stream:
//!          OP-code(1) + address(2) + data(1), with /SS toggled per stream
//!          (W3150A+ datasheet 6.3.2). There is no burst transfer, so every
//!          byte of a buffer access is sent as its own frame.
//!          When burst callbacks are registered (reg_wizchip_spiburst_cbfunc)
//!          a frame is passed as one burst, otherwise byte by byte.
//!          Frame format, buffer access, FSR/RSR read and interrupt clear follow
//!          the WIZnet EVB-B1+ reference driver (iinchip/w3150a.c v3.0.0, SPI mode).
//! \version 1.0.0
//
//*****************************************************************************

#include "w3150a.h"

#if (_WIZCHIP_ == W3150A)

/* Send one write frame. CS must already be asserted. */
static void w3150a_write_frame(uint16_t addr, uint8_t wb)
{
   if(WIZCHIP.IF.SPI._write_burst)
   {
      uint8_t frame[4] = { W3150A_SPI_OP_WRITE, (uint8_t)(addr >> 8), (uint8_t)addr, wb };
      WIZCHIP.IF.SPI._write_burst(frame, 4);
   }
   else
   {
      WIZCHIP.IF.SPI._write_byte(W3150A_SPI_OP_WRITE);
      WIZCHIP.IF.SPI._write_byte((uint8_t)(addr >> 8));
      WIZCHIP.IF.SPI._write_byte((uint8_t)addr);
      WIZCHIP.IF.SPI._write_byte(wb);
   }
}

/* Send one read frame and return the data byte. CS must already be asserted. */
static uint8_t w3150a_read_frame(uint16_t addr)
{
   if(WIZCHIP.IF.SPI._write_burst)
   {
      uint8_t header[3] = { W3150A_SPI_OP_READ, (uint8_t)(addr >> 8), (uint8_t)addr };
      WIZCHIP.IF.SPI._write_burst(header, 3);
   }
   else
   {
      WIZCHIP.IF.SPI._write_byte(W3150A_SPI_OP_READ);
      WIZCHIP.IF.SPI._write_byte((uint8_t)(addr >> 8));
      WIZCHIP.IF.SPI._write_byte((uint8_t)addr);
   }
   return WIZCHIP.IF.SPI._read_byte();
}

void WIZCHIP_WRITE(uint32_t AddrSel, uint8_t wb)
{
   WIZCHIP_CRITICAL_ENTER();
   WIZCHIP.CS._select();
   w3150a_write_frame((uint16_t)AddrSel, wb);
   WIZCHIP.CS._deselect();
   WIZCHIP_CRITICAL_EXIT();
}

uint8_t WIZCHIP_READ(uint32_t AddrSel)
{
   uint8_t ret;

   WIZCHIP_CRITICAL_ENTER();
   WIZCHIP.CS._select();
   ret = w3150a_read_frame((uint16_t)AddrSel);
   WIZCHIP.CS._deselect();
   WIZCHIP_CRITICAL_EXIT();
   return ret;
}

void WIZCHIP_WRITE_BUF(uint32_t AddrSel, uint8_t* pBuf, uint16_t len)
{
   uint16_t i;

   WIZCHIP_CRITICAL_ENTER();
   for(i = 0; i < len; i++)
   {
      WIZCHIP.CS._select();
      w3150a_write_frame((uint16_t)(AddrSel + i), pBuf[i]);
      WIZCHIP.CS._deselect();
   }
   WIZCHIP_CRITICAL_EXIT();
}

void WIZCHIP_READ_BUF(uint32_t AddrSel, uint8_t* pBuf, uint16_t len)
{
   uint16_t i;

   WIZCHIP_CRITICAL_ENTER();
   for(i = 0; i < len; i++)
   {
      WIZCHIP.CS._select();
      pBuf[i] = w3150a_read_frame((uint16_t)(AddrSel + i));
      WIZCHIP.CS._deselect();
   }
   WIZCHIP_CRITICAL_EXIT();
}

/* Same sequence as iinchip_irq() of the reference driver (W3150A+ interrupt method):
 * write back the Sn_IR flags that were read, then write the IR value itself. */
void setIR(uint8_t ir)
{
   uint8_t sn;
   uint8_t sn_ir;

   for(sn = 0; sn < _WIZCHIP_SOCK_NUM_; sn++)
   {
      if(ir & IR_SOCK(sn))
      {
         sn_ir = getSn_IR(sn);
         if(sn_ir) setSn_IR(sn, sn_ir);
      }
   }
   WIZCHIP_WRITE(IR, ir & (IR_COMMON_MASK | 0x0F));
}

/* IMR[3:0] are the socket interrupt enables; IMR[7:5] are kept, bit4 is reserved (0). */
void setSIMR(uint8_t simr)
{
   setIMR((getIMR() & IR_COMMON_MASK) | (simr & 0x0F));
}

/* Read a 16-bit register until two reads agree (datasheet: read upper byte first). */
static uint16_t w3150a_read_u16_stable(uint32_t addr)
{
   uint16_t val = 0, val1 = 0;

   do
   {
      val1 = WIZCHIP_READ(addr);
      val1 = (val1 << 8) + WIZCHIP_READ(WIZCHIP_OFFSET_INC(addr, 1));
      if(val1 != 0)
      {
         val = WIZCHIP_READ(addr);
         val = (val << 8) + WIZCHIP_READ(WIZCHIP_OFFSET_INC(addr, 1));
      }
   } while(val != val1);
   return val;
}

uint16_t getSn_TX_FSR(uint8_t sn)
{
   return w3150a_read_u16_stable(Sn_TX_FSR(sn));
}

uint16_t getSn_RX_RSR(uint8_t sn)
{
   return w3150a_read_u16_stable(Sn_RX_RSR(sn));
}

uint32_t getSn_TxBASE(uint8_t sn)
{
   uint8_t  i;
   uint32_t txbase = _W3150A_IO_BASE_ + _WIZCHIP_IO_TXBUF_;

   for(i = 0; i < sn; i++)
      txbase += getSn_TxMAX(i);
   return txbase;
}

uint32_t getSn_RxBASE(uint8_t sn)
{
   uint8_t  i;
   uint32_t rxbase = _W3150A_IO_BASE_ + _WIZCHIP_IO_RXBUF_;

   for(i = 0; i < sn; i++)
      rxbase += getSn_RxMAX(i);
   return rxbase;
}

void wiz_send_data(uint8_t sn, uint8_t *wizdata, uint16_t len)
{
   uint16_t ptr;
   uint16_t size;
   uint16_t dst_mask;
   uint32_t dst_ptr;

   if(len == 0) return;

   ptr = getSn_TX_WR(sn);
   dst_mask = ptr & getSn_TxMASK(sn);
   dst_ptr = getSn_TxBASE(sn) + dst_mask;

   if((uint32_t)dst_mask + len > getSn_TxMAX(sn))
   {
      size = getSn_TxMAX(sn) - dst_mask;
      WIZCHIP_WRITE_BUF(dst_ptr, wizdata, size);
      wizdata += size;
      size = len - size;
      dst_ptr = getSn_TxBASE(sn);
      WIZCHIP_WRITE_BUF(dst_ptr, wizdata, size);
   }
   else
   {
      WIZCHIP_WRITE_BUF(dst_ptr, wizdata, len);
   }

   ptr += len;
   setSn_TX_WR(sn, ptr);
}

void wiz_recv_data(uint8_t sn, uint8_t *wizdata, uint16_t len)
{
   uint16_t ptr;
   uint16_t size;
   uint16_t src_mask;
   uint32_t src_ptr;

   if(len == 0) return;

   ptr = getSn_RX_RD(sn);
   src_mask = ptr & getSn_RxMASK(sn);
   src_ptr = getSn_RxBASE(sn) + src_mask;

   if((uint32_t)src_mask + len > getSn_RxMAX(sn))
   {
      size = getSn_RxMAX(sn) - src_mask;
      WIZCHIP_READ_BUF(src_ptr, wizdata, size);
      wizdata += size;
      size = len - size;
      src_ptr = getSn_RxBASE(sn);
      WIZCHIP_READ_BUF(src_ptr, wizdata, size);
   }
   else
   {
      WIZCHIP_READ_BUF(src_ptr, wizdata, len);
   }

   ptr += len;
   setSn_RX_RD(sn, ptr);
}

void wiz_recv_ignore(uint8_t sn, uint16_t len)
{
   uint16_t ptr;

   ptr = getSn_RX_RD(sn);
   ptr += len;
   setSn_RX_RD(sn, ptr);
}

#endif /* _WIZCHIP_ == W3150A */

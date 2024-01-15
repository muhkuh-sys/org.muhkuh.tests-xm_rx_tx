#ifndef __XM_RX_TX_H__
#define __XM_RX_TX_H__

typedef enum XM_TRANSMIT_FLAGS_ENUM
{
  XM_TRANSMIT_FLAGS_Use_IO0_as_OE                             = 0x00000001U,
  XM_TRANSMIT_FLAGS_Invert_OE                                 = 0x00000002U,
  XM_TRANSMIT_FLAGS_Invert_TX                                 = 0x00000004U

} XM_TRANSMIT_FLAGS_T;

int xm_receive(unsigned int uiUnit, unsigned long ulSampleClocks, unsigned long ulBufferSize, unsigned char *pucBuffer);
int xm_transmit(unsigned int uiUnit, unsigned long ulTransmitClocks, unsigned long ulBufferSize, const unsigned char *pucBuffer, unsigned long ulFlags);

#endif  /* __XM_RX_TX_H__ */

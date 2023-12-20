#ifndef __XM_RX_TX_H__
#define __XM_RX_TX_H__

int xm_receive(unsigned int uiUnit, unsigned long ulSampleClocks, unsigned long ulBufferSize, unsigned char *pucBuffer);
int xm_transmit(unsigned int uiUnit, unsigned long ulTransmitClocks, unsigned long ulBufferSize, const unsigned char *pucBuffer);

#endif  /* __XM_RX_TX_H__ */

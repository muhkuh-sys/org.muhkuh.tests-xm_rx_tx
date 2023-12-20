#include "xm_rx_tx.h"

#include "netx_io_areas.h"
#include "systime.h"
#include "uprintf.h"
#include "xc_reset.h"


static int setup_xc_unit(HOSTADEF(XMAC) *ptXmac, unsigned long ulClocks)
{
	HOSTDEF(ptXpec0Area);
	int iResult;
	unsigned long ulValue;


	/* Sample clocks must be a 16 bit value. */
	if( ulClocks>0xffff )
	{
		uprintf("The clocks value exceeds the allowed range of [0,65535]: %d", ulClocks);
		iResult = -1;
	}
	else
	{
		/* Setup the RX unix. */
		ptXmac->ulXmac_sbu_rate_mul_add   = 0x0001;
		ptXmac->ulXmac_sbu_rate_mul_start = 0xffffU - ulClocks;
		ptXmac->ulXmac_start_sample_pos   = 0xffffU - ulClocks + 32U;
		ptXmac->ulXmac_stop_sample_pos    = 0xffffU - 32U;

		ulValue  = 1U << HOSTSRT(xmac_config_sbu_count_modulo);
		ulValue |= HOSTMSK(xmac_config_sbu_sync_to_bitstream);
		ptXmac->ulXmac_config_sbu = ulValue;


		/* Setup the TX unit. */
		ulValue  = HOSTMSK(xmac_config_obu_count_modulo);
		ulValue |= HOSTMSK(xmac_config_obu_tx_three_state);
		ptXmac->ulXmac_config_obu = ulValue;

		ptXmac->ulXmac_obu_rate_mul_add = 0x0001U;
		ptXmac->ulXmac_obu_rate_mul_start = 0xffffU - ulClocks;
		ptXmac->ulXmac_start_trans_pos = 0xffffU - ulClocks;
		ptXmac->ulXmac_stop_trans_pos = 0xffffU;
		ptXmac->ulXmac_tx = 0xffffU;


		/* Set the IO0 and IO1 pins to input. */
		ulValue  = 0U << HOSTSRT(statcfg2_gpio0_out); // set output value of gpio0/IO0 to zero
		ulValue |= 0U << HOSTSRT(statcfg2_gpio0_oe);  // set gpio0/IO0 to input
		ulValue |= 0U << HOSTSRT(statcfg2_gpio1_out); // set output value of gpio1/IO1 to zero
		ulValue |= 0U << HOSTSRT(statcfg2_gpio1_oe);  // set gpio1/IO1 to input
		ptXpec0Area->aulStatcfg[2] = ulValue;

		iResult = 0;
	}

	return iResult;
}



static unsigned long aulBuf[256];

int xm_receive(unsigned int uiUnit, unsigned long ulSampleClocks, unsigned long ulBufferSize, unsigned char *pucBuffer)
{
	int iResult;
	unsigned long ulValue;
	unsigned long ulLastValue;
	unsigned char *pucCnt;
	unsigned char *pucEnd;
	unsigned long ulTimer;
	unsigned int uiBufCnt;
	unsigned int uiBufCnt2;
	HOSTADEF(XMAC) *ptXmac;
	HOSTDEF(ptSystimeArea);
	HOSTDEF(ptXmac2Area);


	if( uiUnit!=2 )
	{
		uprintf("Only unit 2 is supported for now.\n");
		iResult = -1;
	}
	else
	{
		/* Reset all XC units. */
		NX500_XC_Reset(0);
		NX500_XC_Reset(1);
		NX500_XC_Reset(2);
		NX500_XC_Reset(3);

		ptXmac = ptXmac2Area;
		iResult = setup_xc_unit(ptXmac, ulSampleClocks);
		if( iResult==0 )
		{
			/* Wait until the RX register is filled. This should take only a few cycles. */
			ulValue = 0xffffffffU;
			ulTimer = systime_get_ms();
			do
			{
				/* Wait at most 250ms. */
				if( (ptSystimeArea->ulSystime_s - ulTimer)>=250U )
				{
					iResult = -1;
					break;
				}

				ulValue = ptXmac2Area->ulXmac_rx;
			} while( ulValue!=0xffffU );
			if( iResult!=0 )
			{
				uprintf("The RX line did not reach a stable idle level after 250ms: 0x%04x\n", ulValue);
			}
			else
			{
				/* Wait at most 5 seconds for some data to arrive. */
				ulTimer = systime_get_ms();
				do
				{
					/* Wait at most 5000ms. */
					if( (ptSystimeArea->ulSystime_s - ulTimer)>=5000U )
					{
						iResult = -1;
						break;
					}

					ulValue = ptXmac2Area->ulXmac_rx;
				} while( ulValue==0xffffU );
				if( iResult!=0 )
				{
					uprintf("No data arrived in the last 5 seconds.\n");
				}
				else
				{
					/* Set the last value to something which will never happen.
					 * As the registers are 16bit only, the upper half will always be 0.
					 * Thus a value of 0xffff.... will never be read from the register.
					 */
					ulLastValue = 0xffffffffU;

					uiBufCnt = 0;

					ulTimer = systime_get_ms();
					pucCnt = pucBuffer;
					pucEnd = pucBuffer + ulBufferSize;
					while(pucCnt<pucEnd)
					{
						/* Wait for a valid frame.
						 * A valid frame starts with one low and one high bit, and ends with a low bit.
						 * With the data the frame looks like this:
						 *
						 * bit  1111110000000000
						 *      5432109876543210
						 * data xxxxx0DDDDDDDD10
						 * mask 0000010000000011
						 * fexp 0000000000000010
						 *
						 * The "mask" value extracts only the start and end bits. They must match the
						 * value defined in "fexp".
						 */
						ulValue = ptXmac2Area->ulXmac_rx;
						if( ulValue!=ulLastValue )
						{
							if( uiBufCnt<(sizeof(aulBuf)/sizeof(aulBuf[0])) )
							{
								aulBuf[uiBufCnt++] = ulValue;
							}

							if( (ulValue&0x00000403)==0x00000002U )
							{
								/* Receive a new character. */
								*(pucCnt++) = (unsigned char)(((ulValue & 0x000003FCU)>>2U)^0x000000ffU);

								/* Count 11 Bits from now. */
#if 0
								ptXmac2Area->ulXmac_rx_hw_count = 10U;
								ulTimer = systime_get_ms();
								do
								{
									/* Wait at most 64ms for 11 bits to pass. This is more than enough. */
									if( (ptSystimeArea->ulSystime_s - ulTimer)>=1000U )
									{
										iResult = -1;
										break;
									}

									ptXmac2Area->ulXmac_rx;
									ulValue = ptXmac2Area->ulXmac_rx_hw_count;
								} while( ulValue!=0x0000U );
#endif
								systime_delay_ms(10);
								ulTimer = systime_get_ms();
							}
							ulLastValue = ulValue;
						}
//						if( iResult!=0 )
//						{
//							uprintf("Timeout waiting for 11 bits to pass.\n");
//							break;
//						}

						/* Wait at most 5000ms. */
						if( (ptSystimeArea->ulSystime_s - ulTimer)>=5000U )
						{
							iResult = -1;
							uprintf("Timeout receiving the next character.\n");
							break;
						}
					}

					if( iResult==0 )
					{
						uprintf("Received data:\n");
						pucCnt = pucBuffer;
						while( pucCnt<pucEnd )
						{
							uprintf("  %02x\n", *(pucCnt++));
						}
						uprintf("Raw:\n");
						for(uiBufCnt2=0; uiBufCnt2<uiBufCnt; ++uiBufCnt2)
						{
							ulValue = aulBuf[uiBufCnt2];
							uprintf("  %04x %04x %02x\n", ulValue, ulValue&0x00000403, ((ulValue & 0x000003FCU)>>2U)^0x000000ffU);
						}
					}
				}
			}
		}
	}

	return iResult;
}



int xm_transmit(unsigned int uiUnit, unsigned long ulTransmitClocks, unsigned long ulBufferSize, const unsigned char *pucBuffer)
{
	int iResult;
	unsigned long ulValue;
	const unsigned char *pucCnt;
	const unsigned char *pucEnd;
	HOSTADEF(XMAC) *ptXmac;
	HOSTDEF(ptXmac2Area);
	HOSTDEF(ptXpec0Area);


	if( uiUnit!=2 )
	{
		uprintf("Only unit 2 is supported for now.\n");
		iResult = -1;
	}
	else
	{
		/* Reset all XC units. */
		NX500_XC_Reset(0);
		NX500_XC_Reset(1);
		NX500_XC_Reset(2);
		NX500_XC_Reset(3);

		ptXmac = ptXmac2Area;
		iResult = setup_xc_unit(ptXmac, ulTransmitClocks);
		if( iResult==0 )
		{
			/* Enable the driver for the TX pin. */
			ulValue  = HOSTMSK(xmac_config_obu_count_modulo);
			ptXmac2Area->ulXmac_config_obu = ulValue;

			/* Drive XM2_IO0 to 1. */
			/* FIXME: This is only necessary for the DP card. Control this with a parameter. */
			ulValue  = ptXpec0Area->aulStatcfg[2];
			ulValue |= HOSTMSK(statcfg2_gpio0_out);
			ulValue |= HOSTMSK(statcfg2_gpio0_oe);
			ptXpec0Area->aulStatcfg[2] = ulValue;

			/* Allow the TX line to settle. */
			systime_delay_ms(100);

			uprintf("Sending buffer:\n");
			hexdump(pucBuffer, ulBufferSize);

			/* Loop over the complete transmit buffer. */
			pucCnt = pucBuffer;
			pucEnd = pucBuffer + ulBufferSize;
			while( pucCnt<pucEnd )
			{
				/* Construct a new data frame. */
				ulValue  = 2U;                                              /* Set a start bit. */
				ulValue |= (((unsigned long)*pucCnt)^0x000000ffU) << 2U;    /* Add the inverted data. */
				ulValue |= 0x0000f800U;                                     /* Add 1 stop bit and go back to idle. */
				uprintf("Sending %04x\n", ulValue);

				ptXmac2Area->ulXmac_tx = ulValue;

				++pucCnt;
			}

			/* Disable the driver for the XM2_IO0 pin. */
			/* FIXME: This is only necessary for the DP card. Control this with a parameter. */
			ulValue  = ptXpec0Area->aulStatcfg[2];
			ulValue &= ~HOSTMSK(statcfg2_gpio0_out);
			ulValue &= ~HOSTMSK(statcfg2_gpio0_oe);
			ptXpec0Area->aulStatcfg[2] = ulValue;

			/* Disable the driver for the TX pin. */
			ulValue  = HOSTMSK(xmac_config_obu_count_modulo);
			ulValue |= HOSTMSK(xmac_config_obu_tx_three_state);
			ptXmac2Area->ulXmac_config_obu = ulValue;
		}
	}

	return iResult;
}

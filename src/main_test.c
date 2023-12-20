/***************************************************************************
 *   Copyright (C) 2023 by Christoph Thelen                                *
 *   doc_bacardi@users.sourceforge.net                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#include "main_test.h"

#include <string.h>

#include "rdy_run.h"
#include "serial_vectors.h"
#include "systime.h"
#include "uprintf.h"
#include "version.h"
#include "xm_rx_tx.h"

/*-------------------------------------------------------------------------*/

TEST_RESULT_T test(XM_RX_TX_PARAMETER_T *ptTestParams)
{
	TEST_RESULT_T tTestResult;
	int iResult;
	unsigned long ulMode_0Rx_1Tx;


	systime_init();

	uprintf("\f. *** XM RX/TX test by doc_bacardi@users.sourceforge.net ***\n");
	uprintf("V" VERSION_ALL "\n\n");

	/* Get the test parameter. */
	uprintf(".    Parameters: 0x%08x\n", (unsigned long)ptTestParams);
	uprintf(".          Mode: 0x%08x\n", ptTestParams->ulMode_0Rx_1Tx);
	uprintf(".          Unit: 0x%08x\n", ptTestParams->uiUnit);
	uprintf(". Sample clocks: 0x%08x\n", ptTestParams->ulSampleClocks);
	uprintf(".   Buffer size: 0x%08x\n", ptTestParams->ulBufferSize);
	uprintf(".        Buffer: 0x%08x\n", ptTestParams->pucBuffer);

	/* Be pessimistic. */
	iResult = -1;

	ulMode_0Rx_1Tx = ptTestParams->ulMode_0Rx_1Tx;
	if( ulMode_0Rx_1Tx==0U )
	{
		uprintf("Receive\n");
		iResult = xm_receive(ptTestParams->uiUnit, ptTestParams->ulSampleClocks, ptTestParams->ulBufferSize, ptTestParams->pucBuffer);
		if( iResult==0 )
		{
			hexdump(ptTestParams->pucBuffer, ptTestParams->ulBufferSize);
		}
	}
	else if( ulMode_0Rx_1Tx==1U )
	{
		uprintf("Transmit\n");
		iResult = xm_transmit(ptTestParams->uiUnit, ptTestParams->ulSampleClocks, ptTestParams->ulBufferSize, ptTestParams->pucBuffer);
	}
	else
	{
		uprintf("! Invalid mode: 0x%08x. Should be 0 or 1.\n", ulMode_0Rx_1Tx);
	}

	if( iResult==0 )
	{
		tTestResult = TEST_RESULT_OK;
	}
	else
	{
		tTestResult = TEST_RESULT_ERROR;
	}

	return tTestResult;
}

/*-----------------------------------*/


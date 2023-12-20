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


#ifndef __MAIN_TEST_H__
#define __MAIN_TEST_H__


typedef struct XM_RX_TX_PARAMETER_STRUCT
{
	unsigned long ulMode_0Rx_1Tx;
	unsigned int  uiUnit;
	unsigned long ulSampleClocks;
	unsigned long ulBufferSize;
	unsigned char *pucBuffer;
} XM_RX_TX_PARAMETER_T;



typedef enum TEST_RESULT_ENUM
{
	TEST_RESULT_OK = 0,
	TEST_RESULT_ERROR = 1
} TEST_RESULT_T;


TEST_RESULT_T test(XM_RX_TX_PARAMETER_T *ptTestParams);


#endif  /* __MAIN_TEST_H__ */


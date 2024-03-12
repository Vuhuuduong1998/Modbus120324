/*
 * modbus_ReadholdingReg.c
 *
 *  Created on: Mar 6, 2024
 *      Author: Admin
 */
#include "modbus_interrupt.h"
#include <stdint.h>
#include "app_uart.h"
//static UART_HandleTypeDef huartExe;
ModbusRequestFrame RequestFrame;
static uint8_t receiveData[50];
extern uint16_t ringCount; 
static SER_BUF_T* receiveRingBufferExe; 
void check_command()
{
	RequestFrame.slaveID = SLAVE_ID;
	if(ringCount == 2)
	{
		for(int i=0; i<2; i++)
		{
			receiveData[i] = receiveRingBufferExe->data[i];
		}
		 if( RequestFrame.slaveID == receiveData[0])
			 {
				 RequestFrame.function_code = receiveData[1];
				 switch(RequestFrame.function_code)
				 {
				 case READ_HOLDING_REGISTER :
					 readHoldingRegisters();
					 break;
				 case WRITE_MULTIPLE_REGISTER :
					 writeMultipleRegisters();
					 break;
				 default:
					 break;
				 }
				 ringCount =0; 
			 }
	 }
}
uint8_t writeMultipleRegisters()
{
	uint16_t crcValue;
	uint16_t numRegs;
	uint8_t byteCount;
	uint16_t startAddr;
	uint8_t ResponseData[8] ;
	uint8_t indx =7;
	while(ringCount < 7); 
	/*Receive 5 byte: startaddr high, startaddr low, quantity high, quantity low, bycount*/
	for (int i=2; i<7; i++)
	    {
	    	receiveData[i] = receiveRingBufferExe->data[i];
	    }
	byteCount =  receiveData[6]; 	// get byte count value
	while(ringCount < ( 7+ byteCount+2)); // for ex: now, ringCount =7;  bytecount = 4 -> bytes are needed to receive = 6 ( 4 + CRC High + CRC Low), 
	for(int i= 7; i<(byteCount + 9); i++)
	{
		receiveData[i] = receiveRingBufferExe->data[i];		// get data bytes ( receiveData[7] -> byteCount + 7) & 2 byte CRC -> byteCount + 9
	}
	RequestFrame.startAddress = (((uint16_t)receiveData[2]) << 8 ) | receiveData[3];
	RequestFrame.quantity = (((uint16_t)receiveData[4]) << 8 ) | receiveData[5];
	startAddr = RequestFrame.startAddress;
	numRegs = RequestFrame.quantity;
	/*exception report*/
	if ((numRegs<1)||(numRegs>125))  // maximum no. of Registers as per the PDF
		{
			modbusException (ILLEGAL_DATA_VALUE);  // send an exception
			return 0;
		}

	uint16_t endAddr = startAddr+numRegs-1;  // end Register
	if (endAddr>49)  // end Register can not be more than 49 as we only have record of 50 Registers in total
	{
		modbusException(ILLEGAL_DATA_ADDRESS);   // send an exception
		return 0;
	}
	/*Save to Holding registers*/
	for (int i=0; i<numRegs; i++)   // Load the actual data into TxData buffer
			{
			 Holding_Registers_Database[startAddr] = ((uint16_t)receiveData[indx++]) << 8   ;  // extract the higher byte Data
			 Holding_Registers_Database[startAddr] |= receiveData[indx++] ; // extract the lower byte Data
			 startAddr++;  // increment the register address
			}
	/*Response from slave to master : 8 byte( slaveid, function code, addrHigh, addrLow, QuantityHigh, QuantityLow, CRCLow, CRCHigh*/
	ResponseData[0] = RequestFrame.slaveID;
	ResponseData[1] = RequestFrame.function_code; // Write multiple register
	ResponseData[2] = (uint8_t)(RequestFrame.startAddress >> 8);
	ResponseData[3] = (uint8_t)(RequestFrame.startAddress);
	ResponseData[4] = (uint8_t)(RequestFrame.quantity >> 8);
	ResponseData[5] = (uint8_t)(RequestFrame.quantity);
	crcValue = calculateCRC16 (ResponseData, 6);
	ResponseData[6] = crcValue & 0xFF;
	ResponseData[7] = (crcValue >> 8) & 0xFF;
	for (int i =0 ; i< 8; i++)
		{
			uartSendByte(ResponseData[i]);
		}
	return 1;
}
uint8_t readHoldingRegisters()
{
	// Implement the Modbus read holding registers function

    // Receive the response frame


    uint16_t crcValue;
    uint16_t numRegs;
    uint16_t startAddr;
    uint8_t ResponseData[50] ;
    uint8_t indx =3;
		while(ringCount < 8);		// wait the ring buffer receive enough 8 byte ( receiveRingBufferExe->data[2]  -> receiveRingBufferExe->data[7]
    /*Receive 6 bytes from master( starAddress High, startAddress Low, Quantity High, Quantity Low, CRC Low, CRC High) */
    for (int i=2; i<8; i++)
    {
    	receiveData[i] = receiveRingBufferExe->data[i];
    }
	RequestFrame.startAddress = (((uint16_t)receiveData[2]) << 8 ) | receiveData[3];
	RequestFrame.quantity = (((uint16_t)receiveData[4]) << 8 ) | receiveData[5];
	RequestFrame.crc = receiveData[6] | (((uint16_t)receiveData[7]) << 8 );
	/*Implement response from slave to master*/
	startAddr = RequestFrame.startAddress;	// get the address to access register
	numRegs = RequestFrame.quantity;	// use for calculating byte count

	/*exception report*/
	if ((numRegs<1)||(numRegs>125))  // maximum no. of Registers as per the PDF
		{
			modbusException (ILLEGAL_DATA_VALUE);  // send an exception
			return 0;
		}

	uint16_t endAddr = startAddr+numRegs-1;  // end Register
	if (endAddr>49)  // end Register can not be more than 49 as we only have record of 50 Registers in total
	{
		modbusException(ILLEGAL_DATA_ADDRESS);   // send an exception
		return 0;
	}
	/*Clear the response data*/
	for(int i= 0; i<sizeof(ResponseData); i++)
	{
		ResponseData[i] = 0;
	}
	/*Send the response from slave to master*/
	ResponseData[0] = RequestFrame.slaveID;
	ResponseData[1] = RequestFrame.function_code; // Read Holding Registers
	ResponseData[2] = numRegs*2; // Byte_count = 2
	for (int i=0; i<numRegs; i++)   // Load the actual data into TxData buffer
		{
		ResponseData[indx++] = (Holding_Registers_Database[startAddr]>>8)&0xFF;  // extract the higher byte Data
		ResponseData[indx++] = (Holding_Registers_Database[startAddr])&0xFF;   // extract the lower byte Data
		startAddr++;  // increment the holding register address
		}
	crcValue = calculateCRC16 (ResponseData, indx);
	ResponseData[indx] = crcValue & 0xFF;
	ResponseData[indx+1] = (crcValue >> 8) & 0xFF;
	for (int i =0 ; i< (indx+2); i++)
	{
		uartSendByte(ResponseData[i]);
	}
	return 1;
}
void modbusException (uint8_t exceptioncode)
{
	uint8_t ExeptionData[5];
	uint16_t crcValue;
	ExeptionData[0] =  RequestFrame.slaveID;
	ExeptionData[1] = RequestFrame.function_code | 0x80;
	ExeptionData[2] = exceptioncode;
	crcValue = calculateCRC16(ExeptionData, 3);
	ExeptionData[3] = crcValue & 0xFF;
	ExeptionData[4] = (crcValue >> 8) & 0xFF;
	for(int i=0; i<5; i++)
	{
		uartSendByte(ExeptionData[i]);
	}
}
uint16_t calculateCRC16(uint8_t *data, uint8_t length) {
    uint16_t crc = 0xFFFF;
    uint8_t i;

    while (length--) {
        crc ^= *data++;
        for (i = 0; i < 8; i++) {
            if (crc & 0x0001) {
                crc >>= 1;
                crc ^= 0xA001; // Polynomial for Modbus CRC16
            } else {
                crc >>= 1;
            }
        }
    }

    return crc;
}
void uartSendByte(uint8_t data)
{
	while (app_uart_put(data) != NRF_SUCCESS);
}


uint8_t uartReceiveByte()
{
	uint8_t data = 0;
	while (app_uart_get(&data) != NRF_SUCCESS);
	return data;
}
void ringBufferCoppy(SER_BUF_T* ringbuffer)
{
	receiveRingBufferExe = ringbuffer; 
}

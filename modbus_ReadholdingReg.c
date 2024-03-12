/*
 * modbus_ReadholdingReg.c
 *
 *  Created on: Mar 6, 2024
 *      Author: Admin
 */
 
 
#include "modbus_ReadholdingReg.h"
//#include "main.h"
#include <stdint.h>
#include "app_uart.h"
#define FUNCTION_SUCESS		1 
static ModbusRequestFrame RequestFrame;
uint8_t receiveUart[50];
uint8_t uartCount =0; 

void check_command()
{
	if(uartCount != 0)
	{
		RequestFrame.slaveID = SLAVE_ID;
//		for(int i=0; i<2; i++)
//		{
//			receiveUart[i] = uartReceiveByte();
//		}
		 if( RequestFrame.slaveID == receiveUart[0])
			 {
				 while (uartCount < 2); 	// wait UART receive function code 
				 RequestFrame.function_code = receiveUart[1];
				 switch(RequestFrame.function_code)
				 {
					 case READ_HOLDING_REGISTER :
						 if(readHoldingRegistersUART()!=FUNCTION_SUCESS)
						 {
								// error -> do something 

						 }							
						 break;
					 case WRITE_MULTIPLE_REGISTER :
						 if(writeMultipleRegistersUART() != FUNCTION_SUCESS)
						 {
								// error -> do something 

						 }
						 break;
					 case WRITE_SINGLE_REGISTER :
						 if(writeSingleRegisterUART() != FUNCTION_SUCESS)
						 {
								// error -> do something 

						 }
						 break;							 
					 default:
						 break;
				}
			}
		uartCount =0; 
 }
}

uint8_t writeSingleRegisterUART()
{
	uint16_t crcValue;
	uint16_t startAddr;
	uint8_t ResponseData[50] ;
//	uint8_t indx =3;

	while (uartCount < 8);
	RequestFrame.startAddress = (((uint16_t)receiveUart[2]) << 8 ) | receiveUart[3];
	RequestFrame.crc = receiveUart[6] | (((uint16_t)receiveUart[7]) << 8 );
	/*Implement response from slave to master*/
	startAddr = RequestFrame.startAddress;	// get the address to access register
	if (startAddr>49)  // end Register can not be more than 49 as we only have record of 50 Registers in total
	{
		modbusException(ILLEGAL_DATA_ADDRESS);   // send an exception
		return 0;
	}
	Holding_Registers_Database[startAddr] = (((uint16_t)receiveUart[4]) << 8 ) | receiveUart[5];
	/*Clear the response data*/
	for(int i= 0; i<sizeof(ResponseData); i++)
	{
		ResponseData[i] = 0;
	}
	/*Send the response from slave to master*/
	for(int i=0; i<6; i++) 
	{
		ResponseData[i] = receiveUart[i]; 
	}
	crcValue = calculateCRC16 (ResponseData, 6);
	ResponseData[6] = crcValue & 0xFF;
	ResponseData[7] = (crcValue >> 8) & 0xFF;
	for (int i =0 ; i< 8; i++)
	{
		uartSendByte(ResponseData[i]);
	}
	return 1;
}

uint8_t writeMultipleRegistersUART()
{
	uint16_t crcValue;
	uint16_t numRegs;
	uint8_t byteCount;
	uint16_t startAddr;
	uint8_t ResponseData[8] ;
	uint8_t indx =7;
	uint32_t endAddr;
	/*Receive 5 byte: startaddr high, startaddr low, quantity high, quantity low, bycount*/
//	for (int i=2; i<7; i++)
//	    {
//	    	receiveUart[i] = uartReceiveByte();
//	    }
	while (uartCount < 7); 
	byteCount =  receiveUart[6]; 	// get byte count value
//	for(int i= 7; i<(byteCount + 9); i++)
//	{
//		receiveUart[i] = uartReceiveByte();		// get data bytes ( 7 -> byteCount + 7) & 2 byte CRC -> byteCount + 9
//	}
	while (uartCount < (byteCount + 9)); 
	RequestFrame.startAddress = (((uint16_t)receiveUart[2]) << 8 ) | receiveUart[3];
	RequestFrame.quantity = (((uint16_t)receiveUart[4]) << 8 ) | receiveUart[5];
	startAddr = RequestFrame.startAddress;
	numRegs = RequestFrame.quantity;
	/*exception report*/
	if ((numRegs<1)||(numRegs>125))  // maximum no. of Registers as per the PDF
		{
			modbusException (ILLEGAL_DATA_VALUE);  // send an exception
			return 0;
		}

	endAddr = startAddr+numRegs-1;  // end Register
	if (endAddr>49)  // end Register can not be more than 49 as we only have record of 50 Registers in total
	{
		modbusException(ILLEGAL_DATA_ADDRESS);   // send an exception
		return 0;
	}
	/*Save to Holding registers*/
	for (int i=0; i<numRegs; i++)   // Load the actual data into TxData buffer
			{
			 Holding_Registers_Database[startAddr] = ((uint16_t)receiveUart[indx++]) << 8   ;  // extract the higher byte Data
			 Holding_Registers_Database[startAddr] |= receiveUart[indx++] ; // extract the lower byte Data
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
//	while(uartDataTransmitCompleted < 7); 		// wait the Tx FIFO Empty -> transmit is completed 
	return 1;
}
uint8_t readHoldingRegistersUART(void)
{
	// Implement the Modbus read holding registers function

    // Receive the response frame


    uint16_t crcValue;
    uint16_t numRegs;
    uint16_t startAddr;
    uint8_t ResponseData[50] ;
    uint8_t indx =3;
		uint32_t endAddr;

    /*Receive 6 bytes from master( starAddress High, startAddress Low, Quantity High, Quantity Low, CRC Low, CRC High) */
//    for (int i=2; i<8; i++)
//    {
//    	receiveUart[i] = uartReceiveByte();
//    }
	while (uartCount < 8);
	RequestFrame.startAddress = (((uint16_t)receiveUart[2]) << 8 ) | receiveUart[3];
	RequestFrame.quantity = (((uint16_t)receiveUart[4]) << 8 ) | receiveUart[5];
	RequestFrame.crc = receiveUart[6] | (((uint16_t)receiveUart[7]) << 8 );
	/*Implement response from slave to master*/
	startAddr = RequestFrame.startAddress;	// get the address to access register
	numRegs = RequestFrame.quantity;	// use for calculating byte count

	/*exception report*/
	if ((numRegs<1)||(numRegs>125))  // maximum no. of Registers as per the PDF
		{
			modbusException (ILLEGAL_DATA_VALUE);  // send an exception
			return 0;
		}

	endAddr = startAddr+numRegs-1;  // end Register
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
//	while(uartDataTransmitCompleted < (indx + 1)); 		// wait the Tx FIFO Empty -> transmit is completed 
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
	for(uint8_t i=0; i<5; i++)
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

void writeToHoldingRegister(uint8_t address, uint16_t data)
{
	Holding_Registers_Database[address] = data; 
}


uint16_t readFromHoldingRegister(uint8_t address)
{
	uint16_t data; 
	data = Holding_Registers_Database[address]; 
	return data; 
}

//void readHoldingRegistersRequest(uint8_t slaveID, uint8_t functionCode, uint16_t startAddr, uint16_t quantity)
//{
//	uint8_t requestFrame[8]; 
//	uint16_t crcValue; 
//	requestFrame[0] = slaveID; 
//	requestFrame[1] = functionCode; 
//	requestFrame[2] = startAddr >> 8; 
//	requestFrame[3] = (uint8_t) startAddr; 
//	requestFrame[4] = quantity >> 8; 
//	requestFrame[5] = (uint8_t) quantity ; 
//	crcValue = calculateCRC16(requestFrame,6); 
//	requestFrame[6] = crcValue & 0xFF;
//	requestFrame[7] = (crcValue >> 8) & 0xFF;
//	for(int i=0; i<8; i++)
//	{
//		uartSendByte(requestFrame[i]);
//	}
//}
 
void writeMultipleRegistersBleRequest(uint8_t slaveID, uint8_t functionCode, uint16_t startAddr, uint16_t quantity, uint16_t* data)
{
	uint8_t requestFrame[50]; 
	uint16_t crcValue; 
	uint8_t byteCount; 
//	uint8_t dataCount = 0; 
	uint8_t indx = 7; 
	requestFrame[0] = slaveID; 
	requestFrame[1] = functionCode; 
	requestFrame[2] = startAddr >> 8; 
	requestFrame[3] = (uint8_t) startAddr; 
	requestFrame[4] = quantity >> 8; 
	requestFrame[5] = (uint8_t) quantity ; 
	byteCount = quantity * 2; 
	requestFrame[6] = byteCount; 
	for(int i=0; i< quantity ; i++) 
	{
		requestFrame[indx++] = (*data) >> 8; 
		requestFrame[indx++] = (uint8_t) (*data);
		data ++; 
	}
	crcValue = calculateCRC16(requestFrame,indx);
	requestFrame[indx] = crcValue & 0xFF;
	requestFrame[indx+1] = (crcValue >> 8) & 0xFF;
	for (int i =0 ; i< (indx+2); i++)
	{
		uartSendByte(requestFrame[i]);
	}
}

void writeMultipleRegistersBleResponse(uint8_t * ble_data, uint8_t size)
{
	uint16_t numRegs;
	uint8_t byteCount;
	uint16_t startAddr;
	uint8_t indx =7;
//	uint32_t endAddr;
	byteCount =  ble_data[6]; 	// get byte count value
	RequestFrame.startAddress = (((uint16_t)ble_data[2]) << 8 ) | ble_data[3];
	RequestFrame.quantity = (((uint16_t)ble_data[4]) << 8 ) | ble_data[5];
	startAddr = RequestFrame.startAddress;
	numRegs = RequestFrame.quantity;
	/*exception report*/
//	if ((numRegs<1)||(numRegs>125))  // maximum no. of Registers as per the PDF
//		{
//			modbusException (ILLEGAL_DATA_VALUE);  // send an exception
//			return 0;
//		}

//	endAddr = startAddr+numRegs-1;  // end Register
//	if (endAddr>49)  // end Register can not be more than 49 as we only have record of 50 Registers in total
//	{
//		modbusException(ILLEGAL_DATA_ADDRESS);   // send an exception
//		return 0;
//	}
	/*Save to Holding registers*/
	for (int i=0; i<numRegs; i++)   // Load the actual data into TxData buffer
			{
			 Holding_Registers_Database[startAddr] = ((uint16_t)ble_data[indx++]) << 8   ;  // extract the higher byte Data
			 Holding_Registers_Database[startAddr] |= ble_data[indx++] ; // extract the lower byte Data
			 startAddr++;  // increment the register address
			}
	/*Response from slave to master : 8 byte( slaveid, function code, addrHigh, addrLow, QuantityHigh, QuantityLow, CRCLow, CRCHigh*/
//	ResponseData[0] = RequestFrame.slaveID;
//	ResponseData[1] = RequestFrame.function_code; // Write multiple register
//	ResponseData[2] = (uint8_t)(RequestFrame.startAddress >> 8);
//	ResponseData[3] = (uint8_t)(RequestFrame.startAddress);
//	ResponseData[4] = (uint8_t)(RequestFrame.quantity >> 8);
//	ResponseData[5] = (uint8_t)(RequestFrame.quantity);
//	crcValue = calculateCRC16 (ResponseData, 6);
//	ResponseData[6] = crcValue & 0xFF;
//	ResponseData[7] = (crcValue >> 8) & 0xFF;
//	for (int i =0 ; i< 8; i++)
//		{
//			uartSendByte(ResponseData[i]);
//		}
}

void writeSingleRegisterBleRequest(uint8_t slaveID, uint8_t functionCode, uint16_t startAddr, uint16_t data)
{
	uint8_t requestFrame[8]; 
	uint16_t crcValue; 
	requestFrame[0] = slaveID; 
	requestFrame[1] = functionCode; 
	requestFrame[2] = startAddr >> 8; 
	requestFrame[3] = (uint8_t) startAddr ; 
	requestFrame[4] = data >> 8; 
	requestFrame[5] = (uint8_t) data;
	crcValue = calculateCRC16(requestFrame,6);
	requestFrame[6] = crcValue & 0xFF;
	requestFrame[7] = (crcValue >> 8) & 0xFF;
	for (int i =0 ; i< 8; i++)
	{
		uartSendByte(requestFrame[i]);
	}
}

void writeSingleRegisterBleResponse(uint8_t* ble_data, uint8_t size)
{
	if(size == 8) 
	{
		uint16_t startAddr;
		startAddr = (((uint16_t)ble_data[2]) << 8 ) | ble_data[3];
		Holding_Registers_Database[startAddr] = (((uint16_t)ble_data[4]) << 8 ) | ble_data[5];
	}
}



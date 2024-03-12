/*
 * modbus_ReadholdingReg.h
 *
 *  Created on: Mar 6, 2024
 *      Author: Admin
 */

#ifndef INC_MODBUS_INTERRUPT_H_
#define INC_MODBUS_INTERRUPT_H_

#include <stdio.h>
#include <stdint.h>


#define SER_BUF_SIZE 	(512)
#define SER_BUF_MASK	(SER_BUF_SIZE - 1ul)

#define SER_BUF_RESET(serBuf)		(serBuf.rdIdx = serBuf.wrIdx = 0)
#define SER_BUF_WR(serBuf, dataIn)	(serBuf.data[SER_BUF_MASK & serBuf.wrIdx++] = (dataIn))
#define SER_BUF_RD(serBuf)			(serBuf.data[SER_BUF_MASK & serBuf.rdIdx++])
#define SER_BUF_EMPTY(serBuf)		(serBuf.rdIdx == serBuf.wrIdx)
#define SER_BUF_FULL(serBuf)		(serBuf.wrIdx == ((serBuf.rdIdx + SER_BUF_SIZE) & 0x0FFFF))
#define SER_BUF_RQ_WR(serBuf, dataIn)	(serBuf.data[SER_BUF_MASK & serBuf.wrIdx] = (dataIn))

/*define Slave ID for the device*/
#define SLAVE_ID						(0x11)
/*define the function code for the device*/
#define READ_HOLDING_REGISTER   		(0x03)
#define WRITE_MULTIPLE_REGISTER 		(0x10)
/*define illegal data and illegal address*/
#define ILLEGAL_DATA_ADDRESS   0x02
#define ILLEGAL_DATA_VALUE     0x03

typedef struct __SER_BUF_T {
	unsigned char data[SER_BUF_SIZE];
	unsigned short wrIdx;
	unsigned short rdIdx;
} SER_BUF_T;
//SER_BUF_T receive_buf;

typedef struct {
	uint8_t slaveID;
	uint8_t function_code;
	uint16_t startAddress;
	uint16_t quantity;
	uint16_t crc;
}ModbusRequestFrame ;


static uint16_t Holding_Registers_Database[50]={
		0000,  1111,  2222,  3333,  4444,  5555,  6666,  7777,  8888,  9999,   // 0-9   40001-40010
		12345, 15432, 15535, 10234, 19876, 13579, 10293, 19827, 13456, 14567,  // 10-19 40011-40020
		21345, 22345, 24567, 25678, 26789, 24680, 20394, 29384, 26937, 27654,  // 20-29 40021-40030
		31245, 31456, 34567, 35678, 36789, 37890, 30948, 34958, 35867, 36092,  // 30-39 40031-40040
		45678, 46789, 47890, 41235, 42356, 43567, 40596, 49586, 48765, 41029,  // 40-49 40041-40050
};

uint16_t calculateCRC16(uint8_t* data, uint8_t length);
void check_command(void);
uint8_t readHoldingRegisters(void);
uint8_t writeMultipleRegisters(void);
void uartSendByte(uint8_t data);
uint8_t uartReceiveByte(void);
//void uart_coppy(UART_HandleTypeDef huart);
void modbusException (uint8_t exceptioncode);
void ringBufferCoppy(SER_BUF_T* ringbuffer);

#endif /* INC_MODBUS_READHOLDINGREG_H_ */

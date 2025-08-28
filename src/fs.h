#pragma once
#include <stdint.h>
#include "timer.h"
#include "crc.h"
#include "encoders/fsk.h"

constexpr int rewindMillisPerChunk = 1500;
constexpr uint32_t bytesPerChunk = 512;
constexpr int headerByteCount = 6;
constexpr int crcByteCount = 2;
constexpr int chunkBufferSize = bytesPerChunk + syncByteCount + headerByteCount + crcByteCount;
constexpr int chunkRepeats = 2;
uint8_t chunkBuffer[chunkBufferSize];

void storeData(int freq, uint8_t *data, uint32_t size)
{
	uint16_t numberOfChunks = (size + bytesPerChunk - 1) / bytesPerChunk;
	uint32_t totalBytesStored = 0;
	//things to store
	//sync signal (4 bytes)
	//included in crc:
		//chunk number 	(2 bytes)
		//total chunks 	(2 bytes)
		//size (2 bytes)
		//data 
		//crc16 (2bytes)

	for(int i = 0; i < numberOfChunks; i++)
	{
		uint16_t chunkSize = i == numberOfChunks - 1 ? size - totalBytesStored : bytesPerChunk;
		uint16_t header[3];
		header[0] = i;
		header[1] = numberOfChunks;
		header[2] = chunkSize;
		uint16_t crc = crc16(&(data[totalBytesStored]), chunkSize);
	
		for(int j = 0; j < chunkRepeats; j++)
		{
			//update chunk states to host
			Serial.write((uint8_t)2);	//chunkStates
			Serial.write((uint8_t)numberOfChunks);
			for(int k = 0; k < numberOfChunks; k++)
				Serial.write(k <= i ? 1 : 0);
			Serial.flush();	
			
			encodePacket(freq, &(data[totalBytesStored]), chunkSize, (uint8_t*)header, headerByteCount, crc);
			delayMs(200);
		}

		totalBytesStored += chunkSize;
	}
}

uint8_t *loadChunk(int freq, int &chunkIndex, int &chunkCount, int &chunkSize, int &state, int timeout = 10000)
{
	state = 0;	//no chunks found
	int decodedBytes = 0;
	bool packetFound = decodePacket(freq, chunkBuffer, chunkBufferSize, decodedBytes, timeout);
	if(!packetFound)
	{
		//no chunks found
		return 0;
	}
	state = 3;	//chunk number not valid yet
	if(decodedBytes < syncByteCount + headerByteCount + crcByteCount)
	{
		//not enough data
		return 0;
	}
	uint16_t *header = (uint16_t*)&(chunkBuffer[syncByteCount]);
	chunkIndex = header[0];
	int newChunkCount = header[1];
	chunkSize = header[2];
	if(chunkIndex >= newChunkCount)
	{
		//header broken?
		return 0;
	}
	state = 2;	//chunk number probably valid but not data wrong
	if(decodedBytes < syncByteCount + headerByteCount + crcByteCount + chunkSize)
	{
		//not enough data
		return 0;
	}
	int dataOffset = syncByteCount + headerByteCount;
	int crcOffset = dataOffset + chunkSize;
	uint16_t crc = crc16(&(chunkBuffer[dataOffset]), chunkSize);
	if(crc != (chunkBuffer[crcOffset] | ((int)chunkBuffer[crcOffset + 1]) << 8))
	{
		//crc failed
		return 0;
	}
	state = 1;	//chunk ok
	chunkCount = newChunkCount;
	return &chunkBuffer[syncByteCount + headerByteCount]; //skip header;
}

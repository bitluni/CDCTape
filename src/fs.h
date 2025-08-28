#pragma once
#include <stdint.h>
#include "timer.h"
#include "crc.h"
#include "encoders/fsk.h"

constexpr uint32_t bytesPerChunk = 512;
constexpr int headerByteCount = 6;
constexpr int crcByteCount = 2;
constexpr int chunkBufferSize = bytesPerChunk + syncByteCount + headerByteCount + crcByteCount;
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
		encodePacket(freq, &(data[totalBytesStored]), chunkSize, (uint8_t*)header, headerByteCount, crc);
		totalBytesStored += chunkSize;
		delayMs(200);
	}
}

bool loadData(int freq, uint8_t *decodedBuffer, int decodedBufferSize, uint8_t *chunkStates, uint16_t maxChunks, int &chunkCount, int &lastWrittenByteNumber, int timeout = 10000)
{
	lastWrittenByteNumber = 0;
	bool foundAtLeastOneChunk = false;
	while(1)
	{
		int chunkNumber = 0xffff;
		int numberOfChunks = 0;
		int chunkSize = 0;
		bool chunkOk = true;
		for(int i = 0; i < 1; i++)
		{
			int decodedBytes = 0;
			bool packetFound = decodePacket(freq, chunkBuffer, chunkBufferSize, decodedBytes, timeout);
			if(!packetFound)
			{
				//no chunks found
				return foundAtLeastOneChunk;
			}
			if(decodedBytes < syncByteCount + headerByteCount + crcByteCount)
			{
				chunkOk = false;
				//not enough data
				break;
			}
			uint16_t *header = (uint16_t*)&(chunkBuffer[syncByteCount]);
			chunkNumber = header[0];
			numberOfChunks = header[1];
			chunkSize = header[2];
			if(decodedBytes < syncByteCount + headerByteCount + crcByteCount + chunkSize)
			{
				chunkOk = false;
				//not enough data
				break;
			}
			if(chunkNumber >= numberOfChunks)
			{
				chunkOk = false;
				//header broken?
				break;
			}
			if(chunkNumber >= maxChunks)
			{
				chunkOk = false;
				//clip chunks that dont fit
				break;
			}
			int dataOffset = syncByteCount + headerByteCount;
			int crcOffset = dataOffset + chunkSize;
			uint16_t crc = crc16(&(chunkBuffer[dataOffset]), chunkSize);
			if(crc != (chunkBuffer[crcOffset] | ((int)chunkBuffer[crcOffset + 1]) << 8))
			{
				chunkOk = false;
				//crc failed
				break;
			}
		}
		if(chunkOk)
		{
			if(chunkStates[chunkNumber] != 1)	// not read yet
			{
				//copy data
				int chunkOffset = bytesPerChunk * chunkNumber;
				for(int i = 0; i < chunkSize; i++)
				{
					decodedBuffer[chunkOffset + i] = chunkBuffer[syncByteCount + headerByteCount + i];
				}
				if(lastWrittenByteNumber < chunkOffset + chunkSize)
					lastWrittenByteNumber = chunkOffset + chunkSize;
				chunkStates[chunkNumber] = 1; //ok
				chunkCount = numberOfChunks;
			}
			foundAtLeastOneChunk = true;
		}
		else
		{
			if(chunkStates[chunkNumber] == 0)
				chunkStates[chunkNumber] = 2;	//found but faulty
		}
		if(chunkNumber == numberOfChunks - 1)
		{
			return foundAtLeastOneChunk;
		}
	}
	return foundAtLeastOneChunk;
}
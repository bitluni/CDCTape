#ifndef __USB_SERIAL_H
#define __USB_SERIAL_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "usb_lib.h"
//#include <cstdarg>
#include <string.h>

uint8_t USBSerial_config(uint32_t baud, uint8_t stop_bit, uint8_t parity_bit, uint8_t data_bit, uint8_t timeout);
void USBSerial_begin(); // inits pins and itc

uint32_t USBSerial_available(); // returen bytes in buffer
unsigned char USBSerial_read();      // returns first unread byte from buffer

unsigned char USBSerial_connected(); // check ep0, 1 connected
signed char USBSerial_writeReady();  // -1 not connected, 0 connected but not write ready, 1 connected and writeready

void USBSerial_write(unsigned char *data, unsigned short len); // writes bytes
void USBSerial_flush(); // write out the tx buffer, called if txbuffer is at 64 bytes

#ifdef __cplusplus
}
#endif

#if defined(__cplusplus)

const uint8_t SERIAL_8N1 = 0x00;

class USBSerial
{
public:
    void begin(uint32_t baud, uint8_t config = SERIAL_8N1, uint8_t timeout = 30)
    {
        _baud = baud;
        _config = config;
        _stop_bit = _config & 0x08 ? 2 : 1;
        _parity_bit = _config >> 4;
        _data_bit = _config & 0x01 ? 9 : 8;
        USBSerial_config(_baud, _stop_bit, _parity_bit, _data_bit, timeout);
        USBSerial_begin();

        while (!USBSerial_connected())
            ;
    }

    inline size_t available(void)
    {
        return USBSerial_available();
    }

    inline uint8_t availableForWrite(void)
    {
        return USBSerial_writeReady();
    }

    inline uint8_t read(void)
    {
        return USBSerial_read();
    }

    inline size_t write(const uint8_t *buffer, size_t size)
    {
        USBSerial_write((unsigned char *)buffer, size);
        USBSerial_flush();
        return size;
    }
    template <typename T>
    inline size_t write(T n)
    {
        return write((uint8_t*)&n, sizeof(uint8_t));
    }

    inline void print(const char *s)
    {
        write((const uint8_t *)s, strlen(s));
    }
    inline void println(const char *s)
    {
        write((const uint8_t *)s, strlen(s));
        write((const uint8_t *)"\n", 1);
    }
	/*
    inline void printf(const char *format, ...)
		
    {
        char buf[64];
        va_list args;
        va_start(args, format);
        vsnprintf(buf, 64, format, args);
        va_end(args);
        write((const uint8_t *)buf, strlen(buf));
    }*/

    inline void flush()
    {
        USBSerial_flush();
    }

    inline operator bool()
    {
        return USBSerial_connected();
    }

private:
    uint32_t _baud;
    uint8_t _config;
    uint8_t _stop_bit;
    uint8_t _parity_bit;
    uint8_t _data_bit;
};

#endif

#endif

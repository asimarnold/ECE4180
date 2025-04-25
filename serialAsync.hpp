/*
 * Serial Async Class
 *
 * This UART driver allows for asynchronous serial communication using
 * hardware DMA channels. Asynchronous writing and reading is supported.
 *
 * (c) Daniel Cooper
 */

#ifndef COLLECTION_SERIAL_INCLUDED
#define COLLECTION_SERIAL_INCLUDED

#include "mbed.h"
#include "dma.h"

class SerialAsync {

    private:

    serial_t serial;
    DMA_CHANNEL* txDma;
    DMA_CHANNEL* rxDma;

    volatile void* receiveBuffer;
    int receiveBufferLength;
    volatile void* transmitBuffer; //If not nullptr implies it should free the address

    void writeCommon(void* buffer, int size);

    public:

    enum StopBits {
        StopBitOne,
        StopBitsTwo
    };

    enum WordLength {
        WordLength5,
        WordLength6,
        WordLength7,
        WordLength8
    };

    SerialAsync(PinName tx, PinName rx);
    
    SerialAsync(PinName tx, PinName rx, int baudrate);

    ~SerialAsync();

    void setBaud(int baudrate);

    /**
     * Waits for any outstanding transmissions to complete. A blocking function.
     */
    void sync();

    /**
     * Reads bytes into the buffer, up to the amount indicated by size.
     * NOTE: Returns instantly with only what was in the receive buffer prior to the call.
     * Can only read up to 16 bytes unless a receive buffer is specified in advance
     * @param buffer the buffer to write read values to
     * @param size the size of the buffer
     * @return the number of bytes read
     */
    int read(void* buffer, int size);

    /**
     * Sets the recieve buffer to be filled with data when it arrives
     * @param buffer A pointer to the first byte of the buffer to use for asynchronous receciving
     * @param size The size of the provided buffer in bytes
     */
    void setReceiveBuffer(void* buffer, int size);

    /**
     * Sets the control paramaters for the UART channel
     * @param parity The parity mode to use
     * @param stop The number of stop bits to use
     * @param bits The length of a transmitted word to use
     */
    void setControl(SerialParity parity, StopBits stop, WordLength bits);

    /**
     * Writes the data in the buffer asynchronously
     * NOTE: This function is non-blocking and will return immediately
     * @param buffer The buffer to transmit
     * @param size The size of the buffer in bytes
     */
    void write(void* buffer, int size);

    /**
     * Writes the data in the buffer asynchronously and frees the data when complete with the transfer.
     * NOTE: This function is non-blocking and will return immediately.
     * @param buffer The buffer to transmit
     * @param size The size of the buffer in bytes
     */
    void writeAndFree(void* buffer, int size);

    /**
     * Flushes the receiving buffer, all data in it will be lost.
     */
    void flushReceiving();

    /**
     * If there is an ISR possibility, then this function must be used to manually
     * free asynchronously written data. This function is blocking until no data
     * is left to be transmitted. This function may NOT under any circumstances be
     * called from an ISR.
     */
    void checkBufferFree();
};

#endif // COLLECTION_SERIAL_INCLUDED
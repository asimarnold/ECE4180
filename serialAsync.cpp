

#include "serialAsync.hpp"
#include <cstdint>
#include <stdint.h>
#include "collectionCommon.hpp"

SerialAsync::SerialAsync(PinName tx, PinName rx) : SerialAsync(tx, rx, 9600) {
    // defaults to baud rate of 9600
}
    
SerialAsync::SerialAsync(PinName tx, PinName rx, int baudrate) {
    serial_init(&this->serial, tx, rx);
    serial_baud(&this->serial, baudrate);

    this->rxDma = allocateDMA();
    this->txDma = allocateDMA();
    //configuring dmas
    this->rxDma->transferType = TRANSFER_PERIPHERAL_TO_MEMORY;
    this->rxDma->destination = DMA_MEMORY;
    this->rxDma->source = (DMA_PERIPHERAL)((int)DMA_UART0_RX + this->serial.index * 2);
    this->rxDma->sourceBurst = DMA_BURST_1;
    this->rxDma->destBurst = DMA_BURST_1;
    this->rxDma->sourceMode = DMA_ADDRESS_STATIC;
    this->rxDma->destMode = DMA_ADDRESS_INCREMENT;
    this->rxDma->sourceWidth = TRANSFER_WIDTH_BYTE;
    this->rxDma->destWidth = TRANSFER_WIDTH_BYTE;
    this->rxDma->sourceAddr = (unsigned long int) &(this->serial.uart->RBR);

    this->txDma->transferType = TRANSFER_MEMORY_TO_PERIPHERAL;
    this->txDma->destination = (DMA_PERIPHERAL)((int)DMA_UART0_TX + this->serial.index * 2);
    this->txDma->source = DMA_MEMORY;
    this->txDma->sourceBurst = DMA_BURST_1;
    this->txDma->destBurst = DMA_BURST_1;
    this->txDma->sourceMode = DMA_ADDRESS_INCREMENT;
    this->txDma->destMode = DMA_ADDRESS_STATIC;
    this->txDma->sourceWidth = TRANSFER_WIDTH_BYTE;
    this->txDma->destWidth = TRANSFER_WIDTH_BYTE;
    this->txDma->destAddr = (unsigned long int) &(this->serial.uart->THR);

    //Configuring serial to use dma
    this->serial.uart->FCR = 0x8F;

    this->transmitBuffer = nullptr;
    this->receiveBuffer = nullptr;
}

SerialAsync::~SerialAsync() {
    // deallocating DMA
    deallocateDMA(this->rxDma);
    deallocateDMA(this->txDma);
}

void SerialAsync::setBaud(int baudrate) {
    this->sync(); //Ensuring nothing is being transmitted
    serial_baud(&this->serial, baudrate);
}

/**
    * Waits for any outstanding transmissions to complete. A blocking function.
    */
void SerialAsync::sync() {
    while(!isDMAFinished(this->txDma) || !(this->serial.uart->LSR & 0x40));
}

/**
    * Reads bytes into the buffer, up to the amount indicated by size.
    * NOTE: Returns instantly with only what was in the receive buffer prior to the call.
    * Can only read up to 16 bytes unless a receive buffer is specified in advance
    * @param buffer the buffer to write read values to
    * @param size the size of the buffer
    * @return the number of bytes read
    */
int SerialAsync::read(void* buffer, int size) {
    //Temporarily pausing dma
    stopDMA(this->rxDma);

    int ret = 0;

    //Getting the dma buffer position and data
    if (this->receiveBuffer) {
        int dmaRxBufSize = getDMADestAddr(this->rxDma) - (unsigned long int) this->receiveBuffer;

        for (int i = 0; (this->receiveBufferLength > i) && (size > i) && (dmaRxBufSize > i); i++) {
            ((uint8_t*) buffer)[ret++] = ((uint8_t*) this->receiveBuffer)[i];
        }

        //Shifting buffer down
        for (int i = 0; dmaRxBufSize - ret > i; i++) {
            ((uint8_t*) this->receiveBuffer)[i] = ((uint8_t*) this->receiveBuffer)[i + ret];
        }

        //Setting new dma start address and size
        this->rxDma->destAddr = ((unsigned long int) this->receiveBuffer) + (dmaRxBufSize - ret);
        this->rxDma->transferSize = this->receiveBufferLength - (dmaRxBufSize - ret);
    }

    //Now fetching anything left in the receive holding buffer
    if (ret < size) {
        //There is still some room in the provided buffer
        for (int i = 0; (16 > i) && (ret < size) && (this->serial.uart->LSR & 0x1); i++) {
            ((uint8_t*) buffer)[ret++] = this->serial.uart->RBR;
        }
    }

    //Resuming dma if buffer is provided
    if (this->receiveBuffer) {
        startDMA(this->rxDma);
    }

    return ret;
}

/**
    * Sets the recieve buffer to be filled with data when it arrives
    * @param buffer A pointer to the first byte of the buffer to use for asynchronous receciving
    * @param size The size of the provided buffer in bytes
    */
void SerialAsync::setReceiveBuffer(void* buffer, int size) {
    this->receiveBuffer = buffer;
    this->receiveBufferLength = size;

    //Configuring receive dma
    this->rxDma->transferSize = size;
    this->rxDma->destAddr = (unsigned long int)buffer;

    startDMA(this->rxDma);
}

/**
    * Sets the control paramaters for the UART channel
    * @param parity The parity mode to use
    * @param stop The number of stop bits to use
    * @param bits The length of a transmitted word to use
    */
void SerialAsync::setControl(SerialParity parity, StopBits stop, WordLength bits) {
    this->sync(); //Ensuring nothing is being transmitted
    serial_format(&this->serial, (int)bits + 5, parity, (int)stop + 1);
}

void SerialAsync::writeCommon(void* buffer, int size) {
    this->txDma->transferSize = size;
    this->txDma->sourceAddr = (unsigned long int)buffer;

    startDMA(this->txDma);
}

/**
    * Writes the data in the buffer asynchronously
    * NOTE: This function is non-blocking and will return immediately
    * @param buffer The buffer to transmit
    * @param size The size of the buffer in bytes
    */
void SerialAsync::write(void* buffer, int size) {
    this->sync();
    
    if (this->transmitBuffer) {
        free_safe((void*)this->transmitBuffer);
        this->transmitBuffer = nullptr;
    }

    this->writeCommon(buffer, size);
}

/**
    * Writes the data in the buffer asynchronously and frees the data when complete with the transfer.
    * NOTE: This function is non-blocking and will return immediately
    * @param buffer The buffer to transmit
    * @param size The size of the buffer in bytes
    */
void SerialAsync::writeAndFree(void* buffer, int size) {
    this->sync();
    if (this->transmitBuffer) {
        free_safe((void*)this->transmitBuffer);
    }
    this->transmitBuffer = buffer;

    this->writeCommon(buffer, size);
}

/**
    * Flushes the receiving buffer, all data in it will be lost.
    */
void SerialAsync::flushReceiving() {
    this->serial.uart->FCR = 0x8B;

    if (this->receiveBuffer) {
        stopDMA(this->rxDma);
        this->rxDma->transferSize = this->receiveBufferLength;
        this->rxDma->destAddr = (unsigned long int) this->receiveBuffer;
        startDMA(this->rxDma);
    }
}

void SerialAsync::checkBufferFree() {
    this->sync();
    if (this->transmitBuffer) {
        free_safe((void*)this->transmitBuffer);
        this->transmitBuffer = nullptr;
    }
}
/*
 * Analog Out Async Class
 *
 * A driver for the DAC peripheral that takes advantage of hardware
 * DMA channels to allow the processor to not be concerned with each
 * sample.
 *
 * (c) 2021 Daniel Cooper
 */


#include "analogOutAsync.hpp"

AnalogOutAsync::AnalogOutAsync(PinName pin) {
    //Get a dma channel
    this->dma = allocateDMA();

    //Initializing DAC peripheral
    dac_t dac;
    analogout_init(&dac, pin);
}

AnalogOutAsync::~AnalogOutAsync() {
    // dealocating the dma channel
    deallocateDMA(this->dma);
}

void AnalogOutAsync::write_u16(uint16_t *buf, int size, int rate) {
    //configuring DMA channel
    this->dma->sourceAddr = (unsigned long int) buf;
    this->dma->destAddr = (unsigned long int) &(LPC_DAC->DACR);
    this->dma->sourceMode = DMA_ADDRESS_INCREMENT;
    this->dma->destMode = DMA_ADDRESS_STATIC;
    this->dma->source = DMA_MEMORY;
    this->dma->destination = DMA_DAC;
    this->dma->sourceBurst = DMA_BURST_1;
    this->dma->destBurst = DMA_BURST_1;
    this->dma->sourceWidth = TRANSFER_WIDTH_HALF_WORD;
    this->dma->destWidth = TRANSFER_WIDTH_HALF_WORD;
    this->dma->transferType = TRANSFER_MEMORY_TO_PERIPHERAL;
    this->dma->transferSize = size >> 1;

    startDMA(this->dma);

    //Configuring DAC if not already configured
    LPC_DAC->DACCNTVAL = (96000000 >> 2) / rate; //96000000 is the speed of the processor
    LPC_DAC->DACCTRL = 0xE;

    //DAC configured and DMA feeding it
}
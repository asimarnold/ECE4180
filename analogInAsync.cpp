/*
 * Analog In Async Class
 *
 * A driver for the ADC peripheral that takes advantage of hardware
 * DMA channels to allow the processor to not be concerned with each
 * sample.
 *
 * (c) 2023 Daniel Cooper
 */


#include "analogInAsync.hpp"

AnalogInAsync::AnalogInAsync(PinName pin) {
    //Get a dma channel
    this->dma = allocateDMA();

    //Initializing ADC peripheral
    analogin_init(&this->adc, pin);
}

AnalogInAsync::~AnalogInAsync() {
    // dealocating the dma channel
    deallocateDMA(this->dma);
}

void AnalogInAsync::read_u16(uint16_t *buf, int size, int rate) {
    //configuring DMA channel
    this->dma->sourceAddr = ((unsigned long int) &(LPC_ADC->ADDR0)) + 4 * (unsigned long long)this->adc.adc;
    this->dma->destAddr = (unsigned long int) buf;
    this->dma->sourceMode = DMA_ADDRESS_STATIC;
    this->dma->destMode = DMA_ADDRESS_INCREMENT;
    this->dma->source = DMA_ADC;
    this->dma->destination = DMA_MEMORY;
    this->dma->sourceBurst = DMA_BURST_1;
    this->dma->destBurst = DMA_BURST_1;
    this->dma->sourceWidth = TRANSFER_WIDTH_HALF_WORD;
    this->dma->destWidth = TRANSFER_WIDTH_HALF_WORD;
    this->dma->transferType = TRANSFER_PERIPHERAL_TO_MEMORY;
    this->dma->transferSize = size >> 1;

    startDMA(this->dma);

    //Configuring DAC if not already configured
    LPC_DAC->DACCNTVAL = (96000000 >> 2) / rate; //96000000 is the speed of the processor
    LPC_DAC->DACCTRL = 0xE;

    //DAC configured and DMA reading from it
}
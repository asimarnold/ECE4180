/*
 * Analog In Async Class
 *
 * A driver for the ADC peripheral that takes advantage of hardware
 * DMA channels to allow the processor to not be concerned with each
 * sample.
 *
 * (c) 2023 Daniel Cooper
 */

#ifndef COLLECTION_ANALOG_IN_ASYNC_INCLUDED
#define COLLECTION_ANALOG_IN_ASYNC_INCLUDED

#include <stdint.h>
#include "dma.h"
#include <mbed.h>

class AnalogInAsync {
    private:

    DMA_CHANNEL* dma;
    analogin_t adc;

    public:

    AnalogInAsync(PinName pin);

    ~AnalogInAsync();

    /**
     * Reads from the pin and stores into the provided buffer.
     * @param buf The buffer containing samples. Samples are between 0 (0V) and 65535 (3.3V)
     * @param size The number of bytes the buffer contains
     * @param rate The samples per second to read, maximum 200kHz
     */
    void read_u16(uint16_t* buf, int size, int rate);

    inline bool isFinished() { return isDMAFinished(this->dma); }
};

#endif // COLLECTION_ANALOG_IN_ASYNC_INCLUDED
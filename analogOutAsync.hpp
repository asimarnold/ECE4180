/*
 * Analog Out Async Class
 *
 * A driver for the DAC peripheral that takes advantage of hardware
 * DMA channels to allow the processor to not be concerned with each
 * sample.
 *
 * (c) 2021 Daniel Cooper
 */

#ifndef COLLECTION_ANALOG_OUT_ASYNC_INCLUDED
#define COLLECTION_ANALOG_OUT_ASYNC_INCLUDED

#include <stdint.h>
#include "dma.h"
#include <mbed.h>

class AnalogOutAsync {
    private:

    DMA_CHANNEL* dma;


    public:

    AnalogOutAsync(PinName pin);

    ~AnalogOutAsync();

    /**
     * Writes the contents of buf to the analog out pin
     * @param buf The buffer containing samples. Samples should be between 0 (0V) and 65535 (3.3V)
     * @param size The number of bytes the buffer contains
     * @param rate The samples per second to output
     */
    void write_u16(uint16_t* buf, int size, int rate);

    inline bool isFinished() { return isDMAFinished(this->dma); }
};

#endif // COLLECTION_ANALOG_OUT_ASYNC_INCLUDED
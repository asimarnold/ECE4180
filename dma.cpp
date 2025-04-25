/*
 * DMA API
 * 
 * Allows for hardware DMA channels be used for transfers
 * 
 * (c) 2021 Daniel Cooper
 */

#include "dma.h"
#include <cstdlib>
#include "collectionCommon.hpp"

char dmaInit = 0;
char dmaAlloced = 0;

void initDMA() {
    dmaInit = 1;
    //Checking if already enabled
    unsigned long int config = LPC_GPDMA->DMACConfig;
    if ((config & 0x1) == 1) {
        //Bit 0 is the enable bit, if it is already set, then we don't have to do anything.
        return;
    }

    //Enabling power
    volatile unsigned long int* powerReg = (unsigned long int*) 0x400FC0C4;
    *powerReg |= 0x20000000;

    LPC_GPDMA->DMACConfig = 0x1; //enabling DMA and setting it to little endian mode
}

/**
 * Allocates from highest priority to lowest, so plan calls accordingly
 */
DMA_CHANNEL* allocateDMA() {
    if (!dmaInit) initDMA();

    //Getting current DMA enabled channels
    unsigned long int channels = LPC_GPDMA->DMACEnbldChns | dmaAlloced;

    DMA_CHANNEL* ret = (DMA_CHANNEL*) malloc(sizeof(DMA_CHANNEL));

    //There are 8 channels that can be used
    if (!(channels & 0x1)) {
        //Channel 0 available
        ret->dmaCH = LPC_GPDMACH0;
        ret->dmaCHNum = 0;
    } else if (!(channels & 0x2)) {
        //Channel 1 available
        ret->dmaCH = LPC_GPDMACH1;
        ret->dmaCHNum = 1;
    } else if (!(channels & 0x4)) {
        //Channel 2 available
        ret->dmaCH = LPC_GPDMACH2;
        ret->dmaCHNum = 2;
    } else if (!(channels & 0x8)) {
        //Channel 3 available
        ret->dmaCH = LPC_GPDMACH3;
        ret->dmaCHNum = 3;
    } else if (!(channels & 0x10)) {
        //Channel 4 available
        ret->dmaCH = LPC_GPDMACH4;
        ret->dmaCHNum = 4;
    } else if (!(channels & 0x20)) {
        //Channel 5 available
        ret->dmaCH = LPC_GPDMACH5;
        ret->dmaCHNum = 5;
    } else if (!(channels & 0x40)) {
        //Channel 6 available
        ret->dmaCH = LPC_GPDMACH6;
        ret->dmaCHNum = 6;
    } else if (!(channels & 0x80)) {
        //Channel 7 available
        ret->dmaCH = LPC_GPDMACH7;
        ret->dmaCHNum = 7;
    } else {
        //No available channels
        free(ret);
        return nullptr;
    }

    dmaAlloced |= 1 << ret->dmaCHNum;

    //Returns with all those other fields uninitialized
    //so it is mandatory that the implementing driver initialize all fields.
    return ret;
}

/**
 * De-allocates a DMA channel
 */
void deallocateDMA(DMA_CHANNEL* ch) {
    stopDMA(ch);
    dmaAlloced &= ~(0x1 << ch->dmaCHNum);
}

/**
 * Starts a DMA transfer
 * Expects that the channel's configuration is already set by writing straight to the
 * DMA_CHANNEL object. 
 */
void startDMA(DMA_CHANNEL* ch) {
    //It is known that DMA is enabled because a DMA channel is being passed in

    //Clearing past interrupts and errors
    //LPC_GPDMA->DMACIntErrClr = 0x1 << ch->dmaCHNum;
    //LPC_GPDMA->DMACIntTCClear = 0x1 << ch->dmaCHNum;

    //Disabling DMA while being edited
    unsigned long int config = 0x0 | ((ch->source & 0xF) << 1) | ((ch->destination & 0xF) << 6) | ((ch->transferType) << 11) | (0x3 << 14);

    ch->dmaCH->DMACCConfig = config;

    //Have to configure the DMA channel
    ch->dmaCH->DMACCSrcAddr = ch->sourceAddr;
    ch->dmaCH->DMACCDestAddr = ch->destAddr;
    
    if (ch->transferSize <= 4092) {
        //No need to create a linked list
        ch->dmaCH->DMACCLLI = 0;
        unsigned long int control = (ch->transferSize & 0xFFF) | ((ch->sourceBurst & 0x7) << 12) |
                                  ((ch->destBurst & 0x7) << 15) | ((ch->sourceWidth & 0x3) << 18) |
                                  ((ch->destWidth & 0x3) << 21) | ((ch->sourceMode & 0x1) << 26) |
                                  ((ch->destMode & 0x1) << 27);
        ch->dmaCH->DMACCControl = control;
    } else {
        //Must make a linked list
        unsigned int sourceByteWidth = 1;
        unsigned int destByteWidth = 1;

        ch->dmaCH->DMACCControl = (4092) | ((ch->sourceBurst & 0x7) << 12) |
                                  ((ch->destBurst & 0x7) << 15) | ((ch->sourceWidth & 0x3) << 18) |
                                  ((ch->destWidth & 0x3) << 21) | ((ch->sourceMode & 0x1) << 26) |
                                  ((ch->destMode & 0x1) << 27);

        if (ch->sourceWidth == TRANSFER_WIDTH_WORD) {
            sourceByteWidth = 4;
        } else if (ch->sourceWidth == TRANSFER_WIDTH_HALF_WORD) {
            sourceByteWidth = 2;
        }

        if (ch->destWidth == TRANSFER_WIDTH_WORD) {
            destByteWidth = 4;
        } else if (ch->destWidth == TRANSFER_WIDTH_HALF_WORD) {
            destByteWidth = 2;
        }

        unsigned int remainingSize = ch->transferSize - 4092;
        unsigned int currentSource = ch->sourceMode == DMA_ADDRESS_INCREMENT ? ch->sourceAddr + 4092 * sourceByteWidth : ch->sourceAddr;
        unsigned int currentDest = ch->destMode == DMA_ADDRESS_INCREMENT ? ch->destAddr + 4092 * destByteWidth : ch->destAddr;
        int numElements = remainingSize % 4092 == 0 ? remainingSize / 4092 : remainingSize / 4092 + 1;

        ch->dmaCH->DMACCLLI = ((unsigned long int)ch->list) & 0xFFFFFFFC;

        for (int i = 0; numElements > i; i++) {
            ch->list[i].startAddr = currentSource;
            ch->list[i].destAddr = currentDest;
            currentSource += ch->sourceMode == DMA_ADDRESS_INCREMENT ? 4092 * sourceByteWidth : 0;
            currentDest += ch->destMode == DMA_ADDRESS_INCREMENT ? 4092 * destByteWidth : 0;

            if (i == numElements - 1) {
                ch->list[i].nextLLI = nullptr;
            } else {
                ch->list[i].nextLLI = ch->list + i + 1;
            }

            unsigned int elementSize = remainingSize > 4092 ? 4092 : remainingSize;
            remainingSize -= elementSize;

            ch->list[i].control = (elementSize) | ((ch->sourceBurst & 0x7) << 12) |
                              ((ch->destBurst & 0x7) << 15) | ((ch->sourceWidth & 0x3) << 18) |
                              ((ch->destWidth & 0x3) << 21) | ((ch->sourceMode & 0x1) << 26) |
                              ((ch->destMode & 0x1) << 27);
        }
    }

    //Either it is using linked list or it isn't but it is now mostly configured

    //Setting the config register which will start the DMA process
    config |= 0x1;// | ((ch->source & 0xF) << 1) | ((ch->destination & 0xF) << 6) | ((ch->transferType) << 11) | (0x3 << 14);

    //printfdbg("config: %d\n", config);
    ch->dmaCH->DMACCConfig = config;
}

void stopDMA(DMA_CHANNEL* ch) {
    //Just disables dma
    unsigned long int config = 0x0 | ((ch->source & 0xF) << 1) | ((ch->destination & 0xF) << 6) | ((ch->transferType) << 11) | (0x3 << 14);
    ch->dmaCH->DMACCConfig = config;
}

unsigned long int getDMADestAddr(DMA_CHANNEL* ch) {
    return ch->dmaCH->DMACCDestAddr;
}

/**
 * Returns 1 if the DMA transfer has completed
 * Returns 0 if the DMA transfer is still on going
 */
char isDMAFinished(DMA_CHANNEL* ch) {
    if (!(ch->dmaCH->DMACCConfig & 0x1)) {
        //Not enabled
        return 1;
    }

    if (ch->dmaCH->DMACCLLI != 0) {
        //Obviously still has stuff to send because it has a linked list item remaining
        return 0;
    }

    return (ch->dmaCH->DMACCControl & 0xFFF) == 0;
}

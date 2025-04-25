/*
 * DMA API
 * 
 * Allows for hardware DMA channels be used for transfers
 * 
 * (c) 2021 Daniel Cooper
 */

#ifndef COLLECTION_DMA_INCLUDED
#define COLLECTION_DMA_INCLUDED

#include "mbed.h"


typedef enum {
    TRANSFER_MEMORY_TO_MEMORY,
    TRANSFER_MEMORY_TO_PERIPHERAL,
    TRANSFER_PERIPHERAL_TO_MEMORY,
    TRANSFER_PERIPHERAL_TO_PERIPHERAL
} DMA_TRANSFER_TYPE;

typedef enum {
    TRANSFER_WIDTH_BYTE,
    TRANSFER_WIDTH_HALF_WORD,
    TRANSFER_WIDTH_WORD
} DMA_TRANSFER_WIDTH;

typedef enum {
    DMA_SSP0_TX,
    DMA_SSP0_RX,
    DMA_SSP1_TX,
    DMA_SSP1_RX,
    DMA_ADC,
    DMA_I2S_0,
    DMA_I2S_1,
    DMA_DAC,
    DMA_UART0_TX,
    DMA_UART0_RX,
    DMA_UART1_TX,
    DMA_UART1_RX,
    DMA_UART2_TX,
    DMA_UART2_RX,
    DMA_UART3_TX,
    DMA_UART3_RX,
    DMA_MEMORY = 0
} DMA_PERIPHERAL;

typedef enum {
    DMA_ADDRESS_STATIC,
    DMA_ADDRESS_INCREMENT
} DMA_ADDRESSING_MODE;

typedef enum {
    DMA_BURST_1,
    DMA_BURST_4,
    DMA_BURST_8,
    DMA_BURST_16,
    DMA_BURST_32,
    DMA_BURST_64,
    DMA_BURST_128,
    DMA_BURST_256
} DMA_BURST_SIZE;

typedef struct DMA_LINKED_LIST_S {
    unsigned long int startAddr;
    unsigned long int destAddr;
    DMA_LINKED_LIST_S* nextLLI;
    unsigned long int control;
} DMA_LINKED_LIST;

/**
 * DMA Channels represent the hardware DMA channel that performs
 * transfers independently of other DMA channels.
 */
typedef struct {
    LPC_GPDMACH_TypeDef* dmaCH;
    int dmaCHNum;
    DMA_LINKED_LIST list[16]; //Used for generating linked lists. Allows up to 250kB transfers.

    unsigned long int sourceAddr;
    unsigned long int destAddr;
    DMA_TRANSFER_TYPE transferType;

    /**
     * Number of transfers measured in transfer transactions, not bytes
     * Will automatically create linked list structures to accomodate large
     * transfers.
     */
    unsigned long int transferSize;

    DMA_TRANSFER_WIDTH sourceWidth;
    DMA_TRANSFER_WIDTH destWidth;
    DMA_PERIPHERAL source;
    DMA_PERIPHERAL destination;
    DMA_ADDRESSING_MODE sourceMode;
    DMA_ADDRESSING_MODE destMode;
    DMA_BURST_SIZE sourceBurst;
    DMA_BURST_SIZE destBurst;

} DMA_CHANNEL;

/**
 * Allocates from highest priority to lowest, so plan calls accordingly
 */
DMA_CHANNEL* allocateDMA();

/**
 * De-allocates a DMA channel
 */
void deallocateDMA(DMA_CHANNEL* ch);

/**
 * Starts a DMA transfer
 * Expects that the channel's configuration is already set by writing straight to the
 * DMA_CHANNEL object. 
 */
void startDMA(DMA_CHANNEL* ch);

void stopDMA(DMA_CHANNEL* ch);

unsigned long int getDMADestAddr(DMA_CHANNEL* ch);

/**
 * Returns 1 if the DMA transfer has completed
 * Returns 0 if the DMA transfer is still on going
 */
char isDMAFinished(DMA_CHANNEL* ch);

 #endif // COLLECTION_DMA_INCLUDED
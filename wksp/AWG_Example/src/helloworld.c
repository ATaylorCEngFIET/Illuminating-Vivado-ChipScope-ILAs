/**
 * @file main.c
 * @brief AXI Stream FIFO Test Application for ZYNQ MPSoC
 * 
 * This application configures an AXI Stream FIFO and writes 256 samples
 * in a ramp waveform pattern to test for lost bits.
 * 
 * Uses Vitis unified flow approach with base address initialization.
 */

#include <stdio.h>
#include <stdlib.h>
#include "xparameters.h"
#include "xil_printf.h"
#include "xllfifo.h"
#include "sleep.h"

/* AXI FIFO base address - update based on your hardware design */
#define FIFO_BASE_ADDR            XPAR_XLLFIFO_0_BASEADDR

/* Number of samples to write to FIFO */
#define NUM_SAMPLES               256

/* Buffer for transmission data */
u32 TxBuffer[NUM_SAMPLES];

/**
 * Generate a ramp waveform pattern
 */
static void GenerateRampWave(u32 *Buffer, u32 NumSamples)
{
    u32 i;
    u32 rampValue = 0;
    u32 shiftedValue = 0;
    float scaleFactor;
    
    /* 14-bit counter max value */
    const u32 MAX_COUNT = 0x3FFF; // 2^14 - 1 = 16383
    
    /* Calculate scale factor to reach MAX_COUNT in (NumSamples-1) steps */
    scaleFactor = (float)MAX_COUNT / (NumSamples - 2);

    for (i = 0; i < NumSamples; i++) {
        if (i == NumSamples - 1) {
            /* Last sample is 0 */
            rampValue = 0;
            shiftedValue = 0;
        } else {
            /* Generate scaled ramp to cover 0 to 16383 over the available samples */
            rampValue = (u32)((i * scaleFactor) + 0.5f);
            
            /* Ensure we don't exceed 14 bits */
            if (rampValue > MAX_COUNT) {
                rampValue = MAX_COUNT;
            }
            
            /* Shift to bits 31:18 */
            shiftedValue = rampValue << 18;
        }
        
        /* Store sample in buffer */
        Buffer[i] = shiftedValue;
        
    }
}

/**
 * Main function for AXI FIFO Triangular Wave Test
 */
int main(void)
{
    XLlFifo FifoInstance;
    int Status;
    u32 TxFrameLength;
    u32 i;
    
    xil_printf("\r\n--- AXI Stream FIFO Triangular Wave Test ---\r\n");
    
    /* Initialize the AXI Stream FIFO device using base address */
    XLlFifo_Initialize(&FifoInstance, FIFO_BASE_ADDR);
    
    /* Reset the device to get it into its initial state */
    XLlFifo_Reset(&FifoInstance);
    XLlFifo_IntClear(&FifoInstance,0xffffffff);
    usleep(1000);
    /* Check if the reset completed successfully */
    Status = XLlFifo_Status(&FifoInstance);
    //if (Status != 0) {
    //    xil_printf("FIFO reset failed. Status: 0x%x\r\n", Status);
    //    return XST_FAILURE;
    //}
    
    /* Generate the ramp wave pattern */
    xil_printf("Generating ramp wave pattern...\r\n");
    GenerateRampWave(TxBuffer, NUM_SAMPLES);
    
    /* Calculate frame length in bytes (4 bytes per word) */
    TxFrameLength = NUM_SAMPLES * 4;
    
    while(1){
        /* Check if there's enough room in the FIFO */
        
        Status = XLlFifo_iTxVacancy(&FifoInstance);
        //xil_printf("TX FIFO vacancy: %d words\r\n", Status);
        
        if (Status < NUM_SAMPLES) {
            xil_printf("Not enough space in TX FIFO. Need %d words, have %d words.\r\n",
                    NUM_SAMPLES, Status);
            return XST_FAILURE;
        }

        /* Write data to FIFO TxD */
        for (i = 0; i < NUM_SAMPLES; i++) {
            XLlFifo_TxPutWord(&FifoInstance, TxBuffer[i]);
        }

        /* Write the frame length to the FIFO transmit length register */
        //xil_printf("Writing %d samples (%d bytes) to AXI Stream FIFO...\r\n", NUM_SAMPLES, TxFrameLength);
        XLlFifo_TxSetLen(&FifoInstance, TxFrameLength);

        /* Check for Tx completion */
        //Status = XLlFifo_IsTxDone(&FifoInstance);
        //if (Status != TRUE) {
        //    xil_printf("Waiting for transmission to complete...\r\n");
            
            /* Poll until transmission is done */
        //    while (XLlFifo_IsTxDone(&FifoInstance) != TRUE) {
                /* NOP */
        //    }
        //}
    
        //xil_printf("Transmission complete\r\n");
        /* Check for any errors */
        Status = XLlFifo_Status(&FifoInstance);
        if (Status & XLLF_INT_ERROR_MASK) {
            xil_printf("ERROR: FIFO transmission error occurred. Status: 0x%x\r\n", Status);
            
            /* Clear errors */
            XLlFifo_IntClear(&FifoInstance, XLLF_INT_ERROR_MASK);
            return XST_FAILURE;
        }

    }

    
    xil_printf("AXI Stream FIFO test completed successfully\r\n");
    return XST_SUCCESS;
}
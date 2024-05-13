/**************************************************************************//**
 * @file     main.c
 * @version  V3.00
 * @brief    Demonstrate one SPI Master self-loopback transfer and two SPI 4-wire/3-wire loopback transfer.
 *           SPI1 will be configured as Master mode and SPI0 will be configured as Slave mode.
 *
 * @copyright SPDX-License-Identifier: Apache-2.0
 * @copyright Copyright (C) 2014 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include <stdio.h>
#include "NUC029xAN.h"

//*** <<< Use Configuration Wizard in Context Menu >>> ***
// <e> Two SPI port loopback transfer
#define TwoPortLoopback     0
//  <o> Bi-direction Interface
//  <0=> 4-wire <1=> 3-wire
#define Slave3WireMode      0
// </e>
//*** <<< end of configuration section >>> ***

#define PLL_CLOCK           50000000

# if defined ( __GNUC__ )
#define TEST_COUNT          16
#else
#define TEST_COUNT          64
#endif

/* Global variable declaration */
#if (!TwoPortLoopback)
static uint32_t s_au32SourceData[TEST_COUNT];
static uint32_t s_au32DestinationData[TEST_COUNT];
#else
static uint32_t s_au32MasterToSlaveTestPattern[TEST_COUNT];
static uint32_t s_au32SlaveToMasterTestPattern[TEST_COUNT];
static uint32_t s_au32MasterRxBuffer[TEST_COUNT];
static uint32_t s_au32SlaveRxBuffer[TEST_COUNT];
static volatile uint32_t s_u32MasterTxDataCount, s_u32MasterRxDataCount;
static volatile uint32_t s_u32SlaveTxDataCount, s_u32SlaveRxDataCount;
#endif

/* Function prototype declaration */
void SYS_Init(void);
void SPI_Init(void);

/* ------------- */
/* Main function */
/* ------------- */
int main(void)
{
#if (!TwoPortLoopback)
    uint32_t u32TestCount, u32Err, u32TimeOutCnt;
#endif
    uint32_t u32DataCount;

    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Init System, IP clock and multi-function I/O. */
    SYS_Init();

    /* Lock protected registers */
    SYS_LockReg();

    /* Configure UART0: 115200, 8-bit word, no parity bit, 1 stop bit. */
    UART_Open(UART0, 115200);

    /* Init SPI */
    SPI_Init();

    printf("\n\n");
    printf("+--------------------------------------------------------------------+\n");
    printf("|                       SPI Driver Sample Code                       |\n");
    printf("+--------------------------------------------------------------------+\n");
    printf("\n");
#if (!TwoPortLoopback)
    printf("This sample code demonstrates SPI1 self loop back data transfer.\n");
    printf(" SPI1 configuration:\n");
    printf("     Master mode; data width 32 bits.\n");
    printf(" I/O connection:\n");
    printf("     P0.5 MOSI_1 <--> P0.6 MISO_1 \n");
    printf("\nSPI1 Loopback test ");

    u32Err = 0;
    for(u32TestCount = 0; u32TestCount < 0x1000; u32TestCount++)
    {
        /* Set the source data and clear the destination buffer */
        for(u32DataCount = 0; u32DataCount < TEST_COUNT; u32DataCount++)
        {
            s_au32SourceData[u32DataCount] = u32DataCount;
            s_au32DestinationData[u32DataCount] = 0;
        }

        u32DataCount = 0;

        if((u32TestCount & 0x1FF) == 0)
        {
            putchar('.');
        }

        while(1)
        {
            /* Write to TX register */
            SPI_WRITE_TX0(SPI1, s_au32SourceData[u32DataCount]);

            /* Trigger SPI data transfer */
            SPI_TRIGGER(SPI1);

            /* Check SPI1 busy status */
            u32TimeOutCnt = SystemCoreClock; /* 1 second time-out */
            while(SPI_IS_BUSY(SPI1))
            {
                if(--u32TimeOutCnt == 0)
                {
                    printf("Wait for SPI busy flag is cleared time-out!\n");
                    u32Err = 1;
                    break;
                }
            }

            if(u32Err)
                break;

            /* Read received data */
            s_au32DestinationData[u32DataCount] = SPI_READ_RX0(SPI1);
            u32DataCount++;
            if(u32DataCount > TEST_COUNT)
                break;
        }

        if(u32Err)
            break;

        /*  Check the received data */
        for(u32DataCount = 0; u32DataCount < TEST_COUNT; u32DataCount++)
        {
            if(s_au32DestinationData[u32DataCount] != s_au32SourceData[u32DataCount])
                u32Err = 1;
        }

        if(u32Err)
            break;
    }

    if(u32Err)
        printf(" [FAIL]\n\n");
    else
        printf(" [PASS]\n\n");

    /* Reset SPI1 */
    SPI_Close(SPI1);
#else
#if (Slave3WireMode)
    printf("This sample code demonstrates Slave 3-wire mode loop back transfer.\n\n");
#else
    printf("This sample code demonstrates SPI0/SPI1 loop back transfer.\n\n");
#endif
    printf("Configure SPI0 as a slave and SPI1 as a master.\n");
    printf("Bit length of a transaction: 32\n");
    printf("Please connect below I/O connections for SPI0 and SPI1:\n");
#if (!Slave3WireMode)
    printf("    SPISS0 (P1.4)   <->   SPISS1 (P0.4)\n");
#endif
    printf("    SPICLK0(P1.7)   <->   SPICLK1(P0.7)\n");
    printf("    MISO_0 (P1.6)   <->   MISO_1 (P0.6)\n");
    printf("    MOSI_0 (P1.5)   <->   MOSI_1 (P0.5)\n\n");
    printf("After the transfer is done, the received data will be printed out.\n");

    for(u32DataCount = 0; u32DataCount < TEST_COUNT; u32DataCount++)
    {
        /* Write the initial value to source buffer */
        s_au32MasterToSlaveTestPattern[u32DataCount] = 0x00550000 + u32DataCount;
        s_au32SlaveToMasterTestPattern[u32DataCount] = 0x00AA0000 + u32DataCount;
        /* Clear destination buffer */
        s_au32MasterRxBuffer[u32DataCount] = 0;
        s_au32SlaveRxBuffer[u32DataCount] = 0;
    }

    s_u32MasterTxDataCount = 0;
    s_u32MasterRxDataCount = 0;
    s_u32SlaveTxDataCount = 0;
    s_u32SlaveRxDataCount = 0;
    printf("Press any key to start transmission ...\n");
    getchar();
    printf("\n");

    /* Access TX and RX FIFO */
    while((s_u32MasterRxDataCount < TEST_COUNT) || (s_u32SlaveRxDataCount < TEST_COUNT))
    {
        /* Check TX FULL flag and TX data count */
        if((SPI_GET_TX_FIFO_FULL_FLAG(SPI1) == 0) && (s_u32MasterTxDataCount < TEST_COUNT))
            SPI_WRITE_TX0(SPI1, s_au32MasterToSlaveTestPattern[s_u32MasterTxDataCount++]); /* Write to TX FIFO */
        /* Check TX FULL flag and TX data count */
        if((SPI_GET_TX_FIFO_FULL_FLAG(SPI0) == 0) && (s_u32SlaveTxDataCount < TEST_COUNT))
            SPI_WRITE_TX0(SPI0, s_au32SlaveToMasterTestPattern[s_u32SlaveTxDataCount++]); /* Write to TX FIFO */
        /* Check RX EMPTY flag */
        if(SPI_GET_RX_FIFO_EMPTY_FLAG(SPI1) == 0)
            s_au32MasterRxBuffer[s_u32MasterRxDataCount++] = SPI_READ_RX0(SPI1); /* Read RX FIFO */
        /* Check RX EMPTY flag */
        if(SPI_GET_RX_FIFO_EMPTY_FLAG(SPI0) == 0)
            s_au32SlaveRxBuffer[s_u32SlaveRxDataCount++] = SPI_READ_RX0(SPI0); /* Read RX FIFO */
    }

    /* Print the received data */
    printf("\tSPI0 Received data:\tSPI1 Received data:\n");
    for(u32DataCount = 0; u32DataCount < TEST_COUNT; u32DataCount++)
    {
        printf("%d:\t0x%X\t\t0x%X\n", u32DataCount, s_au32SlaveRxBuffer[u32DataCount], s_au32MasterRxBuffer[u32DataCount]);
    }

    printf("The data transfer was done.\n");

    printf("\n\nExit SPI driver sample code.\n");

    /* Reset SPI0 */
    SPI_Close(SPI0);
    /* Reset SPI1 */
    SPI_Close(SPI1);
#endif

    while(1);
}

void SYS_Init(void)
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Enable external 12 MHz XTAL */
    CLK_EnableXtalRC(CLK_PWRCON_XTL12M_EN_Msk);

    /* Waiting for clock ready */
    CLK_WaitClockReady(CLK_CLKSTATUS_XTL12M_STB_Msk);

    /* Switch HCLK clock source to HXT and set HCLK divider to 1 */
    CLK_SetHCLK(CLK_CLKSEL0_HCLK_S_HXT, CLK_CLKDIV_HCLK(1));

    /* Set core clock as PLL_CLOCK from PLL */
    CLK_SetCoreClock(PLL_CLOCK);

    /* Select HXT as the clock source of UART0 */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART_S_HXT, CLK_CLKDIV_UART(1));

    /* Select HCLK as the clock source of SPI0 */
    CLK_SetModuleClock(SPI0_MODULE, CLK_CLKSEL1_SPI0_S_HCLK, MODULE_NoMsk);

    /* Select HCLK as the clock source of SPI1 */
    CLK_SetModuleClock(SPI1_MODULE, CLK_CLKSEL1_SPI1_S_HCLK, MODULE_NoMsk);

    /* Enable UART peripheral clock */
    CLK_EnableModuleClock(UART0_MODULE);

    /* Enable SPI0 peripheral clock */
    CLK_EnableModuleClock(SPI0_MODULE);

    /* Enable SPI1 peripheral clock */
    CLK_EnableModuleClock(SPI1_MODULE);

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Set P3 multi-function pins for UART0 RXD and TXD */
    SYS->P3_MFP &= ~(SYS_MFP_P30_Msk | SYS_MFP_P31_Msk);
    SYS->P3_MFP |= (SYS_MFP_P30_RXD0 | SYS_MFP_P31_TXD0);

#if (TwoPortLoopback)
    /* Setup SPI0 multi-function pins */
    SYS->P1_MFP = SYS_MFP_P15_MOSI_0 | SYS_MFP_P16_MISO_0 | SYS_MFP_P17_SPICLK0;
#if (!Slave3WireMode)
    SYS->P1_MFP = SYS_MFP_P14_SPISS0;
#endif
#endif

    /* Setup SPI1 multi-function pins */
    SYS->P0_MFP = SYS_MFP_P05_MOSI_1 | SYS_MFP_P06_MISO_1 | SYS_MFP_P07_SPICLK1;
#if (!Slave3WireMode)
    SYS->P0_MFP = SYS_MFP_P04_SPISS1;
#endif

    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate PllClock, SystemCoreClock and CyclesPerUs automatically. */
    SystemCoreClockUpdate();
}

void SPI_Init(void)
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init SPI                                                                                                */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Configure SPI1 */
    /* Configure SPI1 as a master, clock idle low, 32-bit transaction, drive output on falling clock edge and latch input on rising edge. */
    /* Set IP clock divider. SPI clock rate = 2 MHz */
    SPI_Open(SPI1, SPI_MASTER, SPI_MODE_0, 32, 2000000);

    /* Enable the automatic hardware slave select function. Select the SS pin and configure as low-active. */
    SPI_EnableAutoSS(SPI1, SPI_SS, SPI_SS_ACTIVE_LOW);

    /* Enable FIFO mode */
    SPI1->CNTRL |= SPI_CNTRL_FIFO_Msk;

#if (TwoPortLoopback)
    /* Configure SPI0 */
    /* Configure SPI0 as a slave, clock idle low, 32-bit transaction, drive output on falling clock edge and latch input on rising edge. */
    /* Configure SPI0 as a low level active device. */
    SPI_Open(SPI0, SPI_SLAVE, SPI_MODE_0, 32, (uint32_t)NULL);

    /* Enable FIFO mode */
    SPI0->CNTRL |= SPI_CNTRL_FIFO_Msk;

#if (Slave3WireMode)
    /* Enable slave 3-wire mode */
    SPI_ENABLE_3WIRE_MODE(SPI0);

    /* Trigger SPI data transfer */
    SPI_TRIGGER(SPI0);
#endif
#endif
}

/**************************************************************************//**
 * @file     main.c
 * @version  V3.00
 * @brief    Demonstrate one SPI Master self-loopback transfer and two SPI 4-wire/3-wire loopback transfer.
 *           SPI1 will be configured as Master mode and SPI0 will be configured as Slave mode.
 *
 * @copyright SPDX-License-Identifier: Apache-2.0
 * @copyright Copyright (C) 2023 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include <stdio.h>
#include "NuMicro.h"

//*** <<< Use Configuration Wizard in Context Menu >>> ***
// <e> Two SPI port loopback transfer
#define TwoPortLoopback     0
//  <o> Bi-direction Interface
//  <0=> 4-wire <1=> 3-wire
#define Slave3WireMode      0
// </e>
//*** <<< end of configuration section >>> ***

#define PLLCON_SETTING      CLK_PLLCON_72MHz_HXT

#define TEST_COUNT          64

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
void UART0_Init(void);
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

    /* Init system, IP clock and multi-function I/O. */
    SYS_Init();

    /* Lock protected registers */
    SYS_LockReg();

    /* Init UART0 for printf */
    UART0_Init();

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
    printf("     PC.11 SPI1_MOSI0 <--> PC.10 SPI1_MISO0 \n");
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
            SPI_WRITE_TX(SPI1, s_au32SourceData[u32DataCount]);

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
            s_au32DestinationData[u32DataCount] = SPI_READ_RX(SPI1);
            u32DataCount++;
            if(u32DataCount >= TEST_COUNT)
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

    /* Disable SPI1 peripheral clock */
    CLK->APBCLK &= (~CLK_APBCLK_SPI1_EN_Msk);
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
    printf("    SPI0_SS0  (PC.0)   <->   SPI1_SS0  (PC.8)\n");
#endif
    printf("    SPI0_CLK  (PC.1)   <->   SPI1_CLK  (PC.9)\n");
    printf("    SPI0_MISO0(PC.2)   <->   SPI1_MISO0(PC.10)\n");
    printf("    SPI0_MOSI0(PC.3)   <->   SPI1_MOSI0(PC.11)\n\n");
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
            SPI_WRITE_TX(SPI1, s_au32MasterToSlaveTestPattern[s_u32MasterTxDataCount++]); /* Write to TX FIFO */
        /* Check TX FULL flag and TX data count */
        if((SPI_GET_TX_FIFO_FULL_FLAG(SPI0) == 0) && (s_u32SlaveTxDataCount < TEST_COUNT))
            SPI_WRITE_TX(SPI0, s_au32SlaveToMasterTestPattern[s_u32SlaveTxDataCount++]); /* Write to TX FIFO */
        /* Check RX EMPTY flag */
        if(SPI_GET_RX_FIFO_EMPTY_FLAG(SPI1) == 0)
            s_au32MasterRxBuffer[s_u32MasterRxDataCount++] = SPI_READ_RX(SPI1); /* Read RX FIFO */
        /* Check RX EMPTY flag */
        if(SPI_GET_RX_FIFO_EMPTY_FLAG(SPI0) == 0)
            s_au32SlaveRxBuffer[s_u32SlaveRxDataCount++] = SPI_READ_RX(SPI0); /* Read RX FIFO */
    }

    /* Print the received data */
    printf("\tSPI0 Received data:\tSPI1 Received data:\n");
    for(u32DataCount = 0; u32DataCount < TEST_COUNT; u32DataCount++)
    {
        printf("%d:\t0x%X\t\t0x%X\n", u32DataCount, s_au32SlaveRxBuffer[u32DataCount], s_au32MasterRxBuffer[u32DataCount]);
    }

    printf("The data transfer was done.\n");

    printf("\n\nExit SPI driver sample code.\n");

    /* Disable SPI0 peripheral clock */
    CLK->APBCLK &= (~CLK_APBCLK_SPI0_EN_Msk);
    /* Disable SPI1 peripheral clock */
    CLK->APBCLK &= (~CLK_APBCLK_SPI1_EN_Msk);
#endif

    while(1);
}

void SYS_Init(void)
{
	uint32_t u32TimeOutCnt;

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Enable Internal RC 22.1184 MHz clock. Some peripherals select internal RC oscillator as default clock source. */
    CLK->PWRCON |= CLK_PWRCON_OSC22M_EN_Msk;

    /* Waiting for Internal RC clock ready */
    u32TimeOutCnt = __HIRC;
    while(!(CLK->CLKSTATUS & CLK_CLKSTATUS_OSC22M_STB_Msk))
		if(--u32TimeOutCnt == 0) break;

    /* Enable external 12 MHz XTAL */
    CLK->PWRCON |= CLK_PWRCON_XTL12M_EN_Msk;

    /* Waiting for clock ready */
    u32TimeOutCnt = __HIRC;
    while(!(CLK->CLKSTATUS & CLK_CLKSTATUS_XTL12M_STB_Msk))
		if(--u32TimeOutCnt == 0) break;

    /* Set PLL to Power-down mode and PLL_STB bit in CLKSTATUS register will be cleared by hardware.*/
    CLK->PLLCON |= CLK_PLLCON_PD_Msk;

    /* Enable PLL and Set PLL frequency */
    CLK->PLLCON = PLLCON_SETTING;

    /* Waiting for clock ready */
    u32TimeOutCnt = __HIRC;
    while(!(CLK->CLKSTATUS & CLK_CLKSTATUS_PLL_STB_Msk))
		if(--u32TimeOutCnt == 0) break;

    /* System optimization when CPU runs at 72 MHz */
    FMC->FATCON |= FMC_FATCON_FOMSEL1_Msk | FMC_FATCON_FOMSEL0_Msk;

    /* Select PLL as the system clock source */
    CLK->CLKSEL0 = (CLK->CLKSEL0 & (~CLK_CLKSEL0_HCLK_S_Msk)) | CLK_CLKSEL0_HCLK_S_PLL;

    /* Select HXT as the clock source of UART; select HCLK as the clock source of SPI0; select HCLK as the clock source of SPI1. */
    CLK->CLKSEL1 = (CLK->CLKSEL1 & (~(CLK_CLKSEL1_UART_S_Msk | CLK_CLKSEL1_SPI0_S_Msk | CLK_CLKSEL1_SPI1_S_Msk))) | (CLK_CLKSEL1_UART_S_HXT | CLK_CLKSEL1_SPI0_S_HCLK | CLK_CLKSEL1_SPI1_S_HCLK);

    /* Enable UART, SPI0 and SPI1 clock */
    CLK->APBCLK = CLK_APBCLK_UART0_EN_Msk | CLK_APBCLK_SPI0_EN_Msk | CLK_APBCLK_SPI1_EN_Msk;

    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate PllClock, SystemCoreClock and CyclesPerUs automatically. */
    SystemCoreClockUpdate();

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Set PB multi-function pins for UART0 RXD and TXD */
    SYS->GPB_MFP &= ~(SYS_GPB_MFP_PB0_Msk | SYS_GPB_MFP_PB1_Msk);
    SYS->GPB_MFP |= (SYS_GPB_MFP_PB0_UART0_RXD | SYS_GPB_MFP_PB1_UART0_TXD);

#if (TwoPortLoopback)
    /* Setup SPI0 multi-function pins */
    SYS->GPC_MFP &= ~(SYS_GPC_MFP_PC0_Msk | SYS_GPC_MFP_PC1_Msk | SYS_GPC_MFP_PC2_Msk | SYS_GPC_MFP_PC3_Msk);
    SYS->GPC_MFP |= SYS_GPC_MFP_PC1_SPI0_CLK | SYS_GPC_MFP_PC2_SPI0_MISO0 | SYS_GPC_MFP_PC3_SPI0_MOSI0;
    SYS->ALT_MFP &= ~(SYS_ALT_MFP_PC0_Msk | SYS_ALT_MFP_PC1_Msk | SYS_ALT_MFP_PC2_Msk | SYS_ALT_MFP_PC3_Msk);
    SYS->ALT_MFP |= SYS_ALT_MFP_PC1_SPI0_CLK | SYS_ALT_MFP_PC2_SPI0_MISO0 | SYS_ALT_MFP_PC3_SPI0_MOSI0;
#if (!Slave3WireMode)
    SYS->GPC_MFP |= SYS_GPC_MFP_PC0_SPI0_SS0;
    SYS->ALT_MFP |= SYS_ALT_MFP_PC0_SPI0_SS0;
#endif
#endif

    /* Setup SPI1 multi-function pins */
    SYS->GPC_MFP &= ~(SYS_GPC_MFP_PC8_Msk | SYS_GPC_MFP_PC9_Msk | SYS_GPC_MFP_PC10_Msk | SYS_GPC_MFP_PC11_Msk);
    SYS->GPC_MFP |= SYS_GPC_MFP_PC9_SPI1_CLK | SYS_GPC_MFP_PC10_SPI1_MISO0 | SYS_GPC_MFP_PC11_SPI1_MOSI0;
    SYS->ALT_MFP &= ~(SYS_ALT_MFP_PC8_Msk);
#if (!Slave3WireMode)
    SYS->GPC_MFP |= SYS_GPC_MFP_PC8_SPI1_SS0;
    SYS->ALT_MFP |= SYS_ALT_MFP_PC8_SPI1_SS0;
#endif
}

void UART0_Init(void)
{
    /* Word length is 8 bits; 1 stop bit; no parity bit. */
    UART0->LCR = UART_LCR_WLS_Msk;
    /* Using mode 2 calculation: UART bit rate = UART peripheral clock rate / (BRD setting + 2) */
    /* UART peripheral clock rate 12 MHz; UART bit rate 115200 bps. */
    /* 12000000 / 115200 bps ~= 104 */
    /* 104 - 2 = 0x66. */
    UART0->BAUD = UART_BAUD_DIV_X_EN_Msk | UART_BAUD_DIV_X_ONE_Msk | (0x66);
}

void SPI_Init(void)
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init SPI                                                                                                */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Configure SPI1 */
    /* Configure SPI1 as a master, clock idle low, 32-bit transaction, drive output on falling clock edge and latch input on rising edge. */
    /* Enable FIFO mode */
    SPI1->CNTRL = SPI_CNTRL_FIFO_Msk | SPI_MASTER | SPI_CNTRL_TX_NEG_Msk;
    /* Enable the automatic hardware slave select function. Select the SPI1_SS0 pin and configure as low-active. */
    SPI1->SSR = SPI_SSR_AUTOSS_Msk | SPI_SS;
    /* Set IP clock divider. SPI clock rate = HCLK / ((35+1)*2) = 1 MHz */
    SPI1->DIVIDER = (SPI1->DIVIDER & (~SPI_DIVIDER_DIVIDER_Msk)) | 35;

#if (TwoPortLoopback)
    /* Configure SPI0 */
    /* Configure SPI0 as a slave, clock idle low, 32-bit transaction, drive output on falling clock edge and latch input on rising edge. */
    /* Enable FIFO mode */
    SPI0->CNTRL = SPI_CNTRL_FIFO_Msk | SPI_CNTRL_SLAVE_Msk | SPI_CNTRL_TX_NEG_Msk;
    /* Configure SPI0 as a low level active device. */
    SPI0->SSR = SPI_SSR_SS_LTRIG_Msk;

#if (Slave3WireMode)
    /* Enable slave 3-wire mode */
    SPI0->CNTRL2 |= SPI_CNTRL2_NOSLVSEL_Msk;

    /* Trigger SPI data transfer */
    SPI0->CNTRL |= SPI_CNTRL_GO_BUSY_Msk;
#endif
#endif
}

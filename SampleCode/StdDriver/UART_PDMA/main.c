/****************************************************************************
 * @file     main.c
 * @version  V3.00
 * @brief    M071R_M071S Series UART Interface Controller Driver Sample Code
 *
 * @note
 * Copyright (C) 2014 Nuvoton Technology Corp. All rights reserved.
 *
 ******************************************************************************/
#include <stdio.h>
#include "NuMicro.h"


#define PLL_CLOCK   72000000


#define UART_RX_DMA_CH 0
#define UART_TX_DMA_CH 1


/*---------------------------------------------------------------------------------------------------------*/
/* Global variables                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
int32_t UART_TEST_LENGTH = 64;
uint8_t SrcArray[64];
uint8_t DestArray[64];
volatile int32_t IntCnt;
volatile int32_t IsTestOver;
volatile uint32_t g_u32TwoChannelPdmaTest=0;
extern char GetChar(void);


/*---------------------------------------------------------------------------------------------------------*/
/* Define functions prototype                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
int32_t main(void);


/*---------------------------------------------------------------------------------------------------------*/
/* Clear buffer function                                                                                   */
/*---------------------------------------------------------------------------------------------------------*/
void ClearBuf(uint32_t u32Addr, uint32_t u32Length, uint8_t u8Pattern)
{
    uint8_t* pu8Ptr;
    uint32_t i;

    pu8Ptr = (uint8_t *)u32Addr;

    for (i=0; i<u32Length; i++)
    {
        *pu8Ptr++ = u8Pattern;
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/* Build Src Pattern function                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
void BuildSrcPattern(uint32_t u32Addr, uint32_t u32Length)
{
    uint32_t i=0, j, loop;
    uint8_t* pAddr;

    pAddr = (uint8_t *)u32Addr;

    do {
        if (u32Length > 256)
            loop = 256;
        else
            loop = u32Length;

        u32Length = u32Length - loop;

        for(j=0;j<loop;j++)
            *pAddr++ = (uint8_t)(j+i);

        i++;
    } while ((loop !=0) || (u32Length !=0));
}

/*---------------------------------------------------------------------------------------------------------*/
/* UART Tx PDMA Channel Configuration                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
void PDMA_UART_TxTest(void)
{
    /* UART Tx PDMA channel configuration */
    /* Set transfer width (8 bits) and transfer count */
    PDMA_SetTransferCnt(UART_TX_DMA_CH, PDMA_WIDTH_8, UART_TEST_LENGTH);

    /* Set source/destination address and attributes */
    PDMA_SetTransferAddr(UART_TX_DMA_CH, (uint32_t)SrcArray, PDMA_SAR_INC, (uint32_t)&UART1->THR, PDMA_DAR_FIX);

    /* Set service selection */
    PDMA_SetTransferMode(UART_TX_DMA_CH, PDMA_UART1_TX, FALSE, 0);

    /* Set Memory-to-Peripheral mode */
    PDMA1->CSR = (PDMA1->CSR & (~PDMA_CSR_MODE_SEL_Msk)) | (0x2<<PDMA_CSR_MODE_SEL_Pos);
}

/*---------------------------------------------------------------------------------------------------------*/
/* UART Rx PDMA Channel Configuration                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
void PDMA_UART_RxTest(void)
{
    /* UART Rx PDMA channel configuration */
    /* Set transfer width (8 bits) and transfer count */
    PDMA_SetTransferCnt(UART_RX_DMA_CH, PDMA_WIDTH_8, UART_TEST_LENGTH);

    /* Set source/destination address and attributes */
    PDMA_SetTransferAddr(UART_RX_DMA_CH, (uint32_t)&UART1->RBR, PDMA_SAR_FIX, (uint32_t)DestArray, PDMA_DAR_INC);

    /* Set service selection */
    PDMA_SetTransferMode(UART_RX_DMA_CH, PDMA_UART1_RX, FALSE, 0);

    /* Set Peripheral-to-Memory mode */
    PDMA0->CSR = (PDMA0->CSR & (~PDMA_CSR_MODE_SEL_Msk)) | (0x1<<PDMA_CSR_MODE_SEL_Pos);
}

/*---------------------------------------------------------------------------------------------------------*/
/* PDMA Callback function                                                                                  */
/*---------------------------------------------------------------------------------------------------------*/
void PDMA_Callback_0(void)
{
    printf("\tTransfer Done %d!\r",++IntCnt);

    /* Use PDMA to do UART loopback test 10 times */
    if(IntCnt<10)
    {
        /* Trigger PDMA */
        PDMA_Trigger(UART_RX_DMA_CH);
        PDMA_Trigger(UART_TX_DMA_CH);
    }
    else
    {
        /* Test is over */
        IsTestOver = TRUE;
    }
}

void PDMA_Callback_1(void)
{
    int32_t i;

    printf("\tTransfer Done %d!\t",++IntCnt);

    /* Show UART Rx data */
    for(i=0;i<UART_TEST_LENGTH;i++)
        printf(" 0x%x(%c),",inpb(((uint32_t)DestArray+i)),inpb(((uint32_t)DestArray+i)));
    printf("\n");

    /* Use PDMA to do UART Rx test 10 times */
    if(IntCnt<10)
    {
        /* Trigger PDMA */
        PDMA_Trigger(UART_RX_DMA_CH);
    }
    else
    {
        /* Test is over */
        IsTestOver = TRUE;
    }
}

void PDMA_IRQHandler(void)
{
    /* Get PDMA Block transfer down interrupt status */
    if(PDMA_GET_CH_INT_STS(UART_RX_DMA_CH) & PDMA_ISR_BLKD_IF_Msk)
    {
        /* Clear PDMA Block transfer down interrupt flag */
        PDMA_CLR_CH_INT_FLAG(UART_RX_DMA_CH, PDMA_ISR_BLKD_IF_Msk);

        /* Handle PDMA block transfer done interrupt event */
        if(g_u32TwoChannelPdmaTest == 1)
        {
            PDMA_Callback_0();
        }
        else if(g_u32TwoChannelPdmaTest == 0)
        {
            PDMA_Callback_1();
        }
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/* ISR to handle UART Channel 0 interrupt event                                                            */
/*---------------------------------------------------------------------------------------------------------*/
void UART02_IRQHandler(void)
{
    /* Get UART0 Rx data and send the data to UART1 Tx */
    if( UART_GET_INT_FLAG(UART0, UART_ISR_RDA_INT_Msk) )
        UART1->THR = UART0->RBR;
}


/*---------------------------------------------------------------------------------------------------------*/
/* PDMA Sample Code:                                                                                       */
/*         i32option : ['1'] UART1 TX/RX PDMA Loopback                                                     */
/*                     [Others] UART1 RX PDMA test                                                         */
/*---------------------------------------------------------------------------------------------------------*/
void PDMA_UART(int32_t i32option)
{
    uint32_t u32TimeOutCnt;

    /* Source data initiation */
    BuildSrcPattern((uint32_t)SrcArray, UART_TEST_LENGTH);
    ClearBuf((uint32_t)DestArray, UART_TEST_LENGTH, 0xFF);

    /* Reset PDMA module */
    SYS_ResetModule(PDMA_RST);

    if(i32option =='1')
    {
        printf("  [Using TWO PDMA channel].\n");
        printf("  This sample code will use PDMA to do UART1 loopback test 10 times.\n");
        printf("  Please connect UART1_RXD(PB.4) <--> UART1_TXD(PB.5) before testing.\n");
        printf("  After connecting PB.4 <--> PB.5, press any key to start transfer.\n");
        g_u32TwoChannelPdmaTest = 1;
        getchar();
    }
    else
    {
        UART_TEST_LENGTH = 2;      /* Test Length */
        printf("  [Using ONE PDMA channel].\n");
        printf("  This sample code will use PDMA to do UART1 Rx test 10 times.\n");
        printf("  Please connect UART1_RXD(PB.4) <--> UART1_TXD(PB.5) before testing.\n");
        printf("  After connecting PB.4 <--> PB.5, press any key to start transfer.\n");
        g_u32TwoChannelPdmaTest = 0;
        getchar();
        printf("  Please input %d bytes to trigger PDMA one time.(Ex: Press 'a''b')\n", UART_TEST_LENGTH);
    }

    if(g_u32TwoChannelPdmaTest==1)
    {
        /* Enable PDMA channel clock */
        PDMA_Open( (1 << UART_TX_DMA_CH) );

        /* UART Tx PDMA configuration */
        PDMA_UART_TxTest();
    }

    /* Enable PDMA channel clock */
    PDMA_Open( (1 << UART_RX_DMA_CH) );

    /* UART Rx PDMA configuration */
    PDMA_UART_RxTest();

    /* Enable PDMA Block Transfer Done Interrupt */
    PDMA_EnableInt(UART_RX_DMA_CH, PDMA_IER_BLKD_IE_Msk);
    IntCnt = 0;
    IsTestOver = FALSE;
    NVIC_EnableIRQ(PDMA_IRQn);

    /* Enable UART0 RDA interrupt */
    if(g_u32TwoChannelPdmaTest==0)
    {
        UART_EnableInt(UART0, UART_IER_RDA_IEN_Msk);
    }

    /* Trigger PDMA */
    PDMA_Trigger(UART_RX_DMA_CH);

    if(g_u32TwoChannelPdmaTest==1)
        PDMA_Trigger(UART_TX_DMA_CH);

    /* Enable UART Tx and Rx PDMA function */
    if(g_u32TwoChannelPdmaTest==1)
        UART1->IER |= UART_IER_DMA_TX_EN_Msk;
    else
        UART1->IER &= ~UART_IER_DMA_TX_EN_Msk;

    UART1->IER |= UART_IER_DMA_RX_EN_Msk;

    /* Wait for PDMA operation finish */
    u32TimeOutCnt = SystemCoreClock; /* 1 second time-out */
    while(IsTestOver == FALSE)
    {
        if(--u32TimeOutCnt == 0)
        {
            printf("Wait for PDMA operation finish time-out!\n");
            break;
        }
    }

    /* Disable UART Tx and Rx PDMA function */
    UART1->IER &= ~(UART_IER_DMA_TX_EN_Msk|UART_IER_DMA_RX_EN_Msk);

    /* Disable PDMA channel */
    PDMA_Close();

    /* Disable PDMA Interrupt */
    PDMA_DisableInt(UART_RX_DMA_CH, PDMA_IER_BLKD_IE_Msk);
    NVIC_DisableIRQ(PDMA_IRQn);

    /* Disable UART0 RDA interrupt */
    UART_DisableInt(UART0, UART_IER_RDA_IEN_Msk);
}

void SYS_Init(void)
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Enable Internal RC 22.1184MHz clock */
    CLK_EnableXtalRC(CLK_PWRCON_OSC22M_EN_Msk);

    /* Waiting for Internal RC clock ready */
    CLK_WaitClockReady(CLK_CLKSTATUS_OSC22M_STB_Msk);

    /* Switch HCLK clock source to Internal RC and HCLK source divide 1 */
    CLK_SetHCLK(CLK_CLKSEL0_HCLK_S_HIRC, CLK_CLKDIV_HCLK(1));

    /* Enable external XTAL 12MHz clock */
    CLK_EnableXtalRC(CLK_PWRCON_XTL12M_EN_Msk);

    /* Waiting for external XTAL clock ready */
    CLK_WaitClockReady(CLK_CLKSTATUS_XTL12M_STB_Msk);

    /* Set core clock as PLL_CLOCK from PLL */
    CLK_SetCoreClock(PLL_CLOCK);

    /* Enable UART module clock */
    CLK_EnableModuleClock(UART0_MODULE);
    CLK_EnableModuleClock(UART1_MODULE);

    /* Enable PDMA clock source */
    CLK_EnableModuleClock(PDMA_MODULE);

    /* Select UART module clock source */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART_S_HXT, CLK_CLKDIV_UART(1));
    CLK_SetModuleClock(UART1_MODULE, CLK_CLKSEL1_UART_S_HXT, CLK_CLKDIV_UART(1));

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Set GPB multi-function pins for UART0 RXD and TXD */
    /* Set GPB multi-function pins for UART1 RXD and TXD */

    SYS->GPB_MFP &= ~(SYS_GPB_MFP_PB0_Msk | SYS_GPB_MFP_PB1_Msk |
                      SYS_GPB_MFP_PB4_Msk | SYS_GPB_MFP_PB5_Msk);

    SYS->GPB_MFP |= (SYS_GPB_MFP_PB0_UART0_RXD | SYS_GPB_MFP_PB1_UART0_TXD |
                     SYS_GPB_MFP_PB4_UART1_RXD | SYS_GPB_MFP_PB5_UART1_TXD);

}

void UART0_Init()
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init UART                                                                                               */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Reset UART0 */
    SYS_ResetModule(UART0_RST);

    /* Configure UART0 and set UART0 Baudrate */
    UART_Open(UART0, 115200);
}

void UART1_Init()
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init UART                                                                                               */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Reset UART1 */
    SYS_ResetModule(UART1_RST);

    /* Configure UART1 and set UART1 Baudrate */
    UART_Open(UART1, 115200);
}

/*---------------------------------------------------------------------------------------------------------*/
/* MAIN function                                                                                           */
/*---------------------------------------------------------------------------------------------------------*/

int32_t main(void)
{
    uint8_t unItem;

    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Init System, peripheral clock and multi-function I/O */
    SYS_Init();

    /* Lock protected registers */
    SYS_LockReg();

    /* Init UART0 for printf */
    UART0_Init();

    /* Init UART1 for testing */
    UART1_Init();

    /*---------------------------------------------------------------------------------------------------------*/
    /* SAMPLE CODE                                                                                             */
    /*---------------------------------------------------------------------------------------------------------*/

    printf("\n\nCPU @ %d Hz\n", SystemCoreClock);

    printf("\nUART PDMA Sample Program");

    /* UART PDMA sample function */
    do
    {
        printf("\n\n");
        printf("+------------------------------------------------------------------------+\n");
        printf("|                      UART PDMA Driver Sample Code                      |\n");
        printf("+------------------------------------------------------------------------+\n");
        printf("| [1] Using TWO PDMA channel to test. < TX1(CH1)-->RX1(CH0) >            |\n");
        printf("| [2] Using ONE PDMA channel to test. < TX1-->RX1(CH0) >                 |\n");
        printf("+------------------------------------------------------------------------+\n");
        unItem = getchar();

        IsTestOver = FALSE;
        if((unItem == '1') || (unItem == '2'))
        {
            PDMA_UART(unItem);
            printf("\n\n  UART PDMA sample code is complete.\n");
        }

    }while(unItem!=27);

    while(1);
}


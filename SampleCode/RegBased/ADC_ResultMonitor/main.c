/****************************************************************************
 * @file     main.c
 * @version  V2.0
 * $Revision: 2 $
 * $Date: 15/04/10 5:19p $
 * @brief    M071R_M071S Series ADC Interface Controller Driver Sample Code
 *
 * @note
 * Copyright (C) 2014 Nuvoton Technology Corp. All rights reserved.
 *
 ******************************************************************************/
#include <stdio.h>
#include "NuMicro.h"

#define PLLCON_SETTING  CLK_PLLCON_72MHz_HXT
#define PLL_CLOCK       72000000


/*---------------------------------------------------------------------------------------------------------*/
/* Define Function Prototypes                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
void SYS_Init(void);
void UART0_Init(void);
void AdcResultMonitorTest(void);


/*---------------------------------------------------------------------------------------------------------*/
/* Define global variables and constants                                                                   */
/*---------------------------------------------------------------------------------------------------------*/
volatile uint32_t g_u32AdcCmp0IntFlag;
volatile uint32_t g_u32AdcCmp1IntFlag;


void SYS_Init(void)
{
	uint32_t u32TimeOutCnt;

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Enable Internal RC 22.1184MHz clock */
    CLK->PWRCON |= CLK_PWRCON_OSC22M_EN_Msk;

    /* Waiting for Internal RC clock ready */
    u32TimeOutCnt = __HIRC;
    while(!(CLK->CLKSTATUS & CLK_CLKSTATUS_OSC22M_STB_Msk))
		if(--u32TimeOutCnt == 0) break;

    /* Switch HCLK clock source to Internal RC */
    CLK->CLKSEL0 &= ~CLK_CLKSEL0_HCLK_S_Msk;
    CLK->CLKSEL0 |= CLK_CLKSEL0_HCLK_S_HIRC;

    /* Enable external XTAL 12MHz clock */
    CLK->PWRCON |= CLK_PWRCON_XTL12M_EN_Msk;

    /* Waiting for external XTAL clock ready */
    u32TimeOutCnt = __HIRC;
    while(!(CLK->CLKSTATUS & CLK_CLKSTATUS_XTL12M_STB_Msk))
		if(--u32TimeOutCnt == 0) break;

    /* System optimization when CPU runs at 72MHz */
    FMC->FATCON |= 0x50;

    /* Set core clock as PLL_CLOCK from PLL */
    CLK->PLLCON = PLLCON_SETTING;
    u32TimeOutCnt = __HIRC;
    while(!(CLK->CLKSTATUS & CLK_CLKSTATUS_PLL_STB_Msk))
		if(--u32TimeOutCnt == 0) break;
    CLK->CLKSEL0 &= (~CLK_CLKSEL0_HCLK_S_Msk);
    CLK->CLKSEL0 |= CLK_CLKSEL0_HCLK_S_PLL;

    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate PllClock, SystemCoreClock and CyclesPerUs automatically. */
    //SystemCoreClockUpdate();
    PllClock        = PLL_CLOCK;            // PLL
    SystemCoreClock = PLL_CLOCK / 1;        // HCLK
    CyclesPerUs     = PLL_CLOCK / 1000000;  // For CLK_SysTickDelay()

    /* Enable UART module clock */
    CLK->APBCLK |= CLK_APBCLK_UART0_EN_Msk;

    /* Enable ADC module clock */
    CLK->APBCLK |= CLK_APBCLK_ADC_EN_Msk ;

    /* Select UART module clock source */
    CLK->CLKSEL1 &= ~CLK_CLKSEL1_UART_S_Msk;
    CLK->CLKSEL1 |= CLK_CLKSEL1_UART_S_HXT;

    /* Select ADC module clock source */
    CLK->CLKSEL1 &= CLK_CLKSEL1_ADC_S_Msk ;
    CLK->CLKSEL1 |= CLK_CLKSEL1_ADC_S_HIRC ;

    /* ADC clock source is 22.1184MHz, set divider to 7, ADC clock is 22.1184/7 MHz */
    CLK->CLKDIV  = (CLK->CLKDIV & ~CLK_CLKDIV_ADC_N_Msk) | (((7) - 1) << CLK_CLKDIV_ADC_N_Pos);


    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I/O Multi-function                                                                                 */
    /*---------------------------------------------------------------------------------------------------------*/

    /* Set GPB multi-function pins for UART0 RXD and TXD */
    SYS->GPB_MFP &= ~(SYS_GPB_MFP_PB0_Msk | SYS_GPB_MFP_PB1_Msk);
    SYS->GPB_MFP |= SYS_GPB_MFP_PB0_UART0_RXD | SYS_GPB_MFP_PB1_UART0_TXD;

    /* Disable the GPA0 - GPA3 digital input path to avoid the leakage current. */
    PA->OFFD = 0xF << GPIO_OFFD_OFFD_Pos;

    /* Configure the GPA0 - GPA3 ADC analog input pins */
    SYS->GPA_MFP &= ~(SYS_GPA_MFP_PA0_Msk | SYS_GPA_MFP_PA1_Msk | SYS_GPA_MFP_PA2_Msk | SYS_GPA_MFP_PA3_Msk) ;
    SYS->GPA_MFP |= SYS_GPA_MFP_PA0_ADC0 | SYS_GPA_MFP_PA1_ADC1 | SYS_GPA_MFP_PA2_ADC2 | SYS_GPA_MFP_PA3_ADC3 ;
    SYS->ALT_MFP &= ~(SYS_ALT_MFP_PA1_Msk | SYS_ALT_MFP_PA2_Msk | SYS_ALT_MFP_PA3_Msk);
    SYS->ALT_MFP |= SYS_ALT_MFP_PA1_ADC1 | SYS_ALT_MFP_PA2_ADC2 | SYS_ALT_MFP_PA3_ADC3;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Init UART                                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
void UART0_Init()
{
    /* Reset UART IP */
    SYS->IPRSTC2 |=  SYS_IPRSTC2_UART0_RST_Msk;
    SYS->IPRSTC2 &= ~SYS_IPRSTC2_UART0_RST_Msk;

    /* Configure UART0 and set UART0 Baudrate */
    UART0->BAUD = UART_BAUD_MODE2 | UART_BAUD_MODE2_DIVIDER(__HXT, 115200);
    UART0->LCR = UART_WORD_LEN_8 | UART_PARITY_NONE | UART_STOP_BIT_1;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Function: AdcResultMonitorTest                                                                          */
/*                                                                                                         */
/* Parameters:                                                                                             */
/*   None.                                                                                                 */
/*                                                                                                         */
/* Returns:                                                                                                */
/*   None.                                                                                                 */
/*                                                                                                         */
/* Description:                                                                                            */
/*   ADC result monitor function test.                                                                     */
/*---------------------------------------------------------------------------------------------------------*/
void AdcResultMonitorTest()
{
    uint32_t u32TimeOutCnt;

    printf("\n");
    printf("+----------------------------------------------------------------------+\n");
    printf("|           ADC compare function (result monitor) sample code          |\n");
    printf("+----------------------------------------------------------------------+\n");
    printf("\nIn this test, software will compare the conversion result of channel 2.\n");

    /* Set the ADC operation mode as continuous scan, input mode as single-end and enable the ADC converter */
    ADC->ADCR = (ADC_ADCR_ADMD_CONTINUOUS | ADC_ADCR_DIFFEN_SINGLE_END | ADC_ADCR_ADEN_Msk);
    /* Set the ADC channel 2 */
    ADC->ADCHER |= ((ADC->ADCHER & ~ADC_ADCHER_CHEN_Msk) | (1 << 2));

    /* Enable ADC comparator 0. Compare condition: conversion result < 0x800; match Count=5. */
    printf("   Set the compare condition of comparator 0: channel 2 is less than 0x800; match count is 5.\n");
    ADC->ADCMPR[0] = (ADC->ADCMPR[0] & ~(ADC_ADCMPR_CMPD_Msk | ADC_ADCMPR_CMPMATCNT_Msk | ADC_ADCMPR_CMPCH_Msk | ADC_ADCMPR_CMPCOND_Msk)) | \
                     ((0x800) << ADC_ADCMPR_CMPD_Pos) | \
                     (((5) - 1) << ADC_ADCMPR_CMPMATCNT_Pos) | \
                     ((2) << ADC_ADCMPR_CMPCH_Pos) | \
                     ((ADC_LESS_THAN) << ADC_ADCMPR_CMPCOND_Pos) | ADC_ADCMPR_CMPEN_Msk;

    /* Enable ADC comparator 1. Compare condition: conversion result >= 0x800; match Count=5. */
    printf("   Set the compare condition of comparator 1 : channel 2 is greater than or equal to 0x800; match count is 5.\n");
    ADC->ADCMPR[1] = (ADC->ADCMPR[1] & ~(ADC_ADCMPR_CMPD_Msk | ADC_ADCMPR_CMPMATCNT_Msk | ADC_ADCMPR_CMPCH_Msk | ADC_ADCMPR_CMPCOND_Msk)) | \
                     ((0x800) << ADC_ADCMPR_CMPD_Pos) | \
                     (((5) - 1) << ADC_ADCMPR_CMPMATCNT_Pos) | \
                     ((2) << ADC_ADCMPR_CMPCH_Pos) | \
                     ((ADC_GREATER_OR_EQUAL) << ADC_ADCMPR_CMPCOND_Pos) | ADC_ADCMPR_CMPEN_Msk;

    /* Clear the ADC comparator 0 interrupt flag for safe */
    ADC->ADSR = ADC_ADSR_CMPF0_Msk;
    /* Enable ADC comparator 0 interrupt */
    ADC->ADCMPR[(0)] |= ADC_ADCMPR_CMPIE_Msk;

    /* Clear the ADC comparator 1 interrupt flag for safe */
    ADC->ADSR = ADC_ADSR_CMPF1_Msk;
    /* Enable ADC comparator 1 interrupt */
    ADC->ADCMPR[(1)] |= ADC_ADCMPR_CMPIE_Msk;

    NVIC_EnableIRQ(ADC_IRQn);

    g_u32AdcCmp0IntFlag = 0;
    g_u32AdcCmp1IntFlag = 0;

    /* Clear the ADC interrupt flag */
    ADC->ADSR = ADC_ADSR_ADF_Msk;

    /* Start A/D conversion */
    ADC->ADCR |= ADC_ADCR_ADST_Msk;

    /* Wait ADC compare interrupt */
    u32TimeOutCnt = SystemCoreClock; /* 1 second time-out */
    while((g_u32AdcCmp0IntFlag == 0) && (g_u32AdcCmp1IntFlag == 0))
    {
        if(--u32TimeOutCnt == 0)
        {
            printf("Wait for ADC compare interrupt time-out!\n");
            return;
        }
    }

    /* Stop A/D conversion */
    ADC->ADCR &= ~ADC_ADCR_ADST_Msk;
    /* Disable ADC comparator interrupt */
    ADC->ADCMPR[(0)] &= ~ADC_ADCMPR_CMPIE_Msk;
    ADC->ADCMPR[(1)] &= ~ADC_ADCMPR_CMPIE_Msk;
    /* Disable compare function */
    ADC->ADCMPR[(0)] &= ~ADC_ADCMPR_CMPEN_Msk;
    ADC->ADCMPR[(1)] &= ~ADC_ADCMPR_CMPEN_Msk;

    if(g_u32AdcCmp0IntFlag == 1) {
        printf("Comparator 0 interrupt occurs.\nThe conversion result of channel 2 is less than 0x800\n");
    } else {
        printf("Comparator 1 interrupt occurs.\nThe conversion result of channel 2 is greater than or equal to 0x800\n");
    }
}


/*---------------------------------------------------------------------------------------------------------*/
/* ADC interrupt handler                                                                                   */
/*---------------------------------------------------------------------------------------------------------*/
void ADC_IRQHandler(void)
{
    if(ADC->ADSR & ADC_ADSR_CMPF0_Msk) {
        g_u32AdcCmp0IntFlag = 1;
        ADC->ADSR = ADC_ADSR_CMPF0_Msk;     /* clear the A/D compare flag 0 */
    }

    if(ADC->ADSR & ADC_ADSR_CMPF1_Msk) {
        g_u32AdcCmp1IntFlag = 1;
        ADC->ADSR = ADC_ADSR_CMPF1_Msk;     /* clear the A/D compare flag 1 */
    }
}


/*---------------------------------------------------------------------------------------------------------*/
/* MAIN function                                                                                           */
/*---------------------------------------------------------------------------------------------------------*/
int main(void)
{

    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Init System, IP clock and multi-function I/O */
    SYS_Init();

    /* Lock protected registers */
    SYS_LockReg();

    /* Init UART0 for printf */
    UART0_Init();

    /*---------------------------------------------------------------------------------------------------------*/
    /* SAMPLE CODE                                                                                             */
    /*---------------------------------------------------------------------------------------------------------*/

    printf("\nSystem clock rate: %d Hz", SystemCoreClock);

    /* Result monitor test */
    AdcResultMonitorTest();

    /* Reset ADC module */
    SYS->IPRSTC2 |= (1 << SYS_IPRSTC2_ADC_RST_Pos) ;
    SYS->IPRSTC2 &= ~(1 << (SYS_IPRSTC2_ADC_RST_Pos)) ;

    /* Disable ADC IP clock */
    CLK->APBCLK &= ~CLK_APBCLK_ADC_EN_Msk;

    /* Disable External Interrupt */
    NVIC_DisableIRQ(ADC_IRQn);

    printf("Exit ADC sample code\n");

    while(1);

}

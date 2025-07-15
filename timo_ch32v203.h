#include <ch32v20x.h>
#include <ch32v20x_spi.h>
#include <ch32v20x_gpio.h>
#include <ch32v20x_exti.h>
#include "timo.h"
#include "millis.h"

// protos
void begin_spi();
void begin_pinsanditc();
unsigned char rw_spi(unsigned char data);
unsigned long timoirq();
void timo_csl();
void timo_csh();

// globals
volatile unsigned long timo_irqcounter = 0;
timo_t timo;

void begin_timo(timo_t *fn)
{
    begin_pinsanditc();
    begin_spi();
    fn->spi_send_rec = &rw_spi;
    fn->cs_h = &timo_csh;
    fn->cs_l = &timo_csl;
    fn->irq_pinCl = &timoirq;
    fn->waitUs = &micros;
}

#ifndef micros
// very crude
void micros(unsigned long long num);
void micros(unsigned long long num)
{
    volatile long long st = 0;
    num = num * 144; // 144mhz clock?
    while (st < num)
    {
        st++;
        __asm volatile("nop");
    }
}
#endif

void begin_spi()
{

    SPI_InitTypeDef spiinit;
    RCC_ClocksTypeDef clocks;
    RCC_GetClocksFreq(&clocks);

    spiinit.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    spiinit.SPI_Mode = SPI_Mode_Master;
    spiinit.SPI_DataSize = SPI_DataSize_8b;
    spiinit.SPI_CPOL = SPI_CPOL_Low;
    spiinit.SPI_CPHA = SPI_CPHA_1Edge;
    spiinit.SPI_NSS = SPI_NSS_Soft;
    if (clocks.SYSCLK_Frequency > 128000000)
        spiinit.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_128; // max speed 144/128
    else if (clocks.SYSCLK_Frequency > 64000000)
        spiinit.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64;
    else
        spiinit.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_32;
    spiinit.SPI_FirstBit = SPI_FirstBit_MSB;
    spiinit.SPI_CRCPolynomial = 7;

    GPIO_InitTypeDef gpiosck;
    GPIO_InitTypeDef gpiomosi;
    GPIO_InitTypeDef gpiomiso;
    GPIO_InitTypeDef gpiocs;

    gpiocs.GPIO_Pin = GPIO_Pin_4;
    gpiosck.GPIO_Pin = GPIO_Pin_5;
    gpiomiso.GPIO_Pin = GPIO_Pin_6;
    gpiomosi.GPIO_Pin = GPIO_Pin_7;

    gpiosck.GPIO_Speed = GPIO_Speed_50MHz;
    gpiomosi.GPIO_Speed = GPIO_Speed_50MHz;
    gpiomiso.GPIO_Speed = GPIO_Speed_50MHz;
    gpiocs.GPIO_Speed = GPIO_Speed_50MHz;

    gpiosck.GPIO_Mode = GPIO_Mode_AF_PP;
    gpiomosi.GPIO_Mode = GPIO_Mode_AF_PP;
    gpiomiso.GPIO_Mode = GPIO_Mode_AF_OD;
    gpiocs.GPIO_Mode = GPIO_Mode_Out_PP;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_Init(GPIOA, &gpiosck);
    GPIO_Init(GPIOA, &gpiomosi);
    GPIO_Init(GPIOA, &gpiomiso);
    GPIO_Init(GPIOA, &gpiocs);
    SPI_I2S_DeInit(SPI1);
    SPI_Init(SPI1, &spiinit);

    SPI_Cmd(SPI1, ENABLE);
}

void begin_pinsanditc()
{
    GPIO_InitTypeDef ce;
    GPIO_InitTypeDef it;

    EXTI_InitTypeDef itcA;

    ce.GPIO_Pin = GPIO_Pin_3;
    it.GPIO_Pin = GPIO_Pin_2;

    ce.GPIO_Speed = GPIO_Speed_50MHz;
    it.GPIO_Speed = GPIO_Speed_50MHz;

    ce.GPIO_Mode = GPIO_Mode_Out_PP;
    it.GPIO_Mode = GPIO_Mode_IN_FLOATING;

    itc.EXTI_Line = EXTI_Line2;
    itc.EXTI_Mode = EXTI_Mode_Interrupt;
    itc.EXTI_Trigger = EXTI_Trigger_Falling;
    itc.EXTI_LineCmd = ENABLE;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);

    EXTI_Init(&itc);

    GPIO_Init(GPIOA, &ce);
    GPIO_Init(GPIOA, &it);

    NVIC_EnableIRQ(EXTI2_IRQn);
}

unsigned char rw_spi(unsigned char data)
{
#define SPI_TIMOUT_LOOPS 1000
    volatile unsigned long timout = 0;
    while ((!SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE)) && (timout < SPI_TIMOUT_LOOPS))
    {
        timout++;
        micros(10);
    }
    timout = 0;
    SPI1->DATAR = (data & 0x00ff);
    while ((!SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE)) && (timout < SPI_TIMOUT_LOOPS))
    {
        timout++;
        micros(10);
    }
    data = (0xff & SPI1->DATAR);

    return data;
}
unsigned long timoirq()
{
    unsigned long r = timo_irqcounter;
    timo_irqcounter = 0;
    return r;
}

void timo_csl()
{
    GPIO_WriteBit(GPIOA, GPIO_Pin_3, Bit_RESET);
}

void timo_csh()
{
    GPIO_WriteBit(GPIOA, GPIO_Pin_3, Bit_SET);
}

extern void EXTI2_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void EXTI2_IRQHandler(void)
{
    timo_irqcounter++;
    EXTI_ClearITPendingBit(EXTI_Line2);
}
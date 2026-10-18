#include <ch32x035.h>
#include <ch32x035_spi.h>
#include <ch32x035_gpio.h>
#include <ch32x035_exti.h>
#include "timo/timo.h"
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

void delm(unsigned long long time)
{
    delay_micros(time);
}

void begin_timo(timo_t *fn)
{
    begin_pinsanditc();
    begin_spi();
    fn->spi_send_rec = &rw_spi;
    fn->cs_h = &timo_csh;
    fn->cs_l = &timo_csl;
    fn->irq_pinCl = &timoirq;
    fn->waitUs = &delm;
}

// #ifndef delay_micros
// // very crude
// void delay_micros(unsigned long long num);
// void delay_micros(unsigned long long num)
// {
//     volatile long long st = 0;
//     num = num * 144; // 144mhz clock?
//     while (st < num)
//     {
//         st++;
//         __asm volatile("nop");
//     }
// }
// #endif

void begin_spi()
{

    SPI_InitTypeDef spiinit;

    spiinit.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64;
    spiinit.SPI_CPHA = SPI_CPHA_1Edge;
    spiinit.SPI_CPOL = SPI_CPOL_Low;
    spiinit.SPI_DataSize = SPI_DataSize_8b;
    spiinit.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    spiinit.SPI_CRCPolynomial = 0x07;
    spiinit.SPI_FirstBit = SPI_FirstBit_MSB;
    spiinit.SPI_Mode = SPI_Mode_Master;
    spiinit.SPI_NSS = SPI_NSS_Soft;

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
    gpiomiso.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    gpiocs.GPIO_Mode = GPIO_Mode_Out_PP;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_Init(GPIOA, &gpiosck);
    GPIO_Init(GPIOA, &gpiomosi);
    GPIO_Init(GPIOA, &gpiomiso);
    GPIO_Init(GPIOA, &gpiocs);
    // SPI_I2S_DeInit(SPI1);

    timo_csh();

    SPI_Init(SPI1, &spiinit);

    SPI_Cmd(SPI1, ENABLE);
}

void begin_pinsanditc()
{
    GPIO_InitTypeDef it;

    EXTI_InitTypeDef itc;

    it.GPIO_Pin = GPIO_Pin_3;
    it.GPIO_Speed = GPIO_Speed_50MHz;

    it.GPIO_Mode = GPIO_Mode_IPU;

    itc.EXTI_Line = EXTI_Line3;
    itc.EXTI_Mode = EXTI_Mode_Interrupt;
    itc.EXTI_Trigger = EXTI_Trigger_Falling;
    itc.EXTI_LineCmd = ENABLE;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOC, ENABLE);

    GPIO_Init(GPIOC, &it);
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource3);

    EXTI_Init(&itc);

    NVIC_EnableIRQ(EXTI7_0_IRQn);
    NVIC_SetPriority(EXTI7_0_IRQn, 0xff); // low prio?

}

unsigned char rw_spi(unsigned char data)
{
#define SPI_TIMOUT_LOOPS 1000
    volatile unsigned long timout = 0;
    while ((!SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE)) && (timout < SPI_TIMOUT_LOOPS))
    {
        timout++;
        delay_micros(10);
    }
    timout = 0;
    SPI1->DATAR = (data & 0x00ff);
    while ((!SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE)) && (timout < SPI_TIMOUT_LOOPS))
    {
        timout++;
        delay_micros(10);
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
    GPIO_WriteBit(GPIOA, GPIO_Pin_4, Bit_RESET);
}

void timo_csh()
{
    GPIO_WriteBit(GPIOA, GPIO_Pin_4, Bit_SET);
}

void EXTI7_0_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void EXTI7_0_IRQHandler(void)
{
    timo_irqcounter++;
    EXTI_ClearITPendingBit(EXTI_Line3);
}
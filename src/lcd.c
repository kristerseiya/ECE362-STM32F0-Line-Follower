#include "stm32f0xx.h"
#include "stm32f0_discovery.h"
#include <stdint.h>
#include <stdio.h>

// These are function pointers.  They can be called like functions
// after you set them to point to other functions.
// e.g.  cmd = bitbang_cmd;
// They will be set by the stepX() subroutines to point to the new
// subroutines you write below.
void (*cmd)(char b) = 0;
void (*data)(char b) = 0;
void (*display1)(const char *) = 0;
void (*display2)(const char *) = 0;

int col = 0;
int8_t history[16] = {0};
int8_t lookup[16] = {1,4,7,0xe,2,5,8,0,3,6,9,0xf,0xa,0xb,0xc,0xd};
char char_lookup[16] = {'1','4','7','*','2','5','8','0','3','6','9','#','A','B','C','D'};
extern int count;

// Prototypes for subroutines in support.c
void generic_lcd_startup(void);
void countdown(void);
void step1(void);
void step2(void);
void step3(void);
void step4(void);
void step6(void);

// This array will be used with dma_display1() and dma_display2() to mix
// commands that set the cursor location at zero and 64 with characters.
//
uint16_t dispmem[34] = {
        0x080 + 0,
        0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220,
        0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220,
        0x080 + 64,
        0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220,
        0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220,
};

//=========================================================================
// Subroutines for step 2.
//=========================================================================
void spi_cmd(char b) {
    // Your code goes here.
    while((SPI2->SR & SPI_SR_TXE) == 0);
    SPI2->DR = b;
}

void spi_data(char b) {
    // Your code goes here.
    while((SPI2->SR & SPI_SR_TXE) == 0);
    SPI2->DR = b + 0x200;
}

void spi_init_lcd(void) {
    // Your code goes here.a
    // set gpio clock
    // set gpio
    // set spi clock
    // spi2->cr11 and cr2
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    GPIOB->MODER |= GPIO_MODER_MODER12 | GPIO_MODER_MODER13 | GPIO_MODER_MODER15;
    GPIOB->MODER &= ~GPIO_MODER_MODER12_0 & ~GPIO_MODER_MODER13_0 & ~GPIO_MODER_MODER15_0;
    GPIOB->AFR[1] &= ~GPIO_AFRH_AFRH4 & ~GPIO_AFRH_AFRH5 & ~GPIO_AFRH_AFRH7;

    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
    SPI2->CR1 |= SPI_CR1_BIDIMODE | SPI_CR1_BIDIOE;
    SPI2->CR1 |= SPI_CR1_MSTR;
    SPI2->CR1 |= SPI_CR1_BR;

    // Fastest Clock Rate
    //SPI2->CR1 &= ~SPI_CR1_BR;
    //SPI2->CR1 |= SPI_CR1_BR_0 | SPI_CR1_BR_1;

    SPI2->CR1 &= ~SPI_CR1_CPOL;
    SPI2->CR1 &= ~SPI_CR1_CPHA;
    SPI2->CR2 |= SPI_CR2_DS;
    SPI2->CR2 &= ~SPI_CR2_DS_1 & ~SPI_CR2_DS_2;
    SPI2->CR2 |= SPI_CR2_SSOE;
    SPI2->CR2 |= SPI_CR2_NSSP;
    SPI2->CR1 |= SPI_CR1_SPE;
    generic_lcd_startup();
}

//===========================================================================
// Subroutines for step 3.
//===========================================================================

// Display a string on line 1 using DMA.
void dma_display1(const char *s) {
    // Your code goes here.
    cmd(0x80 + 0);
    int x;
    for(x=0; x<16; x+=1)
        if (s[x])
            dispmem[x+1] = s[x] + 0x200;
        else
            break;
    for(   ; x<16; x+=1)
        dispmem[x+1] = 0x220;

    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    DMA1_Channel5->CCR &= ~DMA_CCR_EN;
    DMA1_Channel5->CMAR = (uint32_t) dispmem;
    DMA1_Channel5->CPAR = (uint32_t) &(SPI2->DR);
    DMA1_Channel5->CNDTR = 17;
    DMA1_Channel5->CCR |= DMA_CCR_DIR;
    DMA1_Channel5->CCR &= ~DMA_CCR_TCIE;
    DMA1_Channel5->CCR &= ~DMA_CCR_HTIE;
    DMA1_Channel5->CCR &= ~DMA_CCR_MSIZE;
    DMA1_Channel5->CCR |= DMA_CCR_MSIZE_0;
    DMA1_Channel5->CCR &= ~DMA_CCR_PSIZE;
    DMA1_Channel5->CCR |= DMA_CCR_PSIZE_0;
    DMA1_Channel5->CCR |= DMA_CCR_MINC;
    DMA1_Channel5->CCR &= ~DMA_CCR_PL;
    SPI2->CR2 |= SPI_CR2_TXDMAEN;
    DMA1_Channel5->CCR |= DMA_CCR_EN;
}

//===========================================================================
// Subroutines for Step 4.
//===========================================================================

void dma_spi_init_lcd(void) {
    // Your code goes here.
    spi_init_lcd();
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    DMA1_Channel5->CCR &= ~DMA_CCR_EN;
    DMA1_Channel5->CMAR = (uint32_t) dispmem;
    DMA1_Channel5->CPAR = (uint32_t) &(SPI2->DR);
    DMA1_Channel5->CNDTR = 34;
    DMA1_Channel5->CCR |= DMA_CCR_DIR;
    DMA1_Channel5->CCR &= ~DMA_CCR_TCIE;
    DMA1_Channel5->CCR &= ~DMA_CCR_HTIE;
    DMA1_Channel5->CCR &= ~DMA_CCR_MSIZE;
    DMA1_Channel5->CCR |= DMA_CCR_MSIZE_0;
    DMA1_Channel5->CCR &= ~DMA_CCR_PSIZE;
    DMA1_Channel5->CCR |= DMA_CCR_PSIZE_0;
    DMA1_Channel5->CCR |= DMA_CCR_MINC;
    DMA1_Channel5->CCR &= ~DMA_CCR_PL;
    DMA1_Channel5->CCR |= DMA_CCR_CIRC;
    SPI2->CR2 |= SPI_CR2_TXDMAEN;
    DMA1_Channel5->CCR |= DMA_CCR_EN;
}

// Display a string on line 1 by copying a string into the
// memory region circularly moved into the display by DMA.
void circdma_display1(const char *s) {
    // Your code goes here.
    int x;
    for(x=0; x<16; x+=1)
        if (s[x])
            dispmem[x+1] = s[x] + 0x200;
        else
            break;
    for(   ; x<16; x+=1)
        dispmem[x+1] = 0x220;
}

//===========================================================================
// Display a string on line 2 by copying a string into the
// memory region circularly moved into the display by DMA.
void circdma_display2(const char *s) {
    // Your code goes here.
    int x;
    for(x=0; x<16; x+=1)
        if (s[x])
            dispmem[x+18] = s[x] + 0x200;
        else
            break;
    for(   ; x<16; x+=1)
        dispmem[x+18] = 0x220;
}

//===========================================================================
// Subroutines for Step 6.
//===========================================================================
void init_keypad() {
    // Your code goes here.
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    GPIOA->MODER &= ~GPIO_MODER_MODER0 & ~GPIO_MODER_MODER1 & ~GPIO_MODER_MODER2 & ~GPIO_MODER_MODER3;
    GPIOA->MODER |= GPIO_MODER_MODER0_0 | GPIO_MODER_MODER1_0 | GPIO_MODER_MODER2_0 | GPIO_MODER_MODER3_0;
    GPIOA->MODER &= ~GPIO_MODER_MODER4 & ~GPIO_MODER_MODER5 & ~GPIO_MODER_MODER6 & ~GPIO_MODER_MODER7;
    GPIOA->PUPDR &= ~GPIO_PUPDR_PUPDR4 & ~GPIO_PUPDR_PUPDR5 & ~GPIO_PUPDR_PUPDR6 & ~GPIO_PUPDR_PUPDR7;
    GPIOA->PUPDR |= GPIO_PUPDR_PUPDR4_1 | GPIO_PUPDR_PUPDR5_1 | GPIO_PUPDR_PUPDR6_1 | GPIO_PUPDR_PUPDR7_1;
}

void init_tim6(void) {
    // Your code goes here.
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;
    TIM6->PSC = 480 - 1;
    TIM6->ARR = 100 - 1;
    TIM6->DIER |= TIM_DIER_UIE;
    NVIC->ISER[0] |= (1<<TIM6_DAC_IRQn);
    TIM6->CR1 |= TIM_CR1_CEN;
}

void TIM6_DAC_IRQHandler() {
    // Your code goes here.
    TIM6->SR &= ~TIM_SR_UIF;
    int row = (GPIOA->IDR >> 4) & 0xf;
    int index = col << 2;
    for (int i = 0; i < 4; i++) {
        history[index+i] = history[index+i] << 1;
        history[index+i] |= (row >> i) & 1;
    }
    col++;
    if (col > 3)
        col = 0;
    GPIOA->ODR = (1<< col);
    countdown();
}

int main(void)
{
    step1();
    //step2();
    //step3();
    //step4();
    //step6();
}

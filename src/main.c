#include "stm32f0xx.h"
#include "stm32f0_discovery.h"
#include <stdlib.h>
#include <math.h>

void setup_gpio(void);
//void setup_gpio_pwm(void);
void setup_pwm(void);
void setup_exti_line(void);
//void setup_gpio_hrsc40(void);
void setup_tim3_gen_pulse(void);
void setup_tim2_timer(void);
int calc_distance(volatile int);
void setup_dac(void);
void setup_timer6(void);
void init_wavetable(void);

void generic_lcd_startup(void);
void circdma_display1(const char *);
void circdma_display2(const char *);
void spi_cmd(char);
void spi_data(char);
void spi_init_lcd(void);
void dma_spi_init_lcd(void);
void step6(void);
void setup_tim14(void);

#define RATE 100000
#define N 1000
short int wavetable[N];

volatile int ten_microseconds_left = 0;
volatile int ten_microseconds_right = 0;
volatile int ten_microseconds_front = 0;

int offset = 0;
int step = 900 * N / 100000.0 * (1 << 16);

void (*cmd)(char b) = spi_cmd;
void (*data)(char b) = spi_data;
void (*display1)(const char *) = circdma_display1;
void (*display2)(const char *) = circdma_display2;

uint16_t dispmem[34] = {
        0x080 + 0,
        0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220,
        0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220,
        0x080 + 64,
        0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220,
        0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220,
};


// Initialize the display.

int main (void) {
	// TIM1 for PWM
	// TIM2 for counter
	// TIM3 for pulse
	// TIM6 for DAC

	// PA1 for generating pulse for HR-SC40 (output)

	// PA4 for DAC_OUT

	// PA5 for reading left echo (input)
	// PA6 for reading right echo (input)
	// PA7 for reading front echo (input)

	// PA8 for TIM1_CH1 (af)
	// PA9 for TIM1_CH2 (af)

	// PA10 for detecting left (output)
	// PA11 for detecting right (output)
	// PA12 for detecting front (output)

	// PC13 for reading left IR sensor (input)
	// PC14 for reading right IR sensor (input)

	// PA10 ~ PA14 as source for EXTI4_15 ISR
	dma_spi_init_lcd();
	display1("object(s) near:");
	display2("none");
	init_wavetable();
	setup_gpio();
	setup_dac();
	setup_timer6();
	//TIM6->CR1 |= TIM_CR1_CEN;
	//DAC->CR |= DAC_CR_EN1;
	setup_tim2_timer();
	setup_tim3_gen_pulse();
	setup_pwm();
	setup_exti_line();
	while(1);
	return 0;
}

void nano_wait(unsigned int n) {
    asm(    "        mov r0,%0\n"
            "repeat: sub r0,#83\n"
            "        bgt repeat\n" : : "r"(n) : "r0", "cc");
}

void generic_lcd_startup(void) {
    nano_wait(100000000); // Give it 100ms to initialize
    cmd(0x38);  // 0011 NF00 N=1, F=0: two lines
    cmd(0x0c);  // 0000 1DCB: display on, no cursor, no blink
    cmd(0x01);  // clear entire display
    nano_wait(6200000); // clear takes 6.2ms to complete
    cmd(0x02);  // put the cursor in the home position
    cmd(0x06);  // 0000 01IS: set display to increment
}

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

void step6(void) {
    // Configure the function pointers.
    cmd = spi_cmd;
    data = spi_data;
    display1 = circdma_display1;
    display2 = circdma_display2;

    // Initialize the display.
    dma_spi_init_lcd();
    char *msg1 = "Speed: 100%";

    char *msg2 = "left";
    char *msg3 = "     front";
    char *msg4 = "           right";

    int offset = 0;
    while(1) {
        display1(msg1);
        if ( (offset % 3) == 0)
        	display2(msg2);
        else if ( (offset % 3) == 1)
        	display2(msg3);
        else
        	display2(msg4);
        offset += 1;

        nano_wait(100000000);
    }
}

void setup_gpio()
{
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
	RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
    // PA1 (output)
    GPIOA->MODER &= ~GPIO_MODER_MODER1;
    GPIOA->MODER |= GPIO_MODER_MODER1_0;
    GPIOA->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR0;
    // PA4
    GPIOA->MODER |= GPIO_MODER_MODER4;
    // PA5, PA6, PA7 (input)
	GPIOA->MODER &= ~GPIO_MODER_MODER5 & ~GPIO_MODER_MODER6 & ~GPIO_MODER_MODER7;
	GPIOA->PUPDR &= ~GPIO_PUPDR_PUPDR5 & ~GPIO_PUPDR_PUPDR6 & ~GPIO_PUPDR_PUPDR7;
	GPIOA->PUPDR |= GPIO_PUPDR_PUPDR5_1 | GPIO_PUPDR_PUPDR6_1 | GPIO_PUPDR_PUPDR7_1;
    // PA8 and PA9 (af)
    GPIOA->MODER &= ~GPIO_MODER_MODER8 & ~GPIO_MODER_MODER9;
    GPIOA->MODER |= GPIO_MODER_MODER8_1 | GPIO_MODER_MODER9_1;
    GPIOA->AFR[1] &= ~GPIO_AFRH_AFRH0 | ~GPIO_AFRH_AFRH1;
    GPIOA->AFR[1] |= (2<<4) | (2<<0);
	// PA10, PA11, PA12 (output)
	GPIOA->MODER &= ~GPIO_MODER_MODER10 & ~GPIO_MODER_MODER11 & ~GPIO_MODER_MODER12;
	GPIOA->MODER |= GPIO_MODER_MODER10_0 | GPIO_MODER_MODER11_0 | GPIO_MODER_MODER12_0;
	GPIOA->PUPDR &= ~GPIO_PUPDR_PUPDR10 & ~GPIO_PUPDR_PUPDR11 & ~GPIO_PUPDR_PUPDR12;
	GPIOA->PUPDR |= GPIO_PUPDR_PUPDR10_1 | GPIO_PUPDR_PUPDR11_1 | GPIO_PUPDR_PUPDR12_1;
	// initialize
	GPIOA->ODR &= ~GPIO_ODR_1 & ~GPIO_ODR_10 & ~GPIO_ODR_11 & ~GPIO_ODR_12;
	//GPIOA->ODR |= GPIO_ODR_10;


}

void setup_pwm()
{
        RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
        TIM1->CR1 &= ~TIM_CR1_CEN;
        TIM1->CCMR1 &= ~(TIM_CCMR1_OC1M | TIM_CCMR1_OC2M);
        TIM1->CCMR1 |= TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1PE;
        TIM1->CCMR1 |= TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2PE;
        TIM1->CCER |= TIM_CCER_CC1E | TIM_CCER_CC2E;
        TIM1->PSC = 47;
        TIM1->ARR = 999;
		TIM1->CCR1 = 800;
		TIM1->CCR2 = 800;
        TIM1->BDTR |= TIM_BDTR_MOE;
        TIM1->CR1 |= TIM_CR1_CEN;
}

void setup_exti_line()
{
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    SYSCFG->EXTICR[2] &= ~SYSCFG_EXTICR3_EXTI10 & ~SYSCFG_EXTICR3_EXTI11;
    SYSCFG->EXTICR[3] &= ~SYSCFG_EXTICR4_EXTI12;
    EXTI->IMR |= EXTI_IMR_MR10 | EXTI_IMR_MR11 | EXTI_IMR_MR12;
    //EXTI->IMR |= EXTI_IMR_MR13 | EXTI_IMR_MR14;

    EXTI->RTSR |= EXTI_RTSR_TR10 | EXTI_RTSR_TR11 | EXTI_RTSR_TR12;
    EXTI->FTSR |= EXTI_FTSR_TR10 | EXTI_FTSR_TR11 | EXTI_FTSR_TR12;
    NVIC_EnableIRQ(EXTI4_15_IRQn);
}

void setup_tim14(){
		RCC->APB1ENR |= RCC_APB1ENR_TIM14EN;
		TIM14->PSC = 600 -1;
		TIM14->ARR = 80 - 1;
		TIM14->CNT = 0;
		TIM14->DIER |= TIM_DIER_UIE;
		NVIC->ISER[0] |= (1 << TIM14_IRQn);
		TIM14->CR1 |= TIM_CR1_CEN;
}

void EXTI4_15_IRQHandler()
{

	EXTI->PR |= EXTI_PR_PR10 | EXTI_PR_PR11 | EXTI_PR_PR12;
	//EXTI->PR |= EXTI_PR_PR13 | EXTI_PR_PR14;
	// detect object in front

	if (GPIOA->ODR & GPIO_ODR_12)
	{
		TIM6->CR1 |= TIM_CR1_CEN;
		step = 900 * N / 100000.0 * (1 << 16);
		//DAC->CR |= DAC_CR_EN1;
		TIM1->CCR1 = 0;
		TIM1->CCR2 = 0;
	}
	// detect object on side
	else if ((GPIOA->ODR & GPIO_ODR_10) || (GPIOA->ODR & GPIO_ODR_11))
	{
		TIM6->CR1 |= TIM_CR1_CEN;
		step = 680 * N / 100000.0 * (1 << 16);
		TIM1->CCR1 = 600;
		TIM1->CCR2 = 600;
	}
	// no object nearby
	else
	{
		TIM6->CR1 &= ~TIM_CR1_CEN;
		TIM1->CCR1 = 800;
		TIM1->CCR2 = 800;
	}


	int detect_status = (GPIOA->ODR & GPIO_ODR_10) >> 8;
	detect_status |= (GPIOA->ODR & GPIO_ODR_11) >> 10;
	detect_status |= (GPIOA->ODR & GPIO_ODR_12) >> 12;
	detect_status &= 7;

	switch (detect_status)
	{
		case 0: display2("none");
				break;

		case 1: display2("     front");
				break;

		case 2: display2("           right");
				break;

		case 3: display2("     front right");
				break;

		case 4: display2("left");
				break;

		case 5: display2("left front");
				break;

		case 6: display2("left       right");
				break;

		case 7: display2("left front right");
				break;

		default: display2("none");
				break;
	}

}

void setup_tim3_gen_pulse (void) {
	RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
	TIM3->PSC = 60 -1;
	TIM3->ARR = 8 - 1;
	TIM3->CNT = 0;
	TIM3->DIER |= TIM_DIER_UIE;
	NVIC->ISER[0] |= (1 << TIM3_IRQn);
	TIM3->CR1 |= TIM_CR1_CEN;
}

void setup_tim2_timer (void) {
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
	TIM2->PSC = 60 - 1;
	TIM2->ARR = 8 - 1;
	TIM2->CNT = 0;
	TIM2->DIER |= TIM_DIER_UIE;
	NVIC->ISER[0] |= (1 << TIM2_IRQn);
	TIM2->CR1 |= TIM_CR1_CEN;
}

int count = 0;

void TIM3_IRQHandler (void) {
	// generate 10us pulse every 100ms
	TIM3->SR &= ~TIM_SR_UIF;

	count++;
	if ((count % 10000) == 0) {
		GPIOA->ODR |= (1 << 1);
		count = 0;
	} else {
		GPIOA->ODR &= ~(1 << 1);
		//TIM3->CR1 &= ~TIM_CR1_CEN;
	}
}

void TIM2_IRQHandler (void) {
	TIM2->SR &= ~TIM_SR_UIF;

	// left sensor
	// echo is 1
	if (GPIOA->IDR & GPIO_IDR_5) {
		ten_microseconds_left++;
	}
	// echo is 0
	else {
		// falling edge
		if (ten_microseconds_left != 0) {
			int distance_left = calc_distance(ten_microseconds_left);
			// if distance is less than 10cm, turn on PA10
			if (distance_left < 200)
			{
				GPIOA->ODR |= GPIO_ODR_10;
			}
			else
			{
				GPIOA->ODR &= ~GPIO_ODR_10;
			}
		}
		ten_microseconds_left = 0;
	}

	// right
	if (GPIOA->IDR & GPIO_IDR_6) {
		ten_microseconds_right++;
	} else {
		if (ten_microseconds_right != 0) {
			int distance_right = calc_distance(ten_microseconds_right);
			if (distance_right < 200)
			{
				GPIOA->ODR |= GPIO_ODR_11;
			}
			else
			{
				GPIOA->ODR &= ~GPIO_ODR_11;
			}
		}
		ten_microseconds_right = 0;
	}

	// front

	if (GPIOA->IDR & GPIO_IDR_7) {
		ten_microseconds_front++;
	} else {
		if (ten_microseconds_front != 0) {
			int distance_front = calc_distance(ten_microseconds_front);
			if (distance_front < 100)
			{
				GPIOA->ODR |= GPIO_ODR_12;
			}
			else
			{
				GPIOA->ODR &= ~GPIO_ODR_12;
			}
		}
		ten_microseconds_front = 0;
	}


}

// calculate distance in mm
int calc_distance (volatile int time) {
	return time / 100000.0 * 340.0 / 2.0 * 1000.0;
}

// sound generation

void setup_dac() {
    // Student code goes here...
    RCC->APB1ENR |= RCC_APB1ENR_DACEN;
    DAC->CR &= ~DAC_CR_EN1;
    DAC->CR &= ~DAC_CR_BOFF1;
    DAC->CR |= DAC_CR_TEN1;
    DAC->CR |= DAC_CR_TSEL1;
    DAC->CR |= DAC_CR_EN1;
}

void setup_timer6() {
    // Student code goes here...
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;
    TIM6->PSC = 79;
    TIM6->ARR = 5;
    TIM6->DIER |= TIM_DIER_UIE;
    NVIC->ISER[0] |= (1<<TIM6_DAC_IRQn);
    //TIM6->CR1 |= TIM_CR1_CEN;
}

void init_wavetable(void)
{
    // Student code goes here...
    int x;
    for(x=0; x<N; x++)
      wavetable[x] = 32767 * sin(2 * M_PI * x / N);
}

void TIM6_DAC_IRQHandler() {
    // Student code goes here...
    offset += step;
    if ((offset>>16) >= N)  // If we go past the end of the array,
      offset -= N<<16;  // wrap around with same offset as overshoot.

    int sample = wavetable[offset>>16];    // get a sample

    sample = sample / 16 + 208;      // adjust for the DAC range
    //if (sample > 4095) sample = 4095;
    //else if (sample < 0) sample = 0;
    DAC->DHR12R1 = sample;
    DAC->SWTRIGR |= DAC_SWTRIGR_SWTRIG1; // trigger DAC here.

    TIM6->SR &= ~1;
}


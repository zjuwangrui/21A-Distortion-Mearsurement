#define CONFIG_H

// Configuration parameters
#define BAUD_RATE 9600
#define BUFFER_SIZE 256

//beep config
#define BEEP_PIN                    GPIO_PIN_8      // PA8引脚
#define BEEP_PORT                   GPIOA           // GPIOA端口
#define BEEP_CLK_ENABLE()           __HAL_RCC_GPIOA_CLK_ENABLE()  // 使能GPIOA时钟

// analog in pins
#define V_PIN PA0
#define T_PIN PA1
#define L_PIN PA2
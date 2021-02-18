# QUECTEL-3G-LTE
STM32 HAL library for QUECTEL module SIM over AT command
- STM32CubeIDE
- STM32F411VETx
- Module SIM Quectel UC15/EC21

Choose 2 UART for Serial and Module Sim communication

![quectel-1](https://user-images.githubusercontent.com/31510769/108330642-6d0bfa00-7200-11eb-95f8-8e5da68bf373.PNG)

Enable global interrupt for Module Sim UART

![quectel-2](https://user-images.githubusercontent.com/31510769/108332034-f2dc7500-7201-11eb-86a5-f39e375fa0a6.PNG)

Configure the UART port where your module is connected in the SIM_STM32.h file

```
/*Change used UART here*/
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
//extern UART_HandleTypeDef huart3;
//extern UART_HandleTypeDef huart4;
//extern UART_HandleTypeDef huart5;
//extern UART_HandleTypeDef huart6;
```

```
/*Change used UART here*/
#if (SIM_DEBUG == 1)
#define LOG_UART 	huart1
#endif
#define SIM_UART 	huart2
#define SIM_USART	USART2
```

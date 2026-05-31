/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "spi.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "w25flash.h"
#include "lcd.h"
#include "lcd_fonts.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t bufferPageRead[256];  // 接受一个page数据的缓冲区，w25q128的page大小是256字节
uint8_t bufferPageWrite[256]; // 用于写入一个page数据的缓冲区
uint8_t lcd_message[30] = {0};      // LCD显示字符串的缓冲区
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* 读取器件ID，根据ID可以判断是哪家的flash */
void flash_test_read_id(void)
{
  uint16_t id_temp = Flash_ReadID();
  sprintf((char *)lcd_message, "ID=0x%04X", id_temp);// 使用sprintf要包含string.h
  LCD_DisplayString(10,3*26,(char *)lcd_message);
}

/* 读状态寄存器SR1和SR2 */
void flash_test_read_status(void)
{
  uint8_t SR1 = Flash_ReadSR1(); // 读寄存器SR1
  sprintf((char *)lcd_message, "Status Reg1 = 0x%02X", SR1); // Hex显示
  LCD_DisplayString(10,4*26,(char *)lcd_message);
  uint8_t SR2 = Flash_ReadSR2(); // 读寄存器SR2
  sprintf((char *)lcd_message, "Status Reg2 = 0x%02X", SR2); // Hex显示
  LCD_DisplayString(10,5*26,(char *)lcd_message);
}

/*
* 测试写入page0和page1
* 注意写之前先擦除
* w25q128的总容量16MB，
* 16MB被分为256个块（Block），
* 每个Block被分为16个扇区（Sector），
* 每个Sector被分为16个页（Page），
* 每个页256字节
*/
void flash_test_write_page(void)
{
  // 写入page0
  uint8_t blockNum = 0; // Block0
  uint16_t sectorNum = 0; // Sector0
  uint16_t pageNum = 0; // Page0
  uint32_t memAddress = 0;
  memAddress = Flash_Addr_byBlockSectorPage(blockNum, sectorNum, pageNum); // 计算全局地址
  uint8_t writeData1[] = "Hello from beginning";
  uint16_t len = strlen((char *)writeData1) + 1; // 注意strlen不包括字符串结尾的'\0'，所以要加1
  Flash_WriteInPage(memAddress, writeData1, len); // 在Page0起始地址写入数据

  uint8_t writeData2[] = "Hello in page";
  len = strlen((char *)writeData2) + 1;
  Flash_WriteInPage(memAddress + 100, writeData2, len); // 在Page0的偏移地址写入数据

  // 写入page1
  uint8_t writeData3[256]; // FLASH_PAGE_SIZE = 256字节
  for (uint16_t i = 0; i < 256; i++)
  {
    writeData3[i] = i; // 准备数据
  } 
  pageNum = 1; // Page1
  memAddress = Flash_Addr_byBlockSectorPage(blockNum, sectorNum, pageNum); // 计算全局地址
  Flash_WriteInPage(memAddress, writeData3, 256); // 写一个Page的数据
}

/* 测试读取page0和page1的数据 */
void flash_test_read_page(void)
{
  // 读取page0
  uint8_t blockNum = 0; // Block0
  uint16_t sectorNum = 0; // Sector0
  uint16_t pageNum = 0; // Page0
  uint32_t memAddress = Flash_Addr_byBlockSectorPage(blockNum, sectorNum, pageNum); // 计算全局地址
  uint8_t readBuffer[50] = {0}; // 用于存储读取的数据
  Flash_ReadBytes(memAddress, readBuffer, 50); // 从Page0起始地址读取一个Page的数据
  LCD_DisplayString(10,1*18,(char *)readBuffer); // 显示读取的数据
  Flash_ReadBytes(memAddress + 100, readBuffer, 50); // 从Page0的偏移地址读取数据
  LCD_DisplayString(10,2*18,(char *)readBuffer); // 显示读取的数据

  // 读取page1
  pageNum = 3; // Page1
  memAddress = Flash_Addr_byBlockSectorPage(blockNum, sectorNum, pageNum); // 计算全局地址
  uint8_t randData = 0; // 随机读一个字节
  randData = Flash_ReadOneByte(memAddress + 12); // 页内偏移地址12，根据之前写入的数据，这个位置应该是数字12
  sprintf((char *)lcd_message, "Random data in page1 = %d", randData); // Decimal显示
  LCD_DisplayString(10,3*18,(char *)lcd_message);
  randData = Flash_ReadOneByte(memAddress + 136); // 页内偏移地址136，根据之前写入的数据，这个位置应该是数字136
  sprintf((char *)lcd_message, "Random data in page1 = %d", randData); // Decimal显示
  LCD_DisplayString(10,4*18,(char *)lcd_message);
  randData = Flash_ReadOneByte(memAddress + 255); // 页内偏移地址255，根据之前写入的数据，这个位置应该是数字255
  sprintf((char *)lcd_message, "Random data in page1 = %d", randData); // Decimal显示
  LCD_DisplayString(10,5*18,(char *)lcd_message);
}

/* 以DMA方式写入1个page的数据 */
void flash_test_write_page_dma(void)
{
  uint8_t blockNum = 0; // Block0
  uint16_t sectorNum = 0; // Sector0
  uint32_t memAddress = 0;

  // 写之前先擦除page所在的扇区
  uint16_t sectorNo = Flash_Addr_byBlockSector(blockNum, sectorNum) / FLASH_SECTOR_SIZE; // 计算扇区编号
  memAddress = Flash_Addr_bySector(sectorNo); // 计算扇区绝对地址
  Flash_EraseSector(memAddress); // 擦除扇区

  for (uint16_t i = 0; i < 256; i++)
  {
    bufferPageWrite[i] = i; // 准备数据
  }

  uint16_t pageNum = 3; // Page3
  memAddress = Flash_Addr_byBlockSectorPage(blockNum, sectorNum, pageNum); // 计算全局地址
  uint8_t byte2, byte3, byte4;
  Flash_SpliteAddr(memAddress, &byte2, &byte3, &byte4); // 24位地址分解为3个字节

  Flash_Write_Enable(); // 写使能
  Flash_Wait_Busy();
  __Select_Flash();           // CS=0
  SPI_TransmitOneByte(0x02);  // Command=0x02，对一个页编程
  SPI_TransmitOneByte(byte2); // 发送24位地址
  SPI_TransmitOneByte(byte3);
  SPI_TransmitOneByte(byte4);
  //以DMA方式连续写入256字节数据
  LCD_DisplayString(10,5*18,"DMA Writing..."); // 显示写入开始的提示信息
  HAL_SPI_Transmit_DMA(&hspi2, bufferPageWrite, 256); // 发送byteCount个字节的数据
}

/* DMA发送中断回调*/
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
  if (hspi->Instance == SPI2) // 判断是哪个SPI的DMA传输完成
  {
    __Deselect_Flash(); // CS=1，结束通信
    Flash_Wait_Busy(); // 等待写入完成
    LCD_DisplayString(10,6*18,"DMA Write Completed"); // 显示写入完成的提示信息
  }
}

/* 以DMA方式读1个page */
void flash_test_read_page_dma(void)
{
	Flash_Wait_Busy();
  uint8_t blockNum = 0; // Block0
  uint16_t sectorNum = 0; // Sector0
  uint16_t pageNum = 3; // Page3
  uint32_t memAddress = Flash_Addr_byBlockSectorPage(blockNum, sectorNum, pageNum); // 计算全局地址
  uint8_t byte2, byte3, byte4;
  Flash_SpliteAddr(memAddress, &byte2, &byte3, &byte4); // 24位地址分解为3个字节

  __Select_Flash();           // CS=0
  SPI_TransmitOneByte(0x03);  // Command=0x03，读数据
  SPI_TransmitOneByte(byte2); // 发送24位地址
  SPI_TransmitOneByte(byte3);
  SPI_TransmitOneByte(byte4);
  
  //以DMA方式连续读取256字节数据到bufferPageRead缓冲区
  LCD_DisplayString(10,5*18,"DMA Reading..."); // 显示读取开始的提示信息
  HAL_SPI_Receive_DMA(&hspi2, bufferPageRead, 256);
}

/* DMA接收中断回调 */
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
  if (hspi->Instance == SPI2) // 判断是哪个SPI的DMA传输完成
  {
    __Deselect_Flash(); // CS=1，结束通信
		Flash_Wait_Busy(); // 等待读取完成
    LCD_DisplayString(10,6*18,"DMA Read Completed"); // 显示读取完成的提示信息
    // 显示读取的数据
    sprintf((char *)lcd_message, "Data in page3: %d %d %d", bufferPageRead[0], bufferPageRead[100], bufferPageRead[255]);
    LCD_DisplayString(10,7*18,(char *)lcd_message);
  }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SPI2_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  LCD_Init();// 屏幕初始化，已经包含等待上电稳定的延时

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  
  LCD_SetAsciiFont(&ASCII_Font24);
	LCD_DisplayString(10,1*26,"SPI_FLASH"); // 驱动库无自动换行
	
	LCD_SetAsciiFont(&ASCII_Font16); // 改个字体大小
  
//  LCD_DisplayString(10,2*26,"Erasing...");
//  Flash_EraseChip(); // 写入数据前应该先擦除
//  LCD_ClearRect(10,2*26,LCD_Width,26); // 擦除提示信息
//  LCD_DisplayString(10,2*26,"Erased");
  
//  LCD_DisplayString(10,3*26,"Writing...");
//  flash_test_write_page(); // 测试写入page0和page1
 
//  flash_test_write_page_dma(); // dma写
//	flash_test_read_page();
//	HAL_Delay(2000);
  flash_test_read_page_dma(); // dma读

  while (1)
  {

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV2;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

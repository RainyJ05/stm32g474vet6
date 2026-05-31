#include "lcd.h"

#define LCD_SPI hspi1 // SPI局部宏，方便修改和移植

static pFONT *LCD_AsciiFonts; // 英文字体，ASCII字符集
static pFONT *LCD_CHFonts;    // 中文字体（同时也包含英文字体）

// 因为这类SPI的屏幕，每次更新显示时，需要先配置坐标区域、再写显存，
// 在显示字符时，如果是一个个点去写坐标写显存，会非常慢，
// 因此开辟一片缓冲区，先将需要显示的数据写进缓冲区，最后再批量写入显存。
// 用户可以根据实际情况去修改此处缓冲区的大小，
// 例如，用户需要显示32*32的汉字时，需要的大小为 32*32*2 = 2048 字节（每个像素点占2字节）
uint16_t LCD_Buff[1024]; // LCD缓冲区，16位宽（每个像素点占2字节）

struct // LCD相关参数结构体
{
   uint32_t Color;       //	LCD当前画笔颜色
   uint32_t BackColor;   //	背景色
   uint8_t ShowNum_Mode; // 数字显示模式
   uint8_t Direction;    //	显示方向
   uint16_t Width;       // 屏幕像素长度
   uint16_t Height;      // 屏幕像素宽度
   uint8_t X_Offset;     // X坐标偏移，用于设置屏幕控制器的显存写入方式
   uint8_t Y_Offset;     // Y坐标偏移，用于设置屏幕控制器的显存写入方式
} LCD;

static int _pointx = 0;
static int _pointy = 0;
void MoveTo(int x, int y)
{
   _pointx = x;
   _pointy = y;
}
TypeXY GetXY(void)
{

   TypeXY m;
   m.x = _pointx;
   m.y = _pointy;
   return m;
}
int GetX(void)
{
   return _pointx;
}
int GetY(void)
{
   return _pointy;
}
void LineTo(int x, int y)
{
   LCD_DrawLine(_pointx, _pointy, x, y);
   _pointx = x;
   _pointy = y;
}

unsigned char ScreenBuffer[8][240] = {0}; // 8  128
unsigned char TempBuffer[8][240] = {0};

TypeRoate _RoateValue = {{0, 0}, 0, 1};

unsigned int OledTimeMs = 0;

uint8_t SPI2_ReadWriteByte(uint8_t TxData)
{
   uint8_t Rxdata;
   HAL_SPI_TransmitReceive(&LCD_SPI, &TxData, &Rxdata, 1, 1000);
   return Rxdata; // 返回收到的数据
}

uint8_t SPI2_WriteData(uint8_t *data, uint16_t size)
{
   return HAL_SPI_Transmit(&LCD_SPI, data, size, 1000);
}

void LCD_SPI_Send(uint8_t *data, uint16_t size)
{
   SPI2_WriteData(data, size);
}

void LCD_WriteCommand(uint8_t lcd_command)
{
   LCD_DC_Command;

   LCD_SPI_Send(&lcd_command, 1);
   LCD_DC_Data;
}

/**
 * @brief	写数据到LCD
 *
 * @param   cmd		需要发送的数据
 *
 * @return  void
 */
void LCD_WriteData_8bit(uint8_t lcd_data)
{
   LCD_DC_Data;
   LCD_SPI_Send(&lcd_data, 1);
}

/**
 * @brief	写半个字的数据到LCD
 *
 * @param   cmd		需要发送的数据
 *
 * @return  void
 */
void LCD_WriteData_16bit(uint16_t lcd_data)
{
   uint8_t data[2] = {0};

   data[0] = lcd_data >> 8;
   data[1] = lcd_data;

   LCD_DC_Data;
   LCD_SPI_Send(data, 2);
}

/****************************************************************************************************************************************
 *	函 数 名: LCD_WriteBuff
 *
 *	入口参数: DataBuff - 数据区，DataSize - 数据长度
 *
 *	函数功能: 批量写入数据到屏幕
 *
 ****************************************************************************************************************************************/

void LCD_WriteBuff(uint16_t *DataBuff, uint16_t DataSize)
{
   uint32_t i;
   LCD_CS_L; // 片选拉低，使能IC

   for (i = 0; i < DataSize; i++)
   {
      LCD_WriteData_16bit(DataBuff[i]);
   }
   LCD_CS_H; // 片选拉高
}

/****************************************************************************************************************************************
 *	函 数 名: SPI_LCD_Init
 *
 *	函数功能: 初始化SPI以及屏幕控制器的各种参数
 *
 ****************************************************************************************************************************************/

void LCD_Init(void)
{
   LCD_DC_Data;       // DC引脚拉高，默认处于写数据状态
   LCD_CS_H;          // 拉高片选，禁止通信
   LCD_Backlight_OFF; // 先关闭背光，初始化完成之后再打开

   HAL_Delay(10); // 屏幕刚完成复位时（包括上电复位），需要等待5ms才能发送指令
   LCD_SetColor(LCD_WHITE);
   LCD_CS_L; // 片选拉低，使能IC，开始通信

   LCD_WriteCommand(0x36);   // 显存访问控制 指令，用于设置访问显存的方式
   LCD_WriteData_8bit(0x00); // 配置成 从上到下、从左到右，RGB像素格式

   LCD_WriteCommand(0x3A);   // 接口像素格式 指令，用于设置使用 12位、16位还是18位色
   LCD_WriteData_8bit(0x05); // 此处配置成 16位 像素格式

   // 接下来很多都是电压设置指令，直接使用厂家给设定值
   LCD_WriteCommand(0xB2);
   LCD_WriteData_8bit(0x0C);
   LCD_WriteData_8bit(0x0C);
   LCD_WriteData_8bit(0x00);
   LCD_WriteData_8bit(0x33);
   LCD_WriteData_8bit(0x33);

   LCD_WriteCommand(0xB7);   // 栅极电压设置指令
   LCD_WriteData_8bit(0x35); // VGH = 13.26V，VGL = -10.43V

   LCD_WriteCommand(0xBB);   // 公共电压设置指令
   LCD_WriteData_8bit(0x19); // VCOM = 1.35V

   LCD_WriteCommand(0xC0);
   LCD_WriteData_8bit(0x2C);

   LCD_WriteCommand(0xC2);   // VDV 和 VRH 来源设置
   LCD_WriteData_8bit(0x01); // VDV 和 VRH 由用户自由配置

   LCD_WriteCommand(0xC3);   // VRH电压 设置指令
   LCD_WriteData_8bit(0x12); // VRH电压 = 4.6+( vcom+vcom offset+vdv)

   LCD_WriteCommand(0xC4);   // VDV电压 设置指令
   LCD_WriteData_8bit(0x20); // VDV电压 = 0v

   LCD_WriteCommand(0xC6);   // 正常模式的帧率控制指令
   LCD_WriteData_8bit(0x0F); // 设置屏幕控制器的刷新帧率为60帧

   LCD_WriteCommand(0xD0);   // 电源控制指令
   LCD_WriteData_8bit(0xA4); // 无效数据，固定写入0xA4
   LCD_WriteData_8bit(0xA1); // AVDD = 6.8V ，AVDD = -4.8V ，VDS = 2.3V

   LCD_WriteCommand(0xE0); // 正极电压伽马值设定
   LCD_WriteData_8bit(0xD0);
   LCD_WriteData_8bit(0x04);
   LCD_WriteData_8bit(0x0D);
   LCD_WriteData_8bit(0x11);
   LCD_WriteData_8bit(0x13);
   LCD_WriteData_8bit(0x2B);
   LCD_WriteData_8bit(0x3F);
   LCD_WriteData_8bit(0x54);
   LCD_WriteData_8bit(0x4C);
   LCD_WriteData_8bit(0x18);
   LCD_WriteData_8bit(0x0D);
   LCD_WriteData_8bit(0x0B);
   LCD_WriteData_8bit(0x1F);
   LCD_WriteData_8bit(0x23);

   LCD_WriteCommand(0xE1); // 负极电压伽马值设定
   LCD_WriteData_8bit(0xD0);
   LCD_WriteData_8bit(0x04);
   LCD_WriteData_8bit(0x0C);
   LCD_WriteData_8bit(0x11);
   LCD_WriteData_8bit(0x13);
   LCD_WriteData_8bit(0x2C);
   LCD_WriteData_8bit(0x3F);
   LCD_WriteData_8bit(0x44);
   LCD_WriteData_8bit(0x51);
   LCD_WriteData_8bit(0x2F);
   LCD_WriteData_8bit(0x1F);
   LCD_WriteData_8bit(0x1F);
   LCD_WriteData_8bit(0x20);
   LCD_WriteData_8bit(0x23);

   LCD_WriteCommand(0x21); // 打开反显，因为面板是常黑型，操作需要反过来

   // 退出休眠指令，LCD控制器在刚上电、复位时，会自动进入休眠模式 ，因此操作屏幕之前，需要退出休眠
   LCD_WriteCommand(0x11); // 退出休眠 指令
   HAL_Delay(120);         // 需要等待120ms，让电源电压和时钟电路稳定下来

   // 打开显示指令，LCD控制器在刚上电、复位时，会自动关闭显示
   LCD_WriteCommand(0x29); // 打开显示

   while ((LCD_SPI.Instance->SR & 0x0080) != RESET)
      ;      //	等待通信完成
   LCD_CS_H; // 片选拉高

   // 以下进行一些驱动的默认设置
   LCD_SetDirection(Direction_V); //	设置显示方向
   LCD_SetBackColor(LCD_BLACK);   // 设置背景色
   LCD_SetColor(LCD_WHITE);       // 设置画笔色
   LCD_Clear();                   // 清屏

   LCD_SetAsciiFont(&ASCII_Font24);       // 设置默认字体
   LCD_ShowNumMode(Fill_Zero); // 设置变量显示模式，多余位填充空格还是填充0

   // 全部设置完毕之后，打开背光
   LCD_Backlight_ON; // 引脚输出高电平点亮背光
}

/****************************************************************************************************************************************
 *	函 数 名:	 LCD_SetAddress
 *
 *	入口参数:	 x1 - 起始水平坐标   y1 - 起始垂直坐标
 *              x2 - 终点水平坐标   y2 - 终点垂直坐标
 *
 *	函数功能:   设置需要显示的坐标区域
 *****************************************************************************************************************************************/

void LCD_SetAddress(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
   LCD_CS_L; // 片选拉低，使能IC

   LCD_WriteCommand(0x2a); //	列地址设置，即X坐标
   LCD_WriteData_16bit(x1 + LCD.X_Offset);
   LCD_WriteData_16bit(x2 + LCD.X_Offset);

   LCD_WriteCommand(0x2b); //	行地址设置，即Y坐标
   LCD_WriteData_16bit(y1 + LCD.Y_Offset);
   LCD_WriteData_16bit(y2 + LCD.Y_Offset);

   LCD_WriteCommand(0x2c); //	开始写入显存，即要显示的颜色数据

   while ((LCD_SPI.Instance->SR & 0x0080) != RESET)
      ;      //	等待通信完成
   LCD_CS_H; // 片选拉高
}

/****************************************************************************************************************************************
 *	函 数 名:	LCD_SetColor
 *
 *	入口参数:	Color - 要显示的颜色，示例：0x0000FF 表示蓝色
 *
 *	函数功能:	此函数用于设置画笔的颜色，例如显示字符、画点画线、绘图的颜色
 *
 *	说    明:	1. 为了方便用户使用自定义颜色，入口参数 Color 使用24位 RGB888的颜色格式，用户无需关心颜色格式的转换
 *					2. 24位的颜色中，从高位到低位分别对应 R、G、B  3个颜色通道
 *
 *****************************************************************************************************************************************/

void LCD_SetColor(uint32_t Color)
{
   uint16_t Red_Value = 0, Green_Value = 0, Blue_Value = 0; // 各个颜色通道的值

   Red_Value = (uint16_t)((Color & 0x00F80000) >> 8); // 转换成 16位 的RGB565颜色
   Green_Value = (uint16_t)((Color & 0x0000FC00) >> 5);
   Blue_Value = (uint16_t)((Color & 0x000000F8) >> 3);

   LCD.Color = (uint16_t)(Red_Value | Green_Value | Blue_Value); // 将颜色写入全局LCD参数
}

/****************************************************************************************************************************************
 *	函 数 名:	LCD_SetBackColor
 *
 *	入口参数:	Color - 要显示的颜色，示例：0x0000FF 表示蓝色
 *
 *	函数功能:	设置背景色,此函数用于清屏以及显示字符的背景色
 *
 *	说    明:	1. 为了方便用户使用自定义颜色，入口参数 Color 使用24位 RGB888的颜色格式，用户无需关心颜色格式的转换
 *					2. 24位的颜色中，从高位到低位分别对应 R、G、B  3个颜色通道
 *
 *****************************************************************************************************************************************/

void LCD_SetBackColor(uint32_t Color)
{
   uint16_t Red_Value = 0, Green_Value = 0, Blue_Value = 0; // 各个颜色通道的值

   Red_Value = (uint16_t)((Color & 0x00F80000) >> 8); // 转换成 16位 的RGB565颜色
   Green_Value = (uint16_t)((Color & 0x0000FC00) >> 5);
   Blue_Value = (uint16_t)((Color & 0x000000F8) >> 3);

   LCD.BackColor = (uint16_t)(Red_Value | Green_Value | Blue_Value); // 将颜色写入全局LCD参数
}

/****************************************************************************************************************************************
 *	函 数 名:	LCD_SetDirection
 *
 *	入口参数:	direction - 要显示的方向
 *
 *	函数功能:	设置要显示的方向
 *
 *	说    明:   1. 可输入参数 Direction_H 、Direction_V 、Direction_H_Flip 、Direction_V_Flip
 *              2. 使用示例 LCD_DisplayDirection(Direction_H) ，即设置屏幕横屏显示
 *
 *****************************************************************************************************************************************/

void LCD_SetDirection(uint8_t direction)
{
   LCD.Direction = direction; // 写入全局LCD参数

   LCD_CS_L; // 片选拉低，使能IC

   if (direction == Direction_H) // 横屏显示
   {
      LCD_WriteCommand(0x36);   // 显存访问控制 指令，用于设置访问显存的方式
      LCD_WriteData_8bit(0x70); // 横屏显示
      LCD.X_Offset = 20;        // 设置控制器坐标偏移量
      LCD.Y_Offset = 0;
      LCD.Width = LCD_Height; // 重新赋值长、宽
      LCD.Height = LCD_Width;
   }
   else if (direction == Direction_V)
   {
      LCD_WriteCommand(0x36);   // 显存访问控制 指令，用于设置访问显存的方式
      LCD_WriteData_8bit(0x00); // 垂直显示
      LCD.X_Offset = 0;         // 设置控制器坐标偏移量
      LCD.Y_Offset = 20;
      LCD.Width = LCD_Width; // 重新赋值长、宽
      LCD.Height = LCD_Height;
   }
   else if (direction == Direction_H_Flip)
   {
      LCD_WriteCommand(0x36);   // 显存访问控制 指令，用于设置访问显存的方式
      LCD_WriteData_8bit(0xA0); // 横屏显示，并上下翻转，RGB像素格式
      LCD.X_Offset = 20;        // 设置控制器坐标偏移量
      LCD.Y_Offset = 0;
      LCD.Width = LCD_Height; // 重新赋值长、宽
      LCD.Height = LCD_Width;
   }
   else if (direction == Direction_V_Flip)
   {
      LCD_WriteCommand(0x36);   // 显存访问控制 指令，用于设置访问显存的方式
      LCD_WriteData_8bit(0xC0); // 垂直显示 ，并上下翻转，RGB像素格式
      LCD.X_Offset = 0;         // 设置控制器坐标偏移量
      LCD.Y_Offset = 20;
      LCD.Width = LCD_Width; // 重新赋值长、宽
      LCD.Height = LCD_Height;
   }
   while ((LCD_SPI.Instance->SR & 0x0080) != RESET)
      ;      //	等待通信完成
   LCD_CS_H; // 片选拉高
}

/****************************************************************************************************************************************
 *	函 数 名:	LCD_SetAsciiFont
 *
 *	入口参数:	*fonts - 要设置的ASCII字体
 *
 *	函数功能:	设置ASCII字体，可选择使用 3216/2412/2010/1608/1206 五种大小的字体
 *
 *	说    明:	1. 使用示例 LCD_SetAsciiFont(&ASCII_Font24) ，即设置 2412的 ASCII字体
 *					2. 相关字模存放在 lcd_fonts.c
 *
 *****************************************************************************************************************************************/

void LCD_SetAsciiFont(pFONT *Asciifonts)
{
   LCD_AsciiFonts = Asciifonts;
}

/****************************************************************************************************************************************
 *	函 数 名:	LCD_Clear
 *
 *	函数功能:	清屏函数，将LCD清除为 LCD.BackColor 的颜色
 *
 *	说    明:	先用 LCD_SetBackColor() 设置要清除的背景色，再调用该函数清屏即可
 *
 *****************************************************************************************************************************************/

void LCD_Clear(void)
{
   uint32_t i;

   LCD_SetAddress(0, 0, LCD.Width - 1, LCD.Height - 1); // 设置坐标

   LCD_CS_L; // 片选拉低，使能IC

   for (i = 0; i < LCD.Width * LCD.Height; i++)
   {
      LCD_WriteData_16bit(LCD.BackColor);
   }
   LCD_CS_H; // 片选拉高
}

/****************************************************************************************************************************************
 *	函 数 名:	LCD_ClearRect
 *
 *	入口参数:	x - 起始水平坐标
 *					y - 起始垂直坐标
 *					width  - 要清除区域的横向长度
 *					height - 要清除区域的纵向宽度
 *
 *	函数功能:	局部清屏函数，将指定位置对应的区域清除为 LCD.BackColor 的颜色
 *
 *	说    明:	1. 先用 LCD_SetBackColor() 设置要清除的背景色，再调用该函数清屏即可
 *				   2. 使用示例 LCD_ClearRect( 10, 10, 100, 50) ，清除坐标(10,10)开始的长100宽50的区域
 *
 *****************************************************************************************************************************************/

void LCD_ClearRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
   uint16_t i;

   LCD_SetAddress(x, y, x + width - 1, y + height - 1); // 设置坐标

   LCD_CS_L; // 片选拉低，使能IC

   for (i = 0; i < width * height; i++)
   {
      LCD_WriteData_16bit(LCD.BackColor);
   }

   LCD_CS_H; // 片选拉高
}

/****************************************************************************************************************************************
 *	函 数 名:	LCD_DrawPoint
 *
 *	入口参数:	x - 起始水平坐标
 *					y - 起始垂直坐标
 *					color  - 要绘制的颜色，使用 24位 RGB888 的颜色格式，用户无需关心颜色格式的转换
 *
 *	函数功能:	在指定坐标绘制指定颜色的点
 *
 *	说    明:	使用示例 LCD_DrawPoint( 10, 10, 0x0000FF) ，在坐标(10,10)绘制蓝色的点
 *
 *****************************************************************************************************************************************/

void LCD_DrawPoint(uint16_t x, uint16_t y, uint32_t color)
{
   LCD_SetAddress(x, y, x, y); //	设置坐标

   LCD_CS_L; // 片选拉低，使能IC

   LCD_WriteData_16bit(LCD.Color);

   while ((LCD_SPI.Instance->SR & 0x0080) != RESET)
      ;      //	等待通信完成
   LCD_CS_H; // 片选拉高
}

/****************************************************************************************************************************************
 *	函 数 名:	LCD_DisplayChar
 *
 *	入口参数:	x - 起始水平坐标
 *					y - 起始垂直坐标
 *					c  - ASCII字符
 *
 *	函数功能:	在指定坐标显示指定的字符
 *
 *	说    明:	1. 可设置要显示的字体，例如使用 LCD_SetAsciiFont(&ASCII_Font24) 设置为 2412的ASCII字体
 *					2.	可设置要显示的颜色，例如使用 LCD_SetColor(0xff0000FF) 设置为蓝色
 *					3. 可设置对应的背景色，例如使用 LCD_SetBackColor(0x000000) 设置为黑色的背景色
 *					4. 使用示例 LCD_DisplayChar( 10, 10, 'a') ，在坐标(10,10)显示字符 'a'
 *
 *****************************************************************************************************************************************/

void LCD_DisplayChar(uint16_t x, uint16_t y, uint8_t c)
{
   uint16_t index = 0, counter = 0, i = 0, w = 0; // 计数变量
   uint8_t disChar;                               // 存储字符的地址

   c = c - 32; // 计算ASCII字符的偏移

   LCD_CS_L; // 片选拉低，使能IC

   for (index = 0; index < LCD_AsciiFonts->Sizes; index++)
   {
      disChar = LCD_AsciiFonts->pTable[c * LCD_AsciiFonts->Sizes + index]; // 获取字符的模值
      for (counter = 0; counter < 8; counter++)
      {
         if (disChar & 0x01)
         {
            LCD_Buff[i] = LCD.Color; // 当前模值不为0时，使用画笔色绘点
         }
         else
         {
            LCD_Buff[i] = LCD.BackColor; // 否则使用背景色绘制点
         }
         disChar >>= 1;
         i++;
         w++;
         if (w == LCD_AsciiFonts->Width) // 如果写入的数据达到了字符宽度，则退出当前循环
         {                               // 进入下一字符的写入的绘制
            w = 0;
            break;
         }
      }
   }
   LCD_SetAddress(x, y, x + LCD_AsciiFonts->Width - 1, y + LCD_AsciiFonts->Height - 1); // 设置坐标
   LCD_WriteBuff(LCD_Buff, LCD_AsciiFonts->Width * LCD_AsciiFonts->Height);             // 写入显存
}

/**************************************************************************************************************************************
 *	函 数 名:	LCD_DisplayString
 *
 *	入口参数:	x - 起始水平坐标
 *					y - 起始垂直坐标
 *					p - ASCII字符串的首地址
 *
 *	函数功能:	在指定坐标显示指定的字符串
 *
 *	说    明:	1. 可设置要显示的字体，例如使用 LCD_SetAsciiFont(&ASCII_Font24) 设置为 2412的ASCII字体
 *					2.	可设置要显示的颜色，例如使用 LCD_SetColor(0x0000FF) 设置为蓝色
 *					3. 可设置对应的背景色，例如使用 LCD_SetBackColor(0x000000) 设置为黑色的背景色
 *
 *****************************************************************************************************************************************/

void LCD_DisplayString(uint16_t x, uint16_t y, char *p)
{
   while ((x < LCD.Width) && (*p != 0)) // 判断显示坐标是否超出显示区域并且字符是否为空字符
   {
      LCD_DisplayChar(x, y, *p);
      x += LCD_AsciiFonts->Width; // 显示下一个字符
      p++;                        // 取下一个字符地址
   }
}

/****************************************************************************************************************************************
 *	函 数 名:	LCD_SetTextFont
 *
 *	入口参数:	*fonts - 要设置的文本字体
 *
 *	函数功能:	设置文本字体，包括中文和ASCII字符，
 *
 *	说    明:	1. 可选择使用 3232/2424/2020/1616/1212 五种大小的中文字体，
 *						并且对应的设置ASCII字体为 3216/2412/2010/1608/1206
 *					2. 相关字模存放在 lcd_fonts.c
 *					3. 中文字库使用的是小字库，即用到了对应的汉字再去取模
 *					4. 使用示例 LCD_SetTextFont(&CH_Font24) ，即设置 2424的中文字体以及2412的ASCII字符字体
 *
 *****************************************************************************************************************************************/

void LCD_SetTextFont(pFONT *fonts)
{
   LCD_CHFonts = fonts; // 设置中文字体
   switch (fonts->Width)
   {
   case 12:
      LCD_AsciiFonts = &ASCII_Font12;
      break; // 设置ASCII字符的字体为 1206
   case 16:
      LCD_AsciiFonts = &ASCII_Font16;
      break; // 设置ASCII字符的字体为 1608
   case 20:
      LCD_AsciiFonts = &ASCII_Font20;
      break; // 设置ASCII字符的字体为 2010
   case 24:
      LCD_AsciiFonts = &ASCII_Font24;
      break; // 设置ASCII字符的字体为 2412
   case 32:
      LCD_AsciiFonts = &ASCII_Font32;
      break; // 设置ASCII字符的字体为 3216
   default:
      break;
   }
}
/******************************************************************************************************************************************
 *	函 数 名:	LCD_DisplayChinese
 *
 *	入口参数:	x - 起始水平坐标
 *					y - 起始垂直坐标
 *					pText - 中文字符
 *
 *	函数功能:	在指定坐标显示指定的单个中文字符
 *
 *	说    明:	1. 可设置要显示的字体，例如使用 LCD_SetTextFont(&CH_Font24) 设置为 2424的中文字体以及2412的ASCII字符字体
 *					2.	可设置要显示的颜色，例如使用 LCD_SetColor(0xff0000FF) 设置为蓝色
 *					3. 可设置对应的背景色，例如使用 LCD_SetBackColor(0xff000000) 设置为黑色的背景色
 *					4. 使用示例 LCD_DisplayChinese( 10, 10, "反") ，在坐标(10,10)显示中文字符"反"
 *
 *****************************************************************************************************************************************/

void LCD_DisplayChinese(uint16_t x, uint16_t y, char *pText)
{
   uint16_t i = 0, index = 0, counter = 0; // 计数变量
   uint16_t addr;                          // 字模地址
   uint8_t disChar;                        // 字模的值
   uint16_t Xaddress = 0;                  // 水平坐标

   while (1)
   {
      // 对比数组中的汉字编码，用以定位该汉字字模的地址
      if (*(LCD_CHFonts->pTable + (i + 1) * LCD_CHFonts->Sizes + 0) == *pText && *(LCD_CHFonts->pTable + (i + 1) * LCD_CHFonts->Sizes + 1) == *(pText + 1))
      {
         addr = i; // 字模地址偏移
         break;
      }
      i += 2; // 每个中文字符编码占两字节

      if (i >= LCD_CHFonts->Table_Rows)
         break; // 字模列表中无相应的汉字
   }
   i = 0;
   for (index = 0; index < LCD_CHFonts->Sizes; index++)
   {
      disChar = *(LCD_CHFonts->pTable + (addr)*LCD_CHFonts->Sizes + index); // 获取相应的字模地址

      for (counter = 0; counter < 8; counter++)
      {
         if (disChar & 0x01)
         {
            LCD_Buff[i] = LCD.Color; // 当前模值不为0时，使用画笔色绘点
         }
         else
         {
            LCD_Buff[i] = LCD.BackColor; // 否则使用背景色绘制点
         }
         i++;
         disChar >>= 1;
         Xaddress++; // 水平坐标自加

         if (Xaddress == LCD_CHFonts->Width) //	如果水平坐标达到了字符宽度，则退出当前循环
         {                                   //	进入下一行的绘制
            Xaddress = 0;
            break;
         }
      }
   }
   LCD_SetAddress(x, y, x + LCD_CHFonts->Width - 1, y + LCD_CHFonts->Height - 1); // 设置坐标
   LCD_WriteBuff(LCD_Buff, LCD_CHFonts->Width * LCD_CHFonts->Height);             // 写入显存
}

/*****************************************************************************************************************************************
 *	函 数 名:	LCD_DisplayText
 *
 *	入口参数:	x - 起始水平坐标
 *					y - 起始垂直坐标
 *					pText - 字符串，可以显示中文或者ASCII字符
 *
 *	函数功能:	在指定坐标显示指定的字符串
 *
 *	说    明:	1. 可设置要显示的字体，例如使用 LCD_SetTextFont(&CH_Font24) 设置为 2424的中文字体以及2412的ASCII字符字体
 *					2.	可设置要显示的颜色，例如使用 LCD_SetColor(0xff0000FF) 设置为蓝色
 *					3. 可设置对应的背景色，例如使用 LCD_SetBackColor(0xff000000) 设置为黑色的背景色
 *
 *****************************************************************************************************************************************/

void LCD_DisplayText(uint16_t x, uint16_t y, char *pText)
{

   while (*pText != 0) // 判断是否为空字符
   {
      if (*pText <= 0x7F) // 判断是否为ASCII码
      {
         LCD_DisplayChar(x, y, *pText); // 显示ASCII
         x += LCD_AsciiFonts->Width;    // 水平坐标调到下一个字符处
         pText++;                       // 字符串地址+1
      }
      else // 若字符为汉字
      {
         LCD_DisplayChinese(x, y, pText); // 显示汉字
         x += LCD_CHFonts->Width;         // 水平坐标调到下一个字符处
         pText += 2;                      // 字符串地址+2，汉字的编码要2字节
      }
   }
}

/*****************************************************************************************************************************************
 *	函 数 名:	LCD_ShowNumMode
 *
 *	入口参数:	mode - 设置变量的显示模式
 *
 *	函数功能:	设置变量显示时多余位补0还是补空格，可输入参数 Fill_Space 填充空格，Fill_Zero 填充零
 *
 *	说    明:   1. 只有 LCD_DisplayNumber() 显示整数 和 LCD_DisplayDecimals()显示小数 这两个函数用到
 *					2. 使用示例 LCD_ShowNumMode(Fill_Zero) 设置多余位填充0，例如 123 可以显示为 000123
 *
 *****************************************************************************************************************************************/

void LCD_ShowNumMode(uint8_t mode)
{
   LCD.ShowNum_Mode = mode;
}

/*****************************************************************************************************************************************
 *	函 数 名:	LCD_DisplayNumber
 *
 *	入口参数:	x - 起始水平坐标
 *					y - 起始垂直坐标
 *					number - 要显示的数字,范围在 -2147483648~2147483647 之间
 *					len - 数字的位数，如果位数超过len，将按其实际长度输出，如果需要显示负数，请预留一个位的符号显示空间
 *
 *	函数功能:	在指定坐标显示指定的整数变量
 *
 *	说    明:	1. 可设置要显示的字体，例如使用 LCD_SetAsciiFont(&ASCII_Font24) 设置为的ASCII字符字体
 *					2.	可设置要显示的颜色，例如使用 LCD_SetColor(0x0000FF) 设置为蓝色
 *					3. 可设置对应的背景色，例如使用 LCD_SetBackColor(0x000000) 设置为黑色的背景色
 *					4. 使用示例 LCD_DisplayNumber( 10, 10, a, 5) ，在坐标(10,10)显示指定变量a,总共5位，多余位补0或空格，
 *						例如 a=123 时，会根据 LCD_ShowNumMode()的设置来显示  123(前面两个空格位) 或者00123
 *
 *****************************************************************************************************************************************/

void LCD_DisplayNumber(uint16_t x, uint16_t y, int32_t number, uint8_t len)
{
   char Number_Buffer[15]; // 用于存储转换后的字符串

   if (LCD.ShowNum_Mode == Fill_Zero) // 多余位补0
   {
      sprintf(Number_Buffer, "%0.*d", len, number); // 将 number 转换成字符串，便于显示
   }
   else // 多余位补空格
   {
      sprintf(Number_Buffer, "%*d", len, number); // 将 number 转换成字符串，便于显示
   }

   LCD_DisplayString(x, y, (char *)Number_Buffer); // 将转换得到的字符串显示出来
}

/***************************************************************************************************************************************
 *	函 数 名:	LCD_DisplayDecimals
 *
 *	入口参数:	x - 起始水平坐标
 *					y - 起始垂直坐标
 *					decimals - 要显示的数字, double型取值1.7 x 10^（-308）~ 1.7 x 10^（+308），但是能确保准确的有效位数为15~16位
 *
 *       			len - 整个变量的总位数（包括小数点和负号），若实际的总位数超过了指定的总位数，将按实际的总长度位输出，
 *							示例1：小数 -123.123 ，指定 len <=8 的话，则实际照常输出 -123.123
 *							示例2：小数 -123.123 ，指定 len =10 的话，则实际输出   -123.123(负号前面会有两个空格位)
 *							示例3：小数 -123.123 ，指定 len =10 的话，当调用函数 LCD_ShowNumMode() 设置为填充0模式时，实际输出 -00123.123
 *
 *					decs - 要保留的小数位数，若小数的实际位数超过了指定的小数位，则按指定的宽度四舍五入输出
 *							 示例：1.12345 ，指定 decs 为4位的话，则输出结果为1.1235
 *
 *	函数功能:	在指定坐标显示指定的变量，包括小数
 *
 *	说    明:	1. 可设置要显示的字体，例如使用 LCD_SetAsciiFont(&ASCII_Font24) 设置为的ASCII字符字体
 *					2.	可设置要显示的颜色，例如使用 LCD_SetColor(0x0000FF) 设置为蓝色
 *					3. 可设置对应的背景色，例如使用 LCD_SetBackColor(0x000000) 设置为黑色的背景色
 *					4. 使用示例 LCD_DisplayDecimals( 10, 10, a, 5, 3) ，在坐标(10,10)显示字变量a,总长度为5位，其中保留3位小数
 *
 *****************************************************************************************************************************************/

void LCD_DisplayDecimals(uint16_t x, uint16_t y, double decimals, uint8_t len, uint8_t decs)
{
   char Number_Buffer[20]; // 用于存储转换后的字符串

   if (LCD.ShowNum_Mode == Fill_Zero) // 多余位填充0模式
   {
      sprintf(Number_Buffer, "%0*.*lf", len, decs, decimals); // 将 number 转换成字符串，便于显示
   }
   else // 多余位填充空格
   {
      sprintf(Number_Buffer, "%*.*lf", len, decs, decimals); // 将 number 转换成字符串，便于显示
   }

   LCD_DisplayString(x, y, (char *)Number_Buffer); // 将转换得到的字符串显示出来
}

/***************************************************************************************************************************************
 *	函 数 名: LCD_DrawLine
 *
 *	入口参数: x1 - 起点 水平坐标
 *			 	 y1 - 起点 垂直坐标
 *
 *				 x2 - 终点 水平坐标
 *            y2 - 终点 垂直坐标
 *
 *	函数功能: 在两点之间画线
 *
 *	说    明: 该函数移植于ST官方评估板的例程
 *
 *****************************************************************************************************************************************/

#define ABS(X) ((X) > 0 ? (X) : -(X))

void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
   int16_t deltax = 0, deltay = 0, x = 0, y = 0, xinc1 = 0, xinc2 = 0,
           yinc1 = 0, yinc2 = 0, den = 0, num = 0, numadd = 0, numpixels = 0,
           curpixel = 0;

   deltax = ABS(x2 - x1); /* The difference between the x's */
   deltay = ABS(y2 - y1); /* The difference between the y's */
   x = x1;                /* Start x off at the first pixel */
   y = y1;                /* Start y off at the first pixel */

   if (x2 >= x1) /* The x-values are increasing */
   {
      xinc1 = 1;
      xinc2 = 1;
   }
   else /* The x-values are decreasing */
   {
      xinc1 = -1;
      xinc2 = -1;
   }

   if (y2 >= y1) /* The y-values are increasing */
   {
      yinc1 = 1;
      yinc2 = 1;
   }
   else /* The y-values are decreasing */
   {
      yinc1 = -1;
      yinc2 = -1;
   }

   if (deltax >= deltay) /* There is at least one x-value for every y-value */
   {
      xinc1 = 0; /* Don't change the x when numerator >= denominator */
      yinc2 = 0; /* Don't change the y for every iteration */
      den = deltax;
      num = deltax / 2;
      numadd = deltay;
      numpixels = deltax; /* There are more x-values than y-values */
   }
   else /* There is at least one y-value for every x-value */
   {
      xinc2 = 0; /* Don't change the x for every iteration */
      yinc1 = 0; /* Don't change the y when numerator >= denominator */
      den = deltay;
      num = deltay / 2;
      numadd = deltax;
      numpixels = deltay; /* There are more y-values than x-values */
   }
   for (curpixel = 0; curpixel <= numpixels; curpixel++)
   {
      LCD_DrawPoint(x, y, LCD.Color); /* Draw the current pixel */
      num += numadd;                  /* Increase the numerator by the top of the fraction */
      if (num >= den)                 /* Check if numerator >= denominator */
      {
         num -= den; /* Calculate the new numerator value */
         x += xinc1; /* Change the x as appropriate */
         y += yinc1; /* Change the y as appropriate */
      }
      x += xinc2; /* Change the x as appropriate */
      y += yinc2; /* Change the y as appropriate */
   }
}

/***************************************************************************************************************************************
 *	函 数 名: LCD_DrawLine_V
 *
 *	入口参数: x - 水平坐标
 *			 	 y - 垂直坐标
 *				 height - 垂直宽度
 *
 *	函数功能: 在指点位置绘制指定长宽的 垂直 线
 *
 *	说    明: 1. 该函数移植于ST官方评估板的例程
 *				 2. 要绘制的区域不能超过屏幕的显示区域
 *            3. 如果只是画垂直的线，优先使用此函数，速度比 LCD_DrawLine 快很多
 *  性能测试：
 *****************************************************************************************************************************************/

void LCD_DrawLine_V(uint16_t x, uint16_t y, uint16_t height)
{
   uint16_t i; // 计数变量

   for (i = 0; i < height; i++)
   {
      LCD_Buff[i] = LCD.Color; // 写入缓冲区
   }
   LCD_SetAddress(x, y, x, y + height - 1); // 设置坐标

   LCD_WriteBuff(LCD_Buff, height); // 写入显存
}

/***************************************************************************************************************************************
 *	函 数 名: LCD_DrawLine_H
 *
 *	入口参数: x - 水平坐标
 *			 	 y - 垂直坐标
 *				 width  - 水平宽度
 *
 *	函数功能: 在指点位置绘制指定长宽的 水平 线
 *
 *	说    明: 1. 该函数移植于ST官方评估板的例程
 *				 2. 要绘制的区域不能超过屏幕的显示区域
 *            3. 如果只是画 水平 的线，优先使用此函数，速度比 LCD_DrawLine 快很多
 *  性能测试：
 ************************************************************************************************************************************/

void LCD_DrawLine_H(uint16_t x, uint16_t y, uint16_t width)
{
   uint16_t i; // 计数变量

   for (i = 0; i < width; i++)
   {
      LCD_Buff[i] = LCD.Color; // 写入缓冲区
   }
   LCD_SetAddress(x, y, x + width - 1, y); // 设置坐标

   LCD_WriteBuff(LCD_Buff, width); // 写入显存
}

/***************************************************************************************************************************************
 *	函 数 名: LCD_DrawRect
 *
 *	入口参数: x - 水平坐标
 *			 	 y - 垂直坐标
 *			 	 width  - 水平宽度
 *				 height - 垂直宽度
 *
 *	函数功能: 在指点位置绘制指定长宽的矩形线条
 *
 *	说    明: 1. 该函数移植于ST官方评估板的例程
 *				 2. 要绘制的区域不能超过屏幕的显示区域
 *
 *****************************************************************************************************************************************/

void LCD_DrawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
   // 绘制水平线
   LCD_DrawLine_H(x, y, width);
   LCD_DrawLine_H(x, y + height - 1, width);

   // 绘制垂直线
   LCD_DrawLine_V(x, y, height);
   LCD_DrawLine_V(x + width - 1, y, height);
}

/***************************************************************************************************************************************
 *	函 数 名: LCD_DrawCircle
 *
 *	入口参数: x - 圆心 水平坐标
 *			 	 y - 圆心 垂直坐标
 *			 	 r  - 半径
 *
 *	函数功能: 在坐标 (x,y) 绘制半径为 r 的圆形线条
 *
 *	说    明: 1. 该函数移植于ST官方评估板的例程
 *				 2. 要绘制的区域不能超过屏幕的显示区域
 *
 *****************************************************************************************************************************************/

void LCD_DrawCircle(uint16_t x, uint16_t y, uint16_t r)
{
   int Xadd = -r, Yadd = 0, err = 2 - 2 * r, e2;
   do
   {

      LCD_DrawPoint(x - Xadd, y + Yadd, LCD.Color);
      LCD_DrawPoint(x + Xadd, y + Yadd, LCD.Color);
      LCD_DrawPoint(x + Xadd, y - Yadd, LCD.Color);
      LCD_DrawPoint(x - Xadd, y - Yadd, LCD.Color);

      e2 = err;
      if (e2 <= Yadd)
      {
         err += ++Yadd * 2 + 1;
         if (-Xadd == Yadd && e2 <= Xadd)
            e2 = 0;
      }
      if (e2 > Xadd)
         err += ++Xadd * 2 + 1;
   } while (Xadd <= 0);
}

/***************************************************************************************************************************************
 *	函 数 名: LCD_DrawEllipse
 *
 *	入口参数: x - 圆心 水平坐标
 *			 	 y - 圆心 垂直坐标
 *			 	 r1  - 水平半轴的长度
 *				 r2  - 垂直半轴的长度
 *
 *	函数功能: 在坐标 (x,y) 绘制水平半轴为 r1 垂直半轴为 r2 的椭圆线条
 *
 *	说    明: 1. 该函数移植于ST官方评估板的例程
 *				 2. 要绘制的区域不能超过屏幕的显示区域
 *
 *****************************************************************************************************************************************/

void LCD_DrawEllipse(int x, int y, int r1, int r2)
{
   int Xadd = -r1, Yadd = 0, err = 2 - 2 * r1, e2;
   float K = 0, rad1 = 0, rad2 = 0;

   rad1 = r1;
   rad2 = r2;

   if (r1 > r2)
   {
      do
      {
         K = (float)(rad1 / rad2);

         LCD_DrawPoint(x - Xadd, y + (uint16_t)(Yadd / K), LCD.Color);
         LCD_DrawPoint(x + Xadd, y + (uint16_t)(Yadd / K), LCD.Color);
         LCD_DrawPoint(x + Xadd, y - (uint16_t)(Yadd / K), LCD.Color);
         LCD_DrawPoint(x - Xadd, y - (uint16_t)(Yadd / K), LCD.Color);

         e2 = err;
         if (e2 <= Yadd)
         {
            err += ++Yadd * 2 + 1;
            if (-Xadd == Yadd && e2 <= Xadd)
               e2 = 0;
         }
         if (e2 > Xadd)
            err += ++Xadd * 2 + 1;
      } while (Xadd <= 0);
   }
   else
   {
      Yadd = -r2;
      Xadd = 0;
      do
      {
         K = (float)(rad2 / rad1);

         LCD_DrawPoint(x - (uint16_t)(Xadd / K), y + Yadd, LCD.Color);
         LCD_DrawPoint(x + (uint16_t)(Xadd / K), y + Yadd, LCD.Color);
         LCD_DrawPoint(x + (uint16_t)(Xadd / K), y - Yadd, LCD.Color);
         LCD_DrawPoint(x - (uint16_t)(Xadd / K), y - Yadd, LCD.Color);

         e2 = err;
         if (e2 <= Xadd)
         {
            err += ++Xadd * 3 + 1;
            if (-Yadd == Xadd && e2 <= Yadd)
               e2 = 0;
         }
         if (e2 > Yadd)
            err += ++Yadd * 3 + 1;
      } while (Yadd <= 0);
   }
}

/***************************************************************************************************************************************
 *	函 数 名: LCD_FillCircle
 *
 *	入口参数: x - 圆心 水平坐标
 *			 	 y - 圆心 垂直坐标
 *			 	 r  - 半径
 *
 *	函数功能: 在坐标 (x,y) 填充半径为 r 的圆形区域
 *
 *	说    明: 1. 该函数移植于ST官方评估板的例程
 *				 2. 要绘制的区域不能超过屏幕的显示区域
 *
 *****************************************************************************************************************************************/

void LCD_FillCircle(uint16_t x, uint16_t y, uint16_t r)
{
   int32_t D;     /* Decision Variable */
   uint32_t CurX; /* Current X Value */
   uint32_t CurY; /* Current Y Value */

   D = 3 - (r << 1);

   CurX = 0;
   CurY = r;

   while (CurX <= CurY)
   {
      if (CurY > 0)
      {
         LCD_DrawLine_V(x - CurX, y - CurY, 2 * CurY);
         LCD_DrawLine_V(x + CurX, y - CurY, 2 * CurY);
      }

      if (CurX > 0)
      {
         // LCD_DrawLine(x - CurY, y - CurX,x - CurY,y - CurX + 2*CurX);
         // LCD_DrawLine(x + CurY, y - CurX,x + CurY,y - CurX + 2*CurX);

         LCD_DrawLine_V(x - CurY, y - CurX, 2 * CurX);
         LCD_DrawLine_V(x + CurY, y - CurX, 2 * CurX);
      }
      if (D < 0)
      {
         D += (CurX << 2) + 6;
      }
      else
      {
         D += ((CurX - CurY) << 2) + 10;
         CurY--;
      }
      CurX++;
   }
   LCD_DrawCircle(x, y, r);
}

/***************************************************************************************************************************************
 *	函 数 名: LCD_FillRect
 *
 *	入口参数: x - 水平坐标
 *			 	 y - 垂直坐标
 *			 	 width  - 水平宽度
 *				 height -垂直宽度
 *
 *	函数功能: 在坐标 (x,y) 填充指定长宽的实心矩形
 *
 *	说    明: 要绘制的区域不能超过屏幕的显示区域
 *
 *****************************************************************************************************************************************/

void LCD_FillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
   uint16_t i;

   LCD_SetAddress(x, y, x + width - 1, y + height - 1); // 设置坐标

   LCD_CS_L; // 片选拉低，使能IC

   for (i = 0; i < width * height; i++)
   {
      LCD_WriteData_16bit(LCD.Color);
   }
   LCD_CS_H; // 片选拉高
}

/***************************************************************************************************************************************
 *	函 数 名: LCD_DrawImage
 *
 *	入口参数: x - 起始水平坐标
 *				 y - 起始垂直坐标
 *			 	 width  - 图片的水平宽度
 *				 height - 图片的垂直宽度
 *				*pImage - 图片数据存储区的首地址
 *
 *	函数功能: 在指定坐标处显示图片
 *
 *	说    明: 1.要显示的图片需要事先进行取模、获悉图片的长度和宽度
 *            2.使用 LCD_SetColor() 函数设置画笔色，LCD_SetBackColor() 设置背景色
 *
 *****************************************************************************************************************************************/

void LCD_DrawImage(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t *pImage)
{
   uint8_t disChar;              // 字模的值
   uint16_t Xaddress = x;        // 水平坐标
   uint16_t Yaddress = y;        // 垂直坐标
   uint16_t i = 0, j = 0, m = 0; // 计数变量
   uint16_t BuffCount = 0;       // 缓冲区计数
   uint16_t Buff_Height = 0;     // 缓冲区的行数

   // 因为缓冲区大小有限，需要分多次写入
   Buff_Height = (sizeof(LCD_Buff) / 2) / height; // 计算缓冲区能够写入图片的多少行

   for (i = 0; i < height; i++) // 循环按行写入
   {
      for (j = 0; j < (float)width / 8; j++)
      {
         disChar = *pImage;

         for (m = 0; m < 8; m++)
         {
            if (disChar & 0x01)
            {
               LCD_Buff[BuffCount] = LCD.Color; // 当前模值不为0时，使用画笔色绘点
            }
            else
            {
               LCD_Buff[BuffCount] = LCD.BackColor; // 否则使用背景色绘制点
            }
            disChar >>= 1;               // 模值移位
            Xaddress++;                  // 水平坐标自加
            BuffCount++;                 // 缓冲区计数
            if ((Xaddress - x) == width) // 如果水平坐标达到了字符宽度，则退出当前循环,进入下一行的绘制
            {
               Xaddress = x;
               break;
            }
         }
         pImage++;
      }
      if (BuffCount == Buff_Height * width) // 达到缓冲区所能容纳的最大行数时
      {
         BuffCount = 0; // 缓冲区计数清0

         LCD_SetAddress(x, Yaddress, x + width - 1, Yaddress + Buff_Height - 1); // 设置坐标
         LCD_WriteBuff(LCD_Buff, width * Buff_Height);                           // 写入显存

         Yaddress = Yaddress + Buff_Height; // 计算行偏移，开始写入下一部分数据
      }
      if ((i + 1) == height) // 到了最后一行时
      {
         LCD_SetAddress(x, Yaddress, x + width - 1, i + y);       // 设置坐标
         LCD_WriteBuff(LCD_Buff, width * (i + 1 + y - Yaddress)); // 写入显存
      }
   }
}

/**********************************************************************************************************************************************************************************************************************************/
//*****************************************************************************************************
//**************************************************************************************
void DrawRoundRect(int x, int y, unsigned char w, unsigned char h, unsigned char r)
{
   // smarter version
   LCD_DrawLine_H(x + r, y, w - 2 * r);         // Top
   LCD_DrawLine_H(x + r, y + h - 1, w - 2 * r); // Bottom
   LCD_DrawLine_V(x, y + r, h - 2 * r);         // Left
   LCD_DrawLine_V(x + w - 1, y + r, h - 2 * r); // Right
   // draw four corners
   DrawCircleHelper(x + r, y + r, r, 1);
   DrawCircleHelper(x + w - r - 1, y + r, r, 2);
   DrawCircleHelper(x + w - r - 1, y + h - r - 1, r, 4);
   DrawCircleHelper(x + r, y + h - r - 1, r, 8);
}

void DrawfillRoundRect(int x, int y, unsigned char w, unsigned char h, unsigned char r)
{
   // smarter version
   LCD_FillRect(x + r, y, w - 2 * r, h);

   // draw four corners
   DrawFillCircleHelper(x + w - r - 1, y + r, r, 1, h - 2 * r - 1);
   DrawFillCircleHelper(x + r, y + r, r, 2, h - 2 * r - 1);
}

void DrawCircleHelper(int x0, int y0, unsigned char r, unsigned char cornername)
{
   int f = 1 - r;
   int ddF_x = 1;
   int ddF_y = -2 * r;
   int x = 0;
   int y = r;
   // Type_color color=GetDrawColor();
   while (x < y)
   {
      if (f >= 0)
      {
         y--;
         ddF_y += 2;
         f += ddF_y;
      }

      x++;
      ddF_x += 2;
      f += ddF_x;

      if (cornername & 0x4)
      {
         LCD_DrawPoint(x0 + x, y0 + y, LCD_GREEN);
         LCD_DrawPoint(x0 + y, y0 + x, LCD_GREEN);
      }
      if (cornername & 0x2)
      {
         LCD_DrawPoint(x0 + x, y0 - y, LCD_GREEN);
         LCD_DrawPoint(x0 + y, y0 - x, LCD_GREEN);
      }
      if (cornername & 0x8)
      {
         LCD_DrawPoint(x0 - y, y0 + x, LCD_GREEN);
         LCD_DrawPoint(x0 - x, y0 + y, LCD_GREEN);
      }
      if (cornername & 0x1)
      {
         LCD_DrawPoint(x0 - y, y0 - x, LCD_GREEN);
         LCD_DrawPoint(x0 - x, y0 - y, LCD_GREEN);
      }
   }
}

void DrawFillCircleHelper(int x0, int y0, unsigned char r, unsigned char cornername, int delta)
{
   // used to do circles and roundrects!
   int f = 1 - r;
   int ddF_x = 1;
   int ddF_y = -2 * r;
   int x = 0;
   int y = r;
   // Type_color color=GetDrawColor();
   while (x < y)
   {
      if (f >= 0)
      {
         y--;
         ddF_y += 2;
         f += ddF_y;
      }

      x++;
      ddF_x += 2;
      f += ddF_x;

      if (cornername & 0x1)
      {
         LCD_DrawLine_V(x0 + x, y0 - y, 2 * y + 1 + delta);
         LCD_DrawLine_V(x0 + y, y0 - x, 2 * x + 1 + delta);
      }

      if (cornername & 0x2)
      {
         LCD_DrawLine_V(x0 - x, y0 - y, 2 * y + 1 + delta);
         LCD_DrawLine_V(x0 - y, y0 - x, 2 * x + 1 + delta);
      }
   }
}

void DrawFillEllipse(int x0, int y0, int rx, int ry)
{
   int x, y;
   int xchg, ychg;
   int err;
   int rxrx2;
   int ryry2;
   int stopx, stopy;

   rxrx2 = rx;
   rxrx2 *= rx;
   rxrx2 *= 2;

   ryry2 = ry;
   ryry2 *= ry;
   ryry2 *= 2;

   x = rx;
   y = 0;

   xchg = 1;
   xchg -= rx;
   xchg -= rx;
   xchg *= ry;
   xchg *= ry;

   ychg = rx;
   ychg *= rx;

   err = 0;

   stopx = ryry2;
   stopx *= rx;
   stopy = 0;

   while (stopx >= stopy)
   {

      LCD_DrawLine_V(x0 + x, y0 - y, y + 1);
      LCD_DrawLine_V(x0 - x, y0 - y, y + 1);
      LCD_DrawLine_V(x0 + x, y0, y + 1);
      LCD_DrawLine_V(x0 - x, y0, y + 1);
      // draw_filled_ellipse_section(u8g, x, y, x0, y0, option);
      y++;
      stopy += rxrx2;
      err += ychg;
      ychg += rxrx2;
      if (2 * err + xchg > 0)
      {
         x--;
         stopx -= ryry2;
         err += xchg;
         xchg += ryry2;
      }
   }

   x = 0;
   y = ry;

   xchg = ry;
   xchg *= ry;

   ychg = 1;
   ychg -= ry;
   ychg -= ry;
   ychg *= rx;
   ychg *= rx;

   err = 0;

   stopx = 0;

   stopy = rxrx2;
   stopy *= ry;

   while (stopx <= stopy)
   {
      LCD_DrawLine_V(x0 + x, y0 - y, y + 1);
      LCD_DrawLine_V(x0 - x, y0 - y, y + 1);
      LCD_DrawLine_V(x0 + x, y0, y + 1);
      LCD_DrawLine_V(x0 - x, y0, y + 1);
      // u8g_draw_filled_ellipse_section(u8g, x, y, x0, y0, option);
      x++;
      stopx += ryry2;
      err += xchg;
      xchg += ryry2;
      if (2 * err + ychg > 0)
      {
         y--;
         stopy -= rxrx2;
         err += ychg;
         ychg += rxrx2;
      }
   }
}

void DrawTriangle(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2)
{
   LCD_DrawLine(x0, y0, x1, y1);
   LCD_DrawLine(x1, y1, x2, y2);
   LCD_DrawLine(x2, y2, x0, y0);
}

void DrawFillTriangle(int x0, int y0, int x1, int y1, int x2, int y2)
{
   int a, b, y, last;
   int dx01, dy01, dx02, dy02, dx12, dy12, sa = 0, sb = 0;
   if (y0 > y1)
   {
      SWAP(y0, y1);
      SWAP(x0, x1);
   }
   if (y1 > y2)
   {
      SWAP(y2, y1);
      SWAP(x2, x1);
   }
   if (y0 > y1)
   {
      SWAP(y0, y1);
      SWAP(x0, x1);
   }

   if (y0 == y2)
   {
      a = b = x0;
      if (x1 < a)
      {
         a = x1;
      }
      else if (x1 > b)
      {
         b = x1;
      }
      if (x2 < a)
      {
         a = x2;
      }
      else if (x2 > b)
      {
         b = x2;
      }
      LCD_DrawLine_H(a, y0, b - a + 1);
      return;
   }
   dx01 = x1 - x0,
   dy01 = y1 - y0,
   dx02 = x2 - x0,
   dy02 = y2 - y0,
   dx12 = x2 - x1,
   dy12 = y2 - y1,
   sa = 0,
   sb = 0;
   if (y1 == y2)
   {
      last = y1; // Include y1 scanline
   }
   else
   {
      last = y1 - 1; // Skip it
   }

   for (y = y0; y <= last; y++)
   {
      a = x0 + sa / dy01;
      b = x0 + sb / dy02;
      sa += dx01;
      sb += dx02;

      if (a > b)
      {
         SWAP(a, b);
      }

      LCD_DrawLine_H(a, y, b - a + 1);
   }
   sa = dx12 * (y - y1);
   sb = dx02 * (y - y0);
   for (; y <= y2; y++)
   {
      a = x1 + sa / dy12;
      b = x0 + sb / dy02;
      sa += dx12;
      sb += dx02;
      if (a > b)
      {
         SWAP(a, b);
      }
      LCD_DrawLine_H(a, y, b - a + 1);
   }
}

void DrawArc(int x, int y, unsigned char r, int angle_start, int angle_end)
{
   float i = 0;
   TypeXY m, temp;
   temp = GetXY();
   SetRotateCenter(x, y);
   SetAnggleDir(0);
   if (angle_end > 360)
      angle_end = 360;
   SetAngle(0);
   m = GetRotateXY(x, y + r);
   MoveTo(m.x, m.y);
   for (i = angle_start; i < angle_end; i += 5)
   {
      SetAngle(i);
      m = GetRotateXY(x, y + r);
      LineTo(m.x, m.y);
   }
   MoveTo(temp.x, temp.y);
}

void SetRotateCenter(int x0, int y0)
{
   _RoateValue.center.x = x0;
   _RoateValue.center.y = y0;
}

void SetAnggleDir(int direction)
{
   _RoateValue.direct = direction;
}

void SetAngle(float angle)
{
   _RoateValue.angle = (float)RADIAN(angle);
}

static void Rotate(int x0, int y0, int *x, int *y, double angle, int direction)
{

   // double r=sqrt((*y-y0)*(*y-y0)+(*x-x0)*(*x-x0));
   int temp = (*y - y0) * (*y - y0) + (*x - x0) * (*x - x0);
   double r = mySqrt(temp);
   double a0 = atan2(*x - x0, *y - y0);
   if (direction)
   {
      *x = x0 + r * cos(a0 + angle);
      *y = y0 + r * sin(a0 + angle);
   }
   else
   {
      *x = x0 + r * cos(a0 - angle);
      *y = y0 + r * sin(a0 - angle);
   }
}

// 点x，y绕x0，y0旋转angle弧度
float mySqrt(float x)
{
   float a = x;
   unsigned int i = *(unsigned int *)&x;
   i = (i + 0x3f76cf62) >> 1;
   x = *(float *)&i;
   x = (float)(x + a / x) * 0.5;
   return x;
}

TypeXY GetRotateXY(int x, int y)
{
   TypeXY temp;
   int m = x, n = y;
   //	if(_RoateValue.angle!=0)
   Rotate(_RoateValue.center.x, _RoateValue.center.y, &m, &n, _RoateValue.angle, _RoateValue.direct);
   temp.x = m;
   temp.y = n;
   return temp;
}

// 星空动画
void ShowStars(void)
{
   int i; // j;
   int count = 0;
   int fps = 60;

   typedef struct START
   {
      short x;
      short y;
      short speed;
      unsigned char speedcount;
      unsigned char isexist;
   } Star;

   Star star[240] = {0};
   srand(2);

   for (i = 0; i < 240; i++)
   {
      if (star[i].isexist == 0)
      {
         star[i].x = rand() % 239;
         star[i].y = rand() % 255;
         star[i].speedcount = 0;
         star[i].speed = rand() % 8 + 1;
         star[i].isexist = 1;
      }
   }
   while (1)
   {
      if (FrameRateUpdateScreen(fps) == 1)
      {
         count++;
         if (count >= fps * 10) // 10秒钟
            return;
      }

      for (i = 0; i < 239; i++)
      {
         if (star[i].isexist == 0)
         {

            star[i].x = 0;
            star[i].y = rand() % 255;
            star[i].speed = rand() % 6 + 1;
            star[i].speedcount = 0;
            star[i].isexist = 1;
         }
         else
         {
            star[i].speedcount++;
            if (star[i].x >= 234)
            {
               star[i].isexist = 0;
            }
            LCD_DrawLine(star[i].x, star[i].y, star[i].x, star[i].y);
            if (star[i].speedcount == star[i].speed)
            {
               star[i].speedcount = 0;
               star[i].x += 1;
            }
            LCD_DrawLine(star[i].x, star[i].y, star[i].x + (6 / star[i].speed) - 1, star[i].y);
         }
      }
   }
}

unsigned char FrameRateUpdateScreen(int value)
{

   if (OledTimeMs == 0)
   {
      LCD_Clear();
      OledTimeMs = 1000 / value;
      return 1;
   }
   return 0;
}

void ShowWatch(void)
{
   int i, j, z;
   //	int count=0;
   LCD_DrawCircle(120, 140, 90);
   for (i = 0; i < 12; i++)
   {
      Clock_Needle(i, 40);
      for (j = 0; j < 60; j++)
      {
         if (j == i)
         {
            Clock_Needle(i, 40);
         }
         for (z = 0; z < 60; z++)
         {
            Clock_Sec(z, 80);
            if (z == j)
            {
               Clock_Needle(j, 60);
               Clock_Needle(i, 40);
            }
            if (z == i)
            {
               Clock_Needle(i, 40);
            }
         }
      }
   }
}

void SetRotateValue(int x, int y, float angle, int direct)
{
   SetRotateCenter(x, y);
   SetAnggleDir(direct);
   SetAngle(angle);
}

void ShowTest(void)
{
   int x0 = 120, y0 = 140;
   unsigned char i = 0, j;
   int n = 1, r = 80, v = 1, count = 0;
   int x[30], y[30];

   while (1)
   {
      LCD_Clear();
      for (i = 0; i < n; i++)
      {
         x[i] = r * cos(2 * 3.1415926 * i / n) + x0;
         y[i] = r * sin(2 * 3.1415926 * i / n) + y0;
      }
      for (i = 0; i <= n - 2; i++)
      {
         for (j = i + 1; j <= n - 1; j++)
         {
            LCD_DrawLine(x[i], y[i], x[j], y[j]);
            HAL_Delay(2);
         }
      }
      n += v;
      if (n == 10 || n == 0)
         v = -v;
      HAL_Delay(100);
      if (++count == 20)
      {
         count = 0;
         return;
      }
      HAL_Delay(100);
   }
}

void ShowSnow(void)
{
   int a[66], i, num = 0;
   struct Snow
   {
      short x;
      short y;
      short speed;
   } snow[100];

   srand(1);
   for (i = 0; i < 66; i++)
      a[i] = (i - 2) * 10;
   LCD_Clear();
   while (1)
   {
      // FrameRateUpdateScreen(60);
      if (num != 100)
      {

         snow[num].speed = 1 + rand() % 4;
         i = rand() % 66;
         snow[num].x = a[i];
         snow[num].y = 0;
         num++;
      }
      for (i = 0; i < num; i++)
      {
         snow[i].y += snow[i].speed;
         DrawPixel(snow[i].x, snow[i].y + 1);
         DrawPixel(snow[i].x + 1, snow[i].y);
         DrawPixel(snow[i].x, snow[i].y);
         DrawPixel(snow[i].x - 1, snow[i].y);
         DrawPixel(snow[i].x, snow[i].y - 1);
         if (snow[i].y > 63)
         {
            snow[i].y = 0;
         }
      }
      HAL_Delay(30);
      LCD_Clear();
   }
}

void DrawPixel(int x, int y)
{
   LCD_DrawPoint(x, y, LIGHT_GREEN);
}

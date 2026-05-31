#include "lcd_model.h"
#include "math.h"

/*****************************************************************************************
*	函 数 名: LCD_Test
*	入口参数: 无
*	返 回 值: 无
*	函数功能: 文本显示功能函数
*	说    明: 无
******************************************************************************************/
void LCD_Test(void)
{
    // 清屏
    LCD_Clear();

    // 设置画笔颜色为浅绿色
    LCD_SetColor(LIGHT_GREEN);

    // 设置ASCII字体大小为12
    LCD_SetAsciiFont(&ASCII_Font12);
    // 显示文本 "Makerbase"，并在每次显示后等待一段时间
    LCD_DisplayString(20, 25, "Makerbase");
    HAL_Delay(200);

    // 设置ASCII字体大小为16
    LCD_SetAsciiFont(&ASCII_Font16);
    // 显示文本 "Makerbase"，并在每次显示后等待一段时间
    LCD_DisplayString(20, 42, "Makerbase");
    HAL_Delay(200);

    // 设置ASCII字体大小为20
    LCD_SetAsciiFont(&ASCII_Font20);
    // 显示文本 "Makerbase"，并在每次显示后等待一段时间
    LCD_DisplayString(20, 65, "Makerbase");
    HAL_Delay(200);

    // 设置ASCII字体大小为24
    LCD_SetAsciiFont(&ASCII_Font24);
    // 显示文本 "Makerbase"，并在每次显示后等待一段时间
    LCD_DisplayString(20, 95, "Makerbase");
    HAL_Delay(200);

    // 设置ASCII字体大小为32
    LCD_SetAsciiFont(&ASCII_Font32);
    // 显示文本 "Makerbase"，并在每次显示后等待一段时间
    LCD_DisplayString(20, 130, "Makerbase");
    HAL_Delay(200);

    HAL_Delay(500);
}
/*****************************************************************************************
*	函 数 名: LCD_Line
*	入口参数: 无
*	返 回 值: 无
*	函数功能: 画线显示功能函数
*	说    明: 无
******************************************************************************************/
void LCD_Line(void)
{
    int i;

    // 清屏
    LCD_Clear();

    // 设置画笔颜色为浅蓝色
    LCD_SetColor(LIGHT_BLUE);

    // 绘制从左上到左下的线
    for (i = 0; i <= 48; i++)
    {
        LCD_DrawLine(12, 14, i * 5, 280);

        // 等待一段时间，形成动态效果
        HAL_Delay(10);
    }

    // 设置画笔颜色为浅绿色
    LCD_SetColor(LIGHT_GREEN);

    // 绘制从右上到右下的线
    for (i = 0; i <= 48; i++)
    {
        LCD_DrawLine(228, 14, 240 - i * 5, 280);

        // 等待一段时间，形成动态效果
        HAL_Delay(10);
    }

    // 设置画笔颜色为浅红色
    LCD_SetColor(LIGHT_RED);

    // 绘制从中上到右下的线
    for (i = 0; i <= 48; i++)
    {
        LCD_DrawLine(120, 139, 240 - i * 5, 280);

        // 等待一段时间，形成动态效果
        HAL_Delay(10);
    }

    HAL_Delay(500);
}
/*****************************************************************************************
*	函 数 名: LCD_Rectangle
*	入口参数: 无
*	返 回 值: 无
*	函数功能: 矩形动态显示功能函数
*	说    明: 无
******************************************************************************************/
void LCD_Rectangle(void)
{
    int i;

    // 清屏
    LCD_Clear();

    // 设置画笔颜色
    LCD_SetColor(LIGHT_GREEN);

    // 动态绘制矩形的轮廓
    for (i = 2; i < 20; i++)
    {
        LCD_DrawRect(i * 6, i * 7, 240 - 12 * i, 280 - 14 * i);

        // 等待一段时间，形成动态效果
        HAL_Delay(30);
    }

    HAL_Delay(200);

    // 动态填充矩形
    for (i = 19; i >= 2; i--)
    {
        LCD_FillRect(i * 6, i * 7, 240 - 12 * i, 280 - 14 * i);

        // 等待一段时间，形成动态效果
        HAL_Delay(30);
    }

    HAL_Delay(200);

    // 用黑色画出边框效果
    LCD_SetColor(LCD_BLACK);
    for (i = 2; i < 20; i++)
    {
        LCD_DrawRect(i * 6, i * 7, 240 - 12 * i, 280 - 14 * i);
        LCD_DrawRect(i * 6 + 1, i * 7 + 1, 240 - 12 * i - 2, 280 - 14 * i - 2);
        LCD_DrawRect(i * 6 + 2, i * 7 + 2, 240 - 12 * i - 4, 280 - 14 * i - 4);
        LCD_DrawRect(i * 6 + 3, i * 7 + 3, 240 - 12 * i - 6, 280 - 14 * i - 6);
        LCD_DrawRect(i * 6 + 4, i * 7 + 4, 240 - 12 * i - 8, 280 - 14 * i - 8);
        LCD_DrawRect(i * 6 + 4, i * 7 + 5, 240 - 12 * i - 8, 280 - 14 * i - 10);

        // 等待一段时间，形成动态效果
        HAL_Delay(30);
    }

    HAL_Delay(200);

    // 用黑色填充矩形，形成黑色边框
    LCD_SetColor(LCD_BLACK);
    for (i = 19; i >= 2; i--)
    {
        LCD_FillRect(i * 6, i * 7, 240 - 12 * i, 280 - 14 * i);

        // 等待一段时间，形成动态效果
        HAL_Delay(30);
    }

    HAL_Delay(500);
}

/*****************************************************************************************
*	函 数 名: LCD_RouRectangle
*	入口参数: 无
*	返 回 值: 无
*	函数功能: 圆角矩形动态显示功能函数
*	说    明: 无
******************************************************************************************/
void LCD_RouRectangle(void)
{
    int i;

    // 清屏
    LCD_Clear();

    // 设置画笔颜色
    LCD_SetColor(LIGHT_GREEN);

    // 动态绘制圆角矩形的轮廓
    for (i = 2; i < 19; i++)
    {
        DrawRoundRect(i * 6, i * 7, 240 - 12 * i, 280 - 14 * i, 12);

        // 等待一段时间，形成动态效果
        HAL_Delay(3);
    }

    HAL_Delay(200);

    // 动态填充圆角矩形
    for (i = 18; i >= 2; i--)
    {
        LCD_SetColor(LIGHT_GREEN);
        DrawfillRoundRect(i * 6, i * 7, 240 - 12 * i, 280 - 14 * i, 12);

        // 等待一段时间，形成动态效果
        HAL_Delay(3);
    }

    HAL_Delay(500);
}
/*****************************************************************************************
*	函 数 名: LCD_Ellipse
*	入口参数: 无
*	返 回 值: 无
*	函数功能: 椭圆显示功能函数
*	说    明: 无
******************************************************************************************/
void LCD_Ellipse(void)
{
    // 清屏
    LCD_Clear();

    // 设置画笔颜色
    LCD_SetColor(LIGHT_GREEN);

    // 绘制椭圆的轮廓
    LCD_DrawEllipse(120, 140, 60, 90);

    HAL_Delay(200);

    // 填充椭圆
    DrawFillEllipse(120, 140, 60, 90);

    HAL_Delay(200);

    // 清屏
    LCD_Clear();

    // 设置画笔颜色为黄色
    LCD_SetColor(LIGHT_YELLOW);

    // 绘制另一个椭圆的轮廓
    LCD_DrawEllipse(120, 140, 80, 50);

    HAL_Delay(200);

    // 填充另一个椭圆
    DrawFillEllipse(120, 140, 80, 50);

    HAL_Delay(500);
}
/*****************************************************************************************
*	函 数 名: LCD_Circle
*	入口参数: 无
*	返 回 值: 无
*	函数功能: 圆显示功能函数
*	说    明: 无
******************************************************************************************/
void LCD_Circle(void)
{
    // 清屏
    LCD_Clear();

    // 设置画笔颜色
    LCD_SetColor(LIGHT_GREEN);

    // 绘制圆的轮廓
    LCD_DrawCircle(120, 140, 100);

    HAL_Delay(200);

    // 填充圆
    LCD_FillCircle(120, 140, 100);

    HAL_Delay(500);
}

/*****************************************************************************************
*	函 数 名: LCD_Triangle
*	入口参数: 无
*	返 回 值: 无
*	函数功能: 三角形显示功能函数
*	说    明: 无
******************************************************************************************/
void LCD_Triangle(void)
{
    // 清屏
    LCD_Clear();

    // 设置画笔颜色
    LCD_SetColor(LIGHT_GREEN);

    // 绘制三角形的轮廓
    DrawTriangle(12, 14, 228, 14, 228, 255);

    HAL_Delay(200);

    // 填充三角形
    DrawFillTriangle(12, 14, 228, 14, 228, 255);

    HAL_Delay(200);

    // 设置画笔颜色为黄色
    LCD_SetColor(LIGHT_YELLOW);

    // 绘制另一个三角形的轮廓
    DrawTriangle(12, 14, 12, 255, 228, 255);

    HAL_Delay(200);

    // 填充另一个三角形
    DrawFillTriangle(12, 14, 12, 255, 228, 255);

    HAL_Delay(500);
}

/*****************************************************************************************
*	函 数 名: LCD_Picture
*	入口参数: 无
*	返 回 值: 无
*	函数功能: 图片显示功能函数
*	说    明: 无
******************************************************************************************/


//void LCD_Picture(void)
//{
//    // 清屏
//    LCD_Clear();

//    // 设置画笔颜色
//    LCD_SetColor(LIGHT_GREEN);

//    // 绘制第一张图片
//    LCD_DrawImage(0, 20, 239, 239, Image_2_239x239);

//    HAL_Delay(1000);

//    // 绘制第二张图片
//    LCD_DrawImage(0, 20, 239, 239, Image_1_239x239);

//    HAL_Delay(1000);
//}

/*****************************************************************************************
*	函 数 名: LCD_Arc
*	入口参数: 无
*	返 回 值: 无
*	函数功能: 圆弧动态显示功能函数
*	说    明: 无
******************************************************************************************/
void LCD_Arc(void)
{
    int i;

    // 清屏
    LCD_Clear();

    // 设置画笔颜色
    LCD_SetColor(LIGHT_GREEN);

    // 画弧
    for (i = 0; i < 360; i++)
    {
        TypeXY temp;

        // 设置当前角度
        SetAngle(i);
        SetRotateCenter(120, 140);

        // 获取旋转后的坐标
        temp = GetRotateXY(120, 140 + 90);

        // 在屏幕上绘制动态点
        LCD_DrawPoint(temp.x, temp.y, LCD_GREEN);

        // 等待一段时间，形成动态效果
        HAL_Delay(5);
    }

    // 绘制圆
    LCD_DrawCircle(120, 140, 90);

    HAL_Delay(200);

    // 中心圆放大
    for (i = 1; i < 30; i++)
    {
        // 在圆心处绘制放大的圆
        LCD_FillCircle(120, 140, i);

        // 等待一段时间，形成放大效果
        HAL_Delay(30);
    }

    HAL_Delay(200);

    // 绕点旋转
    for (i = 0; i < 360; i++)
    {
        TypeXY temp;

        // 设置当前角度
        SetAngle(i);
        SetRotateCenter(120, 140);

        // 获取绕点旋转后的坐标
        temp = GetRotateXY(120, 230);

        // 在屏幕上绘制绕点旋转的圆
        LCD_SetColor(LCD_GREEN);
        LCD_FillCircle(temp.x, temp.y, 10);

        // 等待一段时间，形成动态效果
        HAL_Delay(50);

        // 擦除之前的圆
        LCD_SetColor(LCD_BLACK);
        LCD_FillCircle(temp.x, temp.y, 10);
    }

    HAL_Delay(500);
}


/*****************************************************************************************
*	函 数 名: LCD_Polygon
*	入口参数: 无
*	返 回 值: 无
*	函数功能: 多边形绘画显示功能函数
*	说    明: 无
******************************************************************************************/
void LCD_Polygon(void)
{
    int x0 = 120, y0 = 140;
    unsigned char i = 0, j;
    int n = 1, r = 80, v = 1, count = 0;
    int x[30], y[30];

    // 清屏
    LCD_Clear();

    // 设置画笔颜色
    LCD_SetColor(LIGHT_GREEN);

    while (1)
    {
        // 清屏
        LCD_Clear();

        // 计算多边形顶点坐标
        for (i = 0; i < n; i++)
        {
            x[i] = r * cos(2 * 3.1415926 * i / n) + x0;
            y[i] = r * sin(2 * 3.1415926 * i / n) + y0;
        }

        // 绘制多边形的边
        for (i = 0; i <= n - 2; i++)
        {
            for (j = i + 1; j <= n - 1; j++)
            {
                LCD_DrawLine(x[i], y[i], x[j], y[j]);
                HAL_Delay(2);
            }
        }

        // 调整多边形边数
        n += v;
        if (n == 15 || n == 0)
            v = -v;

        HAL_Delay(50);

        // 控制绘制次数
        if (++count == 20)
        {
            count = 0;
            return;
        }

        HAL_Delay(50);
    }
}
/*****************************************************************************************
*	函 数 名: LCD_Clock
*	入口参数: 无
*	返 回 值: 无
*	函数功能: 时钟显示功能函数
*	说    明: 无
******************************************************************************************/
void LCD_Clock(void)
{
    int i, j, z;

    // 清屏
    LCD_Clear();

    // 设置画笔颜色
    LCD_SetColor(LIGHT_GREEN);

    // 绘制时钟外圈
    LCD_DrawCircle(120, 140, 90);

    // 遍历每个小时
    for (i = 0; i < 1; i++)
    {
        // 绘制当前小时的时钟针
        Clock_Needle(i, 40);

        // 遍历每一分钟
        for (j = 0; j < 3; j++)
        {
            // 如果分钟等于当前小时，绘制时钟针
            if (j == i)
            {
                Clock_Needle(i, 40);
            }

            // 遍历每一秒
            for (z = 0; z < 60; z++)
            {
                // 绘制秒针的轨迹
                Clock_Sec(z, 80);

                // 如果秒钟等于分钟，绘制分钟针和时钟针
                if (z == j)
                {
                    Clock_Needle(j, 60);
                    Clock_Needle(i, 40);
                }

                // 如果秒钟等于当前小时，绘制时钟针
                if (z == i)
                {
                    Clock_Needle(i, 40);
                }
            }
        }
    }
}
// 时钟、分钟移动轨迹
// 函数用于控制时钟和分钟针的移动轨迹

void Clock_Needle(int t, int l)
{
    TypeXY secpoint;

    // 擦除之前的时钟或分钟针
    LCD_SetColor(LCD_BLACK);

    // 如果时间不为零
    if (t != 0)
    {
        // 设置时钟或分钟针的旋转参数
        SetRotateValue(120, 140, (t - 1) * 6, 1);

        // 获取经旋转后的时钟或分钟针末端坐标
        secpoint = GetRotateXY(120 - l, 140);

        // 绘制并擦除时钟或分钟针
        LCD_DrawLine(120, 140, secpoint.x, secpoint.y);
    }
    else
    {
        // 当时间为零时，绘制整个时钟或分钟针的轨迹
        SetRotateValue(120, 140, 0, 1);
        secpoint = GetRotateXY(120 - l, 140);
        LCD_DrawLine(120, 140, secpoint.x, secpoint.y);

        // 绘制整个时钟或分钟针的轨迹
        SetRotateValue(120, 140, 59 * 6, 1);
        secpoint = GetRotateXY(120 - l, 140);
        LCD_DrawLine(120, 140, secpoint.x, secpoint.y);
    }

    // 设置时钟或分钟针的旋转参数
    SetRotateValue(120, 140, t * 6, 1);

    // 获取经旋转后的时钟或分钟针末端坐标
    secpoint = GetRotateXY(120 - l, 140);

    // 绘制时钟或分钟针
    LCD_SetColor(LCD_GREEN);
    LCD_DrawLine(120, 140, secpoint.x, secpoint.y);
}


// 秒针移动轨迹和表盘
// 函数用于控制秒针的移动轨迹和绘制表盘

void Clock_Sec(int t, int l)
{
    unsigned char i = 0;
    TypeXY secpoint, tmp1, tmp2;

    // 设置秒针的旋转参数
    SetRotateValue(120, 140, t * 6, 1);

    // 获取经旋转后的秒针末端坐标
    secpoint = GetRotateXY(120 - l, 140);

    // 绘制并擦除秒针
    LCD_SetColor(LCD_GREEN);
    LCD_DrawLine(120, 140, secpoint.x, secpoint.y);
    HAL_Delay(50);
    LCD_SetColor(LCD_BLACK);
    LCD_DrawLine(120, 140, secpoint.x, secpoint.y);

    // 绘制时钟表盘
    LCD_SetColor(LCD_GREEN);
    for (i = 0; i < 12; i++)
    {
        // 设置每个小时刻度的旋转参数
        SetRotateValue(120, 140, i * 30, 1);

        // 获取每个小时刻度外侧和内侧点的旋转后坐标
        tmp1 = GetRotateXY(120 - 90, 140);
        tmp2 = GetRotateXY(120 - 80, 140);

        // 绘制连接外侧和内侧点的线段，形成小时刻度
        LCD_DrawLine(tmp1.x, tmp1.y, tmp2.x, tmp2.y);
    }
}


/*
 * 光耦隔离电路测试集成示例
 * 
 * 本文件展示如何在main.c中集成光耦隔离电路测试模块
 * 解决SW1+GND_WB施加DC24V后PB9无法置低电平的问题
 */

#include "main.h"
#include "opto_isolator_test_main.h"
#include "usart.h"

// 示例：如何在main.c中集成光耦隔离电路测试模块

int main(void)
{
    // 系统初始化
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART3_UART_Init();
    
    // 初始化光耦隔离电路测试模块
    OptoTestMain_Init();
    
    // 主循环
    while (1)
    {
        // 运行光耦隔离电路测试主循环
        OptoTestMain_Process();
        
        // 其他业务逻辑...
        
        HAL_Delay(1);
    }
}

// 串口接收中断处理函数（如果使用中断接收）
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3)
    {
        static uint8_t rx_char;
        
        // 获取接收到的字符
        if (HAL_UART_Receive(huart, &rx_char, 1, 0) == HAL_OK)
        {
            // 传递给光耦测试模块处理
            OptoTestMain_ProcessChar(rx_char);
        }
        
        // 重新启动接收
        HAL_UART_Receive_IT(huart, &rx_char, 1);
    }
}

/*
 * 使用步骤：
 * 
 * 1. 编译并烧录程序
 * 2. 连接串口工具（115200波特率）
 * 3. 发送命令进行测试：
 *    - 发送 "autotest" 进行自动化测试
 *    - 发送 "help" 查看所有命令
 *    - 发送 "check" 进行手动检测
 * 
 * 4. 按照提示操作：
 *    - 在测试期间对SW1和GND_WB施加/移除DC24V
 *    - 观察K1_EN信号变化
 *    - 查看诊断结果和建议
 * 
 * 5. 根据诊断结果修复硬件问题：
 *    - 检查限流电阻值（推荐2.2kΩ）
 *    - 检查24V电源电压
 *    - 检查光耦器件CTR参数
 *    - 添加滤波电容抑制EMI
 */ 
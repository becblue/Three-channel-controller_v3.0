#include "opto_isolator_test.h"
#include "gpio_control.h"
#include "usart.h"
#include "main.h"
#include <string.h>

/************************************************************
 * 光耦隔离电路测试主程序
 * 提供完整的测试流程和用户交互界面
 * 可以通过串口命令控制测试过程
 ************************************************************/

// 测试命令定义
#define CMD_HELP        "help"
#define CMD_INIT        "init"
#define CMD_START       "start"
#define CMD_STOP        "stop"
#define CMD_STATUS      "status"
#define CMD_CHECK       "check"
#define CMD_AUTOTEST    "autotest"

// 串口接收缓冲区
static char rx_buffer[64];
static uint8_t rx_index = 0;

/**
  * @brief  光耦隔离电路测试主程序初始化
  * @retval 无
  */
void OptoTestMain_Init(void)
{
    // 初始化串口接收缓冲区
    memset(rx_buffer, 0, sizeof(rx_buffer));
    rx_index = 0;
    
    // 输出欢迎信息
    DEBUG_Printf("\r\n");
    DEBUG_Printf("****************************************************************\r\n");
    DEBUG_Printf("*           光耦隔离电路测试诊断系统 V1.0                       *\r\n");
    DEBUG_Printf("*                                                              *\r\n");
    DEBUG_Printf("* 功能：诊断K1_EN、K2_EN、K3_EN光耦隔离电路工作状态              *\r\n");
    DEBUG_Printf("* 用途：解决SW1+GND_WB施加DC24V后PB9无法置低电平的问题           *\r\n");
    DEBUG_Printf("*                                                              *\r\n");
    DEBUG_Printf("* 支持命令：                                                    *\r\n");
    DEBUG_Printf("*   help     - 显示帮助信息                                     *\r\n");
    DEBUG_Printf("*   init     - 初始化测试模块                                   *\r\n");
    DEBUG_Printf("*   start <秒> - 开始测试（指定时长）                            *\r\n");
    DEBUG_Printf("*   stop     - 停止测试                                         *\r\n");
    DEBUG_Printf("*   status   - 显示实时状态                                     *\r\n");
    DEBUG_Printf("*   check    - 手动检测                                         *\r\n");
    DEBUG_Printf("*   autotest - 自动化测试流程                                   *\r\n");
    DEBUG_Printf("*                                                              *\r\n");
    DEBUG_Printf("****************************************************************\r\n");
    DEBUG_Printf("请输入命令（输入help查看详细帮助）：\r\n");
}

/**
  * @brief  处理串口接收到的字符
  * @param  ch: 接收到的字符
  * @retval 无
  */
void OptoTestMain_ProcessChar(char ch)
{
    if(ch == '\r' || ch == '\n') {
        // 回车或换行，处理命令
        if(rx_index > 0) {
            rx_buffer[rx_index] = '\0';
            OptoTestMain_ProcessCommand(rx_buffer);
            rx_index = 0;
            memset(rx_buffer, 0, sizeof(rx_buffer));
        }
        DEBUG_Printf("> ");
    } else if(ch == '\b' || ch == 127) {
        // 退格键
        if(rx_index > 0) {
            rx_index--;
            rx_buffer[rx_index] = '\0';
            DEBUG_Printf("\b \b");
        }
    } else if(ch >= ' ' && ch <= '~') {
        // 可显示字符
        if(rx_index < sizeof(rx_buffer) - 1) {
            rx_buffer[rx_index++] = ch;
            DEBUG_Printf("%c", ch);
        }
    }
}

/**
  * @brief  处理用户输入的命令
  * @param  cmd: 命令字符串
  * @retval 无
  */
void OptoTestMain_ProcessCommand(char* cmd)
{
    DEBUG_Printf("\r\n");
    
    if(strcmp(cmd, CMD_HELP) == 0) {
        OptoTestMain_ShowHelp();
    }
    else if(strcmp(cmd, CMD_INIT) == 0) {
        OptoTest_Init();
    }
    else if(strncmp(cmd, CMD_START, 5) == 0) {
        // 解析时间参数
        char* param = strchr(cmd, ' ');
        if(param != NULL) {
            int duration = atoi(param + 1);
            if(duration > 0 && duration <= 3600) {
                OptoTest_Start(duration);
            } else {
                DEBUG_Printf("[错误] 测试时长必须在1-3600秒范围内\r\n");
            }
        } else {
            DEBUG_Printf("[错误] 请指定测试时长，例如：start 60\r\n");
        }
    }
    else if(strcmp(cmd, CMD_STOP) == 0) {
        OptoTest_Stop();
    }
    else if(strcmp(cmd, CMD_STATUS) == 0) {
        OptoTest_PrintRealTimeStatus();
    }
    else if(strcmp(cmd, CMD_CHECK) == 0) {
        OptoTest_ManualCheck();
    }
    else if(strcmp(cmd, CMD_AUTOTEST) == 0) {
        OptoTestMain_AutoTest();
    }
    else {
        DEBUG_Printf("[错误] 未知命令：%s\r\n", cmd);
        DEBUG_Printf("输入 help 查看可用命令\r\n");
    }
}

/**
  * @brief  显示详细帮助信息
  * @retval 无
  */
void OptoTestMain_ShowHelp(void)
{
    DEBUG_Printf("=== 光耦隔离电路测试系统帮助 ===\r\n");
    DEBUG_Printf("\r\n");
    DEBUG_Printf("问题描述：\r\n");
    DEBUG_Printf("  当SW1和GND_WB加载DC24V后，PB9没有置为低电平\r\n");
    DEBUG_Printf("  系统期望K1_EN=0时开启第一通道，但光耦隔离可能有问题\r\n");
    DEBUG_Printf("\r\n");
    DEBUG_Printf("命令详解：\r\n");
    DEBUG_Printf("  init          - 初始化测试模块，重置所有统计数据\r\n");
    DEBUG_Printf("  start <秒>    - 开始连续测试，监控信号变化\r\n");
    DEBUG_Printf("                  例如：start 60 （测试60秒）\r\n");
    DEBUG_Printf("  stop          - 停止当前测试并输出结果\r\n");
    DEBUG_Printf("  status        - 显示当前所有信号状态\r\n");
    DEBUG_Printf("  check         - 手动检测信号稳定性（连续读取10次）\r\n");
    DEBUG_Printf("  autotest      - 自动化测试流程（推荐使用）\r\n");
    DEBUG_Printf("\r\n");
    DEBUG_Printf("测试流程建议：\r\n");
    DEBUG_Printf("  1. 运行 init 初始化\r\n");
    DEBUG_Printf("  2. 运行 check 查看当前状态\r\n");
    DEBUG_Printf("  3. 运行 start 30 开始30秒测试\r\n");
    DEBUG_Printf("  4. 在测试期间对SW1+GND_WB施加/移除DC24V\r\n");
    DEBUG_Printf("  5. 观察K1_EN信号变化和诊断结果\r\n");
    DEBUG_Printf("\r\n");
    DEBUG_Printf("可能的故障原因：\r\n");
    DEBUG_Printf("  ? 光耦LED电流不足（检查限流电阻）\r\n");
    DEBUG_Printf("  ? 24V电源问题（电压/纹波/接地）\r\n");
    DEBUG_Printf("  ? 光耦CTR过低（器件老化或参数不当）\r\n");
    DEBUG_Printf("  ? EMI干扰（长线缆/无屏蔽/无滤波）\r\n");
    DEBUG_Printf("  ? 硬件连接错误（检查原理图）\r\n");
}

/**
  * @brief  自动化测试流程
  * @retval 无
  */
void OptoTestMain_AutoTest(void)
{
    DEBUG_Printf("=== 开始自动化光耦隔离电路测试 ===\r\n");
    DEBUG_Printf("\r\n");
    
    // 第一步：初始化
    DEBUG_Printf("第1步：初始化测试模块...\r\n");
    OptoTest_Init();
    HAL_Delay(1000);
    
    // 第二步：初始状态检测
    DEBUG_Printf("第2步：检测初始状态...\r\n");
    OptoTest_ManualCheck();
    HAL_Delay(2000);
    
    // 第三步：用户操作提示
    DEBUG_Printf("第3步：请按以下步骤操作：\r\n");
    DEBUG_Printf("  准备工作：确保SW1和GND_WB之间未施加电压\r\n");
    DEBUG_Printf("  现在开始60秒连续监测...\r\n");
    DEBUG_Printf("  \r\n");
    DEBUG_Printf("  请在测试期间执行以下操作：\r\n");
    DEBUG_Printf("  0-10秒：  保持无电压状态，观察基准状态\r\n");
    DEBUG_Printf("  10-30秒： 施加DC24V到SW1和GND_WB\r\n");
    DEBUG_Printf("  30-50秒： 移除DC24V电压\r\n");
    DEBUG_Printf("  50-60秒： 再次施加DC24V\r\n");
    DEBUG_Printf("  \r\n");
    DEBUG_Printf("  系统将自动记录所有状态变化...\r\n");
    
    // 第四步：开始测试
    DEBUG_Printf("第4步：开始60秒自动测试...\r\n");
    OptoTest_Start(60);
    
    // 在测试期间提供实时指导
    uint32_t test_start = HAL_GetTick();
    uint32_t last_prompt = 0;
    
    while(OptoTest_IsRunning()) {
        uint32_t elapsed = (HAL_GetTick() - test_start) / 1000;
        
        // 根据时间给出操作提示
        if(elapsed == 10 && (HAL_GetTick() - last_prompt) > 1000) {
            DEBUG_Printf("\r\n[操作提示] 现在请施加DC24V到SW1和GND_WB\r\n");
            last_prompt = HAL_GetTick();
        }
        else if(elapsed == 30 && (HAL_GetTick() - last_prompt) > 1000) {
            DEBUG_Printf("\r\n[操作提示] 现在请移除DC24V电压\r\n");
            last_prompt = HAL_GetTick();
        }
        else if(elapsed == 50 && (HAL_GetTick() - last_prompt) > 1000) {
            DEBUG_Printf("\r\n[操作提示] 现在请再次施加DC24V\r\n");
            last_prompt = HAL_GetTick();
        }
        
        // 运行测试逻辑
        OptoTest_Process();
        HAL_Delay(10);
    }
    
    // 第五步：最终检测
    DEBUG_Printf("\r\n第5步：最终状态检测...\r\n");
    HAL_Delay(1000);
    OptoTest_ManualCheck();
    
    // 第六步：输出结论
    DEBUG_Printf("\r\n=== 自动化测试完成 ===\r\n");
    DEBUG_Printf("根据以上测试结果，您可以：\r\n");
    DEBUG_Printf("1. 查看诊断建议，确定故障原因\r\n");
    DEBUG_Printf("2. 根据建议修改硬件电路\r\n");
    DEBUG_Printf("3. 重新运行测试验证修复效果\r\n");
    DEBUG_Printf("4. 如有疑问，请联系技术支持\r\n");
}

/**
  * @brief  光耦隔离电路测试主循环
  * @retval 无
  */
void OptoTestMain_Process(void)
{
    // 运行测试逻辑
    OptoTest_Process();
    
    // 检查是否有新的串口数据
    // 注意：这里需要根据您的串口实现方式调整
    // 如果使用中断方式接收，可以在中断中调用OptoTestMain_ProcessChar
    
    // 示例：轮询方式（需要根据实际情况调整）
    /*
    if(HAL_UART_Receive(&huart3, (uint8_t*)&ch, 1, 0) == HAL_OK) {
        OptoTestMain_ProcessChar(ch);
    }
    */
}

/**
  * @brief  检查光耦隔离电路的理论设计
  * @retval 无
  */
void OptoTestMain_CheckDesign(void)
{
    DEBUG_Printf("=== 光耦隔离电路设计检查 ===\r\n");
    DEBUG_Printf("\r\n");
    DEBUG_Printf("基于EL817光耦的典型设计参数：\r\n");
    DEBUG_Printf("  输入电压：DC24V\r\n");
    DEBUG_Printf("  LED正向电压：约1.2V\r\n");
    DEBUG_Printf("  LED正向电流：推荐10mA\r\n");
    DEBUG_Printf("  限流电阻：R1 = (24V - 1.2V) / 10mA = 2.28kΩ\r\n");
    DEBUG_Printf("  实际选择：2.2kΩ（标准值）\r\n");
    DEBUG_Printf("\r\n");
    DEBUG_Printf("  CTR（电流传输比）：50%~600%（典型值100%）\r\n");
    DEBUG_Printf("  集电极电流：10mA × 100% = 10mA\r\n");
    DEBUG_Printf("  输出电压：VCC - Ic × Rc = 3.3V - 10mA × 下拉电阻\r\n");
    DEBUG_Printf("  下拉电阻：建议10kΩ\r\n");
    DEBUG_Printf("\r\n");
    DEBUG_Printf("常见问题分析：\r\n");
    DEBUG_Printf("  1. 如果R1过大 → LED电流不足 → 光耦不导通\r\n");
    DEBUG_Printf("  2. 如果24V电源电压过低 → LED电流不足\r\n");
    DEBUG_Printf("  3. 如果CTR过低 → 集电极电流不足 → 输出电压过高\r\n");
    DEBUG_Printf("  4. 如果无下拉电阻 → 输出悬空 → 状态不确定\r\n");
    DEBUG_Printf("  5. 如果EMI干扰 → 信号不稳定 → 误触发\r\n");
    DEBUG_Printf("\r\n");
    DEBUG_Printf("推荐的改进措施：\r\n");
    DEBUG_Printf("  ? 使用2.2kΩ限流电阻确保10mA LED电流\r\n");
    DEBUG_Printf("  ? 使用10kΩ下拉电阻提供稳定的高电平\r\n");
    DEBUG_Printf("  ? 并联100nF滤波电容抑制EMI\r\n");
    DEBUG_Printf("  ? 添加TVS二极管保护输入端\r\n");
    DEBUG_Printf("  ? 使用屏蔽电缆减少长线传输干扰\r\n");
} 
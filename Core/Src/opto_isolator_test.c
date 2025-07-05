#include "opto_isolator_test.h"
#include "gpio_control.h"
#include "usart.h"
#include "main.h"

/************************************************************
 * 光耦隔离电路测试模块
 * 专门用于测试和诊断K1_EN、K2_EN、K3_EN信号的光耦隔离电路工作状态
 * 包括：信号状态监测、电压测量、干扰检测、统计分析等功能
 ************************************************************/

// 测试统计数据结构
typedef struct {
    uint32_t high_count;        // 高电平计数
    uint32_t low_count;         // 低电平计数
    uint32_t transition_count;  // 电平跳变计数
    uint32_t test_duration;     // 测试持续时间(ms)
    uint8_t last_state;         // 上一次状态
    uint32_t last_change_time;  // 上次状态变化时间
} OptoTestStats_t;

// 三个通道的测试统计数据
static OptoTestStats_t optoStats[3];
static uint32_t test_start_time;
static uint8_t test_running = 0;

/**
  * @brief  初始化光耦隔离电路测试
  * @retval 无
  */
void OptoTest_Init(void)
{
    // 初始化测试统计数据
    for(uint8_t i = 0; i < 3; i++) {
        optoStats[i].high_count = 0;
        optoStats[i].low_count = 0;
        optoStats[i].transition_count = 0;
        optoStats[i].test_duration = 0;
        optoStats[i].last_state = 0xFF; // 初始状态设为未知
        optoStats[i].last_change_time = 0;
    }
    
    test_start_time = HAL_GetTick();
    test_running = 0;
    
    DEBUG_Printf("=== 光耦隔离电路测试模块初始化完成 ===\r\n");
    DEBUG_Printf("测试功能：\r\n");
    DEBUG_Printf("1. 实时监测K1_EN、K2_EN、K3_EN信号状态\r\n");
    DEBUG_Printf("2. 统计高/低电平占比和跳变次数\r\n");
    DEBUG_Printf("3. 检测信号稳定性和抗干扰能力\r\n");
    DEBUG_Printf("4. 输出详细的诊断信息\r\n");
}

/**
  * @brief  开始光耦隔离电路测试
  * @param  test_duration_sec: 测试持续时间(秒)
  * @retval 无
  */
void OptoTest_Start(uint16_t test_duration_sec)
{
    if(test_running) {
        DEBUG_Printf("[警告] 测试已在运行中\r\n");
        return;
    }
    
    // 重置测试数据
    for(uint8_t i = 0; i < 3; i++) {
        optoStats[i].high_count = 0;
        optoStats[i].low_count = 0;
        optoStats[i].transition_count = 0;
        optoStats[i].test_duration = test_duration_sec * 1000;
        optoStats[i].last_state = 0xFF;
        optoStats[i].last_change_time = 0;
    }
    
    test_start_time = HAL_GetTick();
    test_running = 1;
    
    DEBUG_Printf("=== 开始光耦隔离电路测试 ===\r\n");
    DEBUG_Printf("测试时长：%d秒\r\n", test_duration_sec);
    DEBUG_Printf("请在测试期间：\r\n");
    DEBUG_Printf("1. 对SW1和GND_WB施加/移除DC24V\r\n");
    DEBUG_Printf("2. 观察K1_EN信号变化\r\n");
    DEBUG_Printf("3. 测试其他通道的光耦电路\r\n");
    DEBUG_Printf("开始监测...\r\n");
}

/**
  * @brief  停止光耦隔离电路测试
  * @retval 无
  */
void OptoTest_Stop(void)
{
    if(!test_running) {
        DEBUG_Printf("[提示] 测试未在运行\r\n");
        return;
    }
    
    test_running = 0;
    uint32_t actual_duration = HAL_GetTick() - test_start_time;
    
    DEBUG_Printf("=== 光耦隔离电路测试结束 ===\r\n");
    DEBUG_Printf("实际测试时长：%lu毫秒\r\n", actual_duration);
    
    // 输出测试结果
    OptoTest_PrintResults();
}

/**
  * @brief  光耦隔离电路测试主循环（在主循环中调用）
  * @retval 无
  */
void OptoTest_Process(void)
{
    if(!test_running) return;
    
    uint32_t current_time = HAL_GetTick();
    
    // 检查测试是否应该结束
    if((current_time - test_start_time) >= optoStats[0].test_duration) {
        OptoTest_Stop();
        return;
    }
    
    // 读取三个通道的K_EN信号状态
    uint8_t k_en_states[3] = {
        GPIO_ReadK1_EN(),
        GPIO_ReadK2_EN(),
        GPIO_ReadK3_EN()
    };
    
    // 更新统计数据
    for(uint8_t i = 0; i < 3; i++) {
        uint8_t current_state = k_en_states[i];
        
        // 统计高/低电平
        if(current_state == GPIO_PIN_SET) {
            optoStats[i].high_count++;
        } else {
            optoStats[i].low_count++;
        }
        
        // 检测状态跳变
        if(optoStats[i].last_state != 0xFF && optoStats[i].last_state != current_state) {
            optoStats[i].transition_count++;
            uint32_t time_diff = current_time - optoStats[i].last_change_time;
            DEBUG_Printf("[K%d_EN] 状态变化: %s -> %s (间隔:%lums)\r\n", 
                i+1, 
                optoStats[i].last_state ? "高" : "低",
                current_state ? "高" : "低",
                time_diff);
            optoStats[i].last_change_time = current_time;
        }
        
        optoStats[i].last_state = current_state;
    }
    
    // 每5秒输出一次实时状态
    static uint32_t last_status_time = 0;
    if((current_time - last_status_time) >= 5000) {
        OptoTest_PrintRealTimeStatus();
        last_status_time = current_time;
    }
}

/**
  * @brief  输出实时状态信息
  * @retval 无
  */
void OptoTest_PrintRealTimeStatus(void)
{
    uint32_t elapsed_time = (HAL_GetTick() - test_start_time) / 1000;
    
    DEBUG_Printf("=== 实时状态 (已运行%lus) ===\r\n", elapsed_time);
    DEBUG_Printf("K1_EN=%d, K2_EN=%d, K3_EN=%d\r\n", 
        GPIO_ReadK1_EN(), GPIO_ReadK2_EN(), GPIO_ReadK3_EN());
    
    // 同时读取状态反馈信号
    DEBUG_Printf("状态反馈: K1_1=%d K1_2=%d SW1=%d | K2_1=%d K2_2=%d SW2=%d | K3_1=%d K3_2=%d SW3=%d\r\n",
        GPIO_ReadK1_1_STA(), GPIO_ReadK1_2_STA(), GPIO_ReadSW1_STA(),
        GPIO_ReadK2_1_STA(), GPIO_ReadK2_2_STA(), GPIO_ReadSW2_STA(),
        GPIO_ReadK3_1_STA(), GPIO_ReadK3_2_STA(), GPIO_ReadSW3_STA());
    
    // 输出跳变统计
    for(uint8_t i = 0; i < 3; i++) {
        uint32_t total_samples = optoStats[i].high_count + optoStats[i].low_count;
        if(total_samples > 0) {
            float high_percentage = (float)optoStats[i].high_count / total_samples * 100.0f;
            DEBUG_Printf("K%d_EN: 高电平%.1f%% 跳变%lu次\r\n", 
                i+1, high_percentage, optoStats[i].transition_count);
        }
    }
}

/**
  * @brief  输出详细测试结果
  * @retval 无
  */
void OptoTest_PrintResults(void)
{
    DEBUG_Printf("=== 光耦隔离电路测试结果 ===\r\n");
    
    for(uint8_t i = 0; i < 3; i++) {
        uint32_t total_samples = optoStats[i].high_count + optoStats[i].low_count;
        if(total_samples == 0) continue;
        
        float high_percentage = (float)optoStats[i].high_count / total_samples * 100.0f;
        float low_percentage = (float)optoStats[i].low_count / total_samples * 100.0f;
        
        DEBUG_Printf("--- 通道%d (K%d_EN) ---\r\n", i+1, i+1);
        DEBUG_Printf("采样总数: %lu\r\n", total_samples);
        DEBUG_Printf("高电平: %lu次 (%.1f%%)\r\n", optoStats[i].high_count, high_percentage);
        DEBUG_Printf("低电平: %lu次 (%.1f%%)\r\n", optoStats[i].low_count, low_percentage);
        DEBUG_Printf("状态跳变: %lu次\r\n", optoStats[i].transition_count);
        
        // 分析结果
        if(optoStats[i].transition_count == 0) {
            if(high_percentage > 95.0f) {
                DEBUG_Printf("诊断: 信号持续为高电平 - 光耦可能未导通\r\n");
                DEBUG_Printf("建议: 检查24V电源、光耦LED电流、分压电阻值\r\n");
            } else if(low_percentage > 95.0f) {
                DEBUG_Printf("诊断: 信号持续为低电平 - 光耦可能一直导通\r\n");
                DEBUG_Printf("建议: 检查光耦输出端、下拉电阻、EMI干扰\r\n");
            }
        } else if(optoStats[i].transition_count > 100) {
            DEBUG_Printf("诊断: 信号跳变频繁 - 可能存在EMI干扰或接触不良\r\n");
            DEBUG_Printf("建议: 检查线缆屏蔽、接地、滤波电容\r\n");
        } else {
            DEBUG_Printf("诊断: 信号工作正常 - 光耦响应良好\r\n");
        }
    }
    
    // 输出改进建议
    DEBUG_Printf("\n=== 光耦电路优化建议 ===\r\n");
    DEBUG_Printf("1. 限流电阻: 确保LED电流在5-15mA范围内\r\n");
    DEBUG_Printf("2. 下拉电阻: 建议使用10kΩ，提供快速关断和抗干扰\r\n");
    DEBUG_Printf("3. 滤波电容: 在输出端并联100nF陶瓷电容抑制EMI\r\n");
    DEBUG_Printf("4. TVS保护: 在输入端添加TVS二极管保护\r\n");
    DEBUG_Printf("5. CTR余量: 设计时使用保守CTR值(如25%)\r\n");
    DEBUG_Printf("6. 线缆屏蔽: 长距离传输时使用屏蔽电缆\r\n");
}

/**
  * @brief  手动触发单次状态检测
  * @retval 无
  */
void OptoTest_ManualCheck(void)
{
    DEBUG_Printf("=== 手动状态检测 ===\r\n");
    
    // 连续读取10次，检测稳定性
    uint8_t readings[3][10];
    for(uint8_t i = 0; i < 10; i++) {
        readings[0][i] = GPIO_ReadK1_EN();
        readings[1][i] = GPIO_ReadK2_EN();
        readings[2][i] = GPIO_ReadK3_EN();
        HAL_Delay(10); // 间隔10ms
    }
    
    // 分析稳定性
    for(uint8_t ch = 0; ch < 3; ch++) {
        uint8_t stable = 1;
        uint8_t first_value = readings[ch][0];
        
        for(uint8_t i = 1; i < 10; i++) {
            if(readings[ch][i] != first_value) {
                stable = 0;
                break;
            }
        }
        
        DEBUG_Printf("K%d_EN: 当前值=%d, 稳定性=%s\r\n", 
            ch+1, first_value, stable ? "稳定" : "不稳定");
            
        if(!stable) {
            DEBUG_Printf("  详细读数: ");
            for(uint8_t i = 0; i < 10; i++) {
                DEBUG_Printf("%d ", readings[ch][i]);
            }
            DEBUG_Printf("\r\n");
        }
    }
    
    // 输出相关状态
    DEBUG_Printf("当前状态反馈:\r\n");
    DEBUG_Printf("  K1: STA1=%d STA2=%d SW1=%d\r\n", 
        GPIO_ReadK1_1_STA(), GPIO_ReadK1_2_STA(), GPIO_ReadSW1_STA());
    DEBUG_Printf("  K2: STA1=%d STA2=%d SW2=%d\r\n", 
        GPIO_ReadK2_1_STA(), GPIO_ReadK2_2_STA(), GPIO_ReadSW2_STA());
    DEBUG_Printf("  K3: STA1=%d STA2=%d SW3=%d\r\n", 
        GPIO_ReadK3_1_STA(), GPIO_ReadK3_2_STA(), GPIO_ReadSW3_STA());
}

/**
  * @brief  获取当前测试状态
  * @retval 1:测试运行中 0:测试停止
  */
uint8_t OptoTest_IsRunning(void)
{
    return test_running;
}

/**
  * @brief  获取测试经过时间
  * @retval 测试经过时间(毫秒)
  */
uint32_t OptoTest_GetElapsedTime(void)
{
    return HAL_GetTick() - test_start_time;
} 
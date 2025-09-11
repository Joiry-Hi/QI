#include <stdio.h>
#include <conio.h>

// --- 【核心修改】 ---
// 将 AI_Weights 结构体的定义更新为与主项目完全一致的最新版本
typedef struct
{
    // --- 原有权重 ---
    float w_health_urgency;       
    float w_damage_per_point;     
    float w_kill_shot_bonus;      
    float w_interrupt_heal_bonus; 
    float w_low_qi_gather;        
    float w_defend_vs_high_qi;    
    float w_qi_advantage;         
    float w_damage_per_qi;        
    float w_low_hp_penalty;       
    float w_breakthrough_urgency; // 对“攒气突破”的渴望程度
    float w_self_buff_value;      // 对自我强化（Boost）的价值评估
} AI_Weights;

int main() {
    while (1)
    {
    
    AI_Weights weights;
    FILE *file = fopen("ai_weights.dat", "rb");

    if (file == NULL) {
        printf("Error: Cannot open ai_weights.dat. Does it exist?\n");
        return 1;
    }

    // 从文件中读取权重数据到临时的 weights 结构体中
    // 因为结构体定义已经同步，这里的 sizeof() 会计算出正确的大小
    if (fread(&weights, sizeof(AI_Weights), 1, file) != 1) {
        printf("Error: Failed to read data from file.\n");
        fclose(file);
        return 1;
    }
    
    fclose(file);

    // 以人类可读的方式打印出所有权重值
    printf("--- Current AI Weights ---\n");
    // --- 原有的打印信息 ---
    printf("Health Urgency Weight:    %f\n", weights.w_health_urgency);
    printf("Damage Per Point Weight:  %f\n", weights.w_damage_per_point);
    printf("Kill Shot Bonus:          %f\n", weights.w_kill_shot_bonus);
    printf("Interrupt Heal Bonus:     %f\n", weights.w_interrupt_heal_bonus);
    printf("Low QI Gather Weight:     %f\n", weights.w_low_qi_gather);
    printf("Defend vs High QI Weight: %f\n", weights.w_defend_vs_high_qi);
    printf("QI Advantage Weight:      %f\n", weights.w_qi_advantage);
    printf("Damage Per QI Weight:     %f\n", weights.w_damage_per_qi);
    printf("Low HP Penalty Weight:    %f\n", weights.w_low_hp_penalty);
    printf("Breakthrough Urgency:     %f\n", weights.w_breakthrough_urgency);
    printf("Self Buff Value:          %f\n", weights.w_self_buff_value);
    printf("--------------------------\n");

    _getch();
    }

    return 0;
}
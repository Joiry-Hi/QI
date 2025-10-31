# QI
修仙小游戏——看《凡人修仙传》时的一时兴起

windows系统中借助.vscode文件夹中的配置文件
编译得到.exe文件即可直接运行

linux系统中使用
“gcc -o QI QI.c -lm -fexec-charset=UTF-8”指令
即可编译

QI.c是函数定义与主函数所在
QI.h负责定义各类数据结构
ai_weights.dat中储存着人机训练得到的决策权重（也可以利用.c文件中的结构体初始化直接赋值）
config.txt中是一些可配置参数
weight_viewer.c是权重阅读器代码，编译可得到一个权重阅读器

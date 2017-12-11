#ifndef _LOG_H_
#define _LOG_H_
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
int log_fd;

time_t  tt;

// 向日志文件打印信息
void lprintf(char *str);

// 初始化日志文件
void log_init();

// 重定向输出信息到文本
void log_dup();

// 添加日志时间
void log_time();

// 管理日志文件
// 过一天时间从新打开一个文件,文件名为当天时间
void log_file();


#endif

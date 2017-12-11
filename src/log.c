#include "log.h"
//int log_fd;
//int time;  // 文件创建时间

struct tm *t;

time_t *get_time()
{
	time(&tt);
	t = localtime(&tt);
	printf("%4d年%02d月%02d日:%02d:%02d:%02d\n",\
	       t->tm_year+1900,t->tm_mon + 1, t->tm_mday, \
		   t->tm_hour, t->tm_min, t->tm_sec);
	
}

void lprintf(char *str)
{
	get_time();
	char buffer[4096];
	sprintf(buffer,"[%4d-%02d-%02d-%02d-%02d-%02d]: %s\n",\
	       t->tm_year+1900,t->tm_mon + 1, t->tm_mday, \
		   t->tm_hour, t->tm_min, t->tm_sec, str);
	int num;
	num = write(log_fd, buffer, strlen(buffer));
	if(num == strlen(buffer))
		printf("write log success\n");
}


// 初始化日志文件
void log_init()
{
	// 获取当前时间
	get_time();
	
	// 设置文件名
	char buffer[200];
	sprintf(buffer, "./log/%d-%d-%d_camera.txt",\
	                t->tm_year+1900,t->tm_mon + 1, t->tm_mday);

	// 创建文件追加方式
	log_fd = open(buffer, O_WRONLY|O_CREAT|O_APPEND,0644);
	if(log_fd < 0){
		fprintf(stderr, "open log err");
		exit(1);
	}

}

// 重定向输出信息到文本
void log_dup()
{

}

// 添加日志时间
void log_time()
{
}

// 管理日志文件
// 过一天时间从新打开一个文件,文件名为当天时间
void log_file()
{

}



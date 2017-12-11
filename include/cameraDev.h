#ifndef _CAMERADEV_H_
#define _CAMERADEV_H_

int camera_fd;  // 摄像头文件描述符

typedef struct{
	void  *start;  // 映射后的物理首地址
	size_t length; // 存储映射长度
} Videobuf;

Videobuf *buffer; // 存储映射的数组

int bufs_num; // 记录缓存数量

int counter; // 记录当前捕获多少张图像

int okindex; // 记录已经刷新好的缓存的索引号

char *tmp_buf; // 二级缓存地址

int on_off; // 标识是摄像头是否打开

extern int init_dev();

extern int uninit_dev();

extern int get_dev_info();

extern int cam_on();

extern int cam_off();

extern int get_info();

extern int get_frame(int soct_fd);


#endif

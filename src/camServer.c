#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>

#include "cameraDev.h"
#include "camServer.h"
#include "toJPG.h"
#include "log.h"

void out_client(struct sockaddr_in *addr);
int  analy_head(char *req_buf);
void do_service(int fd, int cmd);
void send_image(int fd);
void sys_err(char *str);
void send_html(int fd);
void send_video(int fd);
void pth_exit(int fd);



// 条件变量
pthread_cond_t   qready = PTHREAD_COND_INITIALIZER;
pthread_mutex_t  qlock  = PTHREAD_MUTEX_INITIALIZER;



/*
** 函数名：sighandler()
** 名称：  信号处理
** 说明：  单当浏览器关闭时会close掉和服务器建立的套接字而服务器
** 继续向套接字写数据会产生SIGPIPE信号，而默认处理是退出进程。
*/
void sighandler(int signo)
{
	if(signo == SIGPIPE){
		printf("sigpipe occured!\n");
	//	signal(SIGPIPE, SIG_IGN);
	}
}



/*
** 函数名：cam_th()
** 名称：  线程1
** 说明：  摄像头获取图片并通知其他线程发送图片
*/
void *cam_th(void *str)
{
	//int i = 0;

	while(1)
	{
		if(client_con > 0)
		{
			if(cam_stat == 0){
				printf("camera on\n");
				lprintf("camera on");
				printf("pthread get_camera start\n");
				cam_on();
				cam_stat = 1;
			}
	//		if(i++ == 20){
			//	printf("cam pict\n");
	//			i = 0;
//			}
			usleep(100);
			get_frame(100);                   /* 跟新一帧图片   */
			pthread_mutex_lock(&qlock);       /* 对条件变量上锁 */
			memcpy(tmp_buf, buffer[okindex].start, buffer[okindex].length);
			pthread_cond_broadcast(&qready);  /* 通知其他线程获取图片 */
			pthread_mutex_unlock(&qlock);     /* 解锁           */
		}
		else if(cam_stat == 1){
			printf("camera off\n");
			lprintf("camera off");
			cam_off();
			cam_stat = 0;
		}
		else
			usleep(1);
	}

	return (void *)0;
}

/*
** 函数名：sent_th()
** 名称：  线程2
** 说明：  不断向网络发送图片形成视频效果
*/
void *send_th(void *str)
{
	int     fd = (int)str;
	printf("pthread send start fd=%d\n", fd);
	do_service(fd, 0);

	return (void *)0;
}




/*
** 函数名：main()
** 名称：  主线程
** 说明：  初始化摄像头和网络链接
*/
int main(int argc, char **argv)
{
	if(argc < 2){
		fprintf(stderr, "usage: [dev] [port]");
		exit(1);
	}
	client_con = 0;  // 线程数

	log_init();   // 初始化日志
/*======================摄像头===========================*/
	camera_fd = open("/dev/video0", O_RDWR|O_NONBLOCK);
	init_dev();
	get_dev_info();
	cam_stat = 0;

/*===================绑定信号处理函数=====================*/
	if(signal(SIGPIPE, sighandler) == SIG_ERR)
		sys_err("sig err");
	signal(SIGPIPE, sighandler);


#if 1
/*==================获取图片进程=========================*/
	// 启动进程1
	int     err;
	err = pthread_create(&ntid, NULL, cam_th, NULL);
	if(err !=0 )
		sys_err("pthread create err");		

#endif
/*====================建立网络链接========================*/
	// 1. 创建套接字
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
		sys_err("socket");
	
	// 2. 准备地址并写入内容
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));

	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(argv[1])); // port 

	// 不管是哪个网卡/哪个IP地址接收到的数据， 都处理
	addr.sin_addr.s_addr = INADDR_ANY;  // IP 0.0.0.0

	// 4. 绑定地址到sockfd
	if(bind(sockfd, (struct sockaddr *)&addr,sizeof(addr)) < 0)
		sys_err("bind");
	
	// 5. 监听
	if(listen(sockfd, 300) < 0)
		sys_err("listen");
	
	// 6. 接受客户端的链接
	struct sockaddr_in caddr;
	socklen_t len = sizeof(caddr);
	char req_buf[1024];
	memset(req_buf, 0, 1024);

	int tid_con = 0;
	int cmd_h;
	int x = 0;


	int fd[1024] = {0};
	while(1)
	{
		fd[tid_con] = accept(sockfd, (struct sockaddr *)&caddr, &len);
		if(fd[tid_con] <0)
			sys_err("accept");
		
		printf("================%d===============\n", x++);
		// 显示请求信息
		read(fd[tid_con], req_buf, 1024);
		write(STDOUT_FILENO, req_buf, 1024);
		lprintf(req_buf);
//		cmd_h = analy_head(req_buf);
		
		err = pthread_create((pthread_t *)&(fd[tid_con%1000]),
		                    NULL, send_th, (void *)fd[tid_con++]);
		client_con ++;  // 线程计数
		printf("now client num = %d\n", client_con);
		if(err == -1){
			sys_err("create sent_th err");
		}
		printf("**********************************\n");
	}

	// 关闭摄像头文件
	uninit_dev();
	printf("close camera\n");
	// 8. 关闭服务器
	close(sockfd);
	return 0;
}


/*
** 出错处理
*/
void sys_err(char *str)
{
	fprintf(stderr, "%s: %s\n", str, strerror(errno));
	exit(1);
}


/*
** 打印客户端信息
*/
void out_client(struct sockaddr_in *addr)
{
	// ip port   网络字节转换为主机字节序
	unsigned short port = ntohs(addr->sin_port);
	char ip[16] = {0};
	// 地址格式转换 
	const char *p = inet_ntop(AF_INET,\
					(void *)&addr->sin_addr.s_addr, ip, 16);
	if(p == NULL)
		sys_err("inet_ntop");
	
	printf("client ip: %15s, prot: %d\n", ip, port);

}



/*
** analy_head()
** 处理客户请求信息
*/

//	GET /jpg/Image_07.jpg HTTP/

int analy_head(char *req_buf)
{
	char *req  = req_buf;
	int cont = 0;
	
	
	if(req == NULL){
		fprintf(stderr, "req is NULL\n");
		return -1;
	}
	// 字符串截断
	char * sp1 = " ";
	char * sp2 = "/";
	char * strr[20] = {NULL};

	char * ar = strtok(req, sp1);
	char * br_need = strtok(NULL, sp1);
	
	if(br_need != NULL &&  !strcmp(br_need, "/jpg/Image_07.jpg")){
		printf("ok %s\n", br_need);
		return 1;
	}

	return 0;
}

/*
** do_service()
** 响应请求-向客户端发送数据
*/
void do_service(int fd, int cmd)
{
	
	// 2. 发送一个本地图片
	//	send_image(fd);

	// 3. 发送一个摄像头图片
		//send_image(fd);
	
	// 发送一个html文件
	// cmd=0  请求html
	// cmd=1  请求图片
//	if(cmd == 0)
//		send_html(fd);
//	if(cmd == 1)
//		send_image(fd);

	// 4. 连续发送图片
	send_video(fd);


}


/*
** seng_html() 
** 发送一个html文件
*/
void send_html(int fd)
{
	struct stat st;
	char mes_head[100];
	char buff[1024];
	int rev ;
	int html_fd;

	char *http_head = "HTTP/1.1 200 OK\r\nConnection:Keep_Alive\r\n";
	write(fd, http_head, strlen(http_head));

	html_fd = open("html/video.html", O_RDONLY);
	if(html_fd < 0){
		fprintf(stderr, "open html err\n");
		return ;
	}
	
	rev = fstat(html_fd, &st);
	if(rev < 0){
		fprintf(stderr, "fstat err\n");
		return ;
	}
	
	printf("leng=%ld\n", st.st_size);

	sprintf(mes_head,\
	        "Content-Type:text/html\nContent-Length: %ld\r\n\r\n",\
			st.st_size);
	
	
	write(fd, mes_head, strlen(mes_head));
	memset(buff, 0, 1024);
	while(read(html_fd, buff, 1024) > 0){
		write(fd, buff, 1024);
		memset(buff, 0, 1024);
	}

	close(html_fd);
}



void send_video(int fd)
{
	// 打印http协议头
	int num = 0;
	
	char head[4096];
	sprintf(head, "HTTP/1.0 200 OK\r\n"
                  "Connection: Keep-Alive\r\n"
                  "Server: Network camera\r\n"
                  "Cache-Control: no-cache,no-store,must-revalidate,pre- check=0,max-age=0\r\n"
                  "Pragma: no-cache\r\n"
                  "Content-Type: multipart/x-mixed-replace;boundary=KK\r\n");
	num = write(fd, head, strlen(head));
	if(num != strlen(head)){
		printf("write head err \n");
	//	pthread_exit(0);
	}

	int i;
	//消息报头
	while(1){
		/* 等待条件变量 */
		
		if(i++ == 100){
			printf("fd %d\n", fd);
			i = 0;
		}

		sprintf(head, "\r\n--KK\r\n"				
                  "Content-Type: image/jpeg\n"			
                  "Content-Length: %ld\n\n", buffer[okindex].length+400);
		num = write(fd, head, strlen(head));
		if(num != strlen(head)){
			printf("write err\n");
			break;
		}
		pthread_mutex_lock(&qlock);
		pthread_cond_wait(&qready, &qlock);
		
		print_picture(fd, buffer[okindex].start, buffer[okindex].length);
		pthread_mutex_unlock(&qlock);
		
	}
	pth_exit(fd);
}

void pth_exit(int fd)
{
	printf("pthread exit fd = %d\n", fd);
	close(fd);
	client_con --;
	pthread_exit(NULL);
	
}


/*
** seng_image()
** 向客户端发送一个图片文件
*/
void send_image(int fd)
{
	//获取文件大小
	struct stat st;
	char mes_head[100];
	char * buff[1024];
	int rev;

	lock_jpg(&jpg_file); // 上锁
	
	
	jpg_file.fd = open("jpg/Image_7.jpg", O_RDONLY);
	if(jpg_file.fd < 0){
		fprintf(stderr, "jpg open err\n");
		return;
	}
	
	
	rev = fstat(jpg_file.fd, &st);
	if(rev < 0){
		fprintf(stderr, "fstat err\n");
		return;
	}
	
	// 图片格式传输
	sprintf(mes_head,\
	        "Content-Type:image/jpeg\nContent-Length: %ld\r\n\r\n",\
	        st.st_size);
	write(fd, mes_head, strlen(mes_head));
	
	
	memset(buff, 0 , 1024);
	while(read(jpg_file.fd, buff, 1024) != 0){
		write(fd, buff, 1024);
		memset(buff, 0, 1024);
	}
	close(jpg_file.fd);

	
	unlock_jpg(&jpg_file);  // 解锁
}



/*
** 锁初始化
*/
void lock_init(JpgLockType *jq)
{
	int   err;
	err = pthread_rwlock_init(&jq->q_lock, NULL);
	if(err != 0){
		sys_err("rwlock init err");
	}
}

void lock_jpg(JpgLockType *jq)
{
	pthread_rwlock_wrlock(&jq->q_lock);
}

void unlock_jpg(JpgLockType *jq)
{
	pthread_rwlock_unlock(&jq->q_lock);
}






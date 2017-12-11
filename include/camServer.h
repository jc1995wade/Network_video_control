#ifndef _CAMSERVER_H_
#define _CAMSERVER_H_

#include "common.h"
#include <pthread.h>
int socket_fd;

pthread_t ntid;

int client_con;
int cam_stat;

// 创建读写锁结构体
typedef struct jpglock{
	int fd;
	pthread_rwlock_t q_lock;
} JpgLockType;

JpgLockType jpg_file;

void lock_init(JpgLockType *jq);

void lock_jpg(JpgLockType *jq);

void unlock_jpg(JpgLockType *jq);






#endif

#ifndef __ICMP_INCLUDE_H__
#define __ICMP_INCLUDE_H__

#include "lib_acl.h"
#include <time.h>
#include "icmp/lib_icmp_type.h"

typedef struct ICMP_TIMER ICMP_TIMER;
typedef struct ICMP_STREAM ICMP_STREAM;
typedef struct IP_HDR IP_HDR;
typedef struct ICMP_HDR ICMP_HDR;

/* in icmp_timer.cpp */
/**< 定时器信息结构 */
struct ICMP_TIMER {
	/**< 设置定时任务 */
	time_t (*request)(ICMP_TIMER* timer, ICMP_PKT* pkt, int delay);
	/**< 取消定时任务 */
	time_t (*cancel)(ICMP_TIMER* timer, ICMP_PKT* pkt);
	/**< 查找并删除定时任务 */
	ICMP_PKT* (*find_delete)(ICMP_TIMER* timer, unsigned short i_seq);
	/**< 弹出下一个定时任务 */
	ICMP_PKT* (*popup)(ICMP_TIMER* timer);                    

	ACL_RING timer_header;      /**< 定时任务的链表头 */
	time_t present;             /**< 当前时间 */
	time_t time_left;           /**< 距下一个定时任务的时间 */
};

/**< 发送与接收 ICMP 包的通信流 */
struct ICMP_STREAM {
	ACL_VSTREAM *vstream;       /**< 同步流指针 */
	ACL_ASTREAM *astream;       /**< 异步流指针 */

	struct sockaddr_in from;    /**< 接收数据的源地址 */
	int   from_len;             /**< 存储在 from 中的地址大小 */

	ICMP_HOST *curr_host;       /**< 当前的目的主机对象 */
};

/**< ICMP 通信句柄 */
struct ICMP_CHAT {
	/* 通用的成员变量 */
	unsigned short seq_no;      /**< 每个 icmp 包的序列号 */
	ICMP_STREAM *is;            /**< 与某个客户机相关的流 */
	ACL_RING host_head;         /**< 当前 ICMP 通信对象中的主机组成的链 */
	unsigned short pid;         /**< 由当前进程ID表示 */
	unsigned long  tid;         /**< 由线程ID表示 */
	int   check_tid;            /**< 是否检查响应包中的 tid 值 */

	/* 异步IO的成员变量 */
	ACL_AIO *aio;               /**< 异步IO句柄 */
	int   count;                /**< 当前 ICMP 通信对象中已经完成的主机数 */
	int   check_inter;          /**< 每隔多少秒检查一下定时器里的任务 */
	ICMP_TIMER *timer;          /**< 包发完后等待包应答的定时器 */
};

#define ICMP_ECHO               8
#define ICMP_ECHOREPLY          0

#define ICMP_MIN                8  /**< Minimum 8 byte icmp packet (just header) */

/**< IP 协议头结构 */
struct IP_HDR {
#if __BYTE_ORDER == __LITTLE_ENDIAN
	unsigned int h_len:4;          /**< length of the header */
	unsigned int version:4;        /**< Version of IP */
#elif __BYTE_ORDER == __BIG_ENDIAN
	unsigned int version:4;        /**< Version of IP */
	unsigned int h_len:4;          /**< length of the header */
#else
# error "unknown __BYTE_ORDER"
#endif

	unsigned char tos;             /**< Type of service */
	unsigned short total_len;      /**< total length of the packet */
	unsigned short ident;          /**< unique identifier */
	unsigned short frag_and_flags; /**< flags */
	unsigned char  ttl;            /**< time to live */
	unsigned char proto;           /**< protocol (TCP, UDP etc) */
	unsigned short checksum;       /**< IP checksum */

	unsigned int source_ip;        /**< source IP*/
	unsigned int dest_ip;          /**< dest IP */
};

/**< ICMP header */
struct ICMP_HDR {
	unsigned char type;
	unsigned char code;          /* type sub code */
	unsigned short cksum;
	unsigned short id;
	unsigned short seq;
};

#define MIN_PACKET     32 
#define MAX_PACKET     1024 

/**< ICMP 包数据结构 */
struct ICMP_PKT {
	/* 发送的数据包 */
	ICMP_HDR hdr;                   /**< icmp 头 */
	union {
		unsigned long tid;           /**< 线程ID号 */
		char data[MAX_PACKET];      /**< icmp 数据体 */
	} body;

	/* 本地存储的数据成员，主要为了解析方便 */
	ICMP_HOST *icmp_host;           /**< 目的主机 */
#define pkt_chat icmp_host->chat    /**< ICMP_PKT 中指向 ICMP_CHAT 的快捷方式 */

	ACL_RING pkt_ring;              /**< 发送数据包的双向链 */
	ACL_RING timer_ring;            /**< 定时器结点 */

	size_t write_len;               /**< 发送的数据长度 */
	size_t read_len;                /**< 接收的数据长度 */

	size_t dlen;                    /**< 数据包中数据体长度 */
	struct timeval stamp;           /**< time stamp */

	/* 响应的数据包的分析结果 */
	ICMP_PKT_STATUS pkt_status;
};

#define RING_TO_PKT(r) \
	((ICMP_PKT *) ((char *) (r) - offsetof(ICMP_PKT, pkt_ring)))
#define ICMP_PKT_NEXT(head, curr) \
	(acl_ring_succ(curr) != (head) ? RING_TO_PKT(acl_ring_succ(curr)) : NULL)

#define RING_TO_HOST(r) \
	((ICMP_HOST *) ((char *) (r) - offsetof(ICMP_HOST, host_ring)))
#define ICMP_HOST_NEXT(head, curr) \
	(acl_ring_succ(curr) != (head) ? RING_TO_HOST(acl_ring_succ(curr)) : NULL)

#endif


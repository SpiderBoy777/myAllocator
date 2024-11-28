#ifndef ALLOCPOOL_H
#define ALLOCPOOL_H
#include<iostream>
namespace myAllocPool{
	class AllocPool {
	public:
		//小区块的基本大小单位
		static const int __BASE = 8;
		//小区块的最大大小
		static const int __MAX_BTYES = 128;
		//计算得到链表大小
		static const int __LIST_SIZE = (__MAX_BTYES + __BASE - 1) / __BASE;
		//每次从pool里最多取20个chunk
		static const int __MAX_CHUNK_NUMS = 20;
		static void* allocate(size_t n);
		static void deallocate(void *p, size_t n);
	private:
		/*模拟链表中的节点
		将某块内存用obj的的方式来看待
		这样才能以链表的方式将下一块内存地址存到这块内存的前面四个字节中*/
		union obj {
			union obj* next;
		};
		typedef obj* objList;
		static objList free_list[__LIST_SIZE];
		static inline size_t getListIndex(size_t n) {
			return (n + __BASE - 1) / __BASE - 1;
		}
		//字节对齐，保证是__BASE的倍数,__BASE必须是2的倍数以下函数才可行
		static inline size_t ROUND_UP(size_t n) {
			return (n + __BASE - 1) & ~(__BASE-1);
		}
		static void* refill(size_t n);
		static char* chunk_alloc(size_t size,int& nobjs);
		//pool剩余空间的起始地址
		static char* start_free;
		//pool剩余空间的终止地址
		static char* end_free;
		//已向操作系统申请的内存大小
		static size_t heap_size;
	};


}
#endif

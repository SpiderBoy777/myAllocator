#include "AllocPool.h"
#include<iostream>
#define DEBUG
namespace myAllocPool {
	AllocPool::objList AllocPool::free_list[__LIST_SIZE] = {nullptr};
	char* AllocPool::start_free = nullptr;
	char* AllocPool::end_free = nullptr;
	size_t AllocPool::heap_size = 0;
	void* AllocPool::allocate(size_t n) {
		//字节数过大就改用第一级分配内存
		if (n > __MAX_BTYES) {
#ifdef DEBUG
			printf("使用一级分配器分配字节数：%zu ", n);
			printf("此时内存池剩余字节数=%zu,已向操作系统申请字节数=%zu\n", end_free - start_free, heap_size);
#endif // DEBUG

			
			return malloc(n);
		}
		//后面多线程要用到volatile对my_free_list操作
		objList* current_list;
		//将当前链表移到对应位置
		current_list = free_list + getListIndex(n);
		obj* result;
		result = *current_list;
		//填充池子
		if (result == nullptr) {
			return refill(ROUND_UP(n));
		}
#ifdef DEBUG
		printf("使用二级分配器分配字节数：%zu \n", n);
		//printf("add=%p next->add=%p\n", result, result->next);
#endif // DEBUG

		
		*current_list = result->next;
		return result;
	}
	void AllocPool::deallocate(void* p, size_t n) {
		if (n > __MAX_BTYES) {
			return free(p);
		}
#ifdef DEBUG
		printf("回收内存 size=%zu\n", n);
		//printf("add=%p next->add=%p\n", result, result->next);
#endif // DEBUG
		objList* current_list;
		current_list = free_list + getListIndex(n);
		obj* q = (obj*)p;
		q->next = *current_list;
		*current_list = q;

	}
	//该函数用于将多个chunk连续的内存挂到对应链表上，必须将n调整至8的倍数,并返回第一个chunk
	void* AllocPool::refill(size_t n) {
#ifdef DEBUG
		printf("使用refill\n ");
#endif // DEBUG

		
		int chunkNums = __MAX_CHUNK_NUMS;
		//将chunk返回从pool中申请的要挂到链表上的内存
		char* chunk = chunk_alloc(n, chunkNums);
		if (chunkNums == 1) return chunk;
		objList *current_list;
		current_list = free_list + getListIndex(n);
		//将第一块chunk交给内存使用
		obj* result;
		result = (obj*)chunk;
		obj* current_chunk;
		//获得第二块chunk的地址
		*current_list = current_chunk = (obj*)(chunk+n);
		//从第二块chunk开始，用链表连起来
		for (int i = 1; i < chunkNums; ++i) {
			if (i != chunkNums - 1) {
				//这里指针转是必要的，只有将指针转换成char*在移动是才会是以字节为单位
				current_chunk->next = (obj*)((char*)current_chunk+n);
#ifdef DEBUG
				//printf("i=%d current=%p next=%p\n", i, current_chunk, current_chunk->next);
#endif // DEBUG

				current_chunk = (obj*)current_chunk->next;
			}
			else {
				current_chunk->next = nullptr; 
#ifdef  DEBUG
				//printf("last i=%d current=%p next=%p\n", i, current_chunk, current_chunk->next);
#endif //  DEBUG

			}
			
		}
		return result;
	}
	//该函数用于从内存池中分配一段大小为nobjs*size的区域，如果剩余内存不足，则会调整nobjs
	char* AllocPool::chunk_alloc(size_t size, int& nobjs) {
		char* result=nullptr;
		size_t total_size = size * nobjs;
		size_t left_size = end_free - start_free;
#ifdef  DEBUG
		printf("使用chunk_alloc前,内存池剩余字节数=%zu,已向操作系统申请字节数=%zu\n", end_free - start_free, heap_size);
#endif
		//能够满足申请大小
		if (left_size >= total_size) {
			
			result = start_free;
			start_free += total_size;
#ifdef DEBUG
			printf("使用chunk_alloc后,内存池剩余字节数=%zu,已向操作系统申请字节数=%zu\n", end_free - start_free, heap_size);
#endif // DEBUG

			return result;
		}//虽然无法满足申请大小，但是能申请至少一个chunk
		else if (left_size > size) {
			result = start_free;
			nobjs = left_size / size;
			start_free += nobjs*size;
#ifdef DEBUG
			printf("无法申请20个空间 使用chunk_alloc后,内存池剩余字节数=%zu,已向操作系统申请字节数=%zu 申请chunk数=%d\n", end_free - start_free, heap_size,nobjs);
#endif
			return result;
		}//一个chunk都无法分配
		else {
			//在申请空间前，先将目前的pool中的剩余空间给利用起来，挂到某个链表上
			if (left_size > 0) {
#ifdef DEBUG
				printf("先回收pool中剩余空间\n");
#endif // DEBUG

				objList* currentList;
				//由于我们申请空间释放空间都保证操作的内存是__BASE的倍数，
				//因此肯定当有剩余时，剩余大小也是__BASE的倍数，至少能放到第一个链表中
				currentList = free_list + getListIndex(left_size);
				((obj*)start_free)->next = *currentList;
				*currentList = (obj*)start_free;
				start_free+= left_size/__BASE*__BASE;
#ifdef DEBUG
				printf("回收pool中剩余空间后,内存池剩余字节数=%zu,已向操作系统申请字节数=%zu\n", end_free - start_free, heap_size);
#endif // DEBUG

			}
			//要申请越来越多的空间，这是经验所谈
			size_t bytes_to_get = 2 * total_size + ROUND_UP(heap_size >> 4);
			//试着申请空间
			start_free = (char*)malloc(bytes_to_get);
			if (heap_size > 1000) start_free = 0;
			//申请失败
			if (0 == start_free) {
				//尝试从编号更大的list中的空闲内存来申请空间
				for (int i = getListIndex(size)+1; i < __LIST_SIZE; ++i) {
					objList* currenList = free_list + i;
					if (*currenList != nullptr) {
						char* result = (char*)*currenList;
						*currenList = (*currenList)->next;
						//逻辑上假装这一个chunk就是内存池中剩余的内存，这样方便递归调用
						start_free = result;
						end_free = start_free + (i+1)*__BASE;
#ifdef DEBUG
						printf("使用%d的链表空闲节点进行空间分配\n", i);
#endif // DEBUG
						return chunk_alloc(size, nobjs);
					}
				}
#ifdef DEBUG
				printf("ERROR内存请求无法处理\n");
#endif
				//没有空闲内存，山穷水尽，返回第一级内存,当然这里没有使用第一级分配器，所以就省略不写了
				/*end_free = 0;
				start_free = (char*)malloc::alloc::allocate(bytes_to_get);*/
				abort();
			}
#ifdef DEBUG
			printf("内存池扩充\n");
#endif
			//申请成功，内存池已经扩充，重新调用chunk_alloc从内存池中申请空间给链表用
			heap_size += bytes_to_get;
			end_free = start_free + bytes_to_get;
			return chunk_alloc(size, nobjs);
		}
		return result;
	}
}

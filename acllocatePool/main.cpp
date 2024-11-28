#include <iostream>
#include"AllocPool.h"

template<class dataType>
class allocAdaptor {
public:
	void* operator new(size_t size) {
		std::cout << "-------------" << std::endl;
		return myAllocPool::AllocPool::allocate(size);
	}
	void operator delete(void* p, size_t size) {
		std::cout << "-------------" << std::endl;
		myAllocPool::AllocPool::deallocate(p,size);
	}
private:
	dataType m_data;
};
int main() {
	std::cout << "double_size=" << sizeof(allocAdaptor<double>) << " int_size=" << sizeof(allocAdaptor<int>) << " string_size=" << sizeof(allocAdaptor<std::string>) << std::endl;
	allocAdaptor<double>* p1[10];
	for (int i = 0; i < 10; i++) {
		 p1[i] = new allocAdaptor<double>;
	}
	allocAdaptor<int>* p2[10];
	for (int i = 0; i < 10; i++) {
		 p2[i] = new allocAdaptor<int>;
	}
	allocAdaptor<std::string>* p3[10];
	for (int i = 0; i < 10; i++) {
		p3[i] = new allocAdaptor<std::string>;
	}
	for (int i = 0; i < 10; i++) {
		delete p1[i]; delete p2[i]; delete p3[i];
	}

	return 0;
}

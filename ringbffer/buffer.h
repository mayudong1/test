#ifndef _RING_BUFFER_H_
#define _RING_BUFFER_H_

#include <mutex>

#define DEFAUTL_SIZE (1024*1024*5)

class RingBuffer
{
public:
	RingBuffer();
	~RingBuffer();
	
public:
	int Init(int size = DEFAUTL_SIZE);
	void Destroy();

	int CurrentSize();
	int RemainSize();
	int Put(const char* data, const int size);
	int Get(char* data, int size);
	int Skip(int len);
	int Clear();
private:
	char* _data;
	int _size;
	int _current_size;

	int _start;
	int _end;
	unsigned long long _file_pos;
private:
	std::recursive_mutex _mutex;
};

#endif
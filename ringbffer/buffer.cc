#include "buffer.h"
#include <stdlib.h>
#include <iostream>
#include <memory.h>

using namespace std;

RingBuffer::RingBuffer() : _data(NULL), _size(0), _current_size(0), _start(0), _end(0), _file_pos(0)
{

}

RingBuffer::~RingBuffer()
{
	Destroy();
}

int RingBuffer::Init(int size)
{
	_size = size;
	_data = new char[size];
	_start = 0;
	_end = 0;

	return 0;
}

void RingBuffer::Destroy()
{
	if(_data)
		delete[] _data;
	_data = NULL;
	_size = 0;
}

int RingBuffer::Clear()
{
	_start = 0;
	_end = 0;
	return 0;
}

int RingBuffer::CurrentSize()
{
	lock_guard<recursive_mutex> g(_mutex);
	return _current_size;
}

int RingBuffer::RemainSize()
{
	lock_guard<recursive_mutex> g(_mutex);
	return _size - _current_size;
}

int RingBuffer::Skip(int len)
{
	_file_pos += len;
	_start += len;
	return 0;
}

int RingBuffer::Put(const char* data, const int size)
{
	lock_guard<recursive_mutex> g(_mutex);
	if(size <= 0)
		return 0;

	if(size > RemainSize())
	{
		cout << "size > RemainSize" << endl;
		return -1;
	}

	if(_end >= _start)
	{
		if(_size - _end >= size)
		{
			memcpy(&_data[_end], data, size);
			_end += size;
		}
		else
		{
			int first_size = _size - _end;
			int second_size = size - first_size;
			memcpy(&_data[_end], data, first_size);
			memcpy(&_data[0], data+first_size, second_size);
			_end = second_size;
		}	
	}
	else
	{
		memcpy(&_data[_end], data, size);
		_end += size;
	}
	_current_size += size;

	return 0;
}

int RingBuffer::Get(char* data, int size)
{
	lock_guard<recursive_mutex> g(_mutex);

	int len = std::min(size, _current_size);
	if(len <= 0)
		return 0;

	if(_end > _start)
	{
		memcpy(data, &_data[_start], len);
		_start += len;
	}
	else
	{
		int first_size = _size - _start;
		if(len <= first_size)
		{
			memcpy(data, &_data[_start], len);
			_start += len;
		}
		else
		{
			int second_size = len - first_size;
			memcpy(data, &_data[_start], first_size);
			memcpy(data+first_size, &_data[0], second_size);
			_start = second_size;
		}
	}
	_current_size -= len;
	_file_pos += len;
	return len;
}

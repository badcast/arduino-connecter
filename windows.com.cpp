#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <iostream>
#include <string>
#include <stdint.h>
#include <Windows.h>
#include <vector>
#include <stdexcept>

char _comPort[5] = "COM\0";

bool com_open(HANDLE* handle);

const std::size_t com_write(HANDLE handle, std::uint8_t* bytes, std::size_t size);
const std::size_t com_write(HANDLE handle, std::uint8_t byte);
const std::size_t com_write_fill(HANDLE handle, int byte, std::size_t count);
const std::size_t com_read(HANDLE handle, void* memory, std::size_t size);


void com_close(HANDLE handle);

const std::string get_com_name(HANDLE handle);

std::vector<int> p1, p2, p3, p4;
std::vector<std::vector<int>*> _c;
int __cached_length = 0;

int get_arr_value(int index) {
	int x = 0; // index of the array
	int y = 0; // length of the array

	if (index < 0)
		goto __exception;

	for (; x < _c.size() && index >= y + _c[x]->size(); ++x)
		y += _c[x]->size();

	if (x == _c.size())
		__exception:
	throw std::out_of_range("Range error");

	return _c[x]->operator[](index - y);
}

int get_arr_length() {
	int x;

	if (!__cached_length)
		for (x = 0; x < _c.size(); ++x)
			__cached_length += _c[x]->size();
	return __cached_length;
}

void writeProgress(std::vector<int>& vec, int progress) {
	vec.resize(progress);
	for (; progress; --progress)
		vec[progress - 1] = progress;
}

std::uint8_t _buffer[9];
int com_add(HANDLE handle, int arg1, const int& arg2) {
	_buffer[0] = 1;
	memcpy(&_buffer[1], &arg1, sizeof(arg1));
	memcpy(&_buffer[5], &arg2, sizeof(arg2));
	com_write(handle, _buffer, sizeof(_buffer));
	arg1 = com_read(handle, _buffer, sizeof(int));
	if (arg1 != sizeof(int))
		throw std::bad_exception();
	return *reinterpret_cast<int*>(_buffer);
}
int com_sub(HANDLE handle, int arg1, const int& arg2) {
	_buffer[0] = 2;
	memcpy(&_buffer[1], &arg1, sizeof(arg1));
	memcpy(&_buffer[5], &arg2, sizeof(arg2));
	com_write(handle, _buffer, sizeof(_buffer));
	arg1 = com_read(handle, _buffer, sizeof(int));
	if (arg1 != sizeof(int))
		throw std::bad_exception();
	return *reinterpret_cast<int*>(_buffer);
}
int com_mul(HANDLE handle, int arg1, const int& arg2) {
	_buffer[0] = 3;
	memcpy(&_buffer[1], &arg1, sizeof(arg1));
	memcpy(&_buffer[5], &arg2, sizeof(arg2));
	com_write(handle, _buffer, sizeof(_buffer));
	arg1 = com_read(handle, _buffer, sizeof(int));
	if (arg1 != sizeof(int))
		throw std::bad_exception();
	return *reinterpret_cast<int*>(_buffer);
}
int com_div(HANDLE handle, int arg1, const int& arg2) {
	_buffer[0] = 4;
	memcpy(&_buffer[1], &arg1, sizeof(arg1));
	memcpy(&_buffer[5], &arg2, sizeof(arg2));
	com_write(handle, _buffer, sizeof(_buffer));
	arg1 = com_read(handle, _buffer, sizeof(int));
	if (arg1 != sizeof(int))
		throw std::bad_exception();
	return *reinterpret_cast<int*>(_buffer);
}

std::vector<int> plus2array(HANDLE handle, std::vector<int> a, std::vector<int> b) {
	int x = 0;
	int y = a.size();
	if (y > b.size())
		y = b.size();
	std::vector<int> result(y);

	for (int x = 0; x < y; ++x)
		result[x] = com_add(handle, a[x], b[x]);

	return result;
}

int main(const int argn, const char* const args) {
	HANDLE hSerial;
	bool break_p = false;
	bool cmdStatus;
	std::string command;
	const int N = 100;
	writeProgress(p1, N);
	writeProgress(p2, N);
	writeProgress(p3, N);
	writeProgress(p4, N);

	_c.emplace_back(&p1);
	_c.emplace_back(&p2);
	_c.emplace_back(&p3);
	_c.emplace_back(&p4);

	int len = get_arr_length();
	int v = get_arr_value(0);


	if (com_open(&hSerial))
	{
		std::string com_name = get_com_name(hSerial);
		std::streampos beginPos = std::cout.tellp();

		int x = 1;
		int y = 2;
		int r = com_add(hSerial, x, y);

		auto arr = plus2array(hSerial, p1, p2);

		while (!break_p)
		{
			cmdStatus = false;
			std::cout << "Enter command, wait: ";

			std::cin >> command;
			std::cout << std::endl;
			if (command == "on")
			{
				com_write(hSerial, 0x1);
				cmdStatus = true;
			}
			else if (command == "off")
			{
				com_write(hSerial, 0x0);
				cmdStatus = true;
			}
			else if (command == "break") {
				break_p = true;
			}
			else
			{
				std::cout << "Undefined command. Please try again." << std::endl;
			}

			if (cmdStatus)
				std::cout << "Command complete." << std::endl;

			Sleep(250);
			std::cout << "Refresh" << std::endl;
			Sleep(250);
			//clear console to origin point
			std::cout.seekp(beginPos, std::ios::beg);
			std::cout.clear();

		}

		com_close(hSerial);
		std::cout << "closed " << get_com_name(hSerial) << std::endl;
	}
	else
	{
		std::cout << "Open COM{N} port is failed." << std::endl;
	}

	std::cout << "Good bye." << std::endl;
	std::cout << std::endl;


	system("pause");

	return 0;
}

bool com_open(HANDLE* handle) {
	constexpr int mCOMn = 10;
	bool status;
	int i;

	for (i = 1; i < mCOMn; ++i)
	{
		_itoa(i, &_comPort[3], 0xA);
		*handle = CreateFile(_comPort, GENERIC_READ | GENERIC_WRITE, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (status = (*handle != INVALID_HANDLE_VALUE))
		{
			DCB dcb{ 0 };
			dcb.DCBlength = sizeof(dcb);


			if (!GetCommState(*handle, &dcb)) {
				std::cout << "Error getting data" << std::endl;
			}

			dcb.BaudRate = CBR_9600;
			dcb.ByteSize = 0x8;
			dcb.StopBits = ONESTOPBIT;
			dcb.Parity = NOPARITY;

			if (SetCommState(*handle, &dcb)) {
				std::cout << _comPort << " is ready" << std::endl;
			}

			std::cout << "Turn LED value to (ON, OFF)." << std::endl;
			break;
		}
	}

	return status;
}

void com_close(HANDLE handle) {
	CloseHandle(handle);
}

const std::size_t com_write(HANDLE handle, std::uint8_t* bytes, std::size_t size) {
	DWORD written;
	if (!WriteFile(handle, bytes, size, &written, NULL))
		written = 0;
	return written;
}

const std::size_t com_write(HANDLE handle, std::uint8_t byte) {
	return com_write(handle, &byte, 0x1);
}

const std::size_t com_write_fill(HANDLE handle, int value, std::size_t count) {
	std::size_t writed = 0;
	std::uint8_t bytes[128];
	std::size_t vla = sizeof(bytes);
	if (vla > count)
		vla = count;
	memset(bytes, value, vla);
	while (count) {
		writed += com_write(handle, bytes, vla);
		if ((count -= vla) < vla)
			vla = count;
	}
	return writed;
}

const std::size_t com_read(HANDLE handle, void* memory, std::size_t size) {
	DWORD readed;
	if (!ReadFile(handle, memory, size, &readed, NULL))
		readed = 0;
	return readed;
}

const std::string get_com_name(HANDLE handle) {
	if (handle != INVALID_HANDLE_VALUE && handle != 0x0) {
		return std::string(_comPort);
	}

	return {};
}

#pragma once

//fopen(), FILE:
#include <cstdio>

#include "./output.h"

#include <string>

#ifndef OUTPUT_NO_THREADS
#include "boost/thread.hpp"
#endif

/** Вывод в файл.
Файл открывается-закрывается при каждой записи в него чтобы можно было on-line наблюдать за логами с помощью сторонних текстовых обозревателей (Windows не позволяет открывать один файл сразу в двух программах).
Указанный файл удаляется в конструкторе. Каждая новая запись добавляется в конец файла.*/
class FileOutput: public Output
{
public:
	FileOutput(const char *fileName);
	
	/** Открытие файла; запись в него; закрытие.*/
	void out(const char* msg, const int type);
	
private:

	std::string fileName;

#ifndef OUTPUT_NO_THREADS
	/** Чтобы файл не открывался в двух потоках (вследствие чего файл открывается только в первом потоке - в остальных нет).*/
	boost::mutex outMutex;
#endif
};



#include "file_output.h"

#include "boost/filesystem.hpp"

#include "boost/format.hpp"

#include <boost/date_time/posix_time/posix_time.hpp>

FileOutput::FileOutput(const char *fileName): fileName(fileName)
{
	boost::filesystem::remove(fileName);

	//записываем время создания файла:
	out((to_simple_string(boost::posix_time::second_clock::local_time()).append("\n")).c_str(), 0);
}

void FileOutput::out(const char* message, const int type)
{
	FILE* file;

	const char* mode	= "a+";

#ifndef OUTPUT_NO_THREADS
	outMutex.lock();
#endif

	//разбиваем на различные ОС чтобы не было предупреждений компилятора о небезопасности функции fopen():
#if defined(WINDOWS) || defined(WIN32) || defined(WIN64)
	const int openingResult	= fopen_s(&file, fileName.c_str(), mode);
	if(openingResult != NULL)
	{
		::fputs(str(boost::format("E: fopen_s(\"%s\", \"%s\") failed with error.\n") % fileName % mode).c_str(), stdout);
		file	= NULL;
	}
#else
	file	= fopen(fileName.c_str(), mode);
#endif

	if(file != NULL)
	{
		fputs(message, file);
		fclose(file);
	}

#ifndef OUTPUT_NO_THREADS
	outMutex.unlock();
#endif
}


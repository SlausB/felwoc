
#include "console_output.h"

#include <string>


#if !defined(WINDOWS) && !defined(WIN32) && !defined(__linux__)
#error WINDOWS_or_LINUX_must_be_defined
#endif


#if defined(WINDOWS) || defined(WIN32) || defined(WIN64)

#include <boost/thread.hpp>
/** Для корректного соотнесения изменённых цветов к текстам.*/
static boost::mutex consoleMutex;

#include <windows.h>

/** Хранение первоначальных цветов.*/
class OriginalColorsKeeper
{
public:

	OriginalColorsKeeper()
	{
		HANDLE hConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
		CONSOLE_SCREEN_BUFFER_INFO ConsoleInfo;
		GetConsoleScreenBufferInfo(hConsoleHandle, &ConsoleInfo);
		OriginalColors = ConsoleInfo.wAttributes;
	}

	void ResetToOriginals()
	{
		HANDLE hConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(hConsoleHandle, OriginalColors);
	}

private:

	WORD OriginalColors;
};
static OriginalColorsKeeper originalColorsKeeper;

void SetColor(int attributes)
{
	WORD wColor;
	//We will need this handle to get the current background attribute
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	//We use csbi for the wAttributes word.
	if(GetConsoleScreenBufferInfo(hStdOut, &csbi))
	{
		//Mask out all but the background attribute, and add in the forgournd color
		wColor = (csbi.wAttributes & 0xF0) | attributes;
		SetConsoleTextAttribute(hStdOut, wColor);     
	}
	return;
}

#endif


void ConsoleOutput::out(const char* msg, const int type)
{
	bool colored = false;

#ifdef __linux__
		std::string prefix;
#endif

#if defined(WINDOWS) || defined(WIN32) || defined(WIN64)
		consoleMutex.lock();
#endif

	switch(type)
	{
	case Output::OUTPUT_INFO:
		colored = true;
#if defined(WINDOWS) || defined(WIN32) || defined(WIN64)
		SetColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
#else
		//комментарий и для всех остальных выводов: менять фон (третий атрибут, который здесь отсутствует) не нужно - в разных консолях разный фон по умолчанию, поэтому всегда оставляем только его, в противном случае к нему вернуться будет нельзя:
		prefix.append("\x1B[1;32m");
#endif
		break;

	case Output::OUTPUT_TRACE:
		colored = true;
#if defined(WINDOWS) || defined(WIN32) || defined(WIN64)
		SetColor(FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
#else
		prefix.append("\x1B[2;32m");
#endif
		break;
	
	case Output::OUTPUT_DEBUG:
		colored = true;
#if defined(WINDOWS) || defined(WIN32) || defined(WIN64)
		SetColor(FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
#else
		prefix.append("\x1B[1;37m");
#endif
		break;
	
	case Output::OUTPUT_TODO:
		colored = true;
#if defined(WINDOWS) || defined(WIN32) || defined(WIN64)
		SetColor(FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
#else
		prefix.append("\x1B[1;36m");
#endif
		break;
	
	case Output::OUTPUT_WARNING:
		colored = true;
#if defined(WINDOWS) || defined(WIN32) || defined(WIN64)
		SetColor(FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
#else
		prefix.append("\x1B[1;33m");
#endif
		break;

	case Output::OUTPUT_ERROR:
		colored = true;
#if defined(WINDOWS) || defined(WIN32) || defined(WIN64)
		SetColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
#else
		prefix.append("\x1B[1;31m");
#endif
		break;

	case Output::OUTPUT_FATAL:
		colored = true;
#if defined(WINDOWS) || defined(WIN32) || defined(WIN64)
		SetColor(FOREGROUND_RED | FOREGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY);
#else
		prefix.append("\x1B[4;31m");
#endif
		break;

	case Output::OUTPUT_COMMON:
	default:
		break;
	}
			
	if(colored)
	{
#if defined(WINDOWS) || defined(WIN32) || defined(WIN64)
		fputs(msg, stdout);
		originalColorsKeeper.ResetToOriginals();
#else
		std::string whole;
		whole.append(prefix);
		whole.append(msg);
		whole.append("\x1B[0m");
		fputs(whole.c_str(), stdout);
#endif
	}

	if(!colored)
	{
		fputs(msg, stdout);
	}

#if defined(WINDOWS) || defined(WIN32) || defined(WIN64)
	consoleMutex.unlock();
#endif
}


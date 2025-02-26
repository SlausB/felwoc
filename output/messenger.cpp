
#include "messenger.h"

#include <boost/tokenizer.hpp>

#ifndef OUTPUT_NO_THREADS
#include <boost/thread.hpp>
/** Чтобы выводы из разных потоков не смешивались.*/
static boost::mutex mixingMutex;
#endif//#ifndef OUTPUT_NO_THREADS


Messenger::Messenger()
{
	Init();
}

Messenger::Messenger(Output* output)
{
	Init();
	add(output);
}

Messenger::Messenger(Output* output1, Output* output2)
{
	Init();
	add(output1);
	add(output2);
}

Messenger::Messenger(Output* output1, Output* output2, Output* output3)
{
	Init();
	add(output1);
	add(output2);
	add(output3);
}

void Messenger::write(const boost::format& format)
{
	//boost::format напрямую всё-равно передавать в tokenizer() нельзя - вылетает в run-time с ошибкой "string operator incompatible":
	std::string msg	= str(format);

	parseAndWrite(msg);
}

void Messenger::write(const char* msg)
{
	parseAndWrite(std::string(msg));
}

void Messenger::write(const std::string& msg)
{
	parseAndWrite(msg);
}

void Messenger::write(const boost::format& format, const int type)
{
	//boost::format напрямую всё-равно передавать в tokenizer() нельзя - вылетает в run-time с ошибкой "string operator incompatible":
	std::string msg	= str(format);

	parseAndWrite(msg, type);
}

void Messenger::write(const char* msg, const int type)
{
	parseAndWrite(std::string(msg), type);
}

void Messenger::write(const std::string& msg, const int type)
{
	parseAndWrite(msg, type);
}

void Messenger::operator<<(const boost::format& format)
{
	//boost::format напрямую всё-равно передавать в tokenizer() нельзя - вылетает в run-time с ошибкой "string operator incompatible":
	std::string msg	= str(format);
	parseAndWrite(msg);
}

void Messenger::operator<<(const char* msg)
{
	parseAndWrite(std::string(msg));
}

void Messenger::operator<<(const std::string& msg)
{
	parseAndWrite(msg);
}


void Messenger::common(const boost::format& format)
{
	std::string msg = str(format);
	_write(msg, Output::OUTPUT_COMMON);
}

void Messenger::common(const char* msg)
{
	_write(std::string(msg), Output::OUTPUT_COMMON);
}

void Messenger::common(const std::string& msg)
{
	_write(msg, Output::OUTPUT_COMMON);
}


void Messenger::info(const boost::format& format)
{
	std::string msg = str(format);
	_write(msg, Output::OUTPUT_INFO);
}

void Messenger::info(const char* msg)
{
	_write(std::string(msg), Output::OUTPUT_INFO);
}

void Messenger::info(const std::string& msg)
{
	_write(msg, Output::OUTPUT_INFO);
}


void Messenger::warn(const boost::format& format)
{
	std::string msg = str(format);
	_write(msg, Output::OUTPUT_WARNING);
}

void Messenger::warn(const char* msg)
{
	_write(std::string(msg), Output::OUTPUT_WARNING);
}

void Messenger::warn(const std::string& msg)
{
	_write(msg, Output::OUTPUT_WARNING);
}


void Messenger::warning(const boost::format& format)
{
	std::string msg = str(format);
	_write(msg, Output::OUTPUT_WARNING);
}

void Messenger::warning(const char* msg)
{
	_write(std::string(msg), Output::OUTPUT_WARNING);
}

void Messenger::warning(const std::string& msg)
{
	_write(msg, Output::OUTPUT_WARNING);
}


void Messenger::error(const boost::format& format)
{
	std::string msg = str(format);
	_write(msg, Output::OUTPUT_ERROR);
}

void Messenger::error(const char* msg)
{
	_write(std::string(msg), Output::OUTPUT_ERROR);
}

void Messenger::error(const std::string& msg)
{
	_write(msg, Output::OUTPUT_ERROR);
}


void Messenger::fatal(const boost::format& format)
{
	std::string msg = str(format);
	_write(msg, Output::OUTPUT_FATAL);
}

void Messenger::fatal(const char* msg)
{
	_write(std::string(msg), Output::OUTPUT_FATAL);
}

void Messenger::fatal(const std::string& msg)
{
	_write(msg, Output::OUTPUT_FATAL);
}


void Messenger::trace(const boost::format& format)
{
	std::string msg = str(format);
	_write(msg, Output::OUTPUT_TRACE);
}

void Messenger::trace(const char* msg)
{
	_write(std::string(msg), Output::OUTPUT_TRACE);
}

void Messenger::trace(const std::string& msg)
{
	_write(msg, Output::OUTPUT_TRACE);
}


void Messenger::debug(const boost::format& format)
{
	std::string msg = str(format);
	_write(msg, Output::OUTPUT_DEBUG);
}

void Messenger::debug(const char* msg)
{
	_write(std::string(msg), Output::OUTPUT_DEBUG);
}

void Messenger::debug(const std::string& msg)
{
	_write(msg, Output::OUTPUT_DEBUG);
}


void Messenger::todo(const boost::format& format)
{
	std::string msg = str(format);
	_write(msg, Output::OUTPUT_TODO);
}

void Messenger::todo(const char* msg)
{
	_write(std::string(msg), Output::OUTPUT_TODO);
}

void Messenger::todo(const std::string& msg)
{
	_write(msg, Output::OUTPUT_TODO);
}


void Messenger::set_type(const int preset_type)
{
	presetType = preset_type;
}

void Messenger::reset_type()
{
	presetType = Output::OUTPUT_COMMON;
}



void Messenger::Init()
{
	isOn = true;
	lastType = MESSENGER_ENDL;
	presetType = Output::OUTPUT_COMMON;
}



void Messenger::parseAndWrite(const std::string& msg, const int predefinedType)
{
	int type = Output::OUTPUT_COMMON;
	
	if(predefinedType == Output::OUTPUT_UNDEFINED)
	{
		if(msg.size() >= 3)
		{
			if(msg[1] == ':' && msg[2] == ' ')
			{
				switch(msg[0])
				{
				case 'I':
				case 'i':
					type = Output::OUTPUT_INFO;
					break;
				
				case 'W':
				case 'w':
					type = Output::OUTPUT_WARNING;
					break;
				
				case 'E':
				case 'e':
					type = Output::OUTPUT_ERROR;
					break;
				
				case 'F':
				case 'f':
					type = Output::OUTPUT_FATAL;
					break;
				
				case 'T':
				case 't':
					type = Output::OUTPUT_TRACE;
					break;
				
				case 'D':
				case 'd':
					type = Output::OUTPUT_DEBUG;
					break;
				
				default:
					type = Output::OUTPUT_COMMON;
					break;
				}
			}
			else if(msg.size() >= 6)
			{
				if(msg[4] == ':' && msg[5] == ' ')
				{
					if((msg[0] == 't' || msg[0] == 'T') && (msg[1] == 'o' || msg[1] == 'O') && (msg[2] == 'd' || msg[2] == 'D') && (msg[3] == 'o' || msg[3] == 'O'))
					{
						type = Output::OUTPUT_TODO;
					}
				}
			}
		}
	}
	else
	{
		type = predefinedType;
	}
	
	_write(msg, type);
}

void Messenger::_write(const std::string& msg, int type)
{
	//разбивка на строки:
	typedef boost::tokenizer<boost::char_separator<char> > Tok;
	boost::char_separator<char> sep("", "\n");
	Tok tok(msg, sep);

#ifndef OUTPUT_NO_THREADS	
	mixingMutex.lock();
#endif

	if(type < presetType)
	{
		type = presetType;
	}
	
	for(Tok::iterator tok_iter = tok.begin(); tok_iter != tok.end(); ++tok_iter)
	{
		//если всё в той же строке, то продолжаем писать там где остановились:
		if(lastType == MESSENGER_SIMPLE)
		{
			forEachOutput((*tok_iter).c_str(), type);
		}
		//если с новой строки, то с указанным отступом:
		else
		{
			forEachOutput(str(boost::format("%s%s%s") % tabsIndention % spacesIndention % (*tok_iter)).c_str(), type);
		}
		
		if((*tok_iter).compare("\n") == 0)
		{
			lastType	= MESSENGER_ENDL;
		}
		else
		{
			lastType	= MESSENGER_SIMPLE;
		}
	}
	
#ifndef OUTPUT_NO_THREADS
	mixingMutex.unlock();
#endif
}

/*void Messenger::printf(const char* format, ...) const
{
	if(!isOn)
	{
		return;
	}
	
	std::va_list args;
	char buf[2048];
	
	va_start(args, format);
	//разбиваем на различные ОС чтобы не было предупреждений компилятора о небезопасности функции vsnprintf():
#ifdef WINDOWS
	vsnprintf_s(buf, 2048, 2047, format, args);
#else//#ifdef WINDOWS
	vsnprintf(buf, 2048, format, args);   // avoid buffer overflow
#endif//#ifdef WINDOWS
	va_end(args);
	
	char result[2048];
	//отступ:
	for(int tabs = 0; tabs < currentIndention; tabs++)
	{
		//пишем символ таба (там таб, а не пробел):
		result[tabs]	= '	';
	}
	result[currentIndention]	= '\0';
	//разбиваем на различные ОС чтобы не было предупреждений компилятора о небезопасности функции strcat():
#ifdef WINDOWS
	strcat_s(result, 2048, buf);
#else//#ifdef WINDOWS
	strcat(result, buf);
#endif//#ifdef WINDOWS
	
	fputs(result, file);
	fflush(file);
	forEachListener(result);
	
	::printf(result);
}*/

void Messenger::endl()
{
	forEachOutput("\n", Output::OUTPUT_COMMON);
}

void Messenger::ii()
{
	iit();
}

void Messenger::di()
{
	dit();
}

void Messenger::iit()
{
	tabsIndention.push_back('	');
}

void Messenger::dit()
{
	if(tabsIndention.size() > 0)
	{
		tabsIndention.erase(0, 1);
	}
}

void Messenger::iis()
{
	spacesIndention.push_back(' ');
}

void Messenger::dis()
{
	if(spacesIndention.size() > 0)
	{
		spacesIndention.erase(0, 1);
	}
}

void Messenger::add(Output* output)
{
	outputs.remove(output);
	outputs.push_back(output);
}

void Messenger::remove(Output* output)
{
	outputs.remove(output);
}

void Messenger::forEachOutput(const char* message, const int type)
{
	for(std::list<Output*>::iterator output = outputs.begin(); output != outputs.end(); output++)
	{
		(*output)->out(message, type);
	}
}

void Messenger::on()
{
	isOn	= true;
}

void Messenger::off()
{
	isOn	= false;
}


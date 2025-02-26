
#ifndef MESSENGER_H
#define MESSENGER_H


//va_list
#include <cstdarg>

#include <list>

#include "outputs/output.h"

#include <boost/format.hpp>


/** Вывод отладочных сообщений.*/
class Messenger
{
public:
	/** Конструктор.*/
	Messenger();
	
	/** Сразу же с указанным выводом.*/
	Messenger(Output* output);
	Messenger(Output* output1, Output* output2);
	Messenger(Output* output1, Output* output2, Output* output3);
	
	
	/** Вывод сообщения. Возможно выводить сразу много строк в одной (разделённых '\n') - отступы будут работать корректно.*/
	void write(const boost::format& format);
	void write(const char* msg);
	void write(const std::string& msg);
	/** С предопределённым типов. Мета-префикс будет проигнорирован.*/
	void write(const boost::format& format, const int type);
	void write(const char* msg, const int type);
	void write(const std::string& msg, const int type);
	/** Синоним функции write().*/
	void operator<<(const boost::format& format);
	void operator<<(const char* msg);
	void operator<<(const std::string& msg);
	
	void common(const boost::format& format);
	void common(const char* msg);
	void common(const std::string& msg);
	
	void info(const boost::format& format);
	void info(const char* msg);
	void info(const std::string& msg);
	
	void trace(const boost::format& format);
	void trace(const char* msg);
	void trace(const std::string& msg);
	
	void debug(const boost::format& format);
	void debug(const char* msg);
	void debug(const std::string& msg);
	
	void todo(const boost::format& format);
	void todo(const char* msg);
	void todo(const std::string& msg);
	
	void warn(const boost::format& format);
	void warn(const char* msg);
	void warn(const std::string& msg);
	
	void warning(const boost::format& format);
	void warning(const char* msg);
	void warning(const std::string& msg);
	
	void error(const boost::format& format);
	void error(const char* msg);
	void error(const std::string& msg);
	
	void fatal(const boost::format& format);
	void fatal(const char* msg);
	void fatal(const std::string& msg);
	
	
	/** Выводит символ конца строки.*/
	void endl();
	
	/** Increase indention - увеличить на один текущий отступ табами всех сообщений. Для совместимости с предыдущим кодом. Использовать iit().*/
	void ii();
	/** Decrease indention - уменьшить отступ табами всех сообщений на один. Для совместимости с предыдущим кодом. Использовать dit().*/
	void di();
	/** Increase indention tabs - увеличить на один текущий отступ табами всех сообщений.*/
	void iit();
	/** Decrease indention tabs - уменьшить отступ табами всех сообщений на один.*/
	void dit();
	/** Increase indention spaces - увеличить отступ пробелами для всех сообщений.*/
	void iis();
	/** Decrease indention spaces - уменьшить отступ пробелами для всех сообщений.*/
	void dis();
	
	/** Добавить слушателя - все сообщение будут дублироваться в указанный вывод тоже.*/
	void add(Output* output);
	/** Удалить слушателя - будет выброшен из списка, но, конечно же, не удалён посредством delete.*/
	void remove(Output* output);
	
	/** Включить - печать сообщений возобновится.*/
	void on();
	/** Выключить - прекратится печать.*/
	void off();
	
	/** Порог типов сообщений, ниже которого типы сообщений будут устанавливаться в этот тип.*/
	void set_type(const int preset_type);
	void reset_type();
	
private:

	void Init();

	/** Распарсить текст на тип сообщения по префиксу.*/
	void parseAndWrite(const std::string& msg, const int predefinedType = Output::OUTPUT_UNDEFINED);

	/** Сюда передавать write() и operator<<().*/
	void _write(const std::string& msg, int type);
	
	/** Печать этого сообщения в каждом слушателе.*/
	void forEachOutput(const char* message, const int type);
	
	/** Текущий отступ табами.*/
	std::string tabsIndention;
	/** Текущий отступ пробелами.*/
	std::string spacesIndention;
	
	/** Слушатели - все сообщения дублируются также в указанные выводы.*/
	std::list<Output*> outputs;
	
	/** Включено ли.*/
	bool isOn;

	/** Тип последнего напечатанного символа - для корректного вывода сообщений внутри одной строки.*/
	enum
	{
		MESSENGER_SIMPLE,
		MESSENGER_ENDL,
	};
	int lastType;

	/** Порог типов сообщений, ниже которого типы сообщений будут устанавливаться в этот тип.*/
	int presetType;
};

#endif//MESSENGER_H


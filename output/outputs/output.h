

#ifndef OUTPUT_H
#define OUTPUT_H

/** Для реализации конкретного вывода необходимо наследоваться от этого класса.*/
class Output
{
public:
	/** Вызывается из Messenger'а при выводе сообщения.*/
	virtual void out(const char* msg, const int type) = 0;

	/** Каких типов бывают сообщения.*/
	enum
	{
		OUTPUT_UNDEFINED,	/**< Техническое - не использовать.*/
		OUTPUT_COMMON,	/**< Сообщение будет выведено без модификаций - как если бы было напечатано простым printf() (серым цветом или бледно-белым как у меня в Windows'е и Linux'е соответственно).*/
		OUTPUT_INFO,	/**< Светло-зелёным.*/
		OUTPUT_TRACE,	/**< Белым.*/
		OUTPUT_DEBUG,	/**< Белым.*/
		OUTPUT_TODO,	/**< Сине-зелёный (Cyan).*/
		OUTPUT_WARNING,	/**< Жёлтым.*/
		OUTPUT_ERROR,	/**< Красным.*/
		OUTPUT_FATAL,	/**< Красным и жирным (Linux) или красным на жёлтом фоне (Windows).*/
	};
};

#endif//#ifndef OUTPUT_H


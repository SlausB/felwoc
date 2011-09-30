

#ifndef TABLE_H
#define TABLE_H

class Table
{
public:
	
	Table();
	
	/** Возможные типы вкладок.*/
	enum
	{
		UNTYPED,
		MANY,
		SINGLE,
		PRECISE,
		MORPH,
	};

	/** Тип текущей вкладки.*/
	int type;
};

#endif//#ifndef TABLE_H


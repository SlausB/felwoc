

#ifndef ODS_AS_XML_H
#define ODS_AS_XML_H

#include "../data_source.h"

#include "../../output/messenger.h"

#include <vector>


/** Загрузка таблиц из файла формата .ods, который является, по сути, ZIP-архивом XML-файлов.*/
class OdsAsXml: public DataSource
{
public:

	OdsAsXml(const char* fileName, Messenger* messenger);

	int GetSpreadsheetsCount();

	boost::shared_ptr<Spreadsheet> GetSpreadsheet(const int index);

	bool IsOk();

private:

	bool isOk;

	std::vector<boost::shared_ptr<Spreadsheet> > spreadsheets;
};

#endif//#ifndef ODS_AS_XML_H


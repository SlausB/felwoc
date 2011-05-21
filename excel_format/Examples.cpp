/*
	ExcelFormat Examples.cpp

	Copyright (c) 2009, 2010, 2011 Martin Fuchs <martin-fuchs@gmx.net>

	License: CPOL

	THE WORK (AS DEFINED BELOW) IS PROVIDED UNDER THE TERMS OF THIS CODE PROJECT OPEN LICENSE ("LICENSE").
	THE WORK IS PROTECTED BY COPYRIGHT AND/OR OTHER APPLICABLE LAW. ANY USE OF THE WORK OTHER THAN AS
	AUTHORIZED UNDER THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
	BY EXERCISING ANY RIGHTS TO THE WORK PROVIDED HEREIN, YOU ACCEPT AND AGREE TO BE BOUND BY THE TERMS
	OF THIS LICENSE. THE AUTHOR GRANTS YOU THE RIGHTS CONTAINED HEREIN IN CONSIDERATION OF YOUR ACCEPTANCE
	OF SUCH TERMS AND CONDITIONS. IF YOU DO NOT AGREE TO ACCEPT AND BE BOUND BY THE TERMS OF THIS LICENSE,
	YOU CANNOT MAKE ANY USE OF THE WORK.
*/

#include "ExcelFormat.h"

using namespace ExcelFormat;


#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <crtdbg.h>

#else // _WIN32

#define	FW_NORMAL	400
#define	FW_BOLD		700

#endif // _WIN32


#include "pugixml.hpp"

#include "boost/format.hpp"


static void example1(const char* path)
{
	BasicExcel xls;

	 // create sheet 1 and get the associated BasicExcelWorksheet pointer
	xls.New(1);
	BasicExcelWorksheet* sheet = xls.GetWorksheet(0);

	XLSFormatManager fmt_mgr(xls);


	 // Create a table containing an header row in bold and four rows below.

	ExcelFont font_bold;
	font_bold._weight = FW_BOLD; // 700

	CellFormat fmt_bold(fmt_mgr);
	fmt_bold.set_font(font_bold);

	int col, row = 0;

	for(col=0; col<10; ++col) {
		BasicExcelCell* cell = sheet->Cell(row, col);

		cell->Set("TITLE");
		cell->SetFormat(fmt_bold);
	}

	while(++row < 4) {
		for(int col=0; col<10; ++col)
			sheet->Cell(row, col)->Set("text");
	}


	++row;

	ExcelFont font_red_bold;
	font_red_bold._weight = FW_BOLD;
	font_red_bold._color_index = EGA_RED;

	CellFormat fmt_red_bold(fmt_mgr, font_red_bold);
	fmt_red_bold.set_color1(COLOR1_PAT_SOLID);			// solid background
	fmt_red_bold.set_color2(MAKE_COLOR2(EGA_BLUE,0));	// blue background

	CellFormat fmt_green(fmt_mgr, ExcelFont().set_color_index(EGA_GREEN));

	for(col=0; col<10; ++col) {
		BasicExcelCell* cell = sheet->Cell(row, col);

		cell->Set("xxx");
		cell->SetFormat(fmt_red_bold);

		cell = sheet->Cell(row, ++col);
		cell->Set("yyy");
		cell->SetFormat(fmt_green);
	}


	xls.SaveAs(path);
}


static void example2(const char* path)
{
	BasicExcel xls;

	 // create sheet 1 and get the associated BasicExcelWorksheet pointer
	xls.New(1);
	BasicExcelWorksheet* sheet = xls.GetWorksheet(0);

	XLSFormatManager fmt_mgr(xls);

	ExcelFont font_header;
	font_header.set_weight(FW_BOLD);
	font_header.set_underline_type(EXCEL_UNDERLINE_SINGLE);
	font_header.set_font_name(L"Times New Roman");
	font_header.set_color_index(EGA_BLUE);
	font_header._options = EXCEL_FONT_STRUCK_OUT;

	CellFormat fmt_header(fmt_mgr, font_header);
	fmt_header.set_rotation(30); // rotate the header cell text 30� to the left

	int row = 0;

	for(int col=0; col<10; ++col) {
		BasicExcelCell* cell = sheet->Cell(row, col);

		cell->Set("TITLE");
		cell->SetFormat(fmt_header);
	}

	char buffer[100];

	while(++row < 10) {
		for(int col=0; col<10; ++col) {
			sprintf(buffer, "text %d/%d", row, col);

			sheet->Cell(row, col)->Set(buffer);
		}
	}

	xls.SaveAs(path);
}


static void example3(const char* path)
{
	BasicExcel xls;

	 // create sheet 1 and get the associated BasicExcelWorksheet pointer
	xls.New(1);
	BasicExcelWorksheet* sheet = xls.GetWorksheet(0);

	XLSFormatManager fmt_mgr(xls);
	CellFormat fmt(fmt_mgr);
	BasicExcelCell* cell;


	 // row 1

	fmt.set_format_string(XLS_FORMAT_INTEGER);
	cell = sheet->Cell(0, 0);
	cell->Set(1.);
	cell->SetFormat(fmt);

	fmt.set_format_string(XLS_FORMAT_DECIMAL);
	cell = sheet->Cell(0, 1);
	cell->Set(2.);
	cell->SetFormat(fmt);

	fmt.set_format_string(XLS_FORMAT_DATE);
	fmt.set_font(ExcelFont().set_weight(FW_BOLD));
	cell = sheet->Cell(0, 2);
	cell->Set("03.03.2000");
	cell->SetFormat(fmt);


	 // row 2

	fmt.set_font(ExcelFont().set_weight(FW_NORMAL));
	fmt.set_format_string(XLS_FORMAT_GENERAL);
	cell = sheet->Cell(1, 0);
	cell->Set("normal");
	cell->SetFormat(fmt);

	fmt.set_format_string(XLS_FORMAT_TEXT);
	cell = sheet->Cell(1, 1);
	cell->Set("Text");
	cell->SetFormat(fmt);

	fmt.set_format_string(XLS_FORMAT_GENERAL);
	fmt.set_font(ExcelFont().set_weight(FW_BOLD));
	cell = sheet->Cell(1, 2);
	cell->Set("bold");
	cell->SetFormat(fmt);


	xls.SaveAs(path);
}


static void example_read_write(const char* from, const char* to)
{
	cout << "read " << from << endl;
	BasicExcel xls(from);

	XLSFormatManager fmt_mgr(xls);
	BasicExcelWorksheet* sheet = xls.GetWorksheet(0);

	CellFormat fmt_general(fmt_mgr);

	fmt_general.set_format_string("0.000");

	for(int y=0; y<2; ++y) {
		for(int x=0; x<2; ++x) {
			cout << y << "/" << x;

			BasicExcelCell* cell = sheet->Cell(y, x);

			CellFormat fmt(fmt_mgr, cell);

//			cout << " - xf_idx=" << cell->GetXFormatIdx();

			const Workbook::Font& font = fmt_mgr.get_font(fmt);
			string font_name = stringFromSmallString(font.name_);
			cout << "  font name: " << font_name;

			const wstring& fmt_string = fmt.get_format_string();
			cout << "  format: " << narrow_string(fmt_string);

			cell->SetFormat(fmt_general);

			cout << endl;
		}
	}

	cout << "write: " << from << endl;
	xls.SaveAs(to);
}


static void example4(const char* path)
{
	BasicExcel xls;

	xls.New(1);
	BasicExcelWorksheet* sheet = xls.GetWorksheet(0);

	XLSFormatManager fmt_mgr(xls);

	char buffer[100];
	int i = 0;

	for(int row=0; row<8; ++row) {
		int color = i++;
		int height = 100 * i;

		sprintf(buffer, "Times New Roman %d", height/20);

		ExcelFont font;
		font.set_color_index(color);
		font.set_height(height);
		font.set_font_name(L"Times New Roman");

		CellFormat fmt(fmt_mgr, font);
		fmt.set_background(MAKE_COLOR2(EGA_MAGENTA,0));	// solid magenta background

		BasicExcelCell* cell = sheet->Cell(row, 0);
		cell->Set(buffer);
		cell->SetFormat(fmt);
	}

	xls.SaveAs(path);
}


static void copy_sheet(const char* from, const char* to)
{
	BasicExcel xls;

	xls.Load(from);
	xls.SaveAs(to);
}


static void write_big_sheet(const char* path, const int row_max, const int col_max)
{
	BasicExcel xls;
	char buffer[16];

	 // create sheet 1 and get the associated BasicExcelWorksheet pointer
	xls.New(1);
	BasicExcelWorksheet* sheet = xls.GetWorksheet(0);

	XLSFormatManager fmt_mgr(xls);


	 // Create a table containing header row and column in bold.

	ExcelFont font_bold;
	font_bold._weight = FW_BOLD; // 700

	CellFormat fmt_bold(fmt_mgr);
	fmt_bold.set_font(font_bold);

	int col, row;

	BasicExcelCell* cell = sheet->Cell(0, 0);
	cell->Set("Row / Column");
	cell->SetFormat(fmt_bold);

	for(col=1; col<=col_max; ++col) {
		cell = sheet->Cell(0, col);

		sprintf(buffer, "Column %d", col);
		cell->Set(buffer);
		cell->SetFormat(fmt_bold);
	}

	for(row=1; row<=row_max; ++row) {
		cell = sheet->Cell(row, 0);

		sprintf(buffer, "Row %d", row);
		cell->Set(buffer);
		cell->SetFormat(fmt_bold);
	}

	for(row=1; row<=row_max; ++row) {
		for(int col=1; col<=col_max; ++col) {
			sprintf(buffer, "%d / %d", row, col);

			sheet->Cell(row, col)->Set(buffer);
		}
	}

	xls.SaveAs(path);
}

#include <map>

void readDesign()
{
	BasicExcel xls("design2.xls");

	const int totalWorkSheets	= xls.GetTotalWorkSheets();

	//XML:
	pugi::xml_document xml;

	for(int i = 0; i < totalWorkSheets; i++)
	{
		BasicExcelWorksheet* worksheet = xls.GetWorksheet(i);

		std::string worksheetName(worksheet->GetAnsiSheetName());

		const int rowsCount = worksheet->GetTotalRows();
		const int columnsCount = worksheet->GetTotalCols();

		if(rowsCount < 6)
		{
			printf("WARNING: spreadsheet \"%s\" must have at least 6 rows.\n", worksheetName.c_str());
			continue;
		}
		if(columnsCount < 2)
		{
			printf("WARNING: spreadsheet \"%s\" must have at least 2 columns.\n", worksheetName.c_str());
			continue;
		}

		pugi::xml_node spreadsheetNode = xml.append_child(worksheetName.c_str());

		//��������� ������ ������ ���������� �� ��������� (�������):
		std::map<int, bool> columnsPermissions;

		//�������� ���������:
		std::map<int, std::string> attributes;

		for(int row = 0; row < rowsCount; row++)
		{
			//������ ������ ������ - ��� �������������� ���� �������� flash/server/script...:
			if(row < 4)
			{
				for(int column = 0; column < columnsCount; column++)
				{
					BasicExcelCell* cell = worksheet->Cell(row, column);

					//������ ������� ��� �������������� ������������:
					if(column < 1)
					{

					}
					else
					{
						if(cell->Type() == BasicExcelCell::INT)
						{
							columnsPermissions[column] = cell->GetInteger() != 0;
						}
						else
						{
							columnsPermissions[column] = false;
						}
					}
				}
			}
			//���� �� ������� ������ ��� ������� ���������:
			else if(row == 4)
			{
				for(int column = 1; column < columnsCount; column++)
				{
					BasicExcelCell* cell = worksheet->Cell(row, column);

					if(cell->Type() == BasicExcelCell::UNDEFINED)
					{
						continue;
					}

					attributes[column] = cell->GetString();
				}
			}
			else
			{
				//���� ��� ������� ������ �� �������� ������������:
				if(worksheet->Cell(row, 0)->GetInteger() != 0)
				{
					pugi::xml_node itemNode = spreadsheetNode.append_child("item");

					//������ ������� �������������� ��� �������������� ������������:
					for(int column = 1; column < columnsCount; column++)
					{
						BasicExcelCell* cell = worksheet->Cell(row, column);

						//���� �������-������� ������� ������ �� �������� ������������:
						if(columnsPermissions[column] == true)
						{
							if(cell->Type() == BasicExcelCell::UNDEFINED)
							{
								printf("ERROR: cell at row %d and column %d is undefined! Nothing will be generated.\n", row + 1, column + 1);
								return;
							}

							pugi::xml_attribute attribute = itemNode.append_attribute(attributes[column].c_str());

							switch(cell->Type())
							{
							case BasicExcelCell::UNDEFINED:
								attribute.set_value("__undefined__");
								break;
							case BasicExcelCell::INT:
								attribute.set_value(str(boost::format("%d") % cell->GetInteger()).c_str());
								break;
							case BasicExcelCell::DOUBLE:
								attribute.set_value(str(boost::format("%f") % cell->GetDouble()).c_str());
								break;
							case BasicExcelCell::STRING:
								attribute.set_value(cell->GetString());
								break;
							case BasicExcelCell::WSTRING:
								attribute.set_value("__wchar__");
								break;
							case BasicExcelCell::FORMULA:
								{
									const char* formulaAsString = cell->GetString();
									//�������� ������� ������������ � ���� ������ ������ ���� �������� �� ��� ������:
									if(strlen(formulaAsString) <= 0)
									{
										attribute.set_value(str(boost::format("%f") % cell->GetDouble()).c_str());
									}
									//��������� ������� ����� �������������� ������:
									else
									{
										attribute.set_value(formulaAsString);
									}
								}
								break;
							default:
								printf("UNKNOWN		");
								attribute.set_value("__unknown__");
								break;
							}
						}
					}
				}
			}
		}

		/*for(int row = 0; row < rowsCount; row++)
		{
			printf("	");

			for(int column = 0; column < columnsCount; column++)
			{
				BasicExcelCell* cell = worksheet->Cell(row, column);

				switch(cell->Type())
				{
				case BasicExcelCell::UNDEFINED:
					printf("UNDEFINED		");
					break;
				case BasicExcelCell::INT:
					printf("INT: %d		", cell->GetInteger());
					break;
				case BasicExcelCell::DOUBLE:
					printf("DOUBLE: %f		", (float)cell->GetDouble());
					break;
				case BasicExcelCell::STRING:
					printf("STRING: %s		", cell->GetString());
					break;
				case BasicExcelCell::WSTRING:
					printf("WSTRING		");
					break;
				case BasicExcelCell::FORMULA:
					printf("FORMULA: %f		", (float)cell->GetDouble());
					break;
				default:
					printf("UNKNOWN		");
					break;
				}
			}

			printf("\n");
		}*/
	}


	xml.save_file("design.xml");



	/* // create sheet 1 and get the associated BasicExcelWorksheet pointer
	xls.New(1);
	BasicExcelWorksheet* sheet = xls.GetWorksheet(0);

	XLSFormatManager fmt_mgr(xls);
	CellFormat fmt(fmt_mgr);
	BasicExcelCell* cell;


	 // row 1

	fmt.set_format_string(XLS_FORMAT_INTEGER);
	cell = sheet->Cell(0, 0);
	cell->Set(1.);
	cell->SetFormat(fmt);

	fmt.set_format_string(XLS_FORMAT_DECIMAL);
	cell = sheet->Cell(0, 1);
	cell->Set(2.);
	cell->SetFormat(fmt);

	fmt.set_format_string(XLS_FORMAT_DATE);
	fmt.set_font(ExcelFont().set_weight(FW_BOLD));
	cell = sheet->Cell(0, 2);
	cell->Set("03.03.2000");
	cell->SetFormat(fmt);


	 // row 2

	fmt.set_font(ExcelFont().set_weight(FW_NORMAL));
	fmt.set_format_string(XLS_FORMAT_GENERAL);
	cell = sheet->Cell(1, 0);
	cell->Set("normal");
	cell->SetFormat(fmt);

	fmt.set_format_string(XLS_FORMAT_TEXT);
	cell = sheet->Cell(1, 1);
	cell->Set("Text");
	cell->SetFormat(fmt);

	fmt.set_format_string(XLS_FORMAT_GENERAL);
	fmt.set_font(ExcelFont().set_weight(FW_BOLD));
	cell = sheet->Cell(1, 2);
	cell->Set("bold");
	cell->SetFormat(fmt);


	xls.SaveAs(path);*/
}


int main(int argc, char** argv)
{
/*#ifdef _MSC_VER
	 // detect memory leaks
//	_CrtSetDbgFlag(_CrtSetDbgFlag(0)|_CRTDBG_LEAK_CHECK_DF);
#endif


	 // call example1()
	example1("example1.xls");

	 // call example2()
	example2("example2.xls");

	 // call example3()
	example3("example3.xls");

	 // dump out cell contents of example3.xls and write modified to example3-out.xls
	example_read_write("example3.xls", "example3-out.xls");

	 // call example4()
	example4("example4.xls");

	 // create a table containing 500 x 100 cells
	write_big_sheet("big-example.xls", 500, 100);


#ifdef _WIN32
	 // open the output files in Excel
	ShellExecute(0, NULL, "example1.xls", NULL, NULL, SW_NORMAL);
	ShellExecute(0, NULL, "example2.xls", NULL, NULL, SW_NORMAL);
	ShellExecute(0, NULL, "example3.xls", NULL, NULL, SW_NORMAL);
	ShellExecute(0, NULL, "example3.out.xls", NULL, NULL, SW_NORMAL);
	ShellExecute(0, NULL, "example4.xls", NULL, NULL, SW_NORMAL);
	ShellExecute(0, NULL, "big-example.xls", NULL, NULL, SW_NORMAL);
#endif*/

	readDesign();

	getchar();

	return 0;
}

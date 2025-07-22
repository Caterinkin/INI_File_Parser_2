#include <iostream>
#include <locale>
#include "ini_parser.h"

int main()
{
    try
    {
        // ��������� ������ ��� ��������� �������� �����
        setlocale(LC_ALL, "Russian");
        std::locale::global(std::locale(""));

        // ��� ����� ������������
        const std::string config_file = "config.ini";

        // ������� ������ � �������������� ��������� ������� ��� �������������
        ini_parser parser(config_file, true);

        // ������� ������ ��������
        try
        {
            // ������ �������� �� Section1
            int var1 = parser.get_value<int>("Section1.var1");
            std::string var2 = parser.get_value<std::string>("Section1.var2");

            // ������ �������� �� Section2
            int var1_sec2 = parser.get_value<int>("Section2.var1");
            std::string var2_sec2 = parser.get_value<std::string>("Section2.var2");

            // ����� ��������
            std::cout << "Section1.var1 = " << var1 << std::endl;
            std::cout << "Section1.var2 = " << var2 << std::endl;
            std::cout << "Section2.var1 = " << var1_sec2 << std::endl;
            std::cout << "Section2.var2 = " << var2_sec2 << std::endl;

            // ������ ������ bool �������� (�������� � ������ �������� ���� bool)
            // bool debug = parser.get_value<bool>("Section1.debug_mode");
            // std::cout << "Debug mode: " << std::boolalpha << debug << std::endl;
        }
        catch (const ini_parser_error& e)
        {
            std::cerr << "������ ������ ��������: " << e.what() << std::endl;
            return 1;
        }
    }
    catch (const ini_parser_error& e)
    {
        std::cerr << "������ INI-�������: " << e.what() << std::endl;
        return 1;
    }
    catch (const std::exception& e)
    {
        std::cerr << "����������� ������: " << e.what() << std::endl;
        return 2;
    }

    return 0;
}
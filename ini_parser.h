#pragma once

#include <string>
#include <map>
#include <vector>
#include <stdexcept>
#include <type_traits>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <fstream>

// ����� ��� ��������� ������ �������
class ini_parser_error : public std::runtime_error
{
public:
    // ����������� � ���������� �� ������ � �������������� ������� ������
    explicit ini_parser_error(const std::string& msg, int line = -1)
        : std::runtime_error(line == -1 ? msg : "������ � ������ " + std::to_string(line) + ": " + msg)
    {
    }
};

// �������� ����� ������� INI-������
class ini_parser
{
private:
    // ��������� ��� �������� ������: ������ -> (���� -> ��������)
    std::map<std::string, std::map<std::string, std::string>> data;

    // ��� ����� ������������
    std::string filename;

    // ���� ������������� ���������� ������������
    bool use_default_config;

    // ��������������� ������
    std::string trim(const std::string& str); // �������� ��������
    std::pair<std::string, std::string> split_key_value(const std::string& line, int line_num); // ���������� ����� � ��������
    void validate_section_name(const std::string& name, int line_num); // �������� ����� ������
    void validate_key_name(const std::string& name, int line_num); // �������� ����� �����
    void parse_file(std::istream& stream); // �������� ����� ��������
    std::string get_value_as_string(const std::string& key_path); // ��������� ���������� ��������

    // ��������� ������� �������������� ������ � ������ ���
    template<typename T>
    T convert_value(const std::string& str) const
    {
        static_assert(sizeof(T) == -1, "�� ����������� �������������� ��� ����� ����");
    }

public:
    // ����������� � ������������ �������� ������� �� ���������
    explicit ini_parser(const std::string& filename, bool create_default = false);

    // ��������� ����� ��� ��������� ��������
    template<typename T>
    T get_value(const std::string& key_path)
    {
        std::string str_value = get_value_as_string(key_path);
        return convert_value<T>(str_value);
    }

    // ����������� ����� ��� �������� ������� �� ���������
    static void create_default_config(const std::string& filename);
};

// ������������� ��� ����� �����
template<>
inline int ini_parser::convert_value<int>(const std::string& str) const
{
    try
    {
        return std::stoi(str);
    }
    catch (...)
    {
        throw ini_parser_error("�� ������� ������������� '" + str + "' � int");
    }
}

// ������������� ��� ����� double
template<>
inline double ini_parser::convert_value<double>(const std::string& str) const
{
    try
    {
        // �������� ������� �� ����� ��� ����������� ��������
        std::string normalized = str;
        std::replace(normalized.begin(), normalized.end(), ',', '.');
        return std::stod(normalized);
    }
    catch (...)
    {
        throw ini_parser_error("�� ������� ������������� '" + str + "' � double");
    }
}

// ������������� ��� ����� float
template<>
inline float ini_parser::convert_value<float>(const std::string& str) const
{
    try
    {
        std::string normalized = str;
        std::replace(normalized.begin(), normalized.end(), ',', '.');
        return std::stof(normalized);
    }
    catch (...)
    {
        throw ini_parser_error("�� ������� ������������� '" + str + "' � float");
    }
}

// ������������� ��� �����
template<>
inline std::string ini_parser::convert_value<std::string>(const std::string& str) const
{
    return str;
}

// ������������� ��� ������� ��������
template<>
inline bool ini_parser::convert_value<bool>(const std::string& str) const
{
    std::string lower;
    lower.reserve(str.size());

    // �������� � ������� �������� ��� ���������
    for (char c : str)
    {
        lower.push_back(std::tolower(c));
    }

    // ������������ ������ ������� true
    if (lower == "true" || lower == "1" || lower == "yes" || lower == "on")
    {
        return true;
    }

    // ������������ ������ ������� false
    if (lower == "false" || lower == "0" || lower == "no" || lower == "off")
    {
        return false;
    }

    throw ini_parser_error("�� ������� ������������� '" + str + "' � bool");
}
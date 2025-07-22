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

// Класс для обработки ошибок парсера
class ini_parser_error : public std::runtime_error
{
public:
    // Конструктор с сообщением об ошибке и необязательным номером строки
    explicit ini_parser_error(const std::string& msg, int line = -1)
        : std::runtime_error(line == -1 ? msg : "Ошибка в строке " + std::to_string(line) + ": " + msg)
    {
    }
};

// Основной класс парсера INI-файлов
class ini_parser
{
private:
    // Структура для хранения данных: секция -> (ключ -> значение)
    std::map<std::string, std::map<std::string, std::string>> data;

    // Имя файла конфигурации
    std::string filename;

    // Флаг использования встроенной конфигурации
    bool use_default_config;

    // Вспомогательные методы
    std::string trim(const std::string& str); // Удаление пробелов
    std::pair<std::string, std::string> split_key_value(const std::string& line, int line_num); // Разделение ключа и значения
    void validate_section_name(const std::string& name, int line_num); // Проверка имени секции
    void validate_key_name(const std::string& name, int line_num); // Проверка имени ключа
    void parse_file(std::istream& stream); // Основной метод парсинга
    std::string get_value_as_string(const std::string& key_path); // Получение строкового значения

    // Шаблонная функция преобразования строки в нужный тип
    template<typename T>
    T convert_value(const std::string& str) const
    {
        static_assert(sizeof(T) == -1, "Не реализовано преобразование для этого типа");
    }

public:
    // Конструктор с возможностью создания конфига по умолчанию
    explicit ini_parser(const std::string& filename, bool create_default = false);

    // Шаблонный метод для получения значения
    template<typename T>
    T get_value(const std::string& key_path)
    {
        std::string str_value = get_value_as_string(key_path);
        return convert_value<T>(str_value);
    }

    // Статический метод для создания конфига по умолчанию
    static void create_default_config(const std::string& filename);
};

// Специализация для целых чисел
template<>
inline int ini_parser::convert_value<int>(const std::string& str) const
{
    try
    {
        return std::stoi(str);
    }
    catch (...)
    {
        throw ini_parser_error("Не удалось преобразовать '" + str + "' в int");
    }
}

// Специализация для чисел double
template<>
inline double ini_parser::convert_value<double>(const std::string& str) const
{
    try
    {
        // Заменяем запятые на точки для корректного парсинга
        std::string normalized = str;
        std::replace(normalized.begin(), normalized.end(), ',', '.');
        return std::stod(normalized);
    }
    catch (...)
    {
        throw ini_parser_error("Не удалось преобразовать '" + str + "' в double");
    }
}

// Специализация для чисел float
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
        throw ini_parser_error("Не удалось преобразовать '" + str + "' в float");
    }
}

// Специализация для строк
template<>
inline std::string ini_parser::convert_value<std::string>(const std::string& str) const
{
    return str;
}

// Специализация для булевых значений
template<>
inline bool ini_parser::convert_value<bool>(const std::string& str) const
{
    std::string lower;
    lower.reserve(str.size());

    // Приводим к нижнему регистру для сравнения
    for (char c : str)
    {
        lower.push_back(std::tolower(c));
    }

    // Поддерживаем разные форматы true
    if (lower == "true" || lower == "1" || lower == "yes" || lower == "on")
    {
        return true;
    }

    // Поддерживаем разные форматы false
    if (lower == "false" || lower == "0" || lower == "no" || lower == "off")
    {
        return false;
    }

    throw ini_parser_error("Не удалось преобразовать '" + str + "' в bool");
}

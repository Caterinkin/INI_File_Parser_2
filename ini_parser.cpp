#include <iostream>
#include "ini_parser.h"
#include <locale>
#include <codecvt>

// Удаление пробелов в начале и конце строки
std::string ini_parser::trim(const std::string& str)
{
    // Находим первый непробельный символ
    size_t first = str.find_first_not_of(" \t");

    // Если строка состоит только из пробелов
    if (first == std::string::npos)
    {
        return "";
    }

    // Находим последний непробельный символ
    size_t last = str.find_last_not_of(" \t");
    return str.substr(first, (last - first + 1));
}

// Разделение строки на ключ и значение
std::pair<std::string, std::string> ini_parser::split_key_value(const std::string& line, int line_num)
{
    // Ищем позицию знака равенства
    size_t eq_pos = line.find('=');

    // Если знак равенства не найден
    if (eq_pos == std::string::npos)
    {
        throw ini_parser_error("Некорректный формат строки (отсутствует '=')", line_num);
    }

    // Извлекаем и тримим ключ
    std::string key = trim(line.substr(0, eq_pos));

    // Проверяем что ключ не пустой
    if (key.empty())
    {
        throw ini_parser_error("Пустой ключ", line_num);
    }

    // Извлекаем и тримим значение
    std::string value = trim(line.substr(eq_pos + 1));
    return std::make_pair(key, value);
}

// Проверка корректности имени секции
void ini_parser::validate_section_name(const std::string& name, int line_num)
{
    // Имя секции не может быть пустым
    if (name.empty())
    {
        throw ini_parser_error("Пустое имя секции", line_num);
    }

    // Имя секции не должно содержать пробелов
    for (char c : name)
    {
        if (std::isspace(c))
        {
            throw ini_parser_error("Имя секции содержит пробелы", line_num);
        }
    }
}

// Проверка корректности имени ключа
void ini_parser::validate_key_name(const std::string& name, int line_num)
{
    // Ключ не может быть пустым
    if (name.empty())
    {
        throw ini_parser_error("Пустой ключ", line_num);
    }

    // Ключ не должен содержать пробелов
    for (char c : name)
    {
        if (std::isspace(c))
        {
            throw ini_parser_error("Ключ содержит пробелы", line_num);
        }
    }
}

// Основной метод парсинга INI-файла
void ini_parser::parse_file(std::istream& stream)
{
    std::string current_section;
    std::string line;
    int line_num = 0;

    // Читаем файл построчно
    while (std::getline(stream, line))
    {
        line_num++;
        line = trim(line);

        // Пропускаем пустые строки и комментарии
        if (line.empty() || line[0] == ';')
        {
            continue;
        }

        // Обработка секции
        if (line[0] == '[')
        {
            // Проверяем что секция закрыта
            if (line.back() != ']')
            {
                throw ini_parser_error("Некорректное объявление секции - отсутствует ']'", line_num);
            }

            // Извлекаем имя секции
            current_section = trim(line.substr(1, line.size() - 2));
            validate_section_name(current_section, line_num);

            // Добавляем секцию в данные
            data[current_section];
            continue;
        }

        // Проверяем что ключ-значение находится внутри секции
        if (current_section.empty())
        {
            throw ini_parser_error("Ключ-значение вне секции", line_num);
        }

        // Разделяем ключ и значение
        auto kv_pair = split_key_value(line, line_num);
        validate_key_name(kv_pair.first, line_num);

        // Сохраняем значение
        data[current_section][kv_pair.first] = kv_pair.second;
    }
}

// Конструктор парсера
ini_parser::ini_parser(const std::string& filename, bool create_default)
    : filename(filename), use_default_config(false)
{
    // Пытаемся открыть файл
    std::ifstream file(filename);

    if (!file)
    {
        // Если файл не найден и разрешено создание по умолчанию
        if (create_default)
        {
            // Используем встроенную конфигурацию
            std::istringstream default_config(R"(
[Section1]
; Пример секции с русскими комментариями
var1 = 5
var2 = Привет, мир!

[Section2]
var1 = 42
var2 = Тестовая строка
)");
            use_default_config = true;
            parse_file(default_config);
            create_default_config(filename);
        }
        else
        {
            throw ini_parser_error("Не удалось открыть файл: " + filename);
        }
    }
    else
    {
        // Парсим существующий файл
        parse_file(file);
    }
}

// Получение строкового значения по ключу
std::string ini_parser::get_value_as_string(const std::string& key_path)
{
    // Разделяем путь на секцию и ключ
    size_t dot_pos = key_path.find('.');

    if (dot_pos == std::string::npos)
    {
        throw ini_parser_error("Некорректный формат ключа (отсутствует '.')");
    }

    std::string section = key_path.substr(0, dot_pos);
    std::string key = key_path.substr(dot_pos + 1);

    // Проверяем что секция и ключ не пустые
    if (section.empty() || key.empty())
    {
        throw ini_parser_error("Пустое имя секции или ключа");
    }

    // Ищем секцию
    auto section_it = data.find(section);

    if (section_it == data.end())
    {
        // Формируем список доступных секций для сообщения об ошибке
        std::vector<std::string> available_sections;

        for (const auto& [sec, _] : data)
        {
            available_sections.push_back(sec);
        }

        std::string hint = "Доступные секции: ";

        for (size_t i = 0; i < available_sections.size(); ++i)
        {
            if (i != 0)
            {
                hint += ", ";
            }
            hint += available_sections[i];
        }

        throw ini_parser_error("Секция '" + section + "' не найдена. " + hint);
    }

    // Ищем ключ в секции
    auto key_it = section_it->second.find(key);

    if (key_it == section_it->second.end())
    {
        // Формируем список доступных ключей для сообщения об ошибке
        std::vector<std::string> available_keys;

        for (const auto& [k, _] : section_it->second)
        {
            available_keys.push_back(k);
        }

        std::string hint = "Доступные ключи в секции '" + section + "': ";

        for (size_t i = 0; i < available_keys.size(); ++i)
        {
            if (i != 0)
            {
                hint += ", ";
            }
            hint += available_keys[i];
        }

        throw ini_parser_error("Ключ '" + key + "' не найден в секции '" + section + "'. " + hint);
    }

    return key_it->second;
}

// Создание конфигурационного файла по умолчанию
void ini_parser::create_default_config(const std::string& filename)
{
    std::ofstream out(filename);

    if (out)
    {
        out << R"(
[Section1]
; Пример секции с русскими комментариями
var1 = 5
var2 = Привет, мир!

[Section2]
var1 = 42
var2 = Тестовая строка
)";
        std::cout << "Создан новый конфиг файл: " << filename << std::endl;
    }
    else
    {
        throw ini_parser_error("Не удалось создать файл конфигурации");
    }
}
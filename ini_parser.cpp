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
}#include <iostream>
#include "ini_parser.h"
#include <locale>
#include <codecvt>

// Óäàëåíèå ïðîáåëîâ â íà÷àëå è êîíöå ñòðîêè
std::string ini_parser::trim(const std::string& str)
{
    // Íàõîäèì ïåðâûé íåïðîáåëüíûé ñèìâîë
    size_t first = str.find_first_not_of(" \t");

    // Åñëè ñòðîêà ñîñòîèò òîëüêî èç ïðîáåëîâ
    if (first == std::string::npos)
    {
        return "";
    }

    // Íàõîäèì ïîñëåäíèé íåïðîáåëüíûé ñèìâîë
    size_t last = str.find_last_not_of(" \t");
    return str.substr(first, (last - first + 1));
}

// Ðàçäåëåíèå ñòðîêè íà êëþ÷ è çíà÷åíèå
std::pair<std::string, std::string> ini_parser::split_key_value(const std::string& line, int line_num)
{
    // Èùåì ïîçèöèþ çíàêà ðàâåíñòâà
    size_t eq_pos = line.find('=');

    // Åñëè çíàê ðàâåíñòâà íå íàéäåí
    if (eq_pos == std::string::npos)
    {
        throw ini_parser_error("Íåêîððåêòíûé ôîðìàò ñòðîêè (îòñóòñòâóåò '=')", line_num);
    }

    // Èçâëåêàåì è òðèìèì êëþ÷
    std::string key = trim(line.substr(0, eq_pos));

    // Ïðîâåðÿåì ÷òî êëþ÷ íå ïóñòîé
    if (key.empty())
    {
        throw ini_parser_error("Ïóñòîé êëþ÷", line_num);
    }

    // Èçâëåêàåì è òðèìèì çíà÷åíèå
    std::string value = trim(line.substr(eq_pos + 1));
    return std::make_pair(key, value);
}

// Ïðîâåðêà êîððåêòíîñòè èìåíè ñåêöèè
void ini_parser::validate_section_name(const std::string& name, int line_num)
{
    // Èìÿ ñåêöèè íå ìîæåò áûòü ïóñòûì
    if (name.empty())
    {
        throw ini_parser_error("Ïóñòîå èìÿ ñåêöèè", line_num);
    }

    // Èìÿ ñåêöèè íå äîëæíî ñîäåðæàòü ïðîáåëîâ
    for (char c : name)
    {
        if (std::isspace(c))
        {
            throw ini_parser_error("Èìÿ ñåêöèè ñîäåðæèò ïðîáåëû", line_num);
        }
    }
}

// Ïðîâåðêà êîððåêòíîñòè èìåíè êëþ÷à
void ini_parser::validate_key_name(const std::string& name, int line_num)
{
    // Êëþ÷ íå ìîæåò áûòü ïóñòûì
    if (name.empty())
    {
        throw ini_parser_error("Ïóñòîé êëþ÷", line_num);
    }

    // Êëþ÷ íå äîëæåí ñîäåðæàòü ïðîáåëîâ
    for (char c : name)
    {
        if (std::isspace(c))
        {
            throw ini_parser_error("Êëþ÷ ñîäåðæèò ïðîáåëû", line_num);
        }
    }
}

// Îñíîâíîé ìåòîä ïàðñèíãà INI-ôàéëà
void ini_parser::parse_file(std::istream& stream)
{
    std::string current_section;
    std::string line;
    int line_num = 0;

    // ×èòàåì ôàéë ïîñòðî÷íî
    while (std::getline(stream, line))
    {
        line_num++;
        line = trim(line);

        // Ïðîïóñêàåì ïóñòûå ñòðîêè è êîììåíòàðèè
        if (line.empty() || line[0] == ';')
        {
            continue;
        }

        // Îáðàáîòêà ñåêöèè
        if (line[0] == '[')
        {
            // Ïðîâåðÿåì ÷òî ñåêöèÿ çàêðûòà
            if (line.back() != ']')
            {
                throw ini_parser_error("Íåêîððåêòíîå îáúÿâëåíèå ñåêöèè - îòñóòñòâóåò ']'", line_num);
            }

            // Èçâëåêàåì èìÿ ñåêöèè
            current_section = trim(line.substr(1, line.size() - 2));
            validate_section_name(current_section, line_num);

            // Äîáàâëÿåì ñåêöèþ â äàííûå
            data[current_section];
            continue;
        }

        // Ïðîâåðÿåì ÷òî êëþ÷-çíà÷åíèå íàõîäèòñÿ âíóòðè ñåêöèè
        if (current_section.empty())
        {
            throw ini_parser_error("Êëþ÷-çíà÷åíèå âíå ñåêöèè", line_num);
        }

        // Ðàçäåëÿåì êëþ÷ è çíà÷åíèå
        auto kv_pair = split_key_value(line, line_num);
        validate_key_name(kv_pair.first, line_num);

        // Ñîõðàíÿåì çíà÷åíèå
        data[current_section][kv_pair.first] = kv_pair.second;
    }
}

// Êîíñòðóêòîð ïàðñåðà
ini_parser::ini_parser(const std::string& filename, bool create_default)
    : filename(filename), use_default_config(false)
{
    // Ïûòàåìñÿ îòêðûòü ôàéë
    std::ifstream file(filename);

    if (!file)
    {
        // Åñëè ôàéë íå íàéäåí è ðàçðåøåíî ñîçäàíèå ïî óìîë÷àíèþ
        if (create_default)
        {
            // Èñïîëüçóåì âñòðîåííóþ êîíôèãóðàöèþ
            std::istringstream default_config(R"(
[Section1]
; Ïðèìåð ñåêöèè ñ ðóññêèìè êîììåíòàðèÿìè
var1 = 5
var2 = Ïðèâåò, ìèð!

[Section2]
var1 = 42
var2 = Òåñòîâàÿ ñòðîêà
)");
            use_default_config = true;
            parse_file(default_config);
            create_default_config(filename);
        }
        else
        {
            throw ini_parser_error("Íå óäàëîñü îòêðûòü ôàéë: " + filename);
        }
    }
    else
    {
        // Ïàðñèì ñóùåñòâóþùèé ôàéë
        parse_file(file);
    }
}

// Ïîëó÷åíèå ñòðîêîâîãî çíà÷åíèÿ ïî êëþ÷ó
std::string ini_parser::get_value_as_string(const std::string& key_path)
{
    // Ðàçäåëÿåì ïóòü íà ñåêöèþ è êëþ÷
    size_t dot_pos = key_path.find('.');

    if (dot_pos == std::string::npos)
    {
        throw ini_parser_error("Íåêîððåêòíûé ôîðìàò êëþ÷à (îòñóòñòâóåò '.')");
    }

    std::string section = key_path.substr(0, dot_pos);
    std::string key = key_path.substr(dot_pos + 1);

    // Ïðîâåðÿåì ÷òî ñåêöèÿ è êëþ÷ íå ïóñòûå
    if (section.empty() || key.empty())
    {
        throw ini_parser_error("Ïóñòîå èìÿ ñåêöèè èëè êëþ÷à");
    }

    // Èùåì ñåêöèþ
    auto section_it = data.find(section);

    if (section_it == data.end())
    {
        // Ôîðìèðóåì ñïèñîê äîñòóïíûõ ñåêöèé äëÿ ñîîáùåíèÿ îá îøèáêå
        std::vector<std::string> available_sections;

        for (const auto& [sec, _] : data)
        {
            available_sections.push_back(sec);
        }

        std::string hint = "Äîñòóïíûå ñåêöèè: ";

        for (size_t i = 0; i < available_sections.size(); ++i)
        {
            if (i != 0)
            {
                hint += ", ";
            }
            hint += available_sections[i];
        }

        throw ini_parser_error("Ñåêöèÿ '" + section + "' íå íàéäåíà. " + hint);
    }

    // Èùåì êëþ÷ â ñåêöèè
    auto key_it = section_it->second.find(key);

    if (key_it == section_it->second.end())
    {
        // Ôîðìèðóåì ñïèñîê äîñòóïíûõ êëþ÷åé äëÿ ñîîáùåíèÿ îá îøèáêå
        std::vector<std::string> available_keys;

        for (const auto& [k, _] : section_it->second)
        {
            available_keys.push_back(k);
        }

        std::string hint = "Äîñòóïíûå êëþ÷è â ñåêöèè '" + section + "': ";

        for (size_t i = 0; i < available_keys.size(); ++i)
        {
            if (i != 0)
            {
                hint += ", ";
            }
            hint += available_keys[i];
        }

        throw ini_parser_error("Êëþ÷ '" + key + "' íå íàéäåí â ñåêöèè '" + section + "'. " + hint);
    }

    return key_it->second;
}

// Ñîçäàíèå êîíôèãóðàöèîííîãî ôàéëà ïî óìîë÷àíèþ
void ini_parser::create_default_config(const std::string& filename)
{
    std::ofstream out(filename);

    if (out)
    {
        out << R"(
[Section1]
; Ïðèìåð ñåêöèè ñ ðóññêèìè êîììåíòàðèÿìè
var1 = 5
var2 = Ïðèâåò, ìèð!

[Section2]
var1 = 42
var2 = Òåñòîâàÿ ñòðîêà
)";
        std::cout << "Ñîçäàí íîâûé êîíôèã ôàéë: " << filename << std::endl;
    }
    else
    {
        throw ini_parser_error("Íå óäàëîñü ñîçäàòü ôàéë êîíôèãóðàöèè");
    }
}

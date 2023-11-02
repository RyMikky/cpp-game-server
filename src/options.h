#pragma once

#include <string>

namespace detail {

    struct Arguments {
        bool show_help_list = false;                      // флаг показа листа с помощью
        std::string config_json_path;                     // путь к конфигурационному файлу config.json
        std::string static_content_path;                  // путь к статическим файлам web-сервера
        bool game_timer_launch = false;                   // флаг установки автотаймера системы управления игрой
        std::string game_timer_period;                    // период обновления автотаймера игрового состояния
        bool game_autosave = false;                       // флаг включения автосохранения
        std::string state_file_path;                      // путь к файлу автосохранений игрового состояния
        std::string save_state_period;                    // период автосохранения игрового состояния
        bool randomize_spawn_points = false;              // флаг случайного размещения новых персонажей
        std::string data_base_url;                        // URL строка подключения к базе данных PostgreSQL
        unsigned db_connection_count;                     // количество соединений с базой данных
    };

    [[nodiscard]] Arguments ParseCommandLine(int argc, const char* const argv[]);
}
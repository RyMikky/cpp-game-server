#pragma once

#include <string>

namespace detail {

    struct Arguments {
        bool show_help_list = false;                      // флаг показа листа с помощью
        std::string config_json_path;                     // путь к конфигурационному файлу config.json
        std::string static_content_path;                  // путь к статическим файлам web-сервера
        bool test_frame_launch = false;                   // флаг запуска системы тестирования
        std::string test_content_path;                    // путь к проверочным файлам системы тестирования
        bool game_timer_launch = false;                   // флаг установки автотаймера системы управления игрой
        std::string game_timer_period;                    // период обновления автотаймера игрового состояния
        bool randomize_spawn_points = false;              // флаг случайного размещения новых персонажей
    };

    [[nodiscard]] Arguments ParseCommandLine(int argc, const char* const argv[]);
}
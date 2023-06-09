﻿#include "options.h"

#include <boost/program_options.hpp>

#include <fstream>
#include <iostream>
#include <vector>

using namespace std::literals;

namespace detail {

    [[nodiscard]] Arguments ParseCommandLine(int argc, const char* const argv[]) {
        namespace po = boost::program_options;

        po::options_description description_{ "All options"s };
        Arguments arguments_;
        description_.add_options()
            ("help,h", "produce help message")
            ("tick-period,t", po::value(&arguments_.game_timer_period)->value_name("milliseconds"), "set tick period")
            ("config-file,c", po::value(&arguments_.config_json_path)->value_name("file"), "set config file path")
            ("www-root,w", po::value(&arguments_.static_content_path)->value_name("dir"), "set static files root")
            ("state-file,s", po::value(&arguments_.state_file_path)->value_name("state"), "set serialize file path")
            ("save-state-period,p", po::value(&arguments_.save_state_period)->value_name("milliseconds"), "set serialize period")
            ("randomize-spawn-points", "spawn dogs at random positions")
            /*("test-frame-root,f", po::value(&arguments_.test_content_path)->value_name("dir"), "set test files root")*/;

        po::variables_map variables_map_;
        po::store(po::parse_command_line(argc, argv, description_), variables_map_);
        po::notify(variables_map_);

        // если не задан файл конфига кидаем исключение
        if (!variables_map_.contains("config-file"s)) {
            throw std::runtime_error("Config file have not been specified"s);
        }

        // если не задан файл конфига кидаем исключение
        if (!variables_map_.contains("www-root"s)) {
            throw std::runtime_error("Static files directory have not been specified"s);
        }

        // прочие необязательные флаги

        if (variables_map_.contains("randomize-spawn-points"s)) {
            // поднимаем флаг случайной позиции на старте
            arguments_.randomize_spawn_points = true;
        }

        if (variables_map_.contains("state-file"s)) {
            // активируем автосохранение сервера
            arguments_.game_autosave = true;
        }

        if (variables_map_.contains("tick-period"s)) {
            // активируем автообновление игрового состояния
            arguments_.game_timer_launch = true;
        }

        //if (variables_map_.contains("test-frame-root"s)) {
        //    // активируем запуск сквозных тестов на старте
        //    arguments_.test_frame_launch = true;
        //}

        if (variables_map_.contains("help"s)) {
            std::cout << description_;
            // выводим описание доступных команд и подымаем флаг меню
            // приложение выключится после отображения меню в мейне
            arguments_.show_help_list = true;
        }

        return arguments_;
    }
}
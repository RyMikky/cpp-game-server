#include "options.h"

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
            ("randomize-spawn-points", "spawn dogs at random positions")
            ("test-frame-root,f", po::value(&arguments_.test_content_path)->value_name("dir"), "set test files root");

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
            arguments_.randomize_spawn_points = true;
        }

        if (variables_map_.contains("tick-period"s)) {
            arguments_.game_timer_launch = true;
        }

        if (variables_map_.contains("test-frame-root"s)) {
            arguments_.test_frame_launch = true;
        }

        if (variables_map_.contains("help"s)) {
            std::cout << description_;
            arguments_.show_help_list = true;
        }

        return arguments_;
    }
}
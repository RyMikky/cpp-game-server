#pragma once

#include <string>

namespace detail {

    struct Arguments {
        bool show_help_list = false;                      // ���� ������ ����� � �������
        std::string config_json_path;                     // ���� � ����������������� ����� config.json
        std::string static_content_path;                  // ���� � ����������� ������ web-�������
        bool test_frame_launch = false;                   // ���� ������� ������� ������������
        std::string test_content_path;                    // ���� � ����������� ������ ������� ������������
        bool game_timer_launch = false;                   // ���� ��������� ����������� ������� ���������� �����
        std::string game_timer_period;                    // ������ ���������� ����������� �������� ���������
        bool randomize_spawn_points = false;              // ���� ���������� ���������� ����� ����������
    };

    [[nodiscard]] Arguments ParseCommandLine(int argc, const char* const argv[]);
}
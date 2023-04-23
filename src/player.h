#pragma once
#include "token.h"

using namespace std::literals;

namespace game_handler {

    struct PlayerPosition {
        double x_, y_;
    };

    bool operator==(const PlayerPosition& lhs, const PlayerPosition& rhs);
    bool operator!=(const PlayerPosition& lhs, const PlayerPosition& rhs);

    using PosPtr = PlayerPosition*;

    class PosPtrHasher {
    public:
        std::size_t operator()(PlayerPosition* position) const noexcept {
            return static_cast<size_t>(position->x_ * 37) + static_cast<size_t>(position->y_);
        }
    };

    struct PlayerSpeed {
        double xV_, yV_;
    };

    using SpPtr = PlayerSpeed*;

    enum class PlayerDirection {
        NORTH, SOUTH, WEST, EAST
    };

    enum class PlayerMove {
        UP, DOWN, LEFT, RIGHT, STAY, error
    };

    class Player {
    public:
        Player() = default;

        Player(const Player&) = delete;
        Player& operator=(const Player&) = delete;
        Player(Player&&) = default;
        Player& operator=(Player&&) = default;

        Player(uint16_t id, std::string_view name, const Token* token)
            : id_(id), name_(name), token_(token) {
        };

        uint16_t get_player_id() const {
            return id_;
        }
        std::string_view get_player_name() const {
            return name_;
        }
        std::string_view get_player_token() const {
            return **token_;
        }


        Player& set_position(PlayerPosition&& position);
        Player& set_position(double x, double y);
        // рассчитывает новую позицию согласно времени
        Player& update_position(double time);

        PlayerPosition get_position() const {
            return position_;
        }
        Player& set_speed(PlayerSpeed&& speed);
        Player& set_speed(double xV, double yV);
        PlayerSpeed get_speed() const {
            return speed_;
        }

        Player& set_speed_direction(PlayerDirection&& direction);
        PlayerDirection get_speed_direction() const {
            return speed_dir_;
        }

        Player& set_direction(PlayerDirection&& direction);
        PlayerDirection get_direction() const {
            return direction_;
        }

    private:
        uint16_t id_ = 65535;
        std::string name_ = "dummy"s;
        const Token* token_ = nullptr;

        // ---------------------- блок атрибутов состояния персонажа -----------------

        PlayerPosition position_ = { 0, 0 };
        PlayerSpeed speed_ = { 0, 0 };
        PlayerDirection speed_dir_ = PlayerDirection::NORTH;
        PlayerDirection direction_ = PlayerDirection::NORTH;

    };

    using PlayerPtr = const Player*;

    namespace detail {

        // парсит корректность строки направления движения
        bool check_player_move(std::string_view move);
        // возвращет enum с направлением движения игрока
        PlayerMove parse_player_move(std::string_view move);

    } // namespace detail

} // namespace game_handler 
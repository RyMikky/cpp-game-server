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
        PlayerPosition get_position() const {
            return position_;
        }
        Player& set_speed(PlayerSpeed&& speed);
        PlayerSpeed get_speed() const {
            return speed_;
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
        PlayerDirection direction_ = PlayerDirection::NORTH;

    };

    using PlayerPtr = const Player*;

} // namespace game_handler 
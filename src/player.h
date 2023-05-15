#pragma once
#include "token.h"

#include <unordered_map>

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

    static const std::unordered_map<std::string, PlayerDirection> __PLAYER_DIRECTION_TYPE__ = {
        {"U", PlayerDirection::NORTH}, {"D", PlayerDirection::SOUTH}, {"L", PlayerDirection::WEST}, {"R", PlayerDirection::EAST}
    };

    enum class PlayerMove {
        UP, DOWN, LEFT, RIGHT, STAY, error
    };

    static const std::unordered_map<std::string, PlayerMove> __PLAYER_MOVE_TYPE__ = {
        {"U", PlayerMove::UP}, {"D", PlayerMove::DOWN}, {"L", PlayerMove::LEFT}, {"R", PlayerMove::RIGHT}, {"", PlayerMove::STAY}
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

        uint16_t GetPlayerId() const {
            return id_;
        }
        std::string_view GetPlayerName() const {
            return name_;
        }
        std::string_view GetPlayerToken() const {
            return **token_;
        }

        Player& SetPlayerPosition(PlayerPosition&& position);
        Player& SetPlayerPosition(double x, double y);
        // рассчитывает новую позицию согласно времени
        Player& UpdatePlayerPosition(double time);

        PlayerPosition GetPlayerPosition() const {
            return position_;
        }
        Player& SetPlayerSpeed(PlayerSpeed&& speed);
        Player& SetPlayerSpeed(double xV, double yV);
        PlayerSpeed GetPlayerSpeed() const {
            return speed_;
        }

        Player& SetPlayerDirection(PlayerDirection&& direction);
        PlayerDirection GetPlayerDirection() const {
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

    namespace detail {

        // парсит корректность строки направления движения
        bool CheckPlayerMove(std::string_view move);
        // возвращет enum с направлением движения игрока
        PlayerMove ParsePlayerMove(std::string_view move);

    } // namespace detail

} // namespace game_handler
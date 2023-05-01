#include "player.h"

namespace game_handler {

    bool operator==(const PlayerPosition& lhs, const PlayerPosition& rhs) {
        return lhs.x_ == rhs.x_ && lhs.y_ == rhs.y_;
    }

    bool operator!=(const PlayerPosition& lhs, const PlayerPosition& rhs) {
        return !(lhs == rhs);
    }

    Player& Player::SetPlayerPosition(PlayerPosition&& position) {
        position_ = std::move(position);
        return *this;
    }

    Player& Player::SetPlayerPosition(double x, double y) {
        position_.x_ = x; position_.y_ = y;
        return *this;
    }

    // рассчитывает новую позицию согласно времени
    Player& Player::UpdatePlayerPosition(double time) {
        position_.x_ += speed_.xV_ * time;
        position_.y_ += speed_.yV_ * time;
        return *this;
    }

    Player& Player::SetPlayerSpeed(PlayerSpeed&& speed) {
        speed_ = std::move(speed);
        return *this;
    }

    Player& Player::SetPlayerSpeed(double xV, double yV) {
        speed_.xV_ = xV; speed_.yV_ = yV;
        return *this;
    }

    Player& Player::SetPlayerDirection(PlayerDirection&& direction) {
        direction_ = std::move(direction);
        return *this;
    }

    namespace detail {

        // парсит корректность строки направления движения
        bool CheckPlayerMove(std::string_view move) {
            return __PLAYER_MOVE_TYPE__.count(std::string(move));
        }

        // возвращет enum с направлением движения игрока
        PlayerMove ParsePlayerMove(std::string_view move) {
            try
            {
                return __PLAYER_MOVE_TYPE__.at(std::string(move));
            }
            catch (const std::exception& e)
            {
                throw std::runtime_error("game_handler::detail::ParsePlayerMove::Error" + std::string(e.what()));
            }
        }

    } // namespace detail

} // game_handler
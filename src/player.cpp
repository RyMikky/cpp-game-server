#include "player.h"

namespace game_handler {

    bool operator==(const PlayerPosition& lhs, const PlayerPosition& rhs) {
        return lhs.x_ == rhs.x_ && lhs.y_ == rhs.y_;
    }
    bool operator!=(const PlayerPosition& lhs, const PlayerPosition& rhs) {
        return !(lhs == rhs);
    }

    Player& Player::set_position(PlayerPosition&& position) {
        position_ = std::move(position);
        return *this;
    }
    Player& Player::set_position(double x, double y) {
        position_.x_ = x; position_.y_ = y;
        return *this;
    }

    // рассчитывает новую позицию согласно времени
    Player& Player::update_position(double time) {
        position_.x_ += speed_.xV_ * time;
        position_.y_ += speed_.yV_ * time;
        return *this;
    }

    Player& Player::set_speed(PlayerSpeed&& speed) {
        speed_ = std::move(speed);
        return *this;
    }
    Player& Player::set_speed(double xV, double yV) {
        speed_.xV_ = xV; speed_.yV_ = yV;
        return *this;
    }

    Player& Player::set_speed_direction(PlayerDirection&& direction) {
        speed_dir_ = std::move(direction);
        return *this;
    }

    Player& Player::set_direction(PlayerDirection&& direction) {
        direction_ = std::move(direction);
        return *this;
    }

    namespace detail {

        // парсит корректность строки направления движения
        bool check_player_move(std::string_view move) {
            return move == "U"sv || move == "D"sv || move == "L"sv || move == "R"sv || move == ""sv;
        }
        // возвращет enum с направлением движения игрока
        PlayerMove parse_player_move(std::string_view move) {

            if (move == "U"sv) {
                return PlayerMove::UP;
            }

            if (move == "D"sv) {
                return PlayerMove::DOWN;
            }

            if (move == "L"sv) {
                return PlayerMove::LEFT;
            }

            if (move == "R"sv) {
                return PlayerMove::RIGHT;
            }

            if (move == ""sv) {
                return PlayerMove::STAY;
            }

            return PlayerMove::error;
        }

    } // namespace detail

} // game_handler
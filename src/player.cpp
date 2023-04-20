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

    Player& Player::set_speed(PlayerSpeed&& speed) {
        speed_ = std::move(speed);
        return *this;
    }

    Player& Player::set_direction(PlayerDirection&& direction) {
        direction_ = std::move(direction);
        return *this;
    }

} // game_handler
#include "player.h"

#include <algorithm>

namespace game_handler {

    // добавляет число в диапазоне от плюс до минус дельты к каждому из полей
    PlayerPosition& PlayerPosition::AddRandomPlusMinusDelta(double delta) {
        x_ += model::GetRandomDoubleRoundOne(-delta, delta);
        y_ += model::GetRandomDoubleRoundOne(-delta, delta);
        return *this;
    }

    bool operator==(const PlayerPosition& lhs, const PlayerPosition& rhs) {

        bool check_x = ((std::max(std::abs(lhs.x_), std::abs(rhs.x_))) - 
            (std::min(std::abs(lhs.x_), std::abs(rhs.x_))) < __EPSILON__);

        bool check_y = ((std::max(std::abs(lhs.y_), std::abs(rhs.y_))) -
            (std::min(std::abs(lhs.y_), std::abs(rhs.y_))) < __EPSILON__);

        return check_x && check_y;
    }

    bool operator!=(const PlayerPosition& lhs, const PlayerPosition& rhs) {
        return !(lhs == rhs);
    }

    bool operator==(const PlayerSpeed& lhs, const PlayerSpeed& rhs) {

        bool check_x = ((std::max(std::abs(lhs.xV_), std::abs(rhs.xV_))) -
            (std::min(std::abs(lhs.xV_), std::abs(rhs.xV_))) < __EPSILON__);

        bool check_y = ((std::max(std::abs(lhs.yV_), std::abs(rhs.yV_))) -
            (std::min(std::abs(lhs.yV_), std::abs(rhs.yV_))) < __EPSILON__);

        return check_x && check_y;
    }

    bool operator!=(const PlayerSpeed& lhs, const PlayerSpeed& rhs) {
        return !(lhs == rhs);
    }

    // Назачает игрока, по сути "укладывает" предмет в сумку
    GameLoot& GameLoot::SetPlayerPrt(PlayerPtr player) {
        player_ = player;
        return *this;
    }

    // назначает id игрока
    Player& Player::SetId(size_t id) {
        id_ = id;
        return *this;
    }

    // назначает имя игрока
    Player& Player::SetName(std::string_view name) {
        name_ = name;
        return *this;
    }

    // назначает указатель на уникальный токен игрока
    Player& Player::SetToken(const Token* token) {
        token_ = token;
        return *this;
    }

    // назначает вместимость сумки игрока
    Player& Player::SetBagCapacity(unsigned capacity) {
        bag_capacity_ = capacity;
        return *this;
    }

    // назначает очки игроку
    Player& Player::SetScore(unsigned score) {
        score_ = score;
        return *this;
    }

    // ----------- методы работы с сумкой игрока ------------------------------

    /*
    * Добавляет лут в сумку, в том случае если в сумках есть место
    * index - индекс лута игровой сессии, loot - указатель на данные
    * True - сигнализирует успешнное добавление, False - предмет остался на карте
    */
    bool Player::AddLoot(size_t index, GameLootPtr loot) {

        if (bag_.size() == bag_capacity_) {
            // если сумка заполнена ничего не делаем
            return false; 
        }
        else if (bag_.size() > bag_capacity_) {
            // если сумка даже переполнена, чего быть не должнно, сигнализируем о ошибке
            throw std::out_of_range("game_handler::Player::AddGameLoot::Error::Bag size is out of range");
        }
        
        loot->SetPlayerPrt(this);           // назначаем текущего игрока "владельцем" вещи
        bag_.push_back({ index, loot });      // добавляем вещь в рюкзак

        return true;
    }

    // Удаляет предмет из сумки по индексу в векторе
    bool Player::RemoveLoot(size_t index) {

        if (index >= bag_capacity_) {
            // если индекс больше или равен вместимости, то кидаем исключение
            throw std::out_of_range("game_handler::Player::RemoveLoot::Error::index is out of bag capacity range");
        }

        else if (index >= bag_.size()) {
            // если индекс больше или равен текущей заполненности рюкзака, то кидаем исключение
            throw std::out_of_range("game_handler::Player::RemoveLoot::Error::index is out of current bag size range");
        }

        bag_.erase(bag_.begin() + index);
        return true;
    }
    // Сдаёт предмет из сумки по индексу в векторе в бюро находок, при этом прибавляются очки
    BagItem Player::ReturnLoot(size_t index) {

        if (index >= bag_capacity_) {
            // если индекс больше или равен вместимости, то кидаем исключение
            throw std::out_of_range("game_handler::Player::RemoveLoot::Error::index is out of bag capacity range");
        }

        else if (index >= bag_.size()) {
            // если индекс больше или равен текущей заполненности рюкзака, то кидаем исключение
            throw std::out_of_range("game_handler::Player::RemoveLoot::Error::index is out of current bag size range");
        }

        // если индекс не указывает на nullptr
        if (!bag_[index].IsDummy()) {
            
            BagItem result = bag_[index];                // изымаем указатель обратно
            score_ += result.loot_->GetRawValue();       // прибавляем очки к счету игрока
            // удалять будем всё скопом потом
            //RemoveLoot(index);                           // затираем данные о элементе
            return result;
        }

        return {};                                       // сюда попасть не должны никак
    }

    // Очищает все записи в рюкзаке и обнуляет его
    Player& Player::ClearBag() {
        bag_.clear();
        return *this;
    }
    /*
    * Возвращает, сдаёт все предметы из инвентаря в бюро
    * При этом сумма очков предметов в инвентаре прибавляется к общему счёту игрока
    */
    Player& Player::ReturnLootToTheOffice() {
        score_ += GetLootTotalValue();
        return ClearBag();
    }
    // возвращает сумму очков предметов в сумке
    unsigned Player::GetLootTotalValue() const {

        unsigned result = 0;

        for (const auto& item : bag_) {
            // записываем сумму всех предметов в рюкзаке, если элемент не указывает на nullptr
            if (!item.IsDummy()) {
                result += item.loot_->GetRawValue();
            }
        }

        return result;
    }

    // ----------- геттеры и сеттеры атрибутов состояни игрока ----------------

    // назначает текущую позицию игрока
    Player& Player::SetCurrentPosition(PlayerPosition&& position) {
        current_position_ = std::move(position);
        return *this;
    }
    // назначает текущую позицию игрока
    Player& Player::SetCurrentPosition(double x, double y) {
        current_position_.x_ = x; current_position_.y_ = y;
        return *this;
    }
    // назначает будущую позицию игрока
    Player& Player::SetFuturePosition(PlayerPosition&& position) {
        future_position_ = std::move(position);
        return *this;
    }
    // назначает будущую позицию игрока
    Player& Player::SetFuturePosition(double x, double y) {
        future_position_.x_ = x; future_position_.y_ = y;
        return *this;
    }
    // назначает текущую позицию из будущей позиции
    // применяется обработчиком игровой сессии после работы детектора коллизий
    Player& Player::UpdateCurrentPosition() {
        if (current_position_ != future_position_) {
            current_position_ = future_position_;
        }
        return *this;
    }
    // рассчитывает и назначает новую позицию согласно текущей позиции, скорости и переданного времени
    // если задана будущая позиция, то она будет приравнена к расчётной позиции
    Player& Player::UpdateCurrentPosition(double time) {
        UpdateFuturePosition(time);
        UpdateCurrentPosition();
        return *this;
    }
    // рассчитывает и назначает будущую позицию согласно текущей позиции, скорости и переданного времени
    Player& Player::UpdateFuturePosition(double time) {

        future_position_.x_ = (speed_.xV_ != 0) ? 
            current_position_.x_ + (speed_.xV_ * time) :
            current_position_.x_;

        future_position_.y_ = (speed_.yV_ != 0) ?
            current_position_.y_ + (speed_.yV_ * time) :
            current_position_.y_;

        return *this;
    }
    // назачает скорость движения игрока
    Player& Player::SetSpeed(PlayerSpeed&& speed) {
        speed_ = std::move(speed);
        return *this;
    }
    // назачает скорость движения игрока
    Player& Player::SetSpeed(double xV, double yV) {
        speed_.xV_ = xV; speed_.yV_ = yV;
        return *this;
    }
    // назначает направление игрока
    Player& Player::SetDirection(PlayerDirection&& direction) {
        direction_ = std::move(direction);
        return *this;
    }

    // назначает общее игровое время в миллисекундах, используется при сериализации
    Player& Player::SetTotalInGameTimeMS(int time_ms) {
        total_time_ms_ = time_ms;
        return *this;
    }

    // добавляет общее игровое время в миллисекундах
    Player& Player::AddTotalInGameTimeMS(int time_ms) {
        total_time_ms_ += time_ms;
        return *this;
    }

    // назначает время простоя в миллисекундах, используется при сериализации
    Player& Player::SetRetirementTimeMS(int time_ms) {
        retirement_time_ms_ = time_ms;
        return *this;
    }

    // добавляет время простоя в игре в миллисекундах
    // также увеличивает общее время в игре на указанную величину
    Player& Player::AddRetirementTimeMS(int time_ms) {
        retirement_time_ms_ += time_ms;
        total_time_ms_ += time_ms;
        return *this;
    }

    // сбрасывает время простоя в игре в ноль
    Player& Player::ResetRetirementTime() {
        retirement_time_ms_ = 0;
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

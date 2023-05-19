#pragma once
#include "token.h"
#include "model.h"

#include <vector>
#include <stdexcept>
#include <unordered_map>

using namespace std::literals;

namespace game_handler {

    static const double __EPSILON__ = 10e-6;

    class Player;
    using PlayerPtr = const Player*;

    // позиция игрока или вещи, отличается от модельной тем, что испольщуются double
    struct PlayerPosition {
        PlayerPosition() = default;
        PlayerPosition(double x, double y) 
            : x_(x), y_(y) {
        }
        PlayerPosition(int x, int y)
            : x_(static_cast<double>(x)), y_(static_cast<double>(y)) {
        }
        PlayerPosition(const model::Point& point) 
            : x_(static_cast<double>(point.x)), y_(static_cast<double>(point.y)) {
        }

        // добавляет +/- дельту к каждому из полей
        PlayerPosition& AddRandomPlusMinusDelta(double delta);

        double x_ = 0.0;
        double y_ = 0.0;
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

    // скорость перемещенния игрока - по сути двухмерный вектор в double
    struct PlayerSpeed {
        double xV_, yV_;
    };

    bool operator==(const PlayerSpeed& lhs, const PlayerSpeed& rhs);
    bool operator!=(const PlayerSpeed& lhs, const PlayerSpeed& rhs);

    using SpPtr = PlayerSpeed*;

    // направление взгляда игрока
    enum class PlayerDirection {
        NORTH, SOUTH, WEST, EAST
    };

    static const std::unordered_map<std::string, PlayerDirection> __PLAYER_DIRECTION_TYPE__ = {
        {"U", PlayerDirection::NORTH}, {"D", PlayerDirection::SOUTH}, {"L", PlayerDirection::WEST}, {"R", PlayerDirection::EAST}
    };

    // направление движенния игрока
    enum class PlayerMove {
        UP, DOWN, LEFT, RIGHT, STAY, error
    };

    static const std::unordered_map<std::string, PlayerMove> __PLAYER_MOVE_TYPE__ = {
        {"U", PlayerMove::UP}, {"D", PlayerMove::DOWN}, {"L", PlayerMove::LEFT}, {"R", PlayerMove::RIGHT}, {"", PlayerMove::STAY}
    };

    // игровой лут, используется для контроля вещей в инвентаре игрока
    struct GameLoot : public model::LootType {

        GameLoot(model::LootType loot_type, size_t type, size_t id, PlayerPosition position)
            : model::LootType(loot_type), type_(type), id_(id), pos_(position) {
        }

        GameLoot(const GameLoot& other) = default;
        GameLoot& operator=(const GameLoot& other) = default;

        GameLoot(GameLoot&& other) = default;
        GameLoot& operator=(GameLoot&& other) = default;
        
        /*
        * Возвращает указатель на игрока, у кого в сумке находится предмет
        * Если nullptr, значит находится на карте, иначе - в сумке
        */
        PlayerPtr GetPlayerPtr() const {
            return player_;
        }
        // Назачает игрока, по сути "укладывает" предмет в сумку
        GameLoot& SetPlayerPrt(PlayerPtr player);

        size_t type_ = 0;                // он же индекс в массиве LootTypes на карте
        size_t id_ = 0;                  // id в игровой сессии
        PlayerPosition pos_;             // позиция берется не из модели, а из игрока, так как в модели она в инте, а надо в дабле
        PlayerPtr player_ = nullptr;     // указатель на игрока, у которого вещь находится в сумке, если nullptr - значит на карте
    };

    using GameLootPtr = GameLoot*;

    struct BagItem {

        bool IsDummy() const {
            return loot_ == nullptr;
        }

        size_t index_;
        GameLootPtr loot_ = nullptr;
    };

    using PlayerBag = std::vector<BagItem>;

    class Player {
    public:
        Player() = default;

        Player(const Player&) = delete;
        Player& operator=(const Player&) = delete;
        Player(Player&&) = default;
        Player& operator=(Player&&) = default;

        Player(size_t id, std::string_view name)
            : id_(id), name_(name) {
        };
        Player(size_t id, std::string_view name, const Token* token)
            : id_(id), name_(name), token_(token) {
        };
        Player(size_t id, std::string_view name, const Token* token, unsigned capacity)
            : id_(id), name_(name), token_(token), bag_capacity_(capacity) {
        };

        // ----------- геттеры и сеттеры общих данных игрока ----------------------

        // назначает id игрока
        Player& SetId(size_t);
        // назначает имя игрока
        Player& SetName(std::string_view);
        // назначает указатель на уникальный токен игрока
        Player& SetToken(const Token*);
        // назначает вместимость сумки игрока
        Player& SetBagCapacity(unsigned);
        // назначает очки игроку
        Player& SetScore(unsigned);

        // возвращает Id игрока
        size_t GetId() const {
            return id_;
        }
        // возвращает имя игрока
        std::string_view GetName() const {
            return name_;
        }
        // возвращает строкове представление уникального токена игрока
        std::string_view GetToken() const {
            return **token_;
        }
        // возвращает вместимость рюкзака игрока
        unsigned GetBagCapacity() const {
            return bag_capacity_;
        }
        // возвращает количество предметов в рюкзаке
        size_t GetBagSize() const {
            return bag_.size();
        }
        // возвращает текущую сумму очков игрока
        unsigned GetScore() const {
            return score_;
        }

        // ----------- методы работы с сумкой игрока ------------------------------

        // сообщает есть ли место в инвентаре
        bool CheckFreeBagSpace() {
            return GetBagSize() < GetBagCapacity();
        }

        /*
        * Добавляет лут в сумку, в том случае если в сумках есть место
        * index - индекс лута игровой сессии, loot - указатель на данные
        * True - сигнализирует успешнное добавление, False - предмет остался на карте
        */
        bool AddLoot(size_t index, GameLootPtr loot);

        // Удаляет предмет из сумки по индексу в векторе
        bool RemoveLoot(size_t index);
        // Сдаёт предмет из сумки по индексу в векторе в бюро находок, при этом прибавляются очки
        BagItem ReturnLoot(size_t index);
        // Очищает все записи в рюкзаке и обнуляет его
        Player& ClearBag();
        // возвращает константную ссылку на рюкзак
        const PlayerBag& GetBag() const {
            return bag_;
        }
        /*
        * Возвращает, сдаёт все предметы из инвентаря в бюро
        * При этом сумма очков предметов в инвентаре прибавляется к общему счёту игрока
        */
        Player& ReturnLootToTheOffice();
        // возвращает сумму очков предметов в сумке
        unsigned GetLootTotalValue() const;

        // ----------- геттеры и сеттеры атрибутов состояни игрока ----------------

        // назначает текущую позицию игрока
        Player& SetCurrentPosition(PlayerPosition&& position);
        // назначает текущую позицию игрока
        Player& SetCurrentPosition(double x, double y);
        // возвращает текущую позицию игрока
        PlayerPosition GetCurrentPosition() const {
            return current_position_;
        }

        // назначает будущую позицию игрока
        Player& SetFuturePosition(PlayerPosition&& position);
        // назначает будущую позицию игрока
        Player& SetFuturePosition(double x, double y);
        // возвращает будущую позицию игрока
        PlayerPosition GetFuturePosition() const {
            return future_position_;
        }

        // назначает текущую позицию из будущей позиции
        // применяется обработчиком игровой сессии после работы детектора коллизий
        Player& UpdateCurrentPosition();
        // рассчитывает и назначает новую позицию согласно текущей позиции, скорости и переданного времени
        // если задана будущая позиция, то она будет приравнена к полученной изменением позиции
        Player& UpdateCurrentPosition(double time);
        // рассчитывает и назначает будущую позицию согласно текущей позиции, скорости и переданного времени
        Player& UpdateFuturePosition(double time);
        
        // назачает скорость движения игрока
        Player& SetSpeed(PlayerSpeed&& speed);
        // назачает скорость движения игрока
        Player& SetSpeed(double xV, double yV);
        // возвращает текущую скорость игрока
        PlayerSpeed GetSpeed() const {
            return speed_;
        }

        // назначает направление игрока
        Player& SetDirection(PlayerDirection&& direction);
        // возвращает текущее направление игрока
        PlayerDirection GetDirection() const {
            return direction_;
        }

    private:
        size_t id_ = 65535;                                     // уникальный ID игрока
        std::string name_ = "dummy"s;                           // имя игрока
        const Token* token_ = nullptr;                          // уникальный токен
        unsigned bag_capacity_ = 0;                             // вместимость рюкзака игрока
        unsigned score_ = 0;                                    // общая сумма набранных очков
        PlayerBag bag_ = {};                                    // рюкзак игрока

        // ---------------------- блок атрибутов состояния персонажа -----------------
        
        PlayerSpeed speed_ = { 0, 0 };                          // скорость игрока
        PlayerPosition current_position_ = { 0, 0 };            // текущая позиция игрока
        PlayerPosition future_position_ = { 0, 0 };             // будущая позиция игрока
        PlayerDirection direction_ = PlayerDirection::NORTH;    // направление игрока
    };

    namespace detail {

        // парсит корректность строки направления движения
        bool CheckPlayerMove(std::string_view move);
        // возвращет enum с направлением движения игрока
        PlayerMove ParsePlayerMove(std::string_view move);

    } // namespace detail

} // namespace game_handler
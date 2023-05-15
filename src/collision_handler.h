#pragma once

#include "domain.h"

#include <vector>

namespace game_handler {

    // натройка обработки коллизий, задаёт ширины элементов и объектов
    struct CollisionSetup {
        double loots_width_ = 0.0;
        double players_width_ = 0.0;
        double offices_width_ = 0.0;
    };

    // базовая ширины выданные в условии к заданию
    static const CollisionSetup __BASE_COLLISIONS_SETUP__ = CollisionSetup{ 0.0, 0.6, 0.5 };
    
    struct CollisionResult {
        bool IsCollision(double collision_radius) const {
            return proj_ratio >= 0 && proj_ratio <= 1 && sq_distance <= collision_radius * collision_radius;
        }

        // квадрат расстояния до точки
        double sq_distance;
        // доля пройденного отрезка
        double proj_ratio;
    };

    // провайдер данных о игроках, предметах и офисах в функцию определения коллизии
    class CollisionProvider {
    protected:
        ~CollisionProvider() = default;
    public:

        /*
        * Интерфейс адаптирован под мою игровую модель и общий движок.
        * Основная особенность в том, что в моей реализации игроки и лут хранятся в std::unordered_map
        * Смысл в том, что так проще добавлять и удалять элементы динамически в процессе игры
        * Таким образом методы получения элементов массива по индексу бессмысленны.
        * Словарь и так хранит уникальный id в качестве ключа и данные в качестве значения.
        * Офисы же храятся в модели особым образом, но офисы не добавляются и не удаляются в процессе игры
        * GameSession будет наследовать этот интерфейс и выполнять обновления положения игроков,
        * сбор и сдачу предметов, после расчёта всех возможых коллизий в FindCollisionEvents.
        */

        // возвращает количество офисов бюро находок на карте игровой сессии
        virtual size_t OfficesCount() const = 0;
        // возвращает офис бюро находок по индексу
        virtual const model::Office& GetOffice(size_t idx) const = 0;
        // возвращает ссылку на константную мапу игроков
        virtual const SessionPlayers& GetPlayers() const = 0;
        // возвращает ссылку на константную мапу лута
        virtual const SessionLoots& GetLoots() const = 0;

        //// возвращает количество предметов лута на карте игровой сессии
        //virtual size_t LootsCount() const = 0;
        //// возвращает предмет по индексу
        //virtual const GameLoot& GetLoot(size_t idx) const = 0;
        //// возвращает количество игроков в игровой сессии
        //virtual size_t PlayersCount() const = 0;
        //// возвращает игрока по индексу
        //virtual const Player& GetPlayer(size_t idx) const = 0;
    };

    // указывает тпи 
    enum class CollisionEventType {
        GATHERING, RETURN
    };

    // событие подбора предмета
    struct CollisionEvent {

        CollisionEventType type;        // кип коллизии, или подбор предмета или сдача в бюро
        size_t object_id;               // id объекта взаимодействия, лут или офис бюро находок
        const Token* player_token;      // токен игрока, для которого расчитываются коллизии
        double sq_distance;             // квадрат расстояния до точки коллизии
        double time;                    // время, через скольо коллизия произойдёт
    };

    // компаратор для сортировки массива событий по времени
    bool CollisionEventsTimeComparator(const CollisionEvent& lhs, const CollisionEvent& rhs);

    /*
    * Проверяет возможые коллизии игроков с игровыми прежметами, и игроков с офисами бюро находок
    * Определение коллизий осуществляется только в том случае, если игрок имеет разные текйщие и будущие координаты
    * Запись будущих координат игроков должна быть осуществлена заранее в классе, реализующем интерфейс CollisionProvider
    * Экземпляр интерфейса CollisionProvider подается в качестве константной ссылки
    */
    std::vector<CollisionEvent> FindCollisionEvents(const CollisionProvider& provider);

} // namespace game_handler
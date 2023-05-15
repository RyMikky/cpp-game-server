#include "collision_handler.h"

namespace game_handler {

    static const double GetBasePlayerUnitRadius() {
        return __BASE_COLLISIONS_SETUP__.players_width_ != 0 ? __BASE_COLLISIONS_SETUP__.players_width_ / 2 : 0;
    }

    static const double GetBaseOfficeUnitRadius() {
        return __BASE_COLLISIONS_SETUP__.offices_width_ != 0 ? __BASE_COLLISIONS_SETUP__.offices_width_ / 2 : 0;
    }

    static const double GetBaseLootUnitRadius() {
        return __BASE_COLLISIONS_SETUP__.loots_width_ != 0 ? __BASE_COLLISIONS_SETUP__.loots_width_ / 2 : 0;
    }

    CollisionResult CheckPossibleCollision(PlayerPosition from, PlayerPosition to, PlayerPosition item) {
        // Проверим, что перемещение ненулевое.
        // Тут приходится использовать строгое равенство, а не приближённое,
        // пскольку при сборе заказов придётся учитывать перемещение даже на небольшое
        // расстояние.
        assert(to.x_ != from.x_ || to.y_ != from.y_);
        const double u_x = item.x_ - from.x_;
        const double u_y = item.y_ - from.y_;
        const double v_x = to.x_ - from.x_;
        const double v_y = to.y_ - from.y_;
        const double u_dot_v = u_x * v_x + u_y * v_y;
        const double u_len2 = u_x * u_x + u_y * u_y;
        const double v_len2 = v_x * v_x + v_y * v_y;
        const double proj_ratio = u_dot_v / v_len2;
        const double sq_distance = u_len2 - (u_dot_v * u_dot_v) / v_len2;

        return CollisionResult(sq_distance, proj_ratio);
    }

    // компаратор для сортировки массива событий по времени
    bool CollisionEventsTimeComparator(const CollisionEvent& lhs, const CollisionEvent& rhs) {
        return lhs.time < rhs.time;
    }
    
    std::vector<CollisionEvent> FindCollisionEvents(const CollisionProvider& provider) {

        std::vector<CollisionEvent> result;

        // будем за O(N(K + M)) проверять возможные коллизии через CheckPossibleCollision
        // где N - число игроков, K - число предметов, M - число офисов
        // Цикл с индексом только для офисов, данные которых хранятся в обычном векторе

        for (const auto& [token, player] : provider.GetPlayers()) {

            // проверять будем если чубака перемещается
            // для этого должны быть разные позиции текущих и будущих координат
            // иначе в TryCollectPoint() выстрелит ассерт, а нам этого не надо
            // ровно также если чубака уже стоит на точке с коллизией, то он уже собрал предмет или у него забитый мешок
            if (player.GetCurrentPosition() != player.GetFuturePosition()) {

                // так как данный метод только определяет возможные коллизии, 
                // но никак не влияет на игровое состояние, то не его задача определять
                // есть место у игрока или нету, метод пишет ВСЕ возможные коллизии на пути
                // а уже CollisionProvider сам разберется, что и как исполнять
                // потому тут нет никаких проверок на вместимости и прочее, пишем все, босс разберется

                for (const auto& loot : provider.GetLoots()) {

                    // пробуем проверить возможную коллизию предмета с путём перемещения игрока
                    auto collision = CheckPossibleCollision(
                        player.GetCurrentPosition(), player.GetFuturePosition(), loot.second.pos_);

                    // если удалось свершилась коллизия с предметом
                    if (collision.IsCollision(
                        GetBasePlayerUnitRadius() + GetBaseLootUnitRadius())) {
                        // записываем новый эвент в массиве строкой инициализации
                        // тип эвента, индекс предмета, индекс чубаки, дистанция, время
                        result.push_back({ CollisionEventType::GATHERING,
                            loot.first, token, collision.sq_distance, collision.proj_ratio });
                    }
                }
                
                // вне зависиомти от заполненности сумок игрока может быть
                // обработана коллизия с офисом бюро находок, предметы надо сдавать
                for (size_t i = 0; i != provider.OfficesCount(); ++i) {

                    // берем офис по индексу
                    const model::Office& office = provider.GetOffice(i);
                    // так как позиция в модели задается интами, то придется сначала создать позицию офиса в дабле
                    PlayerPosition office_pos{ 
                        static_cast<double>(office.GetPosition().x),  static_cast<double>(office.GetPosition().y) };
                    // пробуем проверить возможную коллизию офиса с путём перемещения игрока
                    auto collision = CheckPossibleCollision(
                        player.GetCurrentPosition(), player.GetFuturePosition(), office_pos);

                    // если удалось свершилась коллизия с предметом
                    if (collision.IsCollision(
                        GetBasePlayerUnitRadius() + GetBaseOfficeUnitRadius())) {
                        // записываем новый эвент в массиве строкой инициализации
                        // тип эвента, индекс офиса, индекс чубаки, дистанция, время
                        result.push_back({ CollisionEventType::RETURN,
                            i, token, collision.sq_distance, collision.proj_ratio });
                    }
                }
            }
        }

        // события должны быть отсортированны по времени
        std::sort(result.begin(), result.end(), CollisionEventsTimeComparator);
            /*[](const CollisionEvent& lhs, const CollisionEvent& rhs) {
                return lhs.time < rhs.time;
            });*/

        return result;
    }
    
} //namespace game_handler
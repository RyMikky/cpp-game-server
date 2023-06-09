#include "collision_detector.h"
#include <cassert>

namespace collision_detector {

    CollectionResult TryCollectPoint(geom::Point2D a, geom::Point2D b, geom::Point2D c) {
        // Проверим, что перемещение ненулевое.
        // Тут приходится использовать строгое равенство, а не приближённое,
        // пскольку при сборе заказов придётся учитывать перемещение даже на небольшое
        // расстояние.
        assert(b.x != a.x || b.y != a.y);
        const double u_x = c.x - a.x;
        const double u_y = c.y - a.y;
        const double v_x = b.x - a.x;
        const double v_y = b.y - a.y;
        const double u_dot_v = u_x * v_x + u_y * v_y;
        const double u_len2 = u_x * u_x + u_y * u_y;
        const double v_len2 = v_x * v_x + v_y * v_y;
        const double proj_ratio = u_dot_v / v_len2;
        const double sq_distance = u_len2 - (u_dot_v * u_dot_v) / v_len2;

        return CollectionResult(sq_distance, proj_ratio);
    }

    std::vector<GatheringEvent> FindGatherEvents(const ItemGathererProvider& provider) {

        std::vector<GatheringEvent> result;

        // будем за O(N*K) проверять возможные коллизии через TryCollectPoint
        // где N - число игроков, K - число предметов

        for (size_t j = 0; j != provider.GatherersCount(); ++j) {

            // берем чубаку по индексу
            auto gatherer = provider.GetGatherer(j);

            // проверять будем если чубака перемещается
            // для этого должны быть разные точки начала и конца пути
            // иначе в TryCollectPoint() выстрелит ассерт, а нам этого не надо
            // ровно также если чубака уже стоит на точке с коллизией, то он уже собрал предмет
            if (gatherer.start_pos != gatherer.end_pos) {

                for (size_t i = 0; i != provider.ItemsCount(); ++i) {

                    // берем айтем по индексу
                    auto item = provider.GetItem(i);
                    // пробуем "собрать" предмет нашим чубакой
                    auto collect = TryCollectPoint(gatherer.start_pos, gatherer.end_pos, item.position);

                    // если удалось "собрать"
                    if (collect.IsCollected(gatherer.width + item.width)) {
                        // записываем новый эвент в массиве строкой инициализации
                        // индекс предмета, индекс чубаки, дистанция, время
                        result.push_back({ i, j, collect.sq_distance, collect.proj_ratio });
                    }
                }
            }
        }

        std::sort(result.begin(), result.end(),
            [](const GatheringEvent& lhs, const GatheringEvent& rhs) {
                return lhs.time < rhs.time;
            });

        return result;

    }

}  // namespace collision_detector
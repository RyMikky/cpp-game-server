#include "model.h"

#include <stdexcept>
#include <algorithm>
#include <optional>
#include <random>

namespace model {

    using namespace std::literals;

    int random_integer(int from, int to) {

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(from, to);
        return dis(gen);
    }

    Point Road::get_random_position() const {
        return IsHorizontal() ?
            Point{ random_integer(
                std::min(start_.x, end_.x),
                std::max(start_.x, end_.x)), 
            start_.y } :
            Point{ start_.x, random_integer(
                std::min(start_.y, end_.y),
                std::max(start_.y, end_.y)) };
    }

    void Map::AddOffice(Office office) {
        if (warehouse_id_to_index_.contains(office.GetId())) {
            throw std::invalid_argument("Duplicate warehouse");
        }

        const size_t index = offices_.size();
        Office& o = offices_.emplace_back(std::move(office));
        try {
            warehouse_id_to_index_.emplace(o.GetId(), index);
        } catch (...) {
            // Удаляем офис из вектора, если не удалось вставить в unordered_map
            offices_.pop_back();
            throw;
        }
    }
    // возвращает стартовую точку первой дороги на карте
    Point Map::get_first_map_start_position() const {
        if (roads_.size() == 0) {
            // если дорог нет, тогда кидаем исключение, что у нас пусто в листе
            throw std::runtime_error("Map::get_first_map_start_position::Error::No_Roads_on_Map");
        }

        return roads_[0].GetStart();
    }
    // возвращает ссылку на дорогу по переданной позиции и направлению, или вертикальному или горизонтальному
    const Road& Map::get_road_by_position(Point pos, bool vertical) const {

        if (roads_.size() == 0) {
            // если дорог нет, тогда кидаем исключение, что у нас пусто в листе
            throw std::runtime_error("Map::get_road_by_position::Error::No_Roads_on_Map");
        }

        std::optional<const Road*> horizontal_road;            // заготовка под дорогу по горизонтали
        std::optional<const Road*> vertical_road;              // заготовка под дорогу по вертикали 
        
        // дорога должна вернуться одна, в зависимости от переданного флага
        for (const Road& road : roads_) {
            // позиция так или иначе будет приведена к инту, значит чтобы понять на какой мы дороге должны совпасть парные координаты
            bool x_compare = road.GetStart().x == pos.x && road.GetEnd().x == pos.x;              // совпадение по горизонтальной оси
            bool y_compare = road.GetStart().y == pos.y && road.GetEnd().y == pos.y;              // совпадение по вертикальной оси

            // если имеем совпадение по вертикальной оси, значит стоим на горизонтальной дороге
            if (y_compare) horizontal_road.emplace(std::move(&road));
            // если имеем совпадение по горизонтальной оси, значит стоим на вертикальной дороге
            if (x_compare) vertical_road.emplace(std::move(&road));
        }

        // после перебора дорог, точно должны быть заполнен хотя бы один std::optional
        if (horizontal_road && vertical_road) {
            // если нашли две дороги (стоим на перекрестке), то возвращаем ту, куда смотрит и планирует идти игрок
            return vertical ? *vertical_road.value() : *horizontal_road.value();
        }
        else {
            if (horizontal_road) {
                return *horizontal_road.value();
            }
            else if (vertical_road) {
                return *vertical_road.value();
            }
            // иначе пойдём вниз и бросим исключение
        }

        // если мы так ничего и не нашли, также кидаем исключение, что будет сигналом о то, что что-то пошло не так
        // по идее сюда попасть ну никак нельзя, так как все песели должны бегать по дорогам, и позиции должны передаваться
        // так, чтобы оказаться на той или иной дороге, но мало ли что может случиться
        throw std::runtime_error("Map::get_road_by_position::Error::No_Result_Road_By_Position");
    }
    // флаг нахождения точки на горизонтальной дороге
    // требуется передача позиции в формате модели с округлением к int
    const Road* Map::stay_on_horizontal_road(Point pos) const {

        if (roads_.size() == 0) {
            // если дорог нет, тогда кидаем исключение, что у нас пусто в листе
            throw std::runtime_error("Map::get_road_by_position::Error::No_Roads_on_Map");
        }

        for (const Road& road : roads_) {
            // бежим по списку дорог, если вертикальная координата игрока переданной позиции совпадает
            // с вертикальной координатой начала и конца дороги, значит дорога горизонтальна и мы стоим на ней
            if (road.GetStart().y == pos.y && road.GetEnd().y == pos.y) {
                // проверяем совпадение по горизонтальной координате, так как может быть например две и более горизонтальных дорог
                // находящихся на одной и той же вертикальной координате, например {0,0; 10,0} и {20,0; 30,0}, а pos {22,0}
                // значит мы должны выбрать другую дорогу, первая в списке {0,0; 10,0} нам не подходит
 
                if (std::min(road.GetStart().x, road.GetEnd().x) <= pos.x
                    && pos.x <= std::max(road.GetStart().x, road.GetEnd().x)) {
                    // так как координаты дороги могут быть как (start->end) {0,0 -> 10,0}, так и {10,0 -> 0,0}
                    // то во избежание проблем используем std::min / std::max, координата pos.x должна быть между ними включительно
                    return &road;
                }
            }
        }

        return nullptr;
    }
    // флаг нахождения точки на вертикальной дороге
    // требуется передача позиции в формате модели с округлением к int
    const Road* Map::stay_on_vertical_road(Point pos) const {
        if (roads_.size() == 0) {
            // если дорог нет, тогда кидаем исключение, что у нас пусто в листе
            throw std::runtime_error("Map::get_road_by_position::Error::No_Roads_on_Map");
        }

        for (const Road& road : roads_) {
            // бежим по списку дорог, если горизонтальной координата игрока переданной позиции совпадает
            // с горизонтальной координатой начала и конца дороги, значит дорога вертикальная и мы стоим на ней
            if (road.GetStart().x == pos.x && road.GetEnd().x == pos.x) {
                // проверяем совпадение по вертикальной координате, так как может быть например две и более вертикальных дорог
                // находящихся на одной и той же горизонтальной координате, например {0,0; 0,10} и {0,20; 0,30}, а pos {0,22}
                // значит мы должны выбрать другую дорогу, первая в списке {0,0; 0,10} нам не подходит

                if (std::min(road.GetStart().y, road.GetEnd().y) <= pos.y
                    && pos.y <= std::max(road.GetStart().y, road.GetEnd().y)) {
                    // так как координаты дороги могут быть как (start->end) {0,0 -> 0,10}, так и {0,10 -> 0,0}
                    // то во избежание проблем используем std::min / std::max, координата pos.y должна быть между ними включительно
                    return &road;
                }
            }
        }

        return nullptr;
    }

    // возвращает случайную дорого на карте
    const Road& Map::get_random_road() const {
        if (roads_.size() != 0) {
            return roads_[random_integer(0, static_cast<int>(roads_.size() - 1))];
        }
        else {

            // можно сделать конечно так, но пока не будем, ограничимся отбойником
            // по логике, карта должна иметь дороги
            //const_cast<Map*>(this)->AddRoad(Road(Road::VERTICAL, Point{ 0, 0 }, Coord{ 0 }));
            //return roads_[0];

            throw std::runtime_error("Map::get_random_road::Error::No_Roads_on_Map");
        }
    }

    // добавляет карту в игровую модель
    void Game::add_map(Map map) {
        const size_t index = maps_.size();
        if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
            throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
        } else {
            try {
                maps_.emplace_back(std::move(map));
            } catch (...) {
                map_id_to_index_.erase(it);
                throw;
            }
        }
    }

}  // namespace model

#include "model.h"

#include <stdexcept>
#include <algorithm>
#include <optional>
#include <random>

namespace model {

    using namespace std::literals;

    int GetRandomInteger(int from, int to) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(from, to);
        return dis(gen);
    }

    double GetRandomDouble() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0.0, 1.0);
        return dis(gen);
    }

    double GetRandomDouble(double from, double to) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(from, to);
        return dis(gen);
    }

    double GetRandomDoubleRoundOne(double from, double to) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(from, to);

        double result = dis(gen);

        if (result >= 0) {
            return std::floor(result * 10) / 10;
        }
        else {
            return std::ceil(result * 10) / 10;
        }
    }

    Point Road::GetRandomPosition() const {
        return IsHorizontal() ?
            Point{ GetRandomInteger(
                std::min(start_.x, end_.x),
                std::max(start_.x, end_.x)), 
            start_.y } :
            Point{ start_.x, GetRandomInteger(
                std::min(start_.y, end_.y),
                std::max(start_.y, end_.y)) };
    }

    // назначает название лута
    LootType& LootType::SetName(std::string&& name) {
        name_ = std::move(name);
        return *this;
    }
    // назначает путь к файлу лута
    LootType& LootType::SetFile(std::string&& file) {
        file_ = std::move(file);
        return *this;
    }
    // назначает тип лута
    LootType& LootType::SetType(std::string&& type) {
        type_ = std::move(type);
        return *this;
    }
    // назначает цвет объкета
    LootType& LootType::SetColor(std::string&& color) {
        color_ = std::move(color);
        return *this;
    }
    // назначает поворот объекта
    LootType& LootType::SetRotation(int rotation) {
        rotation_ = rotation;
        return *this;
    }
    // назначает масштаб объекта
    LootType& LootType::SetScale(double scale) {
        scale_ = scale;
        return *this;
    }

    // добавляет одну дорогу на карту
    Map& Map::AddRoad(const Road& road) {
        roads_.emplace_back(road);
        return *this;
    }
    // добавляет строение на карту
    Map& Map::AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
        return *this;
    }
    // добавляет один офис на карту
    Map& Map::AddOffice(Office office) {
        if (warehouse_id_to_index_.contains(office.GetId())) {
            throw std::invalid_argument("Duplicate warehouse");
        }

        const size_t index = offices_.size();
        Office& o = offices_.emplace_back(std::move(office));
        try {
            warehouse_id_to_index_.emplace(o.GetId(), index);
        }
        catch (...) {
            // Удаляем офис из вектора, если не удалось вставить в unordered_map
            offices_.pop_back();
            throw;
        }

        return *this;
    }
    // добавляет один тип лута на карту
    Map& Map::AddLootType(LootType&& loot_type) {
        loot_types_.push_back(std::move(loot_type));
        return *this;
    }

    // устанавливает подготовленный массив дорог на карту
    Map& Map::SetRoads(Roads&& roads) {
        roads_ = std::move(roads);
        return *this;
    }
    // устанавливает подготовленный массив строений на карту
    Map& Map::SetBuildings(Buildings&& buildings) {
        buildings_ = std::move(buildings);
        return *this;
    }
    // устанавливает скорость движения песелей
    Map& Map::SetOnMapSpeed(double speed) {
        dog_speed_ = speed;
        return *this;
    }
    // устанавлиает количество типов лута на карте
    Map& Map::SetLootTypesCount(size_t count) {
        loot_types_count_ = count;
        return *this;
    }

    // возвращает тип лута по индексу
    LootType Map::GetLootType(size_t index) const {
        if (index < GetLootTypesCount()) {
            // size_t всегда больше нуля и должно быть меньше размера массива
            return loot_types_[index];
        }
        else {
            throw std::out_of_range("model::Map::GetLootType(size_t)::Error::Index is out of range");
        }
    }

    // возвращает стартовую точку первой дороги на карте
    Point Map::GetFirstRoadStartPosition() const {
        if (roads_.size() == 0) {
            // если дорог нет, тогда кидаем исключение, что у нас пусто в листе
            throw std::runtime_error("Map::get_first_map_start_position::Error::No_Roads_on_Map");
        }

        return roads_[0].GetStart();
    }

    // метод возвращает указатель на горизонтальную дорогу по переданной позиции
    // если позиция каким-то образом некорректна, то вернется nullptr
    // требуется передача позиции в формате модели с округлением к int
    const Road* Map::GetHorizontalRoad(Point pos) const {

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

    // метод возвращает указатель на вертикальную дорогу по переданной позиции
    // если позиция каким-то образом некорректна, то вернется nullptr
    // требуется передача позиции в формате модели с округлением к int
    const Road* Map::GetVerticalRoad(Point pos) const {
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
    const Road& Map::GetRandomRoad() const {
        if (roads_.size() != 0) {
            return roads_[GetRandomInteger(0, static_cast<int>(roads_.size() - 1))];
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
    void Game::AddMap(Map map) {
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
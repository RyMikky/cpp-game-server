#include "model.h"

#include <stdexcept>
#include <random>

namespace model {

    using namespace std::literals;

    int random_integer(int from, int to) {

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(from, to);
        return dis(gen);
    }

    Point Road::GetRandomPosition() const {
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

    const Road& Map::GetRandomRoad() const {
        if (roads_.size() != 0) {
            return roads_[random_integer(0, static_cast<int>(roads_.size() - 1))];
        }
        else {

            // можно сделать конечно так, но пока не будем, ограничимся отбойником
            // по логике, карта должна иметь дороги
            //const_cast<Map*>(this)->AddRoad(Road(Road::VERTICAL, Point{ 0, 0 }, Coord{ 0 }));
            //return roads_[0];

            throw std::runtime_error("Map::GetRandomRoad::Error::No_Roads_on_Map");
        }
    }

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

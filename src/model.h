#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <execution>

#include "tagged.h"

namespace model {

    using Dimension = int;
    using Coord = Dimension;

    struct Point {
        Coord x, y;
    };

    struct Size {
        Dimension width, height;
    };

    struct Rectangle {
        Point position;
        Size size;
    };

    struct Offset {
        Dimension dx, dy;
    };

    int random_integer(int from, int to);

    class Road {
        struct HorizontalTag {
            explicit HorizontalTag() = default;
        };
        struct VerticalTag {
            explicit VerticalTag() = default;
        };

    public:
        constexpr static HorizontalTag HORIZONTAL{};
        constexpr static VerticalTag VERTICAL{};

        Road(HorizontalTag, Point start, Coord end_x) noexcept
            : start_{start}
            , end_{end_x, start.y} {
        }

        Road(VerticalTag, Point start, Coord end_y) noexcept
            : start_{start}
            , end_{start.x, end_y} {
        }

        bool IsHorizontal() const noexcept {
            return start_.y == end_.y;
        }

        bool IsVertical() const noexcept {
            return start_.x == end_.x;
        }

        Point GetStart() const noexcept {
            return start_;
        }

        Point GetEnd() const noexcept {
            return end_;
        }

        Point get_random_position() const;

    private:
        Point start_;
        Point end_;
    };

    class Building {
    public:
        explicit Building(Rectangle bounds) noexcept
            : bounds_{bounds} {
        }

        const Rectangle& GetBounds() const noexcept {
            return bounds_;
        }

    private:
        Rectangle bounds_;
    };

    class Office {
    public:
        using Id = util::Tagged<std::string, Office>;

        Office(Id id, Point position, Offset offset) noexcept
            : id_{std::move(id)}
            , position_{position}
            , offset_{offset} {
        }

        const Id& GetId() const noexcept {
            return id_;
        }

        Point GetPosition() const noexcept {
            return position_;
        }

        Offset GetOffset() const noexcept {
            return offset_;
        }

    private:
        Id id_;
        Point position_;
        Offset offset_;
    };

    class Map {
    public:
        using Id = util::Tagged<std::string, Map>;
        using Roads = std::vector<Road>;
        using Buildings = std::vector<Building>;
        using Offices = std::vector<Office>;

        Map(Id id, std::string name) noexcept
            : id_(std::move(id))
            , name_(std::move(name))
            , dog_speed_(1u) {
            // Скорость персонажей на всех картах задаёт опциональное поле defaultDogSpeed в корневом JSON-объекте. 
            // Если это поле отсутствует, скорость по умолчанию считается равной 1.
        }

        Map(Id id, std::string name, double dog_speed) noexcept
            : id_(std::move(id))
            , name_(std::move(name))
            , dog_speed_(dog_speed) {
            // Скорость персонажей на конкретной карте задаёт опциональное поле dogSpeed в соответствующем объекте карты. 
            // Если это поле отсутствует, на карте используется скорость по умолчанию.
        }

        const Id& GetId() const noexcept {
            return id_;
        }

        const std::string& GetName() const noexcept {
            return name_;
        }
        const Buildings& GetBuildings() const noexcept {
            return buildings_;
        }
        const Roads& GetRoads() const noexcept {
            return roads_;
        }
        const Offices& GetOffices() const noexcept {
            return offices_;
        }

        void AddRoad(const Road& road) {
            roads_.emplace_back(road);
        }
        void AddBuilding(const Building& building) {
            buildings_.emplace_back(building);
        }
        void AddOffice(Office office);

        void set_dog_speed(double speed) {
            dog_speed_ = speed;
        }
        double get_dog_speed() const {
            return dog_speed_;
        }
        // возвращает случайную позицию на случайно выбранной дороге на карте
        Point get_random_road_position() const {
            return get_random_road().get_random_position();
        }
        // возвращает ссылку на дорогу по переданной позиции и направлению, или вертикальному или горизонтальному
        const Road& get_road_by_position(Point pos, bool vertical) const;

        // флаг нахождения точки на горизонтальной дороге
        // требуется передача позиции в формате модели с округлением к int
        const Road* stay_on_horizontal_road(Point pos) const;
        // флаг нахождения точки на вертикальной дороге
        // требуется передача позиции в формате модели с округлением к int
        const Road* stay_on_vertical_road(Point pos) const;

    private:
        using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

        Id id_;
        std::string name_;
        Roads roads_;
        Buildings buildings_;
        double dog_speed_;

        OfficeIdToIndex warehouse_id_to_index_;
        Offices offices_;

        // возвращает случайную дорого на карте
        const Road& get_random_road() const;
    };

    class Game {
    public:
        using Maps = std::vector<Map>;

        // добавляет карту в игровую модель
        void add_map(Map map);
        // возвращает лист карт игровой модели
        const Maps& get_maps() const noexcept {
            return maps_;
        }
        // ищет карту в игровой модели по id 
        const Map* find_map(const Map::Id& id) const noexcept {
            if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
                return &maps_.at(it->second);
            }
            return nullptr;
        }

    private:
        using MapIdHasher = util::TaggedHasher<Map::Id>;
        using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

        std::vector<Map> maps_;
        MapIdToIndex map_id_to_index_;
    };

}  // namespace model

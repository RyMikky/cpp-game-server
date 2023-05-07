#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <execution>

#include "tagged.h"
#include "extra_data.h"
#include "loot_generator.h"

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

    // возвращает случайное целое число в диапазоне from -> to
    int GetRandomInteger(int from, int to);
    // возвращает случайное вещественное число в диапазоне 0.0 -> 1.0
    double GetRandomDouble();
    // возвращает случайное вещественное число в диапазоне from -> to
    double GetRandomDouble(double from, double to);

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

        Point GetRandomPosition() const;

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

    /*
    * Конфиг класса данных о луте на карте.
    * В данной версии кода никак не используется, так как отдано на откуп frontend
    */
    struct LootTypeConfig {
        std::string name;
        std::string file;
        std::string type;
        int rotation;
        std::string color;
        double scale;
    };

    /*
    * Класс данные о луте на карте.
    * В данной версии кода никак не используется, так как отдано на откуп frontend
    */
    class LootType {
    public:
        LootType() = default;

        explicit LootType(LootTypeConfig&& config) :
            name_(std::move(config.name)), file_(std::move(config.file)), type_(std::move(config.type)), 
            rotation_(config.rotation), color_(std::move(config.color)), scale_(config.scale) {
        }

        LootType(std::string&& name, std::string&& file, 
            std::string&& type, int rotation, std::string&& color, double scale)
            : name_(std::move(name)), file_(std::move(file)), 
            type_(std::move(type)), rotation_(rotation), color_(std::move(color)), scale_(scale) {
        }

        // назначает название лута
        LootType& SetName(std::string&&);
        // назначает путь к файлу лута
        LootType& SetFile(std::string&&);
        // назначает тип лута
        LootType& SetType(std::string&&);
        // назначает цвет объкета
        LootType& SetColor(std::string&&);
        // назначает поворот объекта
        LootType& SetRotation(int);
        // назначает масштаб объекта
        LootType& SetScale(double);

        // возвращает название лута
        const std::string GetName() const {
            return name_;
        }
        // возвращает путь к файлу лута
        const std::string GetFile() const {
            return file_;
        }
        // возвращает тип лута
        const std::string GetType() const {
            return type_;
        }
        // возвращает цвет объекта
        const std::string GetColor() const {
            return color_;
        }
        // возвращает текущий поворот объекта
        double GetRotation() const {
            return rotation_;
        }
        // возвращает текущий масштаб объекта
        double GetScale() const {
            return scale_;
        }
        
    private:
        std::string name_ = "";
        std::string file_ = "";
        std::string type_ = "";
        int rotation_ = 0;
        std::string color_ = "";
        double scale_ = 0.0;
    };

    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;
    using LootTypes = std::vector<LootType>;

    /*
    * Карта игровой модели, включает в себя данные о 
    * Дорогах, зданиях, офисах. Наследуется от класса
    * ExtraDataCollector, и может содержать дополнительные данные
    * Не используемые в данной реализации backend-кода
    */
    class Map : public extra_data::ExtraDataCollector {
    public:
        using Id = util::Tagged<std::string, Map>;

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

        // добавляет одну дорогу на карту
        Map& AddRoad(const Road&);
        // добавляет строение на карту
        Map& AddBuilding(const Building&);
        // добавляет один офис на карту
        Map& AddOffice(Office);
        // добавляет один тип лута на карту
        // в данный момент не используется
        Map& AddLootType(LootType&&);

        // устанавливает подготовленный массив дорог на карту
        Map& SetRoads(Roads&&);
        // устанавливает подготовленный массив строений на карту
        Map& SetBuildings(Buildings&&);
        // устанавливает скорость движения песелей
        Map& SetOnMapSpeed(double);
        // устанавлиает количество типов лута на карте
        Map& SetLootTypesCount(size_t);

        // возвращает Id карты
        const Id& GetId() const noexcept {
            return id_;
        }
        // возвращает имя карты
        const std::string& GetName() const noexcept {
            return name_;
        }
        // возвращает массив зданий на карте
        const Buildings& GetBuildings() const noexcept {
            return buildings_;
        }
        // возвращает массив дорог на карте
        const Roads& GetRoads() const noexcept {
            return roads_;
        }
        // возвращает массив офисов на карте
        const Offices& GetOffices() const noexcept {
            return offices_;
        }
        // возвращает массив типов лута на карте
        // на данный момент не используется
        const LootTypes& GetLootTypes() const noexcept {
            return loot_types_;
        }
        // возвращает скорость движения песелей на карте
        double GetOnMapSpeed() const {
            return dog_speed_;
        }

        // возвращает количество типов лута в массиве
        size_t GetLootTypesCount() const {
            return loot_types_count_;
            //return loot_types_.size();
        }
        // возвращает тип лута по индексу
        // на данный момент не используется
        LootType GetLootType(size_t) const;
        
        // возвращает случайную позицию на случайно выбранной дороге на карте
        Point GetRandomPosition() const {
            return GetRandomRoad().GetRandomPosition();
        }
        // возвращает стартовую точку первой дороги на карте
        Point GetFirstRoadStartPosition() const;
        // метод возвращает указатель на горизонтальную дорогу по переданной позиции
        // если позиция каким-то образом некорректна, то вернется nullptr
        // требуется передача позиции в формате модели с округлением к int
        const Road* GetHorizontalRoad(Point pos) const;
        // метод возвращает указатель на вертикальную дорогу по переданной позиции
        // если позиция каким-то образом некорректна, то вернется nullptr
        // требуется передача позиции в формате модели с округлением к int
        const Road* GetVerticalRoad(Point pos) const;

    private:
        using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

        Id id_;                                                          // Id карты
        std::string name_;                                               // Название карты
        Roads roads_;                                                    // Массив с дорогами на карте
        Buildings buildings_;                                            // Массив зданий на карте
        double dog_speed_ = 0.0;                                         // Пёсья скорость перемещения на карте
        size_t loot_types_count_ = 0;                                    // Количество типов лута на карте

        OfficeIdToIndex warehouse_id_to_index_;                          // Мапа быстрого поиска офисов
        Offices offices_;                                                // Массив офисов
        LootTypes loot_types_;                                           // Массив типов лута

        // возвращает случайную дорого на карте
        const Road& GetRandomRoad() const;
    };


    using Maps = std::vector<Map>;

    class Game {
    public:
        // добавляет карту в игровую модель
        void AddMap(Map map);
        // возвращает лист карт игровой модели
        const Maps& GetMaps() const noexcept {
            return maps_;
        }
        // ищет карту в игровой модели по id 
        const Map* FindMap(const Map::Id& id) const noexcept {
            if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
                return &maps_.at(it->second);
            }
            return nullptr;
        }

        // устанавливает базовую скорость движения песелей
        void SetDefaultDogSpeed(double speed) {
            default_dog_speed_ = speed;
        }
        // возвращает базовую скорость движения песелей
        double GetDefaultDogSpeed() const {
            return default_dog_speed_;
        }
        // устанавливает настройки генератора лута
        void SetLootGenConfig(loot_gen::LootGeneratorConfig&& config) {
            loot_gen_config_ = std::move(config);
        }
        // возвращает копию конфига генератора, там всего два дабла, так что не особо накладно
        loot_gen::LootGeneratorConfig GetLootGenConfig() const {
            return loot_gen_config_;
        }

    private:
        using MapIdHasher = util::TaggedHasher<Map::Id>;
        using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

        std::vector<Map> maps_;
        MapIdToIndex map_id_to_index_;

        double default_dog_speed_;                                       // Базовая пёсья скорость перемещения на картах
        loot_gen::LootGeneratorConfig loot_gen_config_;                  // Файл конфигурации генератора лута
    };

}  // namespace model
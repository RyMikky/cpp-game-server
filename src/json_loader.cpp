#include "json_loader.h"

namespace json_loader {

    namespace detail {

        // парсер элементов карты - здания
        void ParseMapBuildingsData(model::Map& map, json::value&& builds) {
        
            for (auto element : builds.as_array()) {

                // создаём точку позиции офисного здания
                model::Point build_position{
                    static_cast<int>(element.at("x").as_int64()),
                    static_cast<int>(element.at("y").as_int64())
                };

                // создаём оффсет офисного здания
                model::Size build_size {
                    static_cast<int>(element.at("w").as_int64()),
                    static_cast<int>(element.at("h").as_int64())
                };

                // добавляем строение на карту
                map.AddBuilding(
                    model::Building{
                        {
                            build_position, build_size
                        }
                    }
                );
            }
        };

        // парсер элементов карты - офисы
        void ParseMapOfficesData(model::Map& map, json::value&& offices) {

            for (auto element : offices.as_array()) {

                // создаём точку позиции офисного здания
                model::Point office_position {
                    static_cast<int>(element.at("x").as_int64()),
                    static_cast<int>(element.at("y").as_int64())
                };

                // создаём оффсет офисного здания
                model::Offset office_offset{
                    static_cast<int>(element.at("offsetX").as_int64()),
                    static_cast<int>(element.at("offsetY").as_int64())
                };
                
                // создаём офисное здание
                model::Office office {
                    model::Office::Id {
                        element.at("id").as_string().data()
                    },
                    office_position, office_offset
                };

                // добавляем здание в карту
                map.AddOffice(office);
            }
        };

        // парсер элементов карты - дороги
        void ParseMapRoadsData(model::Map& map, json::value&& roads) {

            for (auto element : roads.as_array()) {

                // создаём точку начала дороги
                model::Point start_point{
                    static_cast<int>(element.at("x0").as_int64()),
                    static_cast<int>(element.at("y0").as_int64())
                };

                model::Coord x1 = start_point.x;
                model::Coord y1 = start_point.y;

                try
                {
                    // пробуем "найти" следующую координату по иксу
                    x1 = static_cast<int>(element.at("x1").as_int64());
                    // если координата есть, тогда добавляем горизонтальную дорогу
                    map.AddRoad(
                        model::Road(
                            model::Road::HORIZONTAL, start_point, x1
                        ));

                }
                catch (const std::exception&)
                {
                    // пробуем "найти" следующую координату по игрек
                    y1 = static_cast<int>(element.at("y1").as_int64());
                    // если координата есть, тогда добавляем вертикальную дорогу
                    map.AddRoad(
                        model::Road(
                            model::Road::VERTICAL, start_point, y1
                        ));

                    // ловить второе исключение при отсутствии данных нет смысла - программа завершится с ошибкой
                }
            }
        }

        // парсер элементов карты - типы лута
        // на данный момент не используется
        void ParseMapLootTypesData(model::Map& map, json::value&& loot_types) {

            for (auto element : loot_types.as_array()) {

                model::LootType loot;       // создаём пустышку

                loot // заполняем обязательные элементы, которые должны быть в валидном конфиге
                    .SetName(std::move(element.at("name").as_string().data()))
                    .SetFile(std::move(element.at("file").as_string().data()))
                    .SetType(std::move(element.at("type").as_string().data()))
                    .SetScale(element.at("scale").as_double());

                if (element.as_object().count("color")) {
                    // если есть упоминание о цвете, то записываем его
                    loot.SetColor(std::move(element.at("color").as_string().data()));
                }

                if (element.as_object().count("rotation")) {
                    // если есть упоминание о повороте, то записываем его
                    loot.SetRotation(static_cast<int>(element.at("rotation").as_int64()));
                }

                // записываем данные о луте в карту
                map.AddLootType(std::move(loot));
            }
        }

        // парсер карт для созданной игровой модели
        void ParseGameMapsData(model::Game& game, json::value&& maps, double default_dog_speed) {

            // начинаем перебирать массив с данными по картам
            for (auto& element : maps.as_array()) {

                // создаём карту по полученным данным
                model::Map map{
                    model::Map::Id {
                        element.at("id").as_string().data()
                    },
                    element.at("name").as_string().data()
                };

                // если элемент как словарь имеет запись о скорости песелей
                if (element.as_object().count("dogSpeed")) {
                    // назначаем сеттером скорость из словаря
                    map.SetOnMapSpeed(element.at("dogSpeed").as_double());
                }
                else {
                    map.SetOnMapSpeed(default_dog_speed);
                }

                // парсим данные по луту, дорогам, домам и офисам
                // если данных нет, то будет выкинуто исключение, что собственно прекратит работу
                // парсинг типов лута на данном этапе отключен, так как они не нужны backend-у
                //ParseMapLootTypesData(map, element.at("lootTypes").as_array());
                ParseMapRoadsData(map, element.at("roads").as_array());
                ParseMapOfficesData(map, element.at("offices").as_array());
                ParseMapBuildingsData(map, element.at("buildings").as_array());
                
                // назначаем на карте количество типов лута
                map.SetLootTypesCount(element.at("lootTypes").as_array().size());
                // с помощью класса-родителя ExtraDataCollector запоминаем массив с типами лута
                // при обработке REST API api/v1/maps/{карта} эти данные будут добавленны в вывод
                /*map.AddExtraData("lootTypes", model::extra_data::ExtraDataType::template_array, std::move(
                    std::make_shared<model::extra_data::ExtraTemplateArrayData<boost::json::array>>(
                        std::move(boost::json::array{ element.at("lootTypes").as_array() }))));*/

                map.AddExtraArrayData("lootTypes", std::move(boost::json::array{ element.at("lootTypes").as_array() }));

                game.AddMap(map);                       // добавляем карту игровой модели
            }
        }

        // парсер настройки генератора лута
        loot_gen::LootGeneratorConfig ParseGameLootGenConfig(json::value&& config) {

            return loot_gen::LootGeneratorConfig()
                .SetPeriod(config.at("period").as_double())
                .SetProbability(config.at("probability").as_double());
        }

        /*
        * Базовый конфигуратор игровой модели.
        * Подготавливает игровую модель по общим данным.
        * Запрашивает конфигурацию дополнительных элементов у блока парсеров карт и их содержимого.
        */
        model::Game ParseGameBaseConfig(json::object&& config) {

            model::Game result;  // создаём пустое возвращаемое значение

            if (config.count("defaultDogSpeed")) {
                // назначаем базовую скорость перемещения песелей на картах
                result.SetDefaultDogSpeed(config.at("defaultDogSpeed").as_double());
            }
            else {
                // иначе кидаем исключение, так как эти данные должны быть в обязательном порядке
                throw std::runtime_error("json_loader::detail::ParseGameBaseConfig::Error::Configuration file lost data {defaultDogSpeed}");
            }

            if (config.count("lootGeneratorConfig")) {
                // парсим и назначаем настройки генератора лута
                result.SetLootGenConfig(std::move(
                    detail::ParseGameLootGenConfig(
                        std::move(config.at("lootGeneratorConfig")))));
            }
            else {
                // иначе кидаем исключение, так как эти данные должны быть в обязательном порядке
                throw std::runtime_error("json_loader::detail::ParseGameBaseConfig::Error::Configuration file lost data {lootGeneratorConfig}");
            }

            if (config.count("maps")) {
                // выполняем наполнение игровой модели картами
                detail::ParseGameMapsData(result, std::move(config.at("maps")), result.GetDefaultDogSpeed());
            }
            else {
                // иначе кидаем исключение, так как эти данные должны быть в обязательном порядке
                throw std::runtime_error("json_loader::detail::ParseGameBaseConfig::Error::Configuration file lost data {maps}");
            }

            return result;          // возвращаем подготовленную игровую модель
        }

    } // namespace detail

    model::Game LoadGameConfiguration(const std::filesystem::path& json_path) {
        // Загрузить содержимое файла json_path, например, в виде строки
        // Распарсить строку как JSON, используя boost::json::parse
        // Загрузить модель игры из файла

        // для начала создаём поток и пытаемся открыть файл
        std::ifstream file(json_path);
        if (!file.is_open()) {
            // бросаем исключение в случае ошибки
            throw std::runtime_error("Failed to open file: " + json_path.generic_string());
        }

        // загружаем содержимое в строку
        std::string text(
            (std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
        
        try
        {
            // делаем репарсинг данных в boost::json формат пытаясь сразу преобразоваться в словарь
            auto config = json_detail::ParseTextToJSON(text).as_object();
            // передаем блок конфигурации в базовую функцию подготовки игровой модели
            return detail::ParseGameBaseConfig(std::move(config));
        }
        catch (const std::exception& e)
        {
            // на все исключения кидаем отбойник дальше по цепочке вызова
            throw std::runtime_error("json_loader::LoadGame::ParseError::" + std::string(e.what()));
        }
    }

}  // namespace json_loader
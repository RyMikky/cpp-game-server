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

        // базовый парсер элементов полученного config.json
        model::Game ParseGameMapsData(json::value&& maps) {

            model::Game result;  // создаём пустое возвращаемое значение

            // начинаем перебирать массив с данными по картам
            for (auto element : maps.as_array()) {

                // создаём карту по полученным данным
                model::Map map{
                    model::Map::Id {
                        element.at("id").as_string().data()
                    },
                    element.at("name").as_string().data()
                };

                // парсим данные по дорогам, домам и офисам
                // если данных нет, то будет выкинуто исключение, что собственно прекратит работу
                ParseMapRoadsData(map, element.at("roads").as_array());
                ParseMapOfficesData(map, element.at("offices").as_array());
                ParseMapBuildingsData(map, element.at("buildings").as_array());

                result.AddMap(map);              // не забываем добавить созданную карту в игру
            }

            return result;                       // возвращаем результат работы конфигураторов
        }

    } // namespace detail

    model::Game LoadGame(const std::filesystem::path& json_path) {
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
        
        // делаем репарсинг данных в boost::json формат
        auto boost_json_data = json_detail::ParseTextToBoostJson(text);

        try
        {
            // если в полуенном жидомасоне есть массив с картами продолжаем настройку сервера
            json::value maps = boost_json_data.at("maps");
            return detail::ParseGameMapsData(std::move(maps));
        }
        catch (const std::exception&)
        {
            // иначе бросаем исключение, что данных то по факту нету
            throw std::runtime_error("configure JSON has no key \"maps\"");
        }
    }

}  // namespace json_loader
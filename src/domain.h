﻿// базовый инклюд объединяющий раные типы данных применяемые по всей программе
// требуется для работы сервера, обработчика запросов, обработчика игры и её состояний

#pragma once

#include "sdk.h"
#include "model.h"
#include "player.h"
// boost.beast будет использовать std::string_view вместо boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>


#include <unordered_map>
#include <string_view>
#include <filesystem>
#include <variant>
#include <optional>
#include <memory>
#include <deque>

using namespace std::literals;

namespace http_handler {

    namespace fs = std::filesystem;
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace sys = boost::system;
    namespace net = boost::asio;

    // пароль для тестовой системы для обработки запросов из фрейма проверки по установке различных состояний игровой системы
    const std::string __DEBUG_REQUEST_AUTORIZATION_PASSWORD__ = "wCp74sDSs12-D7Er4+471cAwdXzV4q1Y"s;

    using Strand = net::strand<net::io_context::executor_type>;

    static const std::unordered_map<std::string, char> __ASCII_CODE_TO_CHAR__ = {
        {"%20", ' '}, {"%21", '!'}, {"%22", '\"'}, {"%23", '#'}, {"%24", '$'}, {"%25", '%'}, {"%26", '&'}, {"%27", '\''}, {"%28", '('}, {"%29", ')'},
        {"%2a", '*'}, {"%2b", '+'}, {"%2c", ','}, {"%2d", '-'}, {"%2e", '.'}, {"%2f", '/'}, {"%30", '`'}, {"%3a", ':'}, {"%3b", ';'}, {"%3c", '<'},
        {"%3d", '='}, {"%3e", '>'}, {"%3f", '?'}, {"%40", '@'}, {"%5b", '['}, {"%5c", '\\'}, {"%5d", ']'}, {"%5e", '^'}, {"%5f", '_'}
    };

    // Запрос, тело которого представлено в виде строки
    using StringRequest = http::request<http::string_body>;
    // Ответ, тело которого представлено в виде строки
    using StringResponse = http::response<http::string_body>;
    // Ответ, тело которого представленно в виде файла
    using FileResponse = http::response<http::file_body>;
    // Варианты ответов на запросы
    using Response = std::variant<std::monostate, StringResponse, FileResponse>;

#define IS_FILE_RESPONSE(response) std::holds_alternative<http_handler::FileResponse>(response) 
#define IS_STRING_RESPONSE(response) std::holds_alternative<http_handler::StringResponse>(response) 

    struct ContentType {
        ContentType() = delete;
        constexpr static std::string_view TEXT_HTML = "text/html"sv;                  // для .htm, .html
        constexpr static std::string_view TEXT_CSS = "text/css"sv;                    // для .css
        constexpr static std::string_view TEXT_TXT = "text/plain"sv;                  // для .txt
        constexpr static std::string_view TEXT_JS = "text/javascript"sv;              // для .js
        constexpr static std::string_view APP_JSON = "application/json"sv;            // для .json
        constexpr static std::string_view APP_XML = "application/xml"sv;              // для .xml
        constexpr static std::string_view IMAGE_PNG = "image/png"sv;                  // для .png
        constexpr static std::string_view IMAGE_JPEG = "image/jpeg"sv;                // для .jpg, .jpe, .jpeg
        constexpr static std::string_view IMAGE_GIF = "image/gif"sv;                  // для .gif
        constexpr static std::string_view IMAGE_BMP = "image/bmp"sv;                  // для .bmp
        constexpr static std::string_view IMAGE_ICO = "image/vnd.microsoft.icon"sv;   // для .ico
        constexpr static std::string_view IMAGE_TIFF = "image/tiff"sv;                // для .tif, .tiff
        constexpr static std::string_view IMAGE_SVG = "image/svg+xml"sv;              // для .svg, .svgz
        constexpr static std::string_view AUDIO_MPEG = "audio/mpeg"sv;                // для .mp3
        constexpr static std::string_view APP_UNKNOW = "application/octet-stream"sv;  // для .unknow
    };

    struct Method {
        Method() = delete;

        constexpr static std::string_view GET = "GET"sv;
        constexpr static std::string_view HEAD = "HEAD"sv;
        constexpr static std::string_view POST = "POST"sv;
    };

} // namespace http_handler

namespace resource_handler {

    enum class ResourceType {
        root, folder, html, htm, css, txt, json, js, xml, png, jpg, jpe, jpeg, gif, bmp, ico, tif, tiff, svg, svgz, mp3, unknow
    };

    static const std::unordered_map<std::string, ResourceType> __FILES_EXTENSIONS__ = {
        {"html", ResourceType::html}, {"htm", ResourceType::htm}, {"css", ResourceType::css}, {"txt", ResourceType::txt},
        {"json", ResourceType::json}, {"js", ResourceType::js}, {"xml", ResourceType::xml}, {"png", ResourceType::png},
        {"jpg", ResourceType::jpg}, {"jpe", ResourceType::jpe}, {"jpeg", ResourceType::jpeg}, {"gif", ResourceType::gif},
        {"bmp", ResourceType::bmp}, {"ico", ResourceType::ico}, {"tif", ResourceType::tif}, {"tiff", ResourceType::tiff},
        {"svg", ResourceType::svg}, {"svgz", ResourceType::svgz}, {"mp3", ResourceType::mp3}
    };

    struct ResourceItem {
        std::string _name = "";
        std::string _path = "";
        ResourceType _type = ResourceType::unknow;
    };

    using ResourcePtr = ResourceItem*;

    // базовый массив данных в виде "название файла / путь к файлу"
    using FileIndexNameToPath = std::unordered_map<std::string_view, ResourcePtr>;
    // базовый массив данных в виде "путь к файлу / название файла"
    using FileIndexPathToName = std::unordered_map<std::string_view, ResourcePtr>;

} // namespace resource_handler

namespace game_handler {

    const double __ROAD_DELTA__ = 0.4;                     // дельта отступа от центра оси дороги
    const int __MS_IN_ONE_SECOND__ = 1000;                 // миллисекунд в одной секунде

    class TokenHasher {
    public:
        std::size_t operator()(const Token& token) const noexcept {
            return _hasher(*token);
        }
    private:
        std::hash<std::string> _hasher;
    };

    class TokenPtrHasher {
    public:
        std::size_t operator()(const Token* token) const noexcept {
            return _hasher(*(*token));
        }
    private:
        std::hash<std::string> _hasher;
    };

    struct GameLoot/* : public model::LootType */{
        
        GameLoot(/*model::LootType loot_type, */size_t type, PlayerPosition position)
            : /*model::LootType(loot_type),*/ type_(type), pos_(position) {
        }
        // привязка к неиспользуемым элементам модели закомментирована

        size_t type_;                // он же индекс в массиве LootTypes на карте
        PlayerPosition pos_;         // позиция берется не из модели, а из игрока, так как в модели она в инте, а надо в дабле
    };

    using SessionPlayers = std::unordered_map<const Token*, Player, TokenPtrHasher>;
    using SessionLoots = std::unordered_map<size_t, GameLoot>;
    using SessionMapper = std::unordered_map<PosPtr, const Token*, PosPtrHasher>;

    using SPIterator = std::unordered_map<const game_handler::Token* const, game_handler::Player>::const_iterator;
 
} // namespace game_handler
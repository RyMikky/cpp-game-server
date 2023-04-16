// ������� ������ ������������ ����� ���� ������ ����������� �� ���� ���������
// ��������� ��� ������ �������, ����������� ��������, ����������� ���� � � ���������

#pragma once

#include "sdk.h"
#include "model.h"
#include "token.h"
// boost.beast ����� ������������ std::string_view ������ boost::string_view
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

//#include "boost_json.h"

#include <unordered_map>
#include <string_view>
#include <filesystem>
#include <variant>
#include <optional>
#include <memory>
#include <deque>

using namespace std::literals;

namespace http_handler {

    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace sys = boost::system;

    static const std::unordered_map<std::string, char> __ASCII_CODE_TO_CHAR__ = {
        {"%20", ' '}, {"%21", '!'}, {"%22", '\"'}, {"%23", '#'}, {"%24", '$'}, {"%25", '%'}, {"%26", '&'}, {"%27", '\''}, {"%28", '('}, {"%29", ')'},
        {"%2a", '*'}, {"%2b", '+'}, {"%2c", ','}, {"%2d", '-'}, {"%2e", '.'}, {"%2f", '/'}, {"%30", '`'}, {"%3a", ':'}, {"%3b", ';'}, {"%3c", '<'},
        {"%3d", '='}, {"%3e", '>'}, {"%3f", '?'}, {"%40", '@'}, {"%5b", '['}, {"%5c", '\\'}, {"%5d", ']'}, {"%5e", '^'}, {"%5f", '_'}
    };

    // ������, ���� �������� ������������ � ���� ������
    using StringRequest = http::request<http::string_body>;
    // �����, ���� �������� ������������ � ���� ������
    using StringResponse = http::response<http::string_body>;
    // �����, ���� �������� ������������� � ���� �����
    using FileResponse = http::response<http::file_body>;
    // �������� ������� �� �������
    using Response = std::variant<std::monostate, StringResponse, FileResponse>;

#define IS_FILE_RESPONSE(response) std::holds_alternative<http_handler::FileResponse>(response) 
#define IS_STRING_RESPONSE(response) std::holds_alternative<http_handler::StringResponse>(response) 

    struct ContentType {
        ContentType() = delete;
        constexpr static std::string_view TEXT_HTML = "text/html"sv;                  // ��� .htm, .html
        constexpr static std::string_view TEXT_CSS = "text/css"sv;                    // ��� .css
        constexpr static std::string_view TEXT_TXT = "text/plain"sv;                  // ��� .txt
        constexpr static std::string_view TEXT_JS = "text/javascript"sv;              // ��� .js
        constexpr static std::string_view APP_JSON = "application/json"sv;            // ��� .json
        constexpr static std::string_view APP_XML = "application/xml"sv;              // ��� .xml
        constexpr static std::string_view IMAGE_PNG = "image/png"sv;                  // ��� .png
        constexpr static std::string_view IMAGE_JPEG = "image/jpeg"sv;                // ��� .jpg, .jpe, .jpeg
        constexpr static std::string_view IMAGE_GIF = "image/gif"sv;                  // ��� .gif
        constexpr static std::string_view IMAGE_BMP = "image/bmp"sv;                  // ��� .bmp
        constexpr static std::string_view IMAGE_ICO = "image/vnd.microsoft.icon"sv;   // ��� .ico
        constexpr static std::string_view IMAGE_TIFF = "image/tiff"sv;                // ��� .tif, .tiff
        constexpr static std::string_view IMAGE_SVG = "image/svg+xml"sv;              // ��� .svg, .svgz
        constexpr static std::string_view AUDIO_MPEG = "audio/mpeg"sv;                // ��� .mp3
        constexpr static std::string_view APP_UNKNOW = "application/octet-stream"sv;  // ��� .unknow
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

    struct ResourceItem {
        std::string _name = "";
        std::string _path = "";
        ResourceType _type = ResourceType::unknow;
    };

    using ResourcePtr = ResourceItem*;

    // ������� ������ ������ � ���� "�������� ����� / ���� � �����"
    using FileIndexNameToPath = std::unordered_map<std::string_view, ResourcePtr>;
    // ������� ������ ������ � ���� "�������� ����� / ���� � �����"
    using FileIndexPathToName = std::unordered_map<std::string_view, ResourcePtr>;

} // namespace resource_handler

namespace game_handler {

    class Player {
    public:
        Player() = default;

        Player(const Player&) = delete;
        Player& operator=(const Player&) = delete;
        Player(Player&&) = default;
        Player& operator=(Player&&) = default;

        Player(uint16_t id, std::string_view name, const Token* token)
            : id_(id), name_(name), token_(token) {
        };

        uint16_t get_player_id() const {
            return id_;
        }
        std::string_view get_player_name() const {
            return name_;
        }
        std::string_view get_player_token() const {
            return **token_;
        }

    private:
        uint16_t id_ = 65535;
        std::string name_ = "dummy"s;
        const Token* token_ = nullptr;
    };

    using PlayerPtr = const Player*;

} // namespace game_handler
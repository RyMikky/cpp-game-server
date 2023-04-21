﻿#include "request_handler.h"
#include "boost_json.h"

namespace http_handler {

    // возвращает запрошенный документ
    Response RequestHandler::MainFileBodyResponse(StringRequest&& req, const resource_handler::ResourcePtr& resource) {
        FileResponse response(http::status::ok, req.version());

        // в огромном свиче выбираем тип контента
        switch (resource->_type)
        {
        case resource_handler::ResourceType::html:
        case resource_handler::ResourceType::htm:
            response.set(http::field::content_type, ContentType::TEXT_HTML);
            break;

        case resource_handler::ResourceType::txt:
        case resource_handler::ResourceType::folder:
            response.set(http::field::content_type, ContentType::TEXT_TXT);
            break;

        case resource_handler::ResourceType::css:
            response.set(http::field::content_type, ContentType::TEXT_CSS);
            break;

        case resource_handler::ResourceType::js:
            response.set(http::field::content_type, ContentType::TEXT_JS);
            break;

        case resource_handler::ResourceType::json:
            response.set(http::field::content_type, ContentType::APP_JSON);
            break;

        case resource_handler::ResourceType::xml:
            response.set(http::field::content_type, ContentType::APP_XML);
            break;

        case resource_handler::ResourceType::png:
            response.set(http::field::content_type, ContentType::IMAGE_PNG);
            break;

        case resource_handler::ResourceType::jpg:
        case resource_handler::ResourceType::jpe:
        case resource_handler::ResourceType::jpeg:
            response.set(http::field::content_type, ContentType::IMAGE_JPEG);
            break;

        case resource_handler::ResourceType::gif:
            response.set(http::field::content_type, ContentType::IMAGE_GIF);
            break;

        case resource_handler::ResourceType::bmp:
            response.set(http::field::content_type, ContentType::IMAGE_BMP);
            break;

        case resource_handler::ResourceType::ico:
            response.set(http::field::content_type, ContentType::IMAGE_ICO);
            break;

        case resource_handler::ResourceType::tif:
        case resource_handler::ResourceType::tiff:
            response.set(http::field::content_type, ContentType::IMAGE_TIFF);
            break;

        case resource_handler::ResourceType::svg:
        case resource_handler::ResourceType::svgz:
            response.set(http::field::content_type, ContentType::IMAGE_SVG);
            break;

        case resource_handler::ResourceType::mp3:
            response.set(http::field::content_type, ContentType::AUDIO_MPEG);
            break;

        case resource_handler::ResourceType::unknow:
            response.set(http::field::content_type, ContentType::APP_UNKNOW);
            break;

        default:
            break;
        }

        // открываем файл
        http::file_body::value_type file;
        if (sys::error_code ec; file.open(resource->_path.data(), beast::file_mode::read, ec), ec) {
            std::cerr << "Failed to open file "sv << resource->_path << std::endl;
            assert(false);
        }

        response.body() = std::move(file);
        response.prepare_payload();

        return response;
    }

    // возвращает index.html основной страницы
    Response RequestHandler::MainRootIndexResponse(StringRequest&& req) {
        FileResponse response(http::status::ok, req.version());
        response.set(http::field::content_type, ContentType::TEXT_HTML);

        std::string path_line = std::string(resource_.GetRootDirectoryPath()) + "index.html";

        http::file_body::value_type file;
        if (sys::error_code ec; file.open(path_line.data(), beast::file_mode::read, ec), ec) {
            std::cerr << "Failed to open file "sv << path_line << std::endl;
            assert(false);
        }

        response.body() = std::move(file);
        response.prepare_payload();

        return response;
    }

    Response RequestHandler::MainNotFoundResponse(StringRequest&& req) {
        StringResponse response(http::status::not_found, req.version());
        response.set(http::field::content_type, ContentType::TEXT_TXT);
        response.body() = json_detail::GetErrorString("NotFound"sv, "file not found"sv);

        return response;
    }

    Response RequestHandler::MainBadRequestResponse(StringRequest&& req) {
        StringResponse response(http::status::bad_request, req.version());
        response.set(http::field::content_type, ContentType::TEXT_TXT);
        response.body() = json_detail::GetErrorString("BadRequest"sv, "access denied"sv);

        return response;
    }

    // обработчик запросов для к api-игрового сервера
    Response RequestHandler::ApiRequestHandle(StringRequest&& req, std::string_view api_request_line) {

        if (api_request_line.size() == 0) {
            // если предается голое "api", то вызываем ответ по ошибке
            return game_.bad_request_response(std::move(req), "badRequest"sv, "Bad request"sv);
            //return ApiBadRequestResponse(std::move(req));
        }

        if (api_request_line == "/v1/maps"sv) {
            // выводим список доступных карт
            return game_.map_list_response(std::move(req));
            //return ApiMapsListResponse(std::move(req));
        }

        if (api_request_line == "/v1/game/join"sv) {
            // обрабатываем запрос по присоединению к игре
            return game_.join_game_response(std::move(req));
        }

        if (api_request_line == "/v1/game/players"sv) {
            // обрабатываем запрос по выдаче информации о подключенных игроках к сессии
            return game_.player_list_response(std::move(req));
        }

        if (api_request_line == "/v1/game/state"sv) {
            // обрабатываем запрос по получению инфы о игровом состоянии персонажей
            return game_.game_state_response(std::move(req));
        }

        if (api_request_line == "/v1/game/player/action"sv) {
            // обрабатываем запрос по совершению действий персонажем
            return game_.player_action_response(std::move(req));
        }

        // важный момент парсинга - блок сработает только если строка больше 9 символов и первые слова "/v1/maps/"
        // по идее сюда можно добавлять разные элементы, если их будет много то имеет смысл сделать специализированный парсер
        if (api_request_line.size() >= 9 && std::string{ api_request_line.begin(),  api_request_line.begin() + 9 } == "/v1/maps/"sv) {
            // отправляемся на поиски запрошенной карты
            return game_.find_map_response(std::move(req),
                { api_request_line.begin() + 9, api_request_line.end() });
        }

        // на крайний случай просто скажем, что запрос плохой
        return game_.bad_request_response(std::move(req), "badRequest"sv, "Bad request"sv);
    }

    // базовый парсер запросов
    Response RequestHandler::RequestParser(StringRequest&& req) {

        // если у нас просто переход по адресу, или с указанием странички index.html
        if (req.target() == "/"sv || req.target() == "/index.html"sv) {
            return MainRootIndexResponse(std::move(req));
        }

        // либо строка содержит только "api", либо имеет продолжение вида "api/"
        if (req.target().substr(0, 4) == "/api"sv && req.target().size() == 4
            || req.target().substr(0, 5) == "/api/"sv) {

            // передаем управление специализированному методу
            return ApiRequestHandle(std::move(req), { req.target().begin() + 4, req.target().end() });
        }

        // для начала сделаем репарсинг полученной строки запроса, отсекаем первый слеш за ненадобностью
        std::string reparse_line = RequestTargetParser(req.target().begin() + 1, req.target().end());

        // ищем выход за пределы рута, в левом запросе будет так или иначе в начале присутствовать замаскированный обратный слеш
        if (reparse_line.find("..%5C") != std::string::npos
            || reparse_line.find("/../") != std::string::npos) {
            return MainBadRequestResponse(std::move(req));
        }

        // если на краю слеш - то будем искать файл index.html в крайнем каталоге
        if (*(reparse_line.end() - 1) == '/') {
            reparse_line += "index.html";
        }

        // берем путь до основной директории
        std::string root_path = std::string(resource_.GetRootDirectoryPath());

        // если запрос валидный, то проверяем наличие файла по пути
        if (resource_handler::ResourcePtr resource = resource_.GetResourseByPath(fs::path(root_path + reparse_line)); !resource) {
            // если упоминания о файле во внутренних каталогах нет отвечаем, что отдать нечего
            return MainNotFoundResponse(std::move(req));
        }
        else {
            return MainFileBodyResponse(std::move(req), resource);
        }
    }

    Response RequestHandler::HandleRequest(StringRequest&& req) {
        // обработка get, head, post будет в другом месте после парсинга
        return RequestParser(std::move(req));
    }

}  // namespace http_handler
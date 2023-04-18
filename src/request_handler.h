#pragma once
#include "http_server.h"
#include "resource_handler.h"
#include "game_handler.h"      // подключить boost_json.h и json_loader.h
//#include "model.h"
#include "domain.h"

namespace http_handler {

    using namespace std::literals;
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace res = resource_handler;
    namespace game = game_handler;
    namespace fs = std::filesystem;

    class RequestHandler {
    public:
        RequestHandler(game::GameHandler& game, res::ResourceHandler& resource, model::Game& game_simple)
            : game_{ game }, resource_{ resource }, game_simple_{ game_simple } {
        }

        RequestHandler(const RequestHandler&) = delete;
        RequestHandler& operator=(const RequestHandler&) = delete;

        // Создаёт StringResponse с заданными параметрами
        StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
            bool keep_alive, std::string_view content_type = ContentType::TEXT_HTML);

        // Обработчик GET-запросов в общем смысле
        Response GetMethodHandle(StringRequest&& req);

        // Базовый обработчик полученного реквеста
        Response HandleRequest(StringRequest&& req);

        template <typename Body, typename Allocator, typename Send>
        void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {

            // отправляем обработку в базовую функцию подготовки ответа
            send(HandleRequest(std::forward<decltype(req)>(req)));
        }

    private:
        game::GameHandler& game_;
        res::ResourceHandler& resource_;
        model::Game& game_simple_;
        

        // ------------------------------ блок работы с обычными get и head запросами -------------------

        // возвращает запрошенный документ
        Response MainFileBodyResponse(StringRequest&& req, const resource_handler::ResourcePtr& file_path);
        // возвращает index.html основной страницы
        Response MainRootIndexResponse(StringRequest&& req);
        // базовый ответ 404 - not found
        Response MainNotFoundResponse(StringRequest&& req);
        // базовый ответ 400 - bad request
        Response MainBadRequestResponse(StringRequest&& req);
        

        // ------------------------------ блок работы с api-get-запросами -------------------------------

        // возвращает список доступных карт
        Response ApiFindMapResponse(StringRequest&& req, std::string_view find_request_line);
        // возвращает список доступных карт
        Response ApiMapsListResponse(StringRequest&& req);
        // возврат "mapNotFound"
        Response ApiNotFoundResponse(StringRequest&& req);
        // возврат "badRequest"
        Response ApiBadRequestResponse(StringRequest&& req);
        // Обработчик GET-запросов для Api игрового сервера
        Response ApiGetMethodHandle(StringRequest&& req, std::string_view api_request_line);

        template <typename Iterator>
        std::string RequestTargetParser(Iterator begin, Iterator end);

        // базовый парсер запросов
        Response RequestParser(StringRequest&& req);
    };

    template <typename Iterator>
    std::string RequestHandler::RequestTargetParser(Iterator begin, Iterator end) {

        std::string result = ""s;                // строка с результатом работы
        bool IsASCII = false;                    // если появится ASCII-код
        std::string ascii_code = ""s;            // набранный ASCII-код

        // итерируемся по переданному диапазону
        for (auto it = begin; it != end; it++) {

            // пока не встретился ASCII-код
            if (!IsASCII) {

                if (*it == '%') {
                    IsASCII = true;              // подымаем флаг
                    ascii_code += *it;           // записываем элемент в символ
                }
                else if (*it == '+') {
                    result += ' ';               // для корректной обработки запроса file%20with+spaces
                }
                else {
                    result += *it;
                }
            }
            else {

                // пока не набрали три символа в ASCII-коде
                if (ascii_code.size() < 2) {
                    ascii_code += *it;           // записываем элемент в код
                }

                else {
                    ascii_code += *it;        // дописываем третий элемент в код

                    if (__ASCII_CODE_TO_CHAR__.count(ascii_code)) {
                        // если код есть в таблице, то дописываем в результирующий файл
                        result.push_back(__ASCII_CODE_TO_CHAR__.at(ascii_code));
                    }
                    // очищаем строку ASCII-кода и снимаем флаг
                    ascii_code.clear(); IsASCII = false;
                }
            }
        }

        return result;
    }

}  // namespace http_handler

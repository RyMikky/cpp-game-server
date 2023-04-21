#pragma once
#include "http_server.h"
#include "resource_handler.h"
#include "game_handler.h"      // подключить boost_json.h и json_loader.h
#include "domain.h"

namespace http_handler {

    using namespace std::literals;
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace res = resource_handler;
    namespace game = game_handler;
    namespace fs = std::filesystem;
    namespace net = boost::asio;

    class RequestHandler : public std::enable_shared_from_this<RequestHandler> {
    public:

        using Strand = net::strand<net::io_context::executor_type>;

        RequestHandler(game::GameHandler& game, res::ResourceHandler& resource, net::io_context& ioc)
            : game_{ game }, resource_{ resource }, api_strand_(net::make_strand(ioc)){
        }

        RequestHandler(const RequestHandler&) = delete;
        RequestHandler& operator=(const RequestHandler&) = delete;

        template <typename Body, typename Allocator, typename Send>
        void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
            // передаем обработку в парсер вместе с CallBack&&, чтобы в случае чего передать обработку запроса к api в api_strand_
            handle_request(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
        }

    private:
        game::GameHandler& game_;
        res::ResourceHandler& resource_;
        Strand api_strand_;
        
        // ------------------------------ блок работы с запросами к static data -------------------------

        // возвращает запрошенный документ
        Response static_file_body_response(StringRequest&& req, const resource_handler::ResourcePtr& file_path);
        // возвращает index.html основной страницы
        Response static_root_index_response(StringRequest&& req);
        // базовый ответ 404 - not found
        Response static_not_found_response(StringRequest&& req);
        // базовый ответ 400 - bad request
        Response static_bad_request_response(StringRequest&& req);

        // ------------------------------ внутренние обработчики базовой системы ------------------------

        // обработчик для запросов к статическим данным
        Response handle_static_request(StringRequest&& req);
        // обработчик для запросов к api-игрового сервера
        Response handle_api_request(StringRequest&& req, std::string_view api_request_line);

        // ------------------------------ блок парсинга и базовой обработки -----------------------------

        template <typename Iterator>
        std::string parse_target(Iterator begin, Iterator end);
        template <typename Send>
        void handle_request(StringRequest&& req, Send&& send);
    };

    template <typename Iterator>
    std::string RequestHandler::parse_target(Iterator begin, Iterator end) {

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
    template <typename Send>
    void RequestHandler::handle_request(StringRequest&& req, Send&& send) {

        // либо строка содержит только "api", либо имеет продолжение вида "api/"
        if (req.target().substr(0, 4) == "/api"sv && req.target().size() == 4
            || req.target().substr(0, 5) == "/api/"sv) {


            // создаём лямбду с шароварным указателем на экземпляр класса (экземпляр должен быть в куче, иначе все упадет!)
            // + Callback&&, плюс реквест. Чтобы не создавать экземпляр реквеста (лямбда по дефолту преобразует в const Type
            // в std::forward указываем конкретный тип и задаем его "mutable"
            auto handle = [self = shared_from_this(), send, request = std::forward<StringRequest&&>(req)]() mutable {

                try {
                    // Этот assert не выстрелит, так как лямбда-функция будет выполняться внутри strand
                    assert(self->api_strand_.running_in_this_thread());
                    return send(self->handle_api_request(std::forward<StringRequest&&>(request),
                        { request.target().begin() + 4, request.target().end() }));
                }
                catch (...) {
                    send(self->static_bad_request_response(std::forward<StringRequest&&>(request)));
                }
            };
            
            // важно не забыть задиспатчить всё что происходит в стренде. по сути похоже на футур
            return net::dispatch(api_strand_, handle);
        }

        else {
            // если обращение не к api, то уходим в обработку запросов к статическим данным
            return send(handle_static_request(std::move(req)));
        }
    }

}  // namespace http_handler

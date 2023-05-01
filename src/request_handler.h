#pragma once
#include "http_server.h"
#include "resource_handler.h"
#include "game_handler.h"             // подключит boost_json.h и json_loader.h
#include "options.h"                  // подключит аргументы запуска
#include "domain.h"                   // базовый инклюд с разными объявлениями

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
        RequestHandler(detail::Arguments&& arguments, net::io_context& ioc)
            : arguments_(std::move(arguments)), api_strand_(net::make_strand(ioc)) {
            ConfigurationPipeline();
        }

        RequestHandler(const RequestHandler&) = delete;
        RequestHandler& operator=(const RequestHandler&) = delete;

        template <typename Body, typename Allocator, typename Send>
        void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
            // передаем обработку в парсер вместе с CallBack&&, чтобы в случае чего передать обработку запроса к api в api_strand_
            HandleRequest(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
        }

        // включает выполнение автотаймера, выкидывает исключение, если таймер уже включен
        RequestHandler& StartGameTimer();
        // включает выполнение автотаймера, выкидывает исключение, если таймер уже включен
        RequestHandler& StartGameTimer(std::chrono::milliseconds period);
        // выключает выполнение автотаймера, выкидывает исключение, если таймер уже выключен
        RequestHandler& StopGameTimer();

    private:
        Strand api_strand_;
        detail::Arguments arguments_;
        
        std::shared_ptr<res::ResourceHandler> resource_ = nullptr;
        std::shared_ptr<game::GameHandler> game_ = nullptr;
        std::shared_ptr<game::GameTimer> timer_ = nullptr;

        bool timer_enable_ = false;              // флаг активации таймера автоизменения состояния
        bool test_enable_ = false;               // флаг доступа тест-системы к ресурсам и api

        // базовая функция активации всех элементов вызываемая в конструкторе по переданным параметрам
        RequestHandler& ConfigurationPipeline();
        
        // ------------------------------ блок работы с запросами к static data -------------------------

        // возвращает запрошенный документ
        Response StaticFileBodyResponse(StringRequest&& req, const resource_handler::ResourcePtr file_path);
        // возвращает index.html основной страницы
        Response StaticRootIndexResponse(StringRequest&& req);
        // базовый ответ 404 - not found
        Response StaticNotFoundResponse(StringRequest&& req);
        // базовый ответ 400 - bad request
        Response StaticBadRequestResponse(StringRequest&& req);

        // ------------------------------ внутренние обработчики тестовой системы -----------------------

        // возвращает ответ на неверный запрос к дебаговым модулям
        Response DebugCommonFailResponse(http_handler::StringRequest&& req, http::status status, 
            std::string_view code, std::string_view message, [[maybe_unused]]std::string_view allow);
        // возвращает ответ на запрос по удалению всех игровых сессий из обработчика
        Response DebugSessionsResetResponse(StringRequest&& req);
        // возвращает ответ на запрос по установке флага случайного стартового расположения
        Response DebugStartPositionResponse(StringRequest&& req);
        // возвращает ответ на запрос по установке флага случайного стартового расположения из конфига
        Response DebugDefaultPositionResponse(StringRequest&& req);
        // возвращает ответ на отчёт о завершении работы тестовой системы
        Response DebugUnitTestsEndResponse(StringRequest&& req);

        template <typename Function>
        // авторизует и возвращает соответствующий ответ по обращению к дебагу
        Response DebugAuthorizationImpl(StringRequest&& req, Function&& func);

        // ------------------------------ внутренние обработчики базовой системы ------------------------

        // обработчик для запросов к статическим данным
        Response HandleStaticRequest(StringRequest&& req);
        // обработчик для запросов к api-игрового сервера
        Response HandleApiRequest(StringRequest&& req, std::string_view api_request_line);
        // обработчик для конфигурационных запросов от тестовой системы
        Response HandleTestRequest(StringRequest&& req, std::string_view api_request_line);

        // ------------------------------ блок парсинга и базовой обработки -----------------------------

        template <typename Iterator>
        std::string ParseRequestTarget(Iterator begin, Iterator end);

        template <typename Send>
        void HandleRequest(StringRequest&& req, Send&& send);
    };

    template <typename Function>
    Response RequestHandler::DebugAuthorizationImpl(StringRequest&& req, Function&& func) {
        // проверяем корректность метода запроса
        if (req.method_string() != http_handler::Method::POST) {
            // если у нас не POST-запрос, то кидаем отбойник
            return DebugCommonFailResponse(std::move(req), http::status::method_not_allowed,
                "invalidMethod", "Request method not allowed", http_handler::Method::POST);
        }

        // ищем тушку авторизации среди хеддеров запроса
        auto auth_iter = req.find("Authorization");
        if (auth_iter == req.end()) {
            // если нет тушки по авторизации, тогда кидаем отбойник
            return DebugCommonFailResponse(std::move(req), http::status::unauthorized, 
                "invalidToken"sv, "Authorization header is missing"sv, ""sv);
        }

        // из тушки запроса получаем строку
        // так как строка должна иметь строгий вид Bearer <токен>, то мы легко можем распарсить её
        auto auth_reparse = game_handler::detail::BearerParser({ auth_iter->value().begin(), auth_iter->value().end() });

        // если полученный токен не равен ни статическому, ни сгенерированному паролю
        if (!auth_reparse && auth_reparse.value() != __DEBUG_REQUEST_AUTORIZATION_PASSWORD__) {
            // если нет строки Bearer, или она корявая, или токен пустой, то кидаем отбойник
            return DebugCommonFailResponse(std::move(req), http::status::unauthorized,
                "invalidToken"sv, "Authorization header is missing"sv, ""sv);
        }

        // вызываем полученный обработчик
        return func(std::move(req));
    }
    template <typename Iterator>
    std::string RequestHandler::ParseRequestTarget(Iterator begin, Iterator end) {

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
    void RequestHandler::HandleRequest(StringRequest&& req, Send&& send) {

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
                    return send(self->HandleApiRequest(std::forward<StringRequest&&>(request),
                        { request.target().begin() + 4, request.target().end() }));
                }
                catch (...) {
                    send(self->StaticBadRequestResponse(std::forward<StringRequest&&>(request)));
                }
            };
            
            // важно не забыть задиспатчить всё что происходит в стренде. по сути похоже на футур
            return net::dispatch(api_strand_, handle);
        }

        // либо строка содержит только "test_frame", либо имеет продолжение вида "test_frame/"
        else if (req.target().substr(0, 11) == "/test_frame"sv && req.target().size() == 11
            || req.target().substr(0, 12) == "/test_frame/"sv) {

            if (test_enable_) {
                // создаём лямбду с шароварным указателем на экземпляр класса (экземпляр должен быть в куче, иначе все упадет!)
                // + Callback&&, плюс реквест. Чтобы не создавать экземпляр реквеста (лямбда по дефолту преобразует в const Type
                // в std::forward указываем конкретный тип и задаем его "mutable"
                auto handle = [self = shared_from_this(), send, request = std::forward<StringRequest&&>(req)]() mutable {

                    try {
                        // Этот assert не выстрелит, так как лямбда-функция будет выполняться внутри strand
                        assert(self->api_strand_.running_in_this_thread());
                        return send(self->HandleTestRequest(std::forward<StringRequest&&>(request),
                            { request.target().begin() + 11, request.target().end() }));
                    }
                    catch (...) {
                        send(self->StaticBadRequestResponse(std::forward<StringRequest&&>(request)));
                    }
                };

                // важно не забыть задиспатчить всё что происходит в стренде. по сути похоже на футур
                return net::dispatch(api_strand_, handle);
            }

            // если тестовая система не заявлена в конфигурации и не поднят её флаг, то доступ закрыт
            return send(DebugCommonFailResponse(std::move(req), http::status::bad_request, "badRequest"sv, "Invalid endpoint"sv, ""sv));
        }

        else {
            // если обращение не к api, то уходим в обработку запросов к статическим данным
            return send(HandleStaticRequest(std::move(req)));
        }
    }

}  // namespace http_handler
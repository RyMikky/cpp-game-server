#pragma once

#include "logger_handler.h"
#include "domain.h"

#include <iostream>
#include <chrono>

namespace http_server {

    using namespace std::literals;

    namespace net = boost::asio;
    using tcp = net::ip::tcp;
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace sys = boost::system;

    class SessionBase {
    public:
        // Запрещаем копирование и присваивание объектов SessionBase и его наследников
        SessionBase(const SessionBase&) = delete;
        SessionBase& operator=(const SessionBase&) = delete;

        void Run();

    protected:
        explicit SessionBase(tcp::socket&& socket)
            : stream_(std::move(socket)){
        }

        using HttpRequest = http::request<http::string_body>;

        template <typename Body, typename Fields>
        void Write(http::response<Body, Fields>&& response) {
            // Как только ответ пришёл в данный метод замеряем время получения ответа на запрос
            end_ts_ = std::chrono::system_clock::now();
            // Создаём запись о успешном получении ответа
            logger_handler::LogResponse(
                response.result_int(), response.at(http::field::content_type),
                std::chrono::duration_cast<std::chrono::milliseconds>(end_ts_ - start_ts_).count(), HostAdress());

            // Запись выполняется асинхронно, поэтому response перемещаем в область кучи
            auto safe_response = std::make_shared<http::response<Body, Fields>>(std::move(response));

            auto self = GetSharedThis();
            http::async_write(stream_, *safe_response,
                [safe_response, self](beast::error_code ec, std::size_t bytes_written) {
                    self->OnWrite(safe_response->need_eof(), ec, bytes_written);
                });
        }

        std::string HostAdress() const;

        ~SessionBase() = default;
    private:
        // tcp_stream содержит внутри себя сокет и добавляет поддержку таймаутов
        beast::tcp_stream stream_;
        beast::flat_buffer buffer_;
        HttpRequest request_;
        std::chrono::system_clock::time_point start_ts_;
        std::chrono::system_clock::time_point end_ts_;

        void Read();

        void OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read);

        void Close();

        void OnWrite(bool close, beast::error_code ec, [[maybe_unused]] std::size_t bytes_written);

        // Обработку запроса делегируем подклассу
        virtual void HandleRequest(HttpRequest&& request) = 0;

        virtual std::shared_ptr<SessionBase> GetSharedThis() = 0;
    };

    template <typename RequestLambda>
    class Session : public SessionBase, public std::enable_shared_from_this<Session<RequestLambda>> {
    public:
        template <typename RLambda>
        Session(tcp::socket&& socket, RLambda&& request_lambda)
            : SessionBase(std::move(socket))
            , request_lambda_(std::forward<RLambda>(request_lambda)) {
        }

    private:
        RequestLambda request_lambda_;

        std::shared_ptr<SessionBase> GetSharedThis() override {
            return this->shared_from_this();
        }

        void HandleRequest(HttpRequest&& request) override {
            // Захватываем умный указатель на текущий объект Session в лямбде,
            // чтобы продлить время жизни сессии до вызова лямбды.
            // Используется generic-лямбда функция, способная принять response произвольного типа
            request_lambda_(std::move(request), [self = this->shared_from_this()](http_handler::Response&& response) {

                // смотри #define в domain.h
                if (IS_STRING_RESPONSE(response)) {
                    self->Write(std::move(std::get<http_handler::StringResponse>(response)));
                }
                else if (IS_FILE_RESPONSE(response)) {
                    self->Write(std::move(std::get<http_handler::FileResponse>(response)));
                }
            });
        }
    };

    template <typename RequestLambda>
    class Listener : public std::enable_shared_from_this<Listener<RequestLambda>> {
    public:
        template <typename RLambda>
        Listener(net::io_context& ioc, const tcp::endpoint& endpoint, RLambda&& request_lambda)
            : ioc_(ioc)
            // Обработчики асинхронных операций acceptor_ будут вызываться в своём strand
            , acceptor_(net::make_strand(ioc))
            , request_lambda_(std::forward<RLambda>(request_lambda)) {
            // Открываем acceptor, используя протокол (IPv4 или IPv6), указанный в endpoint
            acceptor_.open(endpoint.protocol());

            // После закрытия TCP-соединения сокет некоторое время может считаться занятым,
            // чтобы компьютеры могли обменяться завершающими пакетами данных.
            // Однако это может помешать повторно открыть сокет в полузакрытом состоянии.
            // Флаг reuse_address разрешает открыть сокет, когда он "наполовину закрыт"
            acceptor_.set_option(net::socket_base::reuse_address(true));
            // Привязываем acceptor к адресу и порту endpoint
            acceptor_.bind(endpoint);
            // Переводим acceptor в состояние, в котором он способен принимать новые соединения
            // Благодаря этому новые подключения будут помещаться в очередь ожидающих соединений
            acceptor_.listen(net::socket_base::max_listen_connections);
            // После активации сервера выдаём лог-запись о запуске
            logger_handler::LogStartup(endpoint.port(), endpoint.address());
        }

        void Run() {
            DoAccept();
        }

    private:
        net::io_context& ioc_;
        tcp::acceptor acceptor_;
        RequestLambda request_lambda_;

        void DoAccept();

        // Метод socket::async_accept создаст сокет и передаст его передан в OnAccept
        void OnAccept(sys::error_code ec, tcp::socket socket);

        void AsyncRunSession(tcp::socket&& socket) {
            std::make_shared<Session<RequestLambda>>(std::move(socket), request_lambda_)->Run();
        }
    };

    template <typename RequestLambda>
    void Listener<RequestLambda>::DoAccept() {
        acceptor_.async_accept(
            // Передаём последовательный исполнитель, в котором будут вызываться обработчики
            // асинхронных операций сокета
            net::make_strand(ioc_),
            // С помощью bind_front_handler создаём обработчик, привязанный к методу OnAccept
            // текущего объекта.
            // Так как Listener — шаблонный класс, нужно подсказать компилятору, что
            // shared_from_this — метод класса, а не свободная функция.
            // Для этого вызываем его, используя this
            // Этот вызов bind_front_handler аналогичен
            // namespace ph = std::placeholders;
            // std::bind(&Listener::OnAccept, this->shared_from_this(), ph::_1, ph::_2)
            beast::bind_front_handler(&Listener::OnAccept, this->shared_from_this()));
    }

    // Метод socket::async_accept создаст сокет и передаст его передан в OnAccept
    template <typename RequestLambda>
    void Listener<RequestLambda>::OnAccept(sys::error_code ec, tcp::socket socket) {

        if (ec) {
            return logger_handler::LogError(ec, "accept"sv);
        }

        // Асинхронно обрабатываем сессию
        AsyncRunSession(std::move(socket));

        // Принимаем новое соединение
        DoAccept();
    }

    // Функция запуска сервера принимает два шаблонных параметра
    // Лямбда обработки запроса и ссылфку на логгер

    template <typename RequestLambda>
    void ServeHttp(net::io_context& ioc, const tcp::endpoint& endpoint, RequestLambda&& lambda) {
        // При помощи decay_t исключим ссылки из типа RequestHandler,
        // чтобы Listener хранил RequestHandler по значению
        using MyListener = Listener<std::decay_t<RequestLambda>>;

        std::make_shared<MyListener>(ioc, endpoint, std::forward<RequestLambda>(lambda))->Run();
    }

}  // namespace http_server
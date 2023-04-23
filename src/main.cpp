#include "sdk.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/json/src.hpp>

#include <iostream>
#include <thread>
#include <cassert>

#include "logger_handler.h"                          // базовый инклюд обеспечивающий доступ к логгеру в данном участке кода
#include "request_handler.h"                         // базовый инклюд открывающий доступ к серверу, обработчику ресурсов
#include "test_frame.h"

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;

namespace {

// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n) {
        workers.emplace_back(fn);
    }

    {
        // таким образом как только выйдем из области видимости класс уничтожится
         test::SimpleTest("127.0.0.1", "8080", "../test/");
    }
    
    fn();
}

}  // namespace

int main(int argc, const char* argv[]) {

    setlocale(LC_ALL, "Russian");
    setlocale(LC_NUMERIC, "English");

    if (argc != 3) {
        std::cerr << "Usage: game_server <game-config-json> <static_data_path>"sv << std::endl;
        return EXIT_FAILURE;
    }

    try {
        // 0.1. Инициализируем буст-логгер, для базовой инициализации можно подать любой консольный поток
        logger_handler::detail::BoostLogBaseSetup(std::cout);

        // 1. Загружаем карту из файла и строим модель игры
        game_handler::GameHandler game{ argv[1] };
        // 2. Загружаем данные в обработчик ресурсов
        resource_handler::ResourceHandler resource{ argv[2] };

        // обработчики ресурсов и игры пока что подержим на стеке, так как стек работает быстрее 
        // в случае необходимости уведем их на шару в кучу

        // 3. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        // 4. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                ioc.stop();
                logger_handler::LogShutdown();
            }
            });

        // 5. Создаём обработчик HTTP-запросов в шаре и связываем его с моделью игры, статическими данными и контекстом
        auto request_handler = std::make_shared<http_handler::RequestHandler>(game, resource, ioc );

        // 6. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;

        http_server::ServeHttp(ioc, { address, port }, [request_handler](auto&& req, auto&& send) {
            (*request_handler)(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
            });

        // 7. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });

    } catch (const std::exception& ex) {
        logger_handler::LogException(ex);

        return EXIT_FAILURE;
    }
}

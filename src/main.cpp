#include "sdk.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/json/src.hpp>

#include <iostream>
#include <thread>
#include <cassert>

#include "logger_handler.h"
#include "json_loader.h"
#include "request_handler.h"

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
        // 0.1. Инициализируем буст-логгер
        logger_handler::detail::BoostLogBaseSetup(std::cout);

        // 1. Загружаем карту из файла и построить модель игры
        model::Game game = json_loader::LoadGame(argv[1]);

        // 2. Загружаем данные в обработчик ресурсов
        resource_handler::ResourceHandler resource = resource_handler::detail::LoadFiles( argv[2]);

        // 3. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        // 4. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                ioc.stop();
                logger_handler::LogShutdown();
                //__LOGGER__.LogShutdown();
            }
            });

        // 5. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        http_handler::RequestHandler resource_handler{ game, resource };

        // 6. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;
        http_server::ServeHttp(ioc, {address, port}, [&resource_handler](auto&& req, auto&& send) {
            resource_handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
        });

        // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы 
        //std::cout << "Server has started..."sv << std::endl;
 
        // 7. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });

    } catch (const std::exception& ex) {
        logger_handler::LogException(ex);
        //__LOGGER__.LogException(ex);
        //std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
}

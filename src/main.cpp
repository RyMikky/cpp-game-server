#include "sdk.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/json/src.hpp>

#include <iostream>
#include <thread>
#include <cassert>

#include "logger_handler.h"                          // базовый инклюд обеспечивающий доступ к логгеру в данном участке кода
#include "request_handler.h"                         // базовый инклюд открывающий доступ к серверу, обработчику ресурсов
//#include "test_frame.h"                              // тестовый фрейм, содержащий большое количество тестов системы
#include "options.h"                                 // функционал запуска приложения

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;

namespace {

// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, /*test::TestConfiguration&& test_config,*/ const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n) {
        workers.emplace_back(fn);
    }

    //{
    //    if (test_config.enable_) {
    //        // таким образом как только выйдем из области видимости класс уничтожится
    //        test::SimpleTest(std::move(test_config));
    //    }
    //}
    
    fn();
}

}  // namespace

int main(int argc, const char* argv[]) {

    setlocale(LC_ALL, "Russian");
    setlocale(LC_NUMERIC, "English");

    try
    {
        // 1. Инициализируем буст-логгер, для базовой инициализации можно подать любой консольный поток
        logger_handler::detail::BoostLogBaseSetup(std::cout);

        // 2. Парсим командную строку и получаем конфигурацию запуска
        // в случае нехватки ключевых аргументов будут выброшены исключения
        detail::Arguments command_line = detail::ParseCommandLine(argc, argv);

        if (command_line.show_help_list) {
            return EXIT_SUCCESS;
        }

        //// 3. Если есть упоминания о тестах, подготавливаем конфигурацию запуска тестов
        //test::TestConfiguration test_config;
        //if (command_line.test_frame_launch) {
        //    test_config.address_ = "127.0.0.1";
        //    test_config.port_ = "8080";
        //    test_config.root_ = command_line.test_content_path;
        //    test_config.authorization_ = http_handler::__DEBUG_REQUEST_AUTORIZATION_PASSWORD__;
        //    test_config.enable_ = true;
        //}

        // 4. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        // 5. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                ioc.stop();
                logger_handler::LogShutdown();
            }
            });

        // 6. Создаём обработчик HTTP-запросов в шаре, конструктор обработчика сконфигурирует 
        // модель игры, статические данные, контекст для api, таймер автообновления по полученным настройкам
        auto request_handler = std::make_shared<http_handler::RequestHandler>(std::move(command_line), ioc);

        // 7. Запускаем веб-сервер делегируя поступающие запросы их обработчику
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;

        http_server::ServeHttp(ioc, { address, port }, [request_handler](auto&& req, auto&& send) {
            (*request_handler)(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
            });

        // 8. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads),/* std::move(test_config),*/ [&ioc] {
            ioc.run();
            });

        // 9. Выполняем базовую сериализацию после завершения работы сервера, если поднят флаг сохранения
        if (command_line.game_autosave) {
            request_handler->SerializeGameData();
        }

    }
    catch (const std::exception& ex)
    {
        logger_handler::LogException(ex);
        return EXIT_FAILURE;
    }
}
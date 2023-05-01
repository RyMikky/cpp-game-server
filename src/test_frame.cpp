#include "test_frame.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cassert>

namespace test {

    SimpleTest::SimpleTest(TestConfiguration&& config)
        : config_(config), resolver_(ioc_), stream_(ioc_) {

        try {
            endpoint_ = resolver_.resolve(config_.address_, config_.port_);

            RunAllTests();
        }
        catch (std::exception const& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }


    SimpleTest& SimpleTest::RunAllTests() {

        std::cerr << std::endl;
        std::cerr << "SimpleTest::RunAllTests()::Begin...................................\n" << std::endl;

        assert(TestBasicApiSet());                          // запуск сета проверки базисных команд к API
        assert(TestGameLoginSet());                         // запуск сета проверки входа в игровую сессию
        assert(TestApiAuthorizationSet());                  // запуск сета проверки авторизации и доступа
        assert(TestApiGameStateSet());                      // запуск сета проверки запроса состояния игры
        assert(TestApiPlayerMoveSet());                     // запуск сета проверки запроса на изменение скорости и направления
        assert(TestApiTimeTickSet());                       // запуск сета проверки запроса на изменение скорости и направления


        assert(TestApiDebugSessionsClear());                // очищаем данные тестовых сессий
        std::cerr << "TestApiDebugSessionsClear()::Complete::Status....................Ok\n" << std::endl;
        assert(TestApiDebugResetStartRandomPosition());    // возвращаем флаг рандомного расположения игроков
        std::cerr << "TestApiDebugResetStartRandomPosition()::Complete::Status.........Ok\n" << std::endl;
        assert(TestApiDebugTestFrameEnd());                 // снимает свой флаг у обработчика запросов
        std::cerr << "TestApiDebugTestFrameEnd()::Complete::Status.....................Ok\n" << std::endl;
        assert(TestApiDebugEndpointClose());                // проверяет что доступ по цели /test_frame закрыт
        std::cerr << "TestApiDebugEndpointClose()::Complete::Status....................Ok\n" << std::endl;

        std::cerr << std::endl;
        std::cerr << "SimpleTest::RunAllTests()::Complete......................Status::Ok\n";
        std::cerr << "SimpleTest::TestDataSessions::Clear......................Status::Ok\n";
        std::cerr << "SimpleTest::ControlTransfer..............................Status::Ok\n";
        std::cerr << std::endl;

        return *this;
    }

    bool SimpleTest::TestApiMapsList() {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/api/v1/maps"sv, 
                http::verb::get, BOOST_BEAST_VERSION_STRING, ""sv, ""sv, ""sv));

            // создаем объект ответа
            beast::flat_buffer buffer; http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            // проверяем полученный ответ
            assert(CheckServerResponse(std::move(res), ""sv,
                http_handler::ContentType::APP_JSON, ""sv, 200, 
                config_.root_ + "basic/test_api_maps.txt"));

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }

            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestApiMapOne() {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/api/v1/maps/map1"sv,
                http::verb::get, BOOST_BEAST_VERSION_STRING, ""sv, ""sv, ""sv));

            // создаем объект ответа
            beast::flat_buffer buffer; http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            // проверяем полученный ответ
            assert(CheckServerResponse(std::move(res), ""sv,
                http_handler::ContentType::APP_JSON, ""sv, 200, 
                config_.root_ + "basic/test_api_map1.txt"));

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestApiMapTown() {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/api/v1/maps/town"sv,
                http::verb::get, BOOST_BEAST_VERSION_STRING, ""sv, ""sv, ""sv));

            // создаем объект ответа
            beast::flat_buffer buffer; http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            // проверяем полученный ответ
            assert(CheckServerResponse(std::move(res), ""sv,
                http_handler::ContentType::APP_JSON, ""sv, 200, 
                config_.root_ + "basic/test_api_town.txt"));

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestApiMapNotFound() {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/api/v1/maps/map15"sv,
                http::verb::get, BOOST_BEAST_VERSION_STRING, ""sv, ""sv, ""sv));

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            // проверяем полученный ответ
            assert(CheckServerResponse(std::move(res), ""sv,
                http_handler::ContentType::APP_JSON, ""sv, 404, 
                config_.root_ + "basic/test_api_map_not_found.txt"));

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestApiBadRequest() {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/api/v333/maps/map1"sv,
                http::verb::get, BOOST_BEAST_VERSION_STRING, ""sv, ""sv, ""sv));

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            // проверяем полученный ответ
            assert(CheckServerResponse(std::move(res), ""sv,
                http_handler::ContentType::APP_JSON, ""sv, 400, 
                config_.root_ + "basic/test_api_bad_request.txt"));

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestBasicApiSet() {

        if (TestApiMapsList()) {
            std::cerr << "TestApiMapsList()::Complete::Status..............................Ok\n" << std::endl;
        }
        else {
            return false;
        }
        
        if (TestApiMapOne()) {
            std::cerr << "TestApiMapOne()::Complete::Status................................Ok\n" << std::endl;
        }
        else {
            return false;
        }

        if (TestApiMapTown()) {
            std::cerr << "TestApiMapTown()::Complete::Status...............................Ok\n" << std::endl;
        }
        else {
            return false;
        }

        if (TestApiMapNotFound()) {
            std::cerr << "TestApiMapNotFound()::Complete::Status...........................Ok\n" << std::endl;
        }
        else {
            return false;
        }

        if (TestApiBadRequest()) {
            std::cerr << "TestApiBadRequest()::Complete::Status............................Ok\n" << std::endl;
        }
        else {
            return false;
        }

        return true;
    }


    bool SimpleTest::TestApiGameFirstLogin() {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/api/v1/game/join"sv,
                http::verb::post, BOOST_BEAST_VERSION_STRING, "application/json"sv,
                ""sv, "{\"userName\": \"Scooby Doo\", \"mapId\": \"map1\"}"sv));

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            // проверяем полученный ответ
            assert(CheckServerResponse(std::move(res), ""sv,
                http_handler::ContentType::APP_JSON, "no-cache"sv, 200, ""));

            {
                // дополнительная проверка непредсказуемых ответов (токен всегда будет разный)
                // загоняем ответ сервера из строки в JSON
                auto server_resp = json_detail::ParseTextToJSON(
                    boost::beast::buffers_to_string(res.body().data()));

                // првоеряем в ответе наличие строк о токене и айди
                assert(server_resp.as_object().contains("authToken"));
                assert(server_resp.as_object().contains("playerId"));
                assert(server_resp.as_object().at("playerId") == 0);
            }

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestApiGameSecondLogin() {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/api/v1/game/join"sv,
                http::verb::post, BOOST_BEAST_VERSION_STRING, "application/json"sv,
                ""sv, "{\"userName\": \"Whelma Shnizel\", \"mapId\": \"map1\"}"sv));

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            // проверяем полученный ответ
            assert(CheckServerResponse(std::move(res), ""sv,
                http_handler::ContentType::APP_JSON, "no-cache"sv, 200, ""));

            {
                // дополнительная проверка непредсказуемых ответов (токен всегда будет разный)
                // загоняем ответ сервера из строки в JSON
                auto server_resp = json_detail::ParseTextToJSON(
                    boost::beast::buffers_to_string(res.body().data()));

                // првоеряем в ответе наличие строк о токене и айди
                assert(server_resp.as_object().contains("authToken"));
                assert(server_resp.as_object().contains("playerId"));
                assert(server_resp.as_object().at("playerId") == 1);
            }

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestApiGameLoginMissName() {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/api/v1/game/join"sv,
                http::verb::post, BOOST_BEAST_VERSION_STRING, "application/json"sv,
                ""sv, "{\"mapId\": \"map1\"}"sv));

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            // проверяем полученный ответ
            assert(CheckServerResponse(std::move(res), ""sv,
                http_handler::ContentType::APP_JSON, "no-cache"sv, 400, 
                config_.root_ + "login/test_api_game_login_miss_name_or_map.txt"));

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestApiGameLoginInvalidName() {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/api/v1/game/join"sv,
                http::verb::post, BOOST_BEAST_VERSION_STRING, "application/json"sv,
                ""sv, "{\"userName\": \"\", \"mapId\": \"map1\"}"sv));

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            // проверяем полученный ответ
            assert(CheckServerResponse(std::move(res), ""sv,
                http_handler::ContentType::APP_JSON, "no-cache"sv, 400,
                config_.root_ + "login/test_api_game_login_invalid_name.txt"));

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestApiGameLoginMissMap() {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/api/v1/game/join"sv,
                http::verb::post, BOOST_BEAST_VERSION_STRING, "application/json"sv,
                ""sv, "{\"userName\": \"Vasya\"}"sv));

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            // проверяем полученный ответ
            assert(CheckServerResponse(std::move(res), ""sv,
                http_handler::ContentType::APP_JSON, "no-cache"sv, 400,
                config_.root_ + "login/test_api_game_login_miss_name_or_map.txt"));

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestApiGameLoginMapNotFound() {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/api/v1/game/join"sv,
                http::verb::post, BOOST_BEAST_VERSION_STRING, "application/json"sv,
                ""sv, "{\"userName\": \"Vasya\", \"mapId\": \"map2412\"}"sv));

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            // проверяем полученный ответ
            assert(CheckServerResponse(std::move(res), ""sv,
                http_handler::ContentType::APP_JSON, "no-cache"sv, 404,
                config_.root_ + "basic/test_api_map_not_found.txt"));

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestApiGameLoginInvalidMap() {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/api/v1/game/join"sv,
                http::verb::post, BOOST_BEAST_VERSION_STRING, "application/json"sv,
                ""sv, "{\"userName\": \"Vasya\", \"mapId\": \'  invalid  \'}"sv));

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            // проверяем полученный ответ
            assert(CheckServerResponse(std::move(res), ""sv,
                http_handler::ContentType::APP_JSON, "no-cache"sv, 400,
                config_.root_ + "login/test_api_game_login_invalid_argument.txt"));

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestApiGameLoginInvalidMethod() {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/api/v1/game/join"sv,
                http::verb::get, BOOST_BEAST_VERSION_STRING, "application/json"sv,
                ""sv, "{\"userName\": \"Scooby Doo\", \"mapId\": \"map1\"}"sv));

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            // проверяем полученный ответ
            assert(CheckServerResponse(std::move(res), "POST"sv,
                http_handler::ContentType::APP_JSON, "no-cache"sv, 405,
                config_.root_ + "login/test_api_game_login_invalid_method.txt"));

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestGameLoginSet() {

        if (TestApiGameFirstLogin()) {
            std::cerr << "TestApiGameLogin()::Complete::Status.............................Ok\n" << std::endl;
        }
        else {
            return false;
        }

        if (TestApiGameSecondLogin()) {
            std::cerr << "TestApiGameSecondLogin()::Complete::Status.......................Ok\n" << std::endl;
        }
        else {
            return false;
        }

        if (TestApiGameLoginMissName()) {
            std::cerr << "TestApiGameLoginMissName()::Complete::Status.....................Ok\n" << std::endl;
        }
        else {
            return false;
        }

        if (TestApiGameLoginInvalidName()) {
            std::cerr << "TestApiGameLoginInvalidName()::Complete::Status..................Ok\n" << std::endl;
        }
        else {
            return false;
        }

        if (TestApiGameLoginMissMap()) {
            std::cerr << "TestApiGameLoginMissMap()::Complete::Status......................Ok\n" << std::endl;
        }
        else {
            return false;
        }

        if (TestApiGameLoginMapNotFound()) {
            std::cerr << "TestApiGameLoginMapNotFound()::Complete::Status..................Ok\n" << std::endl;
        }
        else {
            return false;
        }

        if (TestApiGameLoginInvalidMap()) {
            std::cerr << "TestApiGameLoginInvalidMap()::Complete::Status...................Ok\n" << std::endl;
        }
        else {
            return false;
        }

        if (TestApiGameLoginInvalidMethod()) {
            std::cerr << "TestApiGameLoginInvalidMethod()::Complete::Status................Ok\n" << std::endl;
        }
        else {
            return false;
        }


        return true;
    }


    bool SimpleTest::TestApiGameThirdLogin(AuthResp& data) {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/api/v1/game/join"sv,
                http::verb::post, BOOST_BEAST_VERSION_STRING, "application/json"sv,
                ""sv, "{\"userName\": \"Mega Pups\", \"mapId\": \"map1\"}"sv));

            // создаем объект ответа
            beast::flat_buffer buffer; http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            boost::json::value server_resp;

            // проверяем полученный ответ
            assert(CheckServerResponse(std::move(res), ""sv,
                http_handler::ContentType::APP_JSON, "no-cache"sv, 200, ""));

            {
                // дополнительная проверка непредсказуемых ответов (токен всегда будет разный)
                // загоняем ответ сервера из строки в JSON
                server_resp = json_detail::ParseTextToJSON(
                    boost::beast::buffers_to_string(res.body().data()));

                // првоеряем в ответе наличие строк о токене и айди
                assert(server_resp.as_object().contains("authToken"));
                assert(server_resp.as_object().contains("playerId"));
                assert(server_resp.as_object().at("playerId") == 2);
            }

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }

            // сделаем "грязные" преобразования в С-стиле, ай-ай-ай, Страуструб голову бы оторвал)
            data.first = (std::string)server_resp.as_object().at("authToken").as_string();
            data.second = (size_t)server_resp.as_object().at("playerId").as_int64();

            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "SimpleTest::TestApiGameThirdLogin::Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestApiGamePlayerList(AuthResp& data) {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/api/v1/game/players"sv,
                http::verb::get, BOOST_BEAST_VERSION_STRING, "application/json"sv,
                "Bearer " + data.first, ""sv));

            // создаем объект ответа
            beast::flat_buffer buffer; http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            assert(CheckServerResponse(std::move(res), ""sv, 
                http_handler::ContentType::APP_JSON, "no_cache"sv, 200, 
                config_.root_ + "autho/test_api_players_list.txt"));

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "SimpleTest::TestApiGamePlayerList::Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestApiGameAuthorizationMissBody(AuthResp& data) {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/api/v1/game/players"sv,
                http::verb::get, BOOST_BEAST_VERSION_STRING, "application/json"sv, ""sv, ""sv));

            // создаем объект ответа
            beast::flat_buffer buffer; http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            assert(CheckServerResponse(std::move(res), ""sv,
                http_handler::ContentType::APP_JSON, "no_cache"sv, 401, 
                config_.root_ + "autho/test_api_authorization_missing.txt"));

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "SimpleTest::TestApiGameAuthorizationMissBody::Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestApiGameAuthorizationMissToken(AuthResp& data) {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/api/v1/game/players"sv,
                http::verb::get, BOOST_BEAST_VERSION_STRING, "application/json"sv, "Bearer"sv, ""sv));

            // создаем объект ответа
            beast::flat_buffer buffer; http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            assert(CheckServerResponse(std::move(res), ""sv,
                http_handler::ContentType::APP_JSON, "no_cache"sv, 401,
                config_.root_ + "autho/test_api_authorization_missing.txt"));

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "SimpleTest::TestApiGameAuthorizationMissToken::Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestApiGameAuthorizationInvalidBody(AuthResp& data){
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/api/v1/game/players"sv,
                http::verb::get, BOOST_BEAST_VERSION_STRING, "application/json"sv, 
                "Cearer " + data.first, ""sv));

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            assert(CheckServerResponse(std::move(res), ""sv,
                http_handler::ContentType::APP_JSON, "no_cache"sv, 401,
                config_.root_ + "autho/test_api_authorization_missing.txt"));

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "SimpleTest::TestApiGameAuthorizationInvalidBody::Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestApiGameTokenNotFound(AuthResp& data){
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/api/v1/game/players"sv,
                http::verb::get, BOOST_BEAST_VERSION_STRING, "application/json"sv,
                "Bearer 68f75asfdsffaa457a98f7v5d6z4f8f4"sv, ""sv));

            // создаем объект ответа
            beast::flat_buffer buffer; http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            assert(CheckServerResponse(std::move(res), ""sv,
                http_handler::ContentType::APP_JSON, "no_cache"sv, 401,
                config_.root_ + "autho/test_api_token_not_found.txt"));

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "SimpleTest::TestApiGameTokenNotFound::Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestApiGamePlayerListInvalidMethod(AuthResp& data) {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/api/v1/game/players"sv,
                http::verb::post, BOOST_BEAST_VERSION_STRING, "application/json"sv,
                "Bearer " + data.first, ""sv));

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            assert(CheckServerResponse(std::move(res), "GET, HEAD"sv,
                http_handler::ContentType::APP_JSON, "no_cache"sv, 405,
                config_.root_ + "autho/test_api_players_list_invalid_method.txt"));

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "SimpleTest::TestApiGamePlayerListInvalidMethod::Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestApiAuthorizationSet() {

        AuthResp authorization;          // токен для последующего использования в тестах данного пула
                                         // записывается в первом тесте ниже, так как генерируется на сервере
        if (TestApiGameThirdLogin(authorization)) {
            std::cerr << "TestApiGameThirdLogin()::Complete::Status........................Ok\n" << std::endl;
        }
        else {
            return false;
        }

        if (TestApiGamePlayerList(authorization)) {
            std::cerr << "TestApiGamePlayerList()::Complete::Status........................Ok\n" << std::endl;
        }
        else {
            return false;
        }

        if (TestApiGameAuthorizationMissBody(authorization)) {
            std::cerr << "TestApiGameAuthorizationMissBody()::Complete::Status.............Ok\n" << std::endl;
        }
        else {
            return false;
        }

        if (TestApiGameAuthorizationMissToken(authorization)) {
            std::cerr << "TestApiGameAuthorizationMissToken()::Complete::Status............Ok\n" << std::endl;
        }
        else {
            return false;
        }

        if (TestApiGameAuthorizationInvalidBody(authorization)) {
            std::cerr << "TestApiGameAuthorizationInvalidBody()::Complete::Status..........Ok\n" << std::endl;
        }
        else {
            return false;
        }

        if (TestApiGameTokenNotFound(authorization)) {
            std::cerr << "TestApiGameTokenNotFound()::Complete::Status.....................Ok\n" << std::endl;
        }
        else {
            return false;
        }

        if (TestApiGamePlayerListInvalidMethod(authorization)) {
            std::cerr << "TestApiGamePlayerListInvalidMethod()::Complete::Status...........Ok\n" << std::endl;
        }
        else {
            return false;
        }


        return true;
    }


    bool SimpleTest::TestApiDebugSessionsClear() {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/test_frame/reset"sv,
                http::verb::post, BOOST_BEAST_VERSION_STRING, "application/json"sv, "Bearer " + config_.authorization_, ""sv));

            // создаем объект ответа
            beast::flat_buffer buffer; http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            assert(CheckServerResponse(std::move(res), ""sv,
                http_handler::ContentType::APP_JSON, "no_cache"sv, 200,
                config_.root_ + "debug/test_api_debug_data_clear.txt"));

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "SimpleTest::TestApiDebugSessionsClear::Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestApiDebugSetStartRandomPosition(bool flag) {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            std::string request_body = ""s;
            std::string ref_file = ""s;

            if (flag) {
                request_body = "{\"randomPosition\": \"true\"}";
                ref_file = "debug/test_api_debug_start_position_true.txt";
            }
            else {
                request_body = "{\"randomPosition\": \"false\"}";
                ref_file = "debug/test_api_debug_start_position_false.txt";
            }

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/test_frame/position"sv,
                http::verb::post, BOOST_BEAST_VERSION_STRING, "application/json"sv, "Bearer " + config_.authorization_, request_body));

            // создаем объект ответа
            beast::flat_buffer buffer; http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            assert(CheckServerResponse(std::move(res), ""sv,
                http_handler::ContentType::APP_JSON, "no_cache"sv, 200,
                config_.root_ + ref_file));

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "SimpleTest::TestApiDebugSetStartRandomPosition::Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestApiDebugResetStartRandomPosition() {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/test_frame/position/default"sv,
                http::verb::post, BOOST_BEAST_VERSION_STRING, "application/json"sv, "Bearer " + config_.authorization_, ""sv));

            // создаем объект ответа
            beast::flat_buffer buffer; http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            assert(CheckServerResponse(std::move(res), ""sv,
                http_handler::ContentType::APP_JSON, "no_cache"sv, 200,
                config_.root_ + "debug/test_api_debug_start_position_default.txt"));

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "SimpleTest::TestApiDebugResetStartRandomPosition::Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestApiDebugTestFrameEnd() {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/test_frame/test_end"sv,
                http::verb::post, BOOST_BEAST_VERSION_STRING, "application/json"sv, "Bearer " + config_.authorization_, ""sv));

            // создаем объект ответа
            beast::flat_buffer buffer; http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            assert(CheckServerResponse(std::move(res), ""sv,
                http_handler::ContentType::APP_JSON, "no_cache"sv, 200,
                config_.root_ + "debug/test_api_debug_test_end.txt"));

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "SimpleTest::TestApiDebugTestFrameEnd::Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestApiDebugEndpointClose(){
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/test_frame/reset"sv,
                http::verb::post, BOOST_BEAST_VERSION_STRING, "application/json"sv, "Bearer " + config_.authorization_, ""sv));

            // создаем объект ответа
            beast::flat_buffer buffer; http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            assert(CheckServerResponse(std::move(res), ""sv,
                http_handler::ContentType::APP_JSON, "no_cache"sv, 400,
                config_.root_ + "debug/test_api_debug_bad_request.txt"));

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "SimpleTest::TestApiDebugEndpointClose::Error: " << e.what() << std::endl;
            return false;
        }
    }


    bool SimpleTest::TestApiNewFirstLogin(AuthResp& data) {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/api/v1/game/join"sv,
                http::verb::post, BOOST_BEAST_VERSION_STRING, "application/json"sv,
                ""sv, "{\"userName\": \"Scooby Doo\", \"mapId\": \"map1\"}"sv));

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            // проверяем полученный ответ
            assert(CheckServerResponse(std::move(res), ""sv,
                http_handler::ContentType::APP_JSON, "no-cache"sv, 200, ""));

            boost::json::value server_resp;

            {
                // дополнительная проверка непредсказуемых ответов (токен всегда будет разный)
                // загоняем ответ сервера из строки в JSON
                server_resp = json_detail::ParseTextToJSON(
                    boost::beast::buffers_to_string(res.body().data()));

                // првоеряем в ответе наличие строк о токене и айди
                assert(server_resp.as_object().contains("authToken"));
                assert(server_resp.as_object().contains("playerId"));
                assert(server_resp.as_object().at("playerId") == 0);
            }

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }

            // сделаем "грязные" преобразования в С-стиле, ай-ай-ай, Страуструб голову бы оторвал)
            data.first = (std::string)server_resp.as_object().at("authToken").as_string();
            data.second = (size_t)server_resp.as_object().at("playerId").as_int64();

            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "SimpleTest::TestApiNewFirstLogin::Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestApiNewSecondLogin(AuthResp& data) {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/api/v1/game/join"sv,
                http::verb::post, BOOST_BEAST_VERSION_STRING, "application/json"sv,
                ""sv, "{\"userName\": \"Whelma Shnizel\", \"mapId\": \"map1\"}"sv));

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            // проверяем полученный ответ
            assert(CheckServerResponse(std::move(res), ""sv,
                http_handler::ContentType::APP_JSON, "no-cache"sv, 200, ""));

            boost::json::value server_resp;

            {
                // дополнительная проверка непредсказуемых ответов (токен всегда будет разный)
                // загоняем ответ сервера из строки в JSON
                server_resp = json_detail::ParseTextToJSON(
                    boost::beast::buffers_to_string(res.body().data()));

                // првоеряем в ответе наличие строк о токене и айди
                assert(server_resp.as_object().contains("authToken"));
                assert(server_resp.as_object().contains("playerId"));
                assert(server_resp.as_object().at("playerId") == 1);
            }

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }

            // сделаем "грязные" преобразования в С-стиле, ай-ай-ай, Страуструб голову бы оторвал)
            data.first = (std::string)server_resp.as_object().at("authToken").as_string();
            data.second = (size_t)server_resp.as_object().at("playerId").as_int64();

            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "SimpleTest::TestApiNewSecondLogin::Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestApiGameState(AuthResp& data) {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/api/v1/game/state"sv,
                http::verb::get, BOOST_BEAST_VERSION_STRING, "application/json"sv,
                "Bearer " + data.first, ""sv));

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            boost::json::value server_resp = json_detail::ParseTextToJSON(
                boost::beast::buffers_to_string(res.body().data()));
            std::string res_str = boost::beast::buffers_to_string(res.body().data());

            assert(CheckServerResponse(std::move(res), ""sv,
                http_handler::ContentType::APP_JSON, "no_cache"sv, 200,
                config_.root_ + "state/test_api_game_state.txt"));

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "SimpleTest::TestApiGameState::Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestApiGameStateInvalidMethod(AuthResp& data) {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/api/v1/game/state"sv,
                http::verb::post, BOOST_BEAST_VERSION_STRING, "application/json"sv,
                "Bearer " + data.first, ""sv));

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            assert(CheckServerResponse(std::move(res), "GET, HEAD"sv,
                http_handler::ContentType::APP_JSON, "no_cache"sv, 405,
                config_.root_ + "state/test_api_game_state_invalid_method.txt"));

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "SimpleTest::TestApiGameStateInvalidMethod::Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestApiGameStateTokenNotFound(AuthResp& data) {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/api/v1/game/state"sv,
                http::verb::get, BOOST_BEAST_VERSION_STRING, "application/json"sv,
                "Bearer ahfyrgfbdkfjthfbsyednfjwpqotnd34"sv, ""sv));

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            assert(CheckServerResponse(std::move(res), ""sv,
                http_handler::ContentType::APP_JSON, "no_cache"sv, 401,
                config_.root_ + "autho/test_api_token_not_found.txt"));

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "SimpleTest::TestApiGameStateTokenNotFound::Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestApiGameStateSet() {

        if (TestApiDebugSessionsClear()) {
            std::cerr << "TestApiDebugSessionsClear()::Complete::Status....................Ok\n" << std::endl;
        }
        else {
            return false;
        }

        if (TestApiDebugSetStartRandomPosition(false)) {
            std::cerr << "TestApiDebugSetStartRandomPosition()::Complete::Status...........Ok\n" << std::endl;
        }
        else {
            return false;
        }

        AuthResp new_scooby;             // новый токен для входа в игру персонажа Скуби
        AuthResp new_whelma;             // новый токен для входа в игру персонажа Велма

        if (TestApiNewFirstLogin(new_scooby)) {
            std::cerr << "TestApiNewFirstLogin()::Complete::Status.........................Ok\n" << std::endl;
        }
        else {
            return false;
        }

        if (TestApiNewSecondLogin(new_whelma)) {
            std::cerr << "TestApiNewSecondLogin()::Complete::Status........................Ok\n" << std::endl;
        }
        else {
            return false;
        }

        if (TestApiGameState(new_scooby)) {
            std::cerr << "TestApiGameState()::Complete::Status.............................Ok\n" << std::endl;
        }
        else {
            return false;
        }

        if (TestApiGameStateInvalidMethod(new_whelma)) {
            std::cerr << "TestApiGameStateInvalidMethod()::Complete::Status................Ok\n" << std::endl;
        }
        else {
            return false;
        }

        if (TestApiGameStateTokenNotFound(new_whelma)) {
            std::cerr << "TestApiGameStateTokenNotFound()::Complete::Status................Ok\n" << std::endl;
        }
        else {
            return false;
        }

        return true;
    }


    bool SimpleTest::TestApiPlayerMove(AuthResp& data) {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/api/v1/game/player/action"sv,
                http::verb::post, BOOST_BEAST_VERSION_STRING, "application/json"sv,
                "Bearer " + data.first, "{\"move\": \"R\"}"sv));

            // создаем объект ответа
            beast::flat_buffer buffer; http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            assert(CheckServerResponse(std::move(res), ""sv,
                http_handler::ContentType::APP_JSON, "no_cache"sv, 200,
                config_.root_ + "move/test_api_player_move.txt"));

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "SimpleTest::TestApiPlayerMove::Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestApiPlayerInvalidMethod(AuthResp& data)
    {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/api/v1/game/player/action"sv,
                http::verb::get, BOOST_BEAST_VERSION_STRING, "application/json"sv,
                "Bearer " + data.first, "{\"move\": \"R\"}"sv));

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            assert(CheckServerResponse(std::move(res), "POST"sv,
                http_handler::ContentType::APP_JSON, "no_cache"sv, 405,
                config_.root_ + "login/test_api_game_login_invalid_method.txt"));

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "SimpleTest::TestApiPlayerInvalidMethod::Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestApiPlayerInvalidToken(AuthResp& data) {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/api/v1/game/player/action"sv,
                http::verb::post, BOOST_BEAST_VERSION_STRING, "application/json"sv,
                "Cearer " + data.first, "{\"move\": \"R\"}"sv));

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            assert(CheckServerResponse(std::move(res), ""sv,
                http_handler::ContentType::APP_JSON, "no_cache"sv, 401,
                config_.root_ + "autho/test_api_authorization_missing.txt"));

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "SimpleTest::TestApiPlayerInvalidToken::Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestApiPlayerTokenNotFound(AuthResp& data) {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/api/v1/game/player/action"sv,
                http::verb::post, BOOST_BEAST_VERSION_STRING, "application/json"sv,
                "Bearer ahfyrgfbdkfjthfbsyednfjwpqotnd34"sv, "{\"move\": \"R\"}"sv));

            // создаем объект ответа
            beast::flat_buffer buffer; http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            assert(CheckServerResponse(std::move(res), ""sv,
                http_handler::ContentType::APP_JSON, "no_cache"sv, 401,
                config_.root_ + "autho/test_api_token_not_found.txt"));

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "SimpleTest::TestApiPlayerTokenNotFound::Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestApiPlayerInvalidContent(AuthResp& data) {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/api/v1/game/player/action"sv,
                http::verb::post, BOOST_BEAST_VERSION_STRING, "text/css"sv,
                "Bearer " + data.first, "{\"move\": \"R\"}"sv));

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            assert(CheckServerResponse(std::move(res), ""sv,
                http_handler::ContentType::APP_JSON, "no_cache"sv, 400,
                config_.root_ + "move/test_api_player_move_invalid_content_type.txt"));

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "SimpleTest::TestApiPlayerInvalidContent::Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestApiPlayerMissBody(AuthResp& data) {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/api/v1/game/player/action"sv,
                http::verb::post, BOOST_BEAST_VERSION_STRING, "application/json"sv,
                "Bearer " + data.first, ""sv));

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            assert(CheckServerResponse(std::move(res), ""sv,
                http_handler::ContentType::APP_JSON, "no_cache"sv, 400,
                config_.root_ + "move/test_api_player_move_miss_body.txt"));

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "SimpleTest::TestApiPlayerMissBody::Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestApiPlayerInvalidBody(AuthResp& data) {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/api/v1/game/player/action"sv,
                http::verb::post, BOOST_BEAST_VERSION_STRING, "application/json"sv,
                "Bearer " + data.first, "{\"move\": \"F\"}"sv));

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            assert(CheckServerResponse(std::move(res), ""sv,
                http_handler::ContentType::APP_JSON, "no_cache"sv, 400,
                config_.root_ + "move/test_api_player_move_invalid_body.txt"));

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "SimpleTest::TestApiPlayerInvalidBody::Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestApiPlayerMoveState(AuthResp& data) {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/api/v1/game/state"sv,
                http::verb::get, BOOST_BEAST_VERSION_STRING, "application/json"sv,
                "Bearer " + data.first, ""sv));

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            boost::json::value server_resp = json_detail::ParseTextToJSON(
                boost::beast::buffers_to_string(res.body().data()));
            std::string res_str = boost::beast::buffers_to_string(res.body().data());

            assert(CheckServerResponse(std::move(res), ""sv,
                http_handler::ContentType::APP_JSON, "no_cache"sv, 200,
                config_.root_ + "move/test_api_player_move_state.txt"));

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "SimpleTest::TestApiGameState::Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestApiPlayerMoveSet() {

        if (TestApiDebugSessionsClear()) {
            std::cerr << "TestApiDebugSessionsClear()::Complete::Status....................Ok\n" << std::endl;
        }
        else {
            return false;
        }

        AuthResp new_scooby;             // новый токен для входа в игру персонажа Скуби
       
        if (TestApiNewFirstLogin(new_scooby)) {
            std::cerr << "TestApiNewFirstLogin()::Complete::Status.........................Ok\n" << std::endl;
        }
        else {
            return false;
        }

        if (TestApiPlayerMove(new_scooby)) {
            std::cerr << "TestApiPlayerMove()::Complete::Status............................Ok\n" << std::endl;
        }
        else {
            return false;
        }

        if (TestApiPlayerInvalidMethod(new_scooby)) {
            std::cerr << "TestApiPlayerInvalidMethod()::Complete::Status...................Ok\n" << std::endl;
        }
        else {
            return false;
        }

        if (TestApiPlayerInvalidToken(new_scooby)) {
            std::cerr << "TestApiPlayerInvalidToken()::Complete::Status....................Ok\n" << std::endl;
        }
        else {
            return false;
        }

        if (TestApiPlayerTokenNotFound(new_scooby)) {
            std::cerr << "TestApiPlayerTokenNotFound()::Complete::Status...................Ok\n" << std::endl;
        }
        else {
            return false;
        }

        if (TestApiPlayerInvalidContent(new_scooby)) {
            std::cerr << "TestApiPlayerInvalidContent()::Complete::Status..................Ok\n" << std::endl;
        }
        else {
            return false;
        }

        if (TestApiPlayerMissBody(new_scooby)) {
            std::cerr << "TestApiPlayerMissBody()::Complete::Status........................Ok\n" << std::endl;
        }
        else {
            return false;
        }

        if (TestApiPlayerInvalidBody(new_scooby)) {
            std::cerr << "TestApiPlayerInvalidBody()::Complete::Status.....................Ok\n" << std::endl;
        }
        else {
            return false;
        }

        if (TestApiPlayerMoveState(new_scooby)) {
            std::cerr << "TestApiPlayerMoveState()::Complete::Status.......................Ok\n" << std::endl;
        }
        else {
            return false;
        }

        return true;
    }


    bool SimpleTest::TestApiTimeTick() {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/api/v1/game/tick"sv,
                http::verb::post, BOOST_BEAST_VERSION_STRING, "application/json"sv,
                ""sv, "{\"timeDelta\": \"100\"}"sv));

            // создаем объект ответа
            beast::flat_buffer buffer; http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            assert(CheckServerResponse(std::move(res), ""sv,
                http_handler::ContentType::APP_JSON, "no_cache"sv, 200,
                config_.root_ + "time/test_api_time_tick.txt"));

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "SimpleTest::TestApiTimeTick::Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestApiTimeTickMissBody() {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/api/v1/game/tick"sv,
                http::verb::post, BOOST_BEAST_VERSION_STRING, "application/json"sv,
                ""sv, ""sv));

            // создаем объект ответа
            beast::flat_buffer buffer; http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            assert(CheckServerResponse(std::move(res), ""sv,
                http_handler::ContentType::APP_JSON, "no_cache"sv, 400,
                config_.root_ + "time/test_api_time_tick_invalid_body.txt"));

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "SimpleTest::TestApiTimeTickMissBody::Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestApiTimeTickInvalidBody() {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/api/v1/game/tick"sv,
                http::verb::post, BOOST_BEAST_VERSION_STRING, "application/json"sv,
                ""sv, "{\"timeDelta\": \"0.0\"}"sv));

            // создаем объект ответа
            beast::flat_buffer buffer; http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            assert(CheckServerResponse(std::move(res), ""sv,
                http_handler::ContentType::APP_JSON, "no_cache"sv, 400,
                config_.root_ + "time/test_api_time_tick_miss_body.txt"));

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "SimpleTest::TestApiTimeTickInvalidBody::Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestApiTimeTickState(AuthResp& data) {
        try
        {
            // устанавливаем соединение с сервером, используя результаты разрешения
            stream_.connect(endpoint_);

            // отправляем сгенерированный запрос на сервер
            http::write(stream_, MakeTestRequest(endpoint_, "/api/v1/game/state"sv,
                http::verb::get, BOOST_BEAST_VERSION_STRING, "application/json"sv,
                "Bearer " + data.first, ""sv));

            // создаем объект ответа
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> res;

            // получаем ответ от сервера
            http::read(stream_, buffer, res);

            boost::json::value server_resp = json_detail::ParseTextToJSON(
                boost::beast::buffers_to_string(res.body().data()));
            std::string res_str = boost::beast::buffers_to_string(res.body().data());

            assert(CheckServerResponse(std::move(res), ""sv,
                http_handler::ContentType::APP_JSON, "no_cache"sv, 200,
                config_.root_ + "time/test_api_time_tick_state.txt"));

            // закрываем соединение с сервером
            beast::error_code ec;
            stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

            // если произошла ошибка, выводим ее сообщение
            if (ec && ec != beast::errc::not_connected) {
                throw beast::system_error{ ec };
            }
            return true;
        }
        catch (std::exception const& e) {
            std::cerr << "SimpleTest::TestApiTimeTickState::Error: " << e.what() << std::endl;
            return false;
        }
    }

    bool SimpleTest::TestApiTimeTickSet() {

        if (TestApiDebugSessionsClear()) {
            std::cerr << "TestApiDebugSessionsClear()::Complete::Status....................Ok\n" << std::endl;
        }
        else {
            return false;
        }

        AuthResp new_scooby;             // новый токен для входа в игру персонажа Скуби

        if (TestApiNewFirstLogin(new_scooby)) {
            std::cerr << "TestApiNewFirstLogin()::Complete::Status.........................Ok\n" << std::endl;
        }
        else {
            return false;
        }

        if (TestApiPlayerMove(new_scooby)) {
            std::cerr << "TestApiPlayerMove()::Complete::Status............................Ok\n" << std::endl;
        }
        else {
            return false;
        }

        if (TestApiTimeTick()) {
            std::cerr << "TestApiTimeTick()::Complete::Status..............................Ok\n" << std::endl;
        }
        else {
            return false;
        }

        if (TestApiTimeTickMissBody()) {
            std::cerr << "TestApiTimeTickMissBody()::Complete::Status......................Ok\n" << std::endl;
        }
        else {
            return false;
        }

        if (TestApiTimeTickInvalidBody()) {
            std::cerr << "TestApiTimeTickInvalidBody()::Complete::Status...................Ok\n" << std::endl;
        }
        else {
            return false;
        }

        if (TestApiTimeTickState(new_scooby)) {
            std::cerr << "TestApiTimeTickState()::Complete::Status.........................Ok\n" << std::endl;
        }
        else {
            return false;
        }

        return true;
    }


    // конфигурирует запрос по заданным параметрам
    http::request<http::string_body> SimpleTest::MakeTestRequest(resolver_endpoint& endpoint_, std::string_view target,
        http::verb method, std::string_view user_agent, std::string_view content_type, std::string_view authorization, std::string_view body) {

        http::request<http::string_body> req{ method, target, 11 };
        req.set(http::field::host, endpoint_->endpoint().address().to_string());

        if (content_type.size() != 0) {
            req.set(http::field::content_type, content_type);
        }

        if (authorization.size() != 0) {
            req.set(http::field::authorization, authorization);
        }

        if (user_agent.size() != 0) {
            req.set(http::field::user_agent, user_agent);
        }

        if (body.size() != 0) {
            req.body() = body;
            req.prepare_payload();
        }
        
        return req;
    }

    // првоеряет ответ на тестовый запрос согласно переданным параметрам
    bool SimpleTest::CheckServerResponse(http::response<http::dynamic_body>&& res, std::string_view allow,
        std::string_view content_type, std::string_view no_cache, int result_code, std::string_view reference_file_path) {

        // проверяем совпадение кода ответа сервера
        assert(res.result_int() == result_code);

        // проверяем совпадение типа контента
        auto const& content_ = res.find(http::field::content_type);
        if (content_type.size() != 0 && content_ != res.end()) {
            assert(content_->value() == content_type);
        } 

        // проверяем allow, если он указан
        auto const& allow_ = res.find(http::field::allow);
        if (allow.size() != 0 && allow_ != res.end()) {
            assert(allow_->value() == allow);
        } 

        // проверяем cache-control
        auto const& cache_ = res.find(http::field::cache_control);
        if (no_cache.size() != 0 && cache_ != res.end()) {
            assert(cache_->value() == "no-cache"sv);
        }

        if (reference_file_path.size() != 0) {

            // проверяем совпадение строки ответа с заготовкой в файле
            // открываем файл с заготовкой ответа
            std::fstream file(std::string(reference_file_path), std::ios::in);

            // чтобы не заморачиваться с переносами строк и прочим битово читаем файл в поток
            std::stringstream buffer;
            buffer << file.rdbuf(); file.close();

            // загоняем ответ сервера из строки в JSON
            auto server_resp = json_detail::ParseTextToJSON(
                boost::beast::buffers_to_string(res.body().data()));
            // загоняем буфер из файла в JSON
            auto file_resp = json_detail::ParseTextToJSON(buffer.str());

            // сравниваем полученные значения
            assert(server_resp == file_resp);
        }

        return true;

    }

} // namespace test
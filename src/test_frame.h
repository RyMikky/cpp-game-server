#pragma once

#include "boost_json.h"           // отсюда получим работу с JSON и domain.h с базовыми инклюдами

#include <boost/beast.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <string_view>
#include <iostream>
#include <string>
#include <thread>

namespace beast = boost::beast;
namespace http = beast::http;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

using namespace std::literals;

namespace test {

	struct TestConfiguration {
		std::string address_;
		std::string port_;
		std::string root_;
		std::string authorization_;

		bool enable_ = false;
	};

	class SimpleTest {
		using AuthResp = std::pair<std::string, size_t>;    // ответ на вход в игру, токен и id
		using resolver_endpoint = boost::asio::ip::basic_resolver_results<boost::asio::ip::tcp>;
	public:
		explicit SimpleTest(TestConfiguration&& config);

		SimpleTest& RunAllTests();
	private:
		TestConfiguration config_;
		asio::io_context ioc_;
		tcp::resolver resolver_;
		resolver_endpoint endpoint_;
		beast::tcp_stream stream_;

		// ---------- блок проверки базовых запросов к API ---------

		bool TestApiMapsList();                                             // запрос списка карт на сервере
		bool TestApiMapOne();                                               // запрос информации по первой карте
		bool TestApiMapTown();                                              // запрос информации по карте город
		bool TestApiMapNotFound();                                          // запрос с несуществующей картой
		bool TestApiBadRequest();                                           // некорректный запрос к API

		bool TestBasicApiSet();                                             // запуск вышеизложенного сета

		// ---------- блок проверки входа в игровую сессию ---------

		bool TestApiGameFirstLogin();                                       // запрос на вход в игру
		bool TestApiGameSecondLogin();                                      // запрос на вход в игру
		bool TestApiGameLoginMissName();                                    // запрос на вход без имени
		bool TestApiGameLoginInvalidName();                                 // запрос на вход с невалидным именем
		bool TestApiGameLoginMissMap();                                     // запрос на вход без карты
		bool TestApiGameLoginInvalidMap();                                  // запрос на вход с невалидной картой
		bool TestApiGameLoginMapNotFound();                                 // запрос на вход с несуществующей картой
		bool TestApiGameLoginInvalidMethod();                               // запрос на вход с невалидным методом

		bool TestGameLoginSet();                                            // запуск вышеизложенного сета

		// ---------- блок проверки авторизации и запроса списка игроков ---------

		bool TestApiGameThirdLogin(AuthResp& data);                         // запрос на вход в игру и запись токена и id
		bool TestApiGamePlayerList(AuthResp& data);                         // запрос на просмотр листа игроков в сессии
		bool TestApiGameAuthorizationMissBody(AuthResp& data);              // запрос на просмотр без тела запроса
		bool TestApiGameAuthorizationMissToken(AuthResp& data);             // запрос на просмотр без токена в запросе
		bool TestApiGameAuthorizationInvalidBody(AuthResp& data);           // запрос на просмотр с невалидным телом запроса
		bool TestApiGameTokenNotFound(AuthResp& data);                      // запрос на просмотр с неизвестным токеном
		bool TestApiGamePlayerListInvalidMethod(AuthResp& data);            // запрос на просмотр с невалидным методом

		bool TestApiAuthorizationSet();                                     // запуск вышеизложенного сета
		
		// ---------- блок управляющих запросов тест-системы -------

		bool TestApiDebugSessionsClear();                                   // запрос на удаление всех игровых сессий
		bool TestApiDebugSetStartRandomPosition(bool flag);                 // запрос на назначение флага случайного размещения новых игроков
		bool TestApiDebugResetStartRandomPosition();                        // то же что и выше, но по параметрам из конфигурационного файла
		bool TestApiDebugTestFrameEnd();                                    // запрос на прекращение работы тест-системы
		bool TestApiDebugEndpointClose();                                   // запрос о проверке доступа после завершения тестов

		// ---------- блок проверки запроса состояния игры ---------

		bool TestApiNewFirstLogin(AuthResp& data);                          // запрос на новый вход в игру после очистки данных
		bool TestApiNewSecondLogin(AuthResp& data);                         // запрос на новый вход в игру после очистки данных
		bool TestApiGameState(AuthResp& data);                              // запрос на получение состояния персонажей на карте
		bool TestApiGameStateInvalidMethod(AuthResp& data);                 // запрос на получение состояния персонажей на карте с невалидным методом
		bool TestApiGameStateTokenNotFound(AuthResp& data);                 // запрос на получение состояния персонажей на карте с неизвестным токеном

		bool TestApiGameStateSet();                                         // запуск вышеизложенного сета
		
		// ---------- блок проверки записи движения игрока ---------

		bool TestApiPlayerMove(AuthResp& data);                             // запрос на назначение персонажу скорости и направления
		bool TestApiPlayerInvalidMethod(AuthResp& data);                    // запрос на назначение персонажу скорости и направления с невалидным методом
		bool TestApiPlayerInvalidToken(AuthResp& data);                     // запрос на назначение персонажу скорости и направления с невалидным токеном
		bool TestApiPlayerTokenNotFound(AuthResp& data);                    // запрос на назначение персонажу скорости и направления с неизвестным токеном
		bool TestApiPlayerInvalidContent(AuthResp& data);                   // запрос на назначение персонажу скорости и направления с невалидными content-type
		bool TestApiPlayerMissBody(AuthResp& data);                         // запрос на назначение персонажу скорости и направления без тела запроса
		bool TestApiPlayerInvalidBody(AuthResp& data);                      // запрос на назначение персонажу скорости и направления с невалидным телом запроса
		bool TestApiPlayerMoveState(AuthResp& data);                        // запрос на получение состояния персонажей на карте после изменения скорости

		bool TestApiPlayerMoveSet();                                        // запуск вышеизложенного сета

		// ---------- блок проверки изменения состояния во времени ---------

		bool TestApiTimeTick();                                             // запрос на изменение состояния во времени
		bool TestApiTimeTickMissBody();                                     // запрос на изменение состояния во времени без тела запроса
		bool TestApiTimeTickInvalidBody();                                  // запрос на изменение состояния во времени с невалидным телом запроса
		bool TestApiTimeTickState(AuthResp& data);                          // запрос на получение состояния персонажей на карте после изменения времени

		bool TestApiTimeTickSet();                                          // запуск вышеизложенного сета

		// конфигурирует запрос по заданным параметрам
		http::request<http::string_body> MakeTestRequest(resolver_endpoint& endpoint_, std::string_view target, http::verb method,
			std::string_view user_agent, std::string_view content_type, std::string_view authorization, std::string_view body);

		// првоеряет ответ на тестовый запрос согласно переданным параметрам
		bool CheckServerResponse(http::response<http::dynamic_body>&& res, std::string_view allow,
			std::string_view content_type, std::string_view no_cache, int result_code, std::string_view reference_file_path);

	};

} // namespace test
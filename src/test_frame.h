#pragma once

#include "boost_json.h"           // отсюда получим работу с JSON и domain.h с базовыми инклюдами

#include <boost/beast.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <iostream>
#include <string>
#include <thread>

namespace beast = boost::beast;
namespace http = beast::http;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

namespace test {

	class SimpleTest {
		using resolver_endpoint = boost::asio::ip::basic_resolver_results<boost::asio::ip::tcp>;
	public:
		SimpleTest(std::string_view address, std::string_view port);
		SimpleTest& RunAllTests();

	private:
		asio::io_context ioc_;
		tcp::resolver resolver_;
		resolver_endpoint endpoint_;
		beast::tcp_stream stream_;

		void TestApiMapsList();
		void TestApiMapOne();
		void TestApiMapNotFound();
		void TestApiBadRequest();

		void TestApiGameLogin();
		void TestApiGameSecondLogin();
		void TestApiGameNutLogin();

		void TestApiGameLoginMissName();
		void TestApiGameLoginInvalidName();
		void TestApiGameLoginMissMap();
		void TestApiGameLoginInvalidMap();
		void TestApiGameLoginMapNotFound();
		
		void TestApiGameLoginInvalidMethod();

		using AuthResp = std::pair<std::string, size_t>;
		AuthResp TestApiGameThirdLogin();
		void TestApiGamePlayerList(AuthResp data);
		void TestApiGameAuthorizationMissBody(AuthResp data);
		void TestApiGameAuthorizationMissToken(AuthResp data);
		void TestApiGameAuthorizationMissing(AuthResp data);
		void TestApiGameTokenNotFound(AuthResp data);
		void TestApiGamePlayerListInvalidMethod(AuthResp data);
		void TestAuthorizationSet();

	};

} // namespace test
#pragma once

#include <string>
#include <random>
#include "tagged.h"

using namespace std::literals;

namespace game_handler {

	namespace detail {

		static std::random_device __RANDOM_DEVICE__;

		static std::mt19937_64 __LOWER_GENERATOR__{ [] {
			std::uniform_int_distribution<std::mt19937_64::result_type> dist;
			return dist(__RANDOM_DEVICE__);
		}() };

		static std::mt19937_64 __UPPER_GENERATOR__{ [] {
			std::uniform_int_distribution<std::mt19937_64::result_type> dist;
			return dist(__RANDOM_DEVICE__);
		}() };

		uint64_t GenerateLower();

		uint64_t GenerateUpper();

		std::string GenerateToken32Hex();

		struct TokenTag {};

	}  // namespace detail

	using Token = util::Tagged<std::string, detail::TokenTag>;



	/*class Token {
	public:

	protected:
		explicit Token(std::string label) 
			: token_label_(label){
			token_32_hex_ = detail::GenerateToken32Hex();
		}

	private:
		std::string token_label_;
		std::string token_32_hex_;
	};


	class sToken : public Token {
	public:
		explicit sToken(std::string label) : Token(label) {

		}
	};*/


} //namespace game_handler
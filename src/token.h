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

		uint64_t GenerateLowerTokenPart();

		uint64_t GenerateUpperTokenPart();

		std::string GenerateToken32Hex();

		struct TokenTag {};

	}  // namespace detail

	using Token = util::Tagged<std::string, detail::TokenTag>;

} //namespace game_handler
#include "token.h"

#include <iomanip>                   // для работы std::hex
#include <sstream>

namespace game_handler {

	namespace detail {

		uint64_t GenerateLower() {
			return __LOWER_GENERATOR__();
		}

		uint64_t GenerateUpper() {
			return __UPPER_GENERATOR__();
		}
		
		std::string GenerateToken32Hex() {

			// генерируем младшее и старшее плечо
			uint64_t lower_ = GenerateLower();
			uint64_t upper_ = GenerateUpper();

			// побитово складываю два числа
			uint64_t combined = (upper_ & lower_);

			// повторно "ломаем" полученное скомбинированное число, еще раз частично перемешивя с ранее сгенерированными
			uint64_t upper = (combined << 16) | ((lower_ & 0xffffull) << 48);
			uint64_t lower = (combined >> 16) | ((upper_ & 0xffffull) << 48);

			std::ostringstream out;
			// создаём поток для вывода получившегося числа в текстовом представлении
			out << std::hex << std::setfill('0') << std::setw(16) << upper << std::setw(16) << lower;
			
			return out.str();
		}
	}
}
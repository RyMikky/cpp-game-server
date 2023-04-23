#include "token.h"

#include <iomanip>                   // ��� ������ std::hex
#include <sstream>

namespace game_handler {

	namespace detail {

		uint64_t geterate_lower_token_part() {
			return __LOWER_GENERATOR__();
		}

		uint64_t generate_upper_token_part() {
			return __UPPER_GENERATOR__();
		}
		
		std::string generate_token_32_hex() {

			// ���������� ������� � ������� �����
			uint64_t lower_ = geterate_lower_token_part();
			uint64_t upper_ = generate_upper_token_part();

			// �������� ��������� ��� �����
			uint64_t combined = (upper_ & lower_);

			// �������� "������" ���������� ���������������� �����, ��� ��� �������� ���������� � ����� ����������������
			uint64_t upper = (combined << 16) | ((lower_ & 0xffffull) << 48);
			uint64_t lower = (combined >> 16) | ((upper_ & 0xffffull) << 48);

			std::ostringstream out;
			// ������ ����� ��� ������ ������������� ����� � ��������� �������������
			out << std::hex << std::setfill('0') << std::setw(16) << upper << std::setw(16) << lower;
			
			return out.str();
		}
	}
}
#include "token.h"

#include <iomanip>                   // ��� ������ std::hex
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

			// ���������� ������� � ������� �����
			uint64_t lower_ = GenerateLower();
			uint64_t upper_ = GenerateUpper();

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
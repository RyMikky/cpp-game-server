#include "time_handler.h"

#include <boost/asio.hpp>
#include <algorithm>

namespace time_handler {

	// ��������� ������� �� ����������� �������
	void OnTimeCommandImpl::Execute(sys::error_code ec) {
		if (execution_) {
			try
			{
				timer_.async_wait(
					net::bind_executor(strand_, [self = shared_from_this()](sys::error_code ec) {
						// ����� ����� �������� �� ����������
						self->timer_.expires_from_now(self->period_);
						// ������������ ���������� �������
						self->command_->Execute();
						// ����� �������� ����� ����������
						self->Execute(ec);

						}));

			}
			catch (const std::exception& e)
			{
				throw std::runtime_error("OnTimeCommandImpl::Execute::Error\n" + std::string(e.what()));
			}
		}
		else {
			timer_.cancel();            // ���� ���� ���������� ����, �� ������������� ������
		}
	}

	// ������������� ���������� �������
	void OnTimeCommandImpl::Restart(sys::error_code ec) {
		execution_ = true;
		Execute(ec);
	}


	// -------------------------- class TimeHandler --------------------------
	
	// ��������� ������� � ���� �������, ������ ����� ��������� � ������������
	TimeHandler& TimeHandler::AddCommand(std::string period, std::shared_ptr<ICommand>&& command) {
		return AddCommand(std::stoi(period), std::move(command));
	}

	// ��������� ������� � ���� �������, ������ ����� ��������� � ������������
	TimeHandler& TimeHandler::AddCommand(int period, std::shared_ptr<ICommand>&& command) {
		return AddCommand(std::chrono::milliseconds(period), std::move(command));
	}

	// ��������� ������� � ���� �������
	TimeHandler& TimeHandler::AddCommand(std::chrono::milliseconds period, std::shared_ptr<ICommand>&& command) {

		if (!commands_.count(period)) {
			// ���� � �������� ������ �� ����������� ������� ������ ������ ����� ����
			commands_.insert(std::pair{period, CommandLine{}});
		}

		// �������� ���������� ������ ��� ���������� �������
		auto correct_period = CalculateWaitTime(period);

		// ��������� ������� � ���� �� ���������� ���������� ����������
		commands_.at(period).push_back(
			std::make_shared<OnTimeCommandImpl>( api_strand_, correct_period, std::move(command) ));
		return *this;
	}

	// ��������� ���������� �������
	TimeHandler& TimeHandler::Start() {
		if (!execution_) {
			execution_ = true;
			sys::error_code error_code;
			ExecuteCommandLine(error_code);
		}
		return *this;
	}

	// ������������� ���������� �������
	TimeHandler& TimeHandler::Stop() {
		if (execution_) {
			execution_ = false;
		}
		return *this;
	}

	// ��������� ����� ��������� �������� �������
	TimeHandler& TimeHandler::SetAlignPeriod(std::chrono::milliseconds period) {
		if (!execution_) {
			align_period_ = period;
			return *this;
		}

		throw std::runtime_error("GameTimer::SetAlignPeriod::Error::Timer execution flag is \"true\"");
	}

	// ����������� ����� ���������� ��������� �������, � ����������� �� ������� ������������
	std::chrono::milliseconds TimeHandler::CalculateWaitTime(std::chrono::milliseconds period) {
		
		if (period > align_period_) {
			// ���� ������� ������ ������ ������� ������������ ���������� ���������
			// ����� �� ���������� ������� ������������, ������������� ����������� ������
			// ��� ��� ������ � ����� ������ �����������, ���� ���������, �� ��� ��� 20 ���, �� 30
			// ������� ��� ���� ����� ��� ����� �����, ��� ��� ��� ������� �������� �����
			// ������ ����� �������� � 2, ��� ��� ��� �������, ��� align_period_ * 1 ������ ���������
			std::chrono::milliseconds result;                  // ������������� ���������
			for (int scaler = 2; ; ++scaler) {
				result = (scaler * align_period_);             // �������� ������ ������������ �� ������
				if (result >= period) {
					// ���� ���������� ����� ��������� ��� ����� ��������� �������, �� ���������� ���
					return result;
				}
			}
		}
		
		else {
			// ���� ������ ���������� ������� ������ ��� ����� ������� ������������, �� ������ �� �������
			return period;
		}
	}

	// ��������� ���������� ����� ���� ������
	void TimeHandler::ExecuteCommandLine(sys::error_code ec) {
		
		// ���� �� ���� ��������� ��������
		for (auto& [period, command_line] : commands_) {
			// ���� �� ���� ��������
			for (auto& command : command_line) {

				if (execution_) {
					// ��������� ���������� ���������� �������
					command->Execute(ec);
				}
				else {
					command->Abort();
				}
			}
		}
	}

} // namespace time_handler
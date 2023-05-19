#pragma once

#include "domain.h"

#include <map>
#include <vector>
#include <chrono>

namespace time_handler {

	namespace beast = boost::beast;
	namespace http = beast::http;
	namespace net = boost::asio;
	namespace sys = boost::system;
	
	template <typename Argv>
	using Function = std::function<void(Argv)>;

	class ICommand {
	public:
		virtual ~ICommand() = default;
		virtual void Execute() = 0;
	};

	// ������� ��� ����� ������� � ����� ��������� ����������, ��� ��� ����
	// ���� ��� ����������, ����� ���������� �������� ������� (void*), � ���������� ���������� nullptr
	template <typename Argv>
	class OnTimeCommand : public ICommand {
	public:
		OnTimeCommand() = delete;
		OnTimeCommand(const OnTimeCommand&) = delete;
		OnTimeCommand& operator=(const OnTimeCommand&) = delete;

		OnTimeCommand(OnTimeCommand&&) = default;
		OnTimeCommand& operator=(OnTimeCommand&&) = default;

		~OnTimeCommand() = default;

		OnTimeCommand(Function<Argv>&& function, Argv argv)
			: function_(std::move(function)), argv_(argv) {}

		void Execute() override {
			function_(argv_);
		}

	private:
		Function<Argv> function_;
		Argv argv_;
	};

	// ������� ���������� ������� � ����������� �������� ����������
	// ��� ������ ������� ������ � ������ ����������
	class OnTimeCommandImpl : public std::enable_shared_from_this<OnTimeCommandImpl> {
		using Timer = net::steady_timer;
	public:
		OnTimeCommandImpl(http_handler::Strand& strand, std::chrono::milliseconds period, std::shared_ptr<ICommand>&& command)
			: strand_(strand), period_(period), command_(std::move(command)) {
		}

		// ��������� ������� �� ����������� �������
		void Execute(sys::error_code ec);
		// ������������� ���������� �������
		void Restart(sys::error_code ec);
		// ������� ���� ���������� ������� ��� ��������� ���������
		void Abort() {
			execution_ = false;
		}
		// �������� ����� ���������� �������
		void SetPeriod(std::chrono::milliseconds period) {
			period_ = period;
		}

	private:	
		http_handler::Strand& strand_;
		std::chrono::milliseconds period_;
		Timer timer_{ strand_, period_ };
		std::shared_ptr<ICommand> command_;

		bool execution_ = true;
	};


	class TimeHandler : public std::enable_shared_from_this<TimeHandler> {
		using CommandLine = std::vector<std::shared_ptr<OnTimeCommandImpl>>;
		// ������ ���� � ��������� ������� ����������� � ����� ��������
		using CommandBase = std::map<std::chrono::milliseconds, CommandLine>;
	public:
		TimeHandler(http_handler::Strand& strand, std::chrono::milliseconds align_period)
			: api_strand_(strand), align_period_(align_period) {
		}

		TimeHandler(const TimeHandler&) = delete;
		TimeHandler& operator=(const TimeHandler&) = delete;
		TimeHandler(TimeHandler&&) = default;
		TimeHandler& operator=(TimeHandler&&) = default;

		// ��������� ������� � ���� �������, ������ ����� ��������� � ������������
		TimeHandler& AddCommand(std::string period, std::shared_ptr<ICommand>&& command);
		// ��������� ������� � ���� �������, ������ ����� ��������� � ������������
		TimeHandler& AddCommand(int period, std::shared_ptr<ICommand>&& command);
		// ��������� ������� � ���� �������
		TimeHandler& AddCommand(std::chrono::milliseconds period, std::shared_ptr<ICommand>&& command);

		// ��������� ���������� �������
		TimeHandler& Start();
		// ������������� ���������� �������
		TimeHandler& Stop();
		// ��������� ����� ��������� �������� �������
		TimeHandler& SetAlignPeriod(std::chrono::milliseconds period);

	private:
		http_handler::Strand& api_strand_;
		std::chrono::milliseconds align_period_;

		bool execution_ = false;
		// ����������� ����� ���������� ��������� �������, � ����������� �� ������� ������������
		std::chrono::milliseconds CalculateWaitTime(std::chrono::milliseconds period);

		// ��������� ���������� ����� ���� ������
		void ExecuteCommandLine(sys::error_code ec);

		CommandBase commands_;
	};


} // namespace time_handler
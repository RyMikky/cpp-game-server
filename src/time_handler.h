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

	// обертка для любой функции с одним шаблонным параметром, или без оных
	// если без параметров, тогда необходимо шаблоном ставить (void*), и аргументом передавать nullptr
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

	// обертка выполнения команды с собственным таймером выполнения
	// для работы требует стренд и период выполнения
	class OnTimeCommandImpl : public std::enable_shared_from_this<OnTimeCommandImpl> {
		using Timer = net::steady_timer;
	public:
		OnTimeCommandImpl(http_handler::Strand& strand, std::chrono::milliseconds period, std::shared_ptr<ICommand>&& command)
			: strand_(strand), period_(period), command_(std::move(command)) {
		}

		// выполняет команду на собственном таймере
		void Execute(sys::error_code ec);
		// перезапускает выполнение команды
		void Restart(sys::error_code ec);
		// снимает флаг выполнения команды чем завершает обработку
		void Abort() {
			execution_ = false;
		}
		// азначает ериод повторения команды
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
		// хранит мапу с векторами комманд выполняемых с одним периодом
		using CommandBase = std::map<std::chrono::milliseconds, CommandLine>;
	public:
		TimeHandler(http_handler::Strand& strand, std::chrono::milliseconds align_period)
			: api_strand_(strand), align_period_(align_period) {
		}

		TimeHandler(const TimeHandler&) = delete;
		TimeHandler& operator=(const TimeHandler&) = delete;
		TimeHandler(TimeHandler&&) = default;
		TimeHandler& operator=(TimeHandler&&) = default;

		// добавляет команду в базу таймера, период будет переведен в миллисекунды
		TimeHandler& AddCommand(std::string period, std::shared_ptr<ICommand>&& command);
		// добавляет команду в базу таймера, период будет переведен в миллисекунды
		TimeHandler& AddCommand(int period, std::shared_ptr<ICommand>&& command);
		// добавляет команду в базу таймера
		TimeHandler& AddCommand(std::chrono::milliseconds period, std::shared_ptr<ICommand>&& command);

		// запускает выполнение таймера
		TimeHandler& Start();
		// останавливает выполнение таймера
		TimeHandler& Stop();
		// назначает новый временной интервал таймера
		TimeHandler& SetAlignPeriod(std::chrono::milliseconds period);

	private:
		http_handler::Strand& api_strand_;
		std::chrono::milliseconds align_period_;

		bool execution_ = false;
		// расчитывает время выполнения следующей команды, в зависимости от периода выравнивания
		std::chrono::milliseconds CalculateWaitTime(std::chrono::milliseconds period);

		// запускает выполнение всего пула команд
		void ExecuteCommandLine(sys::error_code ec);

		CommandBase commands_;
	};


} // namespace time_handler
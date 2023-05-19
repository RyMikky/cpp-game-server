#include "time_handler.h"

#include <boost/asio.hpp>
#include <algorithm>

namespace time_handler {

	// выполняет команду на собственном таймере
	void OnTimeCommandImpl::Execute(sys::error_code ec) {
		if (execution_) {
			try
			{
				timer_.async_wait(
					net::bind_executor(strand_, [self = shared_from_this()](sys::error_code ec) {
						// задаём время ожидания до исполнения
						self->timer_.expires_from_now(self->period_);
						// заппрашиваем исполнение команды
						self->command_->Execute();
						// снова вызываем метод выполнения
						self->Execute(ec);

						}));

			}
			catch (const std::exception& e)
			{
				throw std::runtime_error("OnTimeCommandImpl::Execute::Error\n" + std::string(e.what()));
			}
		}
		else {
			timer_.cancel();            // если флаг выполнения снят, то останавливаем таймер
		}
	}

	// перезапускает выполнение команды
	void OnTimeCommandImpl::Restart(sys::error_code ec) {
		execution_ = true;
		Execute(ec);
	}


	// -------------------------- class TimeHandler --------------------------
	
	// добавляет команду в базу таймера, период будет переведен в миллисекунды
	TimeHandler& TimeHandler::AddCommand(std::string period, std::shared_ptr<ICommand>&& command) {
		return AddCommand(std::stoi(period), std::move(command));
	}

	// добавляет команду в базу таймера, период будет переведен в миллисекунды
	TimeHandler& TimeHandler::AddCommand(int period, std::shared_ptr<ICommand>&& command) {
		return AddCommand(std::chrono::milliseconds(period), std::move(command));
	}

	// добавляет команду в базу таймера
	TimeHandler& TimeHandler::AddCommand(std::chrono::milliseconds period, std::shared_ptr<ICommand>&& command) {

		if (!commands_.count(period)) {
			// если в заданный период не выполняется никаких команд создаём новый блок
			commands_.insert(std::pair{period, CommandLine{}});
		}

		// считтаем корректный период для выполнения команды
		auto correct_period = CalculateWaitTime(period);

		// добавляем команду в блок по указанному временному промежутку
		commands_.at(period).push_back(
			std::make_shared<OnTimeCommandImpl>( api_strand_, correct_period, std::move(command) ));
		return *this;
	}

	// запускает выполнение таймера
	TimeHandler& TimeHandler::Start() {
		if (!execution_) {
			execution_ = true;
			sys::error_code error_code;
			ExecuteCommandLine(error_code);
		}
		return *this;
	}

	// останавливает выполнение таймера
	TimeHandler& TimeHandler::Stop() {
		if (execution_) {
			execution_ = false;
		}
		return *this;
	}

	// назначает новый временной интервал таймера
	TimeHandler& TimeHandler::SetAlignPeriod(std::chrono::milliseconds period) {
		if (!execution_) {
			align_period_ = period;
			return *this;
		}

		throw std::runtime_error("GameTimer::SetAlignPeriod::Error::Timer execution flag is \"true\"");
	}

	// расчитывает время выполнения следующей команды, в зависимости от периода выравнивания
	std::chrono::milliseconds TimeHandler::CalculateWaitTime(std::chrono::milliseconds period) {
		
		if (period > align_period_) {
			// если заданый период больше периода выравнивания необходимо расчитать
			// время до ближайшего периода выравнивания, воспользуемся бесконечным циклом
			// тут нет смысла в очень мощных вычислениях, цикл пробежися, ну дай бог 20 раз, ну 30
			// умножая при этом счиай два целых числа, так что тут никаких нагрузок будет
			// скаляр сразу ринимаем с 2, так как уже понятно, что align_period_ * 1 меньше заданного
			std::chrono::milliseconds result;                  // заготавливаем результат
			for (int scaler = 2; ; ++scaler) {
				result = (scaler * align_period_);             // умножаем период выравнивания на скаляр
				if (result >= period) {
					// если полученное число превысило или равно исходному периоду, то возвращаем его
					return result;
				}
			}
		}
		
		else {
			// если период выполнения команды меньше или равен периоду выравнивания, то ничего не считаем
			return period;
		}
	}

	// запускает выполнение всего пула команд
	void TimeHandler::ExecuteCommandLine(sys::error_code ec) {
		
		// идем по всем временным периодам
		for (auto& [period, command_line] : commands_) {
			// идем по всем командам
			for (auto& command : command_line) {

				if (execution_) {
					// запускаем выполнение конкретной команды
					command->Execute(ec);
				}
				else {
					command->Abort();
				}
			}
		}
	}

} // namespace time_handler
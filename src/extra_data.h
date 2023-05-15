#pragma once

/*
* Шаблонный блок-обертка для хранения экстра-данных
*/

#include <unordered_map>

namespace extra_data {

	template <typename T>
	class ExtraTemplateData;                    // предварительное объявление

	template <typename T>
	class ExtraTemplateArrayData;               // предварительное объявление

	class ExtraDataCollector;                   // предварительное объявление

	enum class ExtraDataType {
		/*
		* Типы классов экстра данных, расширяется вместе с реализацией новых классов
		*/
		simple, template_data, template_array
	};

	/*
	* Базовый класс-пустышка предназначен для "обмана" системы.
	* Шаровый указатель на данный базовый класс содержится в базе коллектора
	* Однако, на самом деле можно вложить любой шаблонный и не шаблонный наследник
	*/
	class ExtraSimpleData {
		friend class ExtraDataCollector;
	protected:
		/*
		* ВНИМАНИЕ! Использует reinterpret_cast!
		* Обращение к данному методу, только когда точно известно, что внутри указателя.
		* Возвращает ре-интерпретированный указатель на ExtraTemplateArrayData<T>
		*/
		template <typename T>
		ExtraTemplateArrayData<T>* AsArray() {
			return reinterpret_cast<ExtraTemplateArrayData<T>*>(this);
		}
	};

	/*
	* Первый шаблонный закрытый класс в иерархии.
	* Хранит любое шаблонное значение всех последующих шаблонных наследников 
	* Не имеет конструктора по умолчанию! Может быть сконструирован только наследниками.
	*/
	template <typename T>
	class ExtraTemplateData : public ExtraSimpleData {
	public:
		ExtraTemplateData() = delete;

	protected:
		ExtraTemplateData(T&& data) 
			: data_(std::move(data)) {
		}

		const T& operator()() const {
			return data_;
		}

		T& operator()() {
			return data_;
		}

		const T& GetExtraData() const {
			return data_;
		}

		T& GetExtraData() {
			return data_;
		}

		T data_;
	};

	/*
	* Шаблонная обертка над каким-нибудь массивом
	* В данной реализации оборачивается вокруг boost::json::array
	* В модуле json_loader.h, для хранения данных о типах лута
	*/
	template <typename T>
	class ExtraTemplateArrayData : public ExtraTemplateData<T> {
	public:
		/*
		* Шаблонная обертка над каким-нибудь массивом
		* В данной реализации оборачивается вокруг boost::json::array
		* В модуле json_loader.h, для хранения данных о типах лута
		* Даёт базовый функционал Data(), Size(), Begin(), End(), At() 
		*/
		ExtraTemplateArrayData(T&& data) 
			: ExtraTemplateData<T>(std::move(data)) {
		}

		// даёт доступ по константной ссылке к данным внутри
		const T& Data() const {
			return this->GetExtraData();
		}
		// даёт доступ по неконстантной ссылке к данным внутри
		T& Data() {
			return this->GetExtraData();
		}
		// возвращает размер массива внутри
		size_t Size() const {
			return this->GetExtraData().size();
		}
		// возвращает итератор произвольного доступа
		auto Begin() {
			return this->GetExtraData().begin();
		}
		// возвращает итератор произвольного доступа
		auto End() {
			return this->GetExtraData().end();
		}

		// возвращает значение по индексу если индекс не превышает размер массива иначе кидает std::out_of_range
		template <typename V>
		V& At(size_t index) {
			if (index < Size()) {
				return this->GetExtraData().at(index);
			}
			throw std::out_of_range("model::extra_data::ExtraTemplateArrayData<T>::At::Out Of Range");
		}
	};

	/*
	* Указатель на действительные данные.
	* Изначально указывает на нешаблонный класс-родитель.
	* Однако будет многократно "обманут", так как сюда же можно
	* спокойно загонять и наследников через std::make_shared<>()
	*/
	using ExtraDataPtr = std::shared_ptr<ExtraSimpleData>;

	struct ExtraData {
		ExtraData() = default;
		ExtraData(ExtraDataType type, ExtraDataPtr&& ptr)
			: type_(type), ptr_(std::move(ptr)) {
		}

		ExtraDataType type_;               // тип хранимых данных
		ExtraDataPtr ptr_;                 // указатель на данные
	};

	using ExtraDataBase = std::unordered_map<std::string, ExtraData>;

	class ExtraDataCollector {
	public:
		ExtraDataCollector() = default;

		/*
		* Метод принимает любые данные, но нужно самостоятельно выбирать тип данных для указателя
		* И самостоятельно в коде сделать std::shared_ptr<ExtraSimpleData> или std::shared_ptr<ExtraTemplateArrayData<T>>
		* Выполняется проверка на предмет уже существующих данных с таким наименованием
		* В случае совпадения названий выбрасывается исключение
		*/
		inline void AddExtraData(std::string&& name, ExtraDataType type, ExtraDataPtr&& data) {
			if (base_.count(name)) {
				throw std::invalid_argument("ExtraDataCollector::AddExtraData::Error::Data with name {" + name + "} already exist");
			}
			base_.emplace(std::move(name), ExtraData{ type, std::move(data) });
		}

		/*
		* Метод принимает данные подходящие под определение коллекции, массивы, вектора и прочее
		* Флаг типа данных устанавливается автоматически, std::shared_ptr создается автоматически
		* Выполняется проверка на предмет уже существующих данных с таким наименованием
		* В случае совпадения названий выбрасывается исключение
		*/
		template <typename T>
		void AddExtraArrayData(std::string&& name, T&& data);

		/*
		* Возвращает ссылку на исходные данные записанные в коллектор
		* Автоматически приводит к нужному типу и возвращает то, что получил
		*/
		template <typename T>
		T& GetRawExtraDataAs(const std::string& name) const;
			
		inline ExtraSimpleData* GetExtraData(const std::string& name) const {
			if (base_.count(name)) {
				return base_.at(name).ptr_.get();
			}
			throw std::invalid_argument("ExtraDataCollector::GetData::Error::No data with name {" + name + "}");
		}

		template <typename T>
		ExtraTemplateArrayData<T>* GetExtraDataAsArray(const std::string& name) const;

		size_t GetExtraDataCount() const {
			return base_.size();
		}

	private:
		ExtraDataBase base_ = {};
	};

	/*
	* Метод принимает данные подходящие под определение коллекции, массивы, вектора и прочее
	* Флаг типа данных устанавливается автоматически, std::shared_ptr создается автоматически
	* Выполняется проверка на предмет уже существующих данных с таким наименованием
	* В случае совпадения названий выбрасывается исключение
	*/
	template <typename T>
	void ExtraDataCollector::AddExtraArrayData(std::string&& name, T&& data) {
		if (base_.count(name)) {
			throw std::invalid_argument("ExtraDataCollector::AddExtraArrayData::Error::Data with name {" + name + "} already exist");
		}

		AddExtraData(std::move(name), ExtraDataType::template_array, std::move(std::make_shared<ExtraTemplateArrayData<T>>(std::move(data))));
	}

	/*
	* Возвращает ссылку на исходные данные записанные в коллектор
	* Автоматически приводит к нужному типу и возвращает то, что получил
	*/
	template <typename T>
	T& ExtraDataCollector::GetRawExtraDataAs(const std::string& name) const {
		if (!base_.count(name)) {
			// если таких данных нет, то кидаем std::invalid_argument
			throw std::invalid_argument("ExtraDataCollector::GetExtraDataRaw::Error::No data with name {" + name + "}");
		}

		switch (base_.at(name).type_)
		{
		case ExtraDataType::template_array:
			return base_.at(name).ptr_->AsArray<T>()->Data();
		default:
			throw std::runtime_error("ExtraDataCollector::GetExtraDataRaw::Error::Unknow data-type on data with name {" + name + "}");
		}
	}


	template <typename T>
	ExtraTemplateArrayData<T>* ExtraDataCollector::GetExtraDataAsArray(const std::string& name) const {

		if (!base_.count(name)) {
			// если таких данных нет, то кидаем std::invalid_argument
			throw std::invalid_argument("ExtraDataCollector::GetExtraDataAsArray::Error::No data with name {" + name + "}");
		}

		if (base_.at(name).type_ != ExtraDataType::template_array) {
			// если данные при создании не были указаны как шаблонный массив, то кидаем std::runtime_error
			throw std::runtime_error("ExtraDataCollector::GetExtraDataAsArray::Error::Type data with name {" + name + "} is not ExtraTemplateArrayData<T>");
		}

		return base_.at(name).ptr_->AsArray<T>();
	}

} // namespace extra_data
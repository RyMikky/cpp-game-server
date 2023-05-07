#pragma once

/*
* ��������� ����-������� ��� �������� ������-������
*/

#include <unordered_map>

namespace model {

	namespace extra_data {

		template <typename T>
		class ExtraTemplateData;                    // ��������������� ����������

		template <typename T>
		class ExtraTemplateArrayData;               // ��������������� ����������

		class ExtraDataCollector;                   // ��������������� ����������

		enum class ExtraDataType {
			/*
			* ���� ������� ������ ������, ����������� ������ � ����������� ����� �������
			*/
			simple, template_data, template_array
		};

		/*
		* ������� �����-�������� ������������ ��� "������" �������.
		* ������� ��������� �� ������ ������� ����� ���������� � ���� ����������
		* ������, �� ����� ���� ����� ������� ����� ��������� � �� ��������� ���������
		*/
		class ExtraSimpleData {
			friend class ExtraDataCollector;
		protected:
			/*
			* ��������! ���������� reinterpret_cast!
			* ��������� � ������� ������, ������ ����� ����� ��������, ��� ������ ���������.
			* ���������� ��-������������������ ��������� �� ExtraTemplateArrayData<T>
			*/
			template <typename T>
			ExtraTemplateArrayData<T>* AsArray() {
				return reinterpret_cast<ExtraTemplateArrayData<T>*>(this);
			}
		};

		/*
		* ������ ��������� �������� ����� � ��������.
		* ������ ����� ��������� �������� ���� ����������� ��������� ����������� 
		* �� ����� ������������ �� ���������! ����� ���� �������������� ������ ������������.
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
		* ��������� ������� ��� �����-������ ��������
		* � ������ ���������� ������������� ������ boost::json::array
		* � ������ json_loader.h, ��� �������� ������ � ����� ����
		*/
		template <typename T>
		class ExtraTemplateArrayData : public ExtraTemplateData<T> {
		public:
			/*
			* ��������� ������� ��� �����-������ ��������
			* � ������ ���������� ������������� ������ boost::json::array
			* � ������ json_loader.h, ��� �������� ������ � ����� ����
			* ��� ������� ���������� Data(), Size(), Begin(), End(), At() 
			*/
			ExtraTemplateArrayData(T&& data) 
				: ExtraTemplateData<T>(std::move(data)) {
			}

			// ��� ������ �� ����������� ������ � ������ ������
			const T& Data() const {
				return this->GetExtraData();
			}
			// ��� ������ �� ������������� ������ � ������ ������
			T& Data() {
				return this->GetExtraData();
			}
			// ���������� ������ ������� ������
			size_t Size() const {
				return this->GetExtraData().size();
			}
			// ���������� �������� ������������� �������
			auto Begin() {
				return this->GetExtraData().begin();
			}
			// ���������� �������� ������������� �������
			auto End() {
				return this->GetExtraData().end();
			}

			// ���������� �������� �� ������� ���� ������ �� ��������� ������ ������� ����� ������ std::out_of_range
			template <typename V>
			V At(size_t index) {
				if (index < Size()) {
					return this->GetExtraData().at(index);
				}
				throw std::out_of_range("model::extra_data::ExtraTemplateArrayData<T>::At::Out Of Range");
			}
		};

		/*
		* ��������� �� �������������� ������.
		* ���������� ��������� �� ����������� �����-��������.
		* ������ ����� ����������� "�������", ��� ��� ���� �� �����
		* �������� �������� � ����������� ����� std::make_shared<>()
		*/
		using ExtraDataPtr = std::shared_ptr<ExtraSimpleData>;

		struct ExtraData {
			ExtraData() = default;
			ExtraData(ExtraDataType type, ExtraDataPtr&& ptr)
				: type_(type), ptr_(std::move(ptr)) {
			}

			ExtraDataType type_;               // ��� �������� ������
			ExtraDataPtr ptr_;                 // ��������� �� ������
		};

		using ExtraDataBase = std::unordered_map<std::string, ExtraData>;

		class ExtraDataCollector {
		public:
			ExtraDataCollector() = default;

			void AddExtraData(std::string&& name, ExtraDataType type, ExtraDataPtr&& data) {
				base_.emplace(std::move(name), ExtraData{ type, std::move(data) });
			}
			
			inline ExtraSimpleData* GetExtraData(const std::string& name) const {
				if (base_.count(name)) {
					return base_.at(name).ptr_.get();
				}
				throw std::invalid_argument("ExtraDataCollector::GetData::Error::No data with name {" + name + "}");
			}

			template <typename T>
			ExtraTemplateArrayData<T>* GetExtraDataAsArray(const std::string& name) const;

		private:
			ExtraDataBase base_ = {};
		};

		template <typename T>
		ExtraTemplateArrayData<T>* ExtraDataCollector::GetExtraDataAsArray(const std::string& name) const {

			if (!base_.count(name)) {
				// ���� ����� ������ ���, �� ������ std::invalid_argument
				throw std::invalid_argument("ExtraDataCollector::GetDataAsArray::Error::No data with name {" + name + "}");
			}

			if (base_.at(name).type_ != ExtraDataType::template_array) {
				// ���� ������ ��� �������� �� ���� ������� ��� ��������� ������, �� ������ std::runtime_error
				throw std::runtime_error("ExtraDataCollector::GetDataAsArray::Error::Type data with name {" + name + "} is not ExtraTemplateArrayData<T>");
			}

			return base_.at(name).ptr_->AsArray<T>();
		}

	} // namespace extra_data

} // namespace model
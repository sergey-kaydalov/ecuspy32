/*
 * config.hpp
 *
 *  Created on: Jan 21, 2018
 *      Author: ksu
 */

#ifndef MAIN_CONFIG_HPP_
#define MAIN_CONFIG_HPP_

#include <array>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <thread>
#include <mutex>

#include "templates.hpp"

namespace ecuspy {

enum ConfigCat_t{
	cfgCatWIFI,
	cfgCatELM327
};

enum ConfigValueType_t{
	cfgTypeBOOL,
	cfgTypeInt8,
	cfgTypeUint8,
	cfgTypeInt16,
	cfgTypeUint16,
	cfgTypeInt32,
	cfgTypeUint32,
	cfgTypeInt64,
	cfgTypeUint64,
	cfgTypeString,
	cfgTypeDouble,
	cfgTypeTotal,
	cfgTypeCustom
};


struct ConfigEntry {

	constexpr ConfigEntry(const char* _id, const char* _name, const char* _desc,
		ConfigCat_t _cat, ConfigValueType_t _type, size_t _len)
	: id(_id),name(_name),desc(_desc), cat(_cat), type(_type), value_len(_len), min(0), max(0)
	{}

	constexpr ConfigEntry(const char* _id, const char* _name, const char* _desc,
		ConfigCat_t _cat, ConfigValueType_t _type, size_t _len, int _min, int _max)
	: id(_id),name(_name),desc(_desc), cat(_cat), type(_type), value_len(_len), min(_min), max(_max)
	{}


	const char* id;
	const char* name;
	const char* desc;
	ConfigCat_t cat;
	ConfigValueType_t type;
	size_t value_len;
	int min;
	int max;
};

using validator=std::function<bool(const ConfigEntry&, const char*)>;

template <typename T>
struct NumEntry : public ConfigEntry {

	constexpr NumEntry(const char* _id, const char* _name, const char* _desc,
		ConfigCat_t _cat, ConfigValueType_t _type, size_t _len, T _min, T _max)
	: ConfigEntry(_id, _name, _desc, _cat, _type, _len), min(_min), max(_max) {}
	T min;
	T max;
};

struct CustomValidatorEntry : public ConfigEntry {

	constexpr CustomValidatorEntry(const char* _id, const char* _name, const char* _desc,
		ConfigCat_t _cat, size_t _len, const validator& _v)
	: ConfigEntry(_id, _name, _desc, _cat, cfgTypeCustom, _len), v(_v) {}

	const validator& v;
};

template<typename T>
class IntValidator {
	public:
		bool operator() (const ConfigEntry& cfg, const char* str) const {
			const NumEntry<T>& l = static_cast<const NumEntry<T>&>(cfg);
			T  val = strtoll(str, NULL, 10);
			return val >= l.min && val <= l.max;
		}
};

class RealValidator{
	public:
		bool operator() (const ConfigEntry& cfg, const char* str) const {
			const NumEntry<double>& l = static_cast<const NumEntry<double>&>(cfg);
			double  val = strtof(str, NULL);
			return val >= l.min && val <= l.max;
		}

	private:
		double m_min,m_max;
};


const validator cfg_validators[cfgTypeTotal] = {
		[] (const ConfigEntry& cfg, const char* str)
			{return !strcmp(str, "true") || !strcmp(str, "false");},
		IntValidator<uint8_t>{},
		IntValidator<int8_t>{},
		IntValidator<uint16_t>{},
		IntValidator<int16_t>{},
		IntValidator<uint32_t>{},
		IntValidator<int32_t>{},
		IntValidator<uint64_t>{},
		IntValidator<int64_t>{},
		[] (const ConfigEntry& cfg, const char* str) {return true;},
		RealValidator{}
};

constexpr size_t BADINDEX=0xFFFFFFFF;

class Config : public tpl::Singleton<Config>  {
public:
	void initialize(const ConfigEntry* _cfg, size_t len) {
		if (!m_len) {
			m_len = len;
			m_cfg = _cfg;
			m_offsets = new size_t[len];
			m_offsets[0] = 0;
			for (int i = 1; i < len; i++) {
				m_offsets[i] = m_cfg[i-1].value_len + 1 + m_offsets[i-1];
			}
			m_values_size = m_offsets[len-1] + m_cfg[len-1].value_len + 1;
			m_values = new char[m_values_size];
			memset(m_values, 0, m_values_size);
			m_tr_values = new char[m_values_size];
			memset(m_tr_values, 0, m_values_size);
		}
	}

	const char* getValueStr(size_t index) const {
		return m_values + m_offsets[index];
	}

	bool validate(size_t index, const char* str) {
		ConfigValueType_t type = m_cfg[index].type;
		if (type == cfgTypeCustom) {
				const CustomValidatorEntry& e = static_cast<const CustomValidatorEntry&>(m_cfg[index]);
				return e.v(e, str);
		} else
			return cfg_validators[type](m_cfg[index], str);
	}

	void setValueStr(size_t index, const char* value) {
		std::lock_guard<std::mutex> guard{m_lock};
		if (m_tr_counter)
			strncpy(m_tr_values + m_offsets[index], value, m_cfg[index].value_len);
		else
			strncpy(m_values + m_offsets[index], value, m_cfg[index].value_len);
	}

	void startTransaction() {
		std::lock_guard<std::mutex> guard{m_lock};
		if (!m_tr_counter)
			memcpy(m_tr_values, m_values, m_values_size);
		m_tr_counter++;
	}

	void stopTransaction() {
		std::lock_guard<std::mutex> guard{m_lock};
		if (m_tr_counter == 1) {
			memcpy( m_values, m_tr_values, m_values_size);
			m_tr_counter = 0;
		} else
			m_tr_counter--;
	}

	void abortTransaction() {
		std::lock_guard<std::mutex> guard{m_lock};
		m_tr_counter = 0;
	}

	size_t indexByID(const char* id) {
		/**
		 * TODO optimization: make it O(lg N) or O(1)
		 */
		size_t index = BADINDEX;
		for (size_t i = 0; i < m_len; i++) {
			if (!strcmp(m_cfg[i].id, id)) {
				index = i;
				break;
			}
		}
		return index;
	}

	~Config() {
		if (m_len) {
			delete [] m_offsets;
			delete [] m_values;
			delete [] m_tr_values;
		}
	}

	private:
		friend class Singleton<Config>;

		Config()
		: m_len(0),
		  m_values_size(0),
		  m_cfg(nullptr),
		  m_offsets(nullptr),
		  m_values(nullptr),
		  m_tr_values(nullptr),
		  m_tr_counter(0) {}

		Config(Config&) = delete;
		Config(Config&&) = delete;

	private:
		size_t m_len = 0;
		size_t m_values_size;
		const ConfigEntry* m_cfg;
		size_t* m_offsets;
		char* m_values = NULL;
		char* m_tr_values = NULL;
		int m_tr_counter = 0;
		std::mutex m_lock;

};

class Transaction {
	public:
		Transaction() {
			Config::instance().startTransaction();
		}
		~Transaction() {
			if (std::uncaught_exception())
				Config::instance().abortTransaction();
			else
				Config::instance().stopTransaction();
		}
};

namespace impl {
inline const char* get(size_t index) {
	return Config::instance().getValueStr(index);
}
}

template<typename T>
T getConfig(size_t index);

template<>
const char* getConfig(size_t index) {
	return impl::get(index);
}

template<>
int getConfig(size_t index) {
	return strtol(impl::get(index), NULL, 10);
}

template<>
double getConfig(size_t index) {
	return strtof(impl::get(index), NULL);
}

template<>
bool getConfig(size_t index) {
	return strcmp(impl::get(index), "true") == 0 ||
			strcmp(impl::get(index), "True") ||
			strcmp(impl::get(index), "TRUE");
}

template<typename T>
T getConfig(const char* id) throw(std::exception) {
	size_t n = Config::instance().indexByID(id);
	if (n == BADINDEX)
		throw (std::exception());
	return getConfig<T>(n);
}

}



#endif /* MAIN_CONFIG_HPP_ */

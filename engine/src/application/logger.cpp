#include "logger.h"

#include <ctime>

LUMINA_SINGLETON_DECL_INSTANCE(lumina::lumina_logger);

namespace lumina
{
	template<>
	lumina_logger::log_t& lumina_logger::log<std::string>(std::string log_data, log_types_e log_level)
	{
		// Lock the function for vector thread safety
		log_safety_mutex_.lock();
		
		// Push the new log
		std::time_t time = std::time(0);
		std::tm* local_time_now = std::localtime(&time);
		logs_.push_back(
			{
				"[" + std::to_string(local_time_now->tm_hour) + ":" +
				std::to_string(local_time_now->tm_min) + ":" +
				std::to_string(local_time_now->tm_sec) + "]",
				log_data,
				log_level
			}
		);

		// Get's a ref to the return value before unlocking, thats made for safety purpose
		log_t& ret_val_ref = logs_[logs_.size() - 1];

		// Unlock the function
		log_safety_mutex_.unlock();

		return ret_val_ref;
	}

	template<>
	lumina_logger::log_t& lumina_logger::log<l_cstr8>(l_cstr8 log_data, log_types_e log_level)
	{
		return log<std::string>(log_data, log_level);
	}
}
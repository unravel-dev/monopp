#include "mono_logger.h"
#include <map>
namespace mono
{
namespace detail
{
auto get_log_handler() -> std::map<std::string, log_handler>&
{
    static std::map<std::string, log_handler> log_callbacks;
	return log_callbacks;
}
}

auto get_log_handler(const std::string& category) -> const log_handler&
{
	return detail::get_log_handler()[category];
}

void set_log_handler(const std::string& category, const log_handler& handler)
{
	detail::get_log_handler()[category] = handler;
}

void log_message(const std::string& message, const std::string& category)
{
	const auto& logger = get_log_handler(category);
	if(logger)
	{
		logger(message);
	}
	else
	{
		const auto& default_logger = get_log_handler("default");
		if(default_logger)
		{
			default_logger(message);
		}
	}
}

} // namespace mono

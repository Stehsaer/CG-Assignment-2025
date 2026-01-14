///
/// @file error.hpp
/// @brief Provides an Error class containing an error trace
///

#pragma once

#include <expected>
#include <functional>
#include <iostream>
#include <source_location>
#include <string>
#include <vector>

namespace util
{
	///
	/// @brief Error class containing an error trace
	///
	///
	class Error
	{
	  public:

		///
		/// @brief Error trace entry
		///
		///
		struct TraceEntry
		{
			std::source_location location;  // Source location of the error
			std::string message;            // Information about the error
		};

	  private:

		std::vector<TraceEntry> entries;

	  public:

		Error(const Error&) = default;
		Error(Error&&) = default;
		Error& operator=(const Error&) = default;
		Error& operator=(Error&&) = default;

		///
		/// @brief Constructs and error
		///
		/// @param message Error information
		/// @param location Location where the error occurred, default to current location
		///
		Error(
			std::string message,
			const std::source_location& location = std::source_location::current()
		) noexcept;

		///
		/// @brief forward the error by adding a new trace entry
		///
		/// @param message Additional error information
		/// @param location Location where the error is propagated, default to current location
		/// @return
		///
		Error forward(
			std::string message = "",
			const std::source_location& location = std::source_location::current()
		) const noexcept;

		static std::function<util::Error(util::Error&&)> forward_fn(
			std::string message = "",
			const std::source_location& location = std::source_location::current()
		) noexcept;

		template <typename V>
		operator std::expected<V, Error>() const noexcept
		{
			return std::unexpected<Error>(*this);
		}

		///
		/// @brief Dumps the error trace to an output stream
		///
		/// @param os Output stream, default to std::cerr
		/// @param color Whether to use colored output, default to true
		///
		void dump_trace(std::ostream& os = std::cerr, bool color = true) const noexcept;

		const std::vector<TraceEntry>& operator*() const noexcept { return entries; }
		const std::vector<TraceEntry>* operator->() const noexcept { return &(**this); }
	};
}
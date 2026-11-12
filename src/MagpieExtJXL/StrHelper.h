#pragma once
#include <string>
#include <charconv>

namespace MagpieExtJXL {

struct StrHelper {
	static std::wstring UTF8ToUTF16(std::string_view str) noexcept;

	static std::string UTF16ToUTF8(std::wstring_view str) noexcept;

	static std::string UTF16ToANSI(std::wstring_view str) noexcept;

	template <typename CHAR_T>
	static constexpr size_t StrLen(const CHAR_T* str) noexcept {
		// std::char_traits 相比 std::strlen 支持更多字符类型，也可以在编译时计算
		// 字符串常量的长度。
		return std::char_traits<CHAR_T>::length(str);
	}

	template <typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
	static std::string ToString(T value) noexcept {
		// 可以容纳所有算数类型，来自
		// https://github.com/microsoft/STL/blob/edd1486e5fe753616b6fd3695da96f352b4092d2/stl/inc/format#L1719-L1729
		// 不要直接构造 std::string，这样无法利用 SSO。
		std::array<char, 24> buffer;

		std::to_chars_result result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
		assert(result.ec == std::errc{});
		return std::string(buffer.data(), result.ptr);
	}

	template <typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
	static std::wstring ToWString(T value) noexcept {
		if constexpr (std::is_integral_v<T>) {
			// 对于整数 STL 的实现很高效
			return std::to_wstring(value);
		} else {
			std::string temp = ToString(value);
			std::wstring result(temp.size(), 0);
			// 全是 ASCII 字符，可以直接复制
			for (size_t i = 0; i < temp.size(); ++i) {
				result[i] = temp[i];
			}
			return result;
		}
	}

	template <typename T1, typename T2, typename... AV,
		typename CHAR_T = std::conditional_t<std::is_constructible_v<std::basic_string_view<char>, T1>, char, wchar_t>>
		static std::basic_string<CHAR_T> Concat(T1&& s1, T2&& s2, AV&&... args) noexcept {
		return _Concat<CHAR_T>(std::forward<T1>(s1), std::forward<T2>(s2), std::forward<AV>(args)...);
	}

private:
	template <typename CHAR_T>
	static std::basic_string<CHAR_T> _Concat(
		const std::basic_string_view<CHAR_T>& s1,
		const std::basic_string_view<CHAR_T>& s2
	) noexcept {
		std::basic_string<CHAR_T> result;
		result.reserve(s1.size() + s2.size());
		result.append(s1).append(s2);
		return result;
	}

	template <typename CHAR_T>
	static std::basic_string<CHAR_T> _Concat(
		const std::basic_string_view<CHAR_T>& s1,
		const std::basic_string_view<CHAR_T>& s2,
		const std::basic_string_view<CHAR_T>& s3
	) noexcept {
		std::basic_string<CHAR_T> result;
		result.reserve(s1.size() + s2.size() + s3.size());
		result.append(s1).append(s2).append(s3);
		return result;
	}

	template <typename CHAR_T>
	static std::basic_string<CHAR_T> _Concat(
		const std::basic_string_view<CHAR_T>& s1,
		const std::basic_string_view<CHAR_T>& s2,
		const std::basic_string_view<CHAR_T>& s3,
		const std::basic_string_view<CHAR_T>& s4
	) noexcept {
		std::basic_string<CHAR_T> result;
		result.reserve(s1.size() + s2.size() + s3.size() + s4.size());
		result.append(s1).append(s2).append(s3).append(s4);
		return result;
	}

	template <typename CHAR_T>
	static std::basic_string<CHAR_T> _Concat(
		const std::basic_string_view<CHAR_T>& s1,
		const std::basic_string_view<CHAR_T>& s2,
		const std::basic_string_view<CHAR_T>& s3,
		const std::basic_string_view<CHAR_T>& s4,
		const std::basic_string_view<CHAR_T>& s5
	) noexcept {
		std::basic_string<CHAR_T> result;
		result.reserve(s1.size() + s2.size() + s3.size() + s4.size() + s5.size());
		result.append(s1).append(s2).append(s3).append(s4).append(s5);
		return result;
	}

	template <typename CHAR_T, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename... AV>
	static std::basic_string<CHAR_T> _Concat(
		T1&& s1,
		T2&& s2,
		T3&& s3,
		T4&& s4,
		T5&& s5,
		T6&& s6,
		AV&&... args
	) noexcept {
		return _ConcatHelper<CHAR_T>({
			std::forward<T1>(s1),
			std::forward<T2>(s2),
			std::forward<T3>(s3),
			std::forward<T4>(s4),
			std::forward<T5>(s5),
			std::forward<T6>(s6),
			std::forward<AV>(args)...
		});
	}

	template <typename CHAR_T>
	static std::basic_string<CHAR_T> _ConcatHelper(std::initializer_list<std::basic_string_view<CHAR_T>> args) noexcept {
		std::basic_string<CHAR_T> result;

		size_t size = 0;
		for (const std::basic_string_view<CHAR_T>& s : args) {
			size += s.size();
		}

		result.reserve(size);

		for (const std::basic_string_view<CHAR_T>& s : args) {
			result.append(s);
		}

		return result;
	}
};

}

/*
This file is part of Telegram Desktop,
the official desktop version of Telegram messaging app, see https://telegram.org

Telegram Desktop is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

It is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

In addition, as a special exception, the copyright holders give permission
to link the code of portions of this program with the OpenSSL library.

Full license: https://github.com/telegramdesktop/tdesktop/blob/master/LICENSE
Copyright (c) 2014-2017 John Preston, https://desktop.telegram.org
*/
#pragma once

namespace rpl {
namespace details {

struct base_mapper {
};

template <typename Type>
constexpr bool is_mapper_v = std::is_base_of_v<
	base_mapper,
	std::decay_t<Type>>;

template <std::size_t Index>
struct argument_mapper : base_mapper {
	template <
		typename Arg,
		typename ...Args,
		typename = std::enable_if_t<(sizeof...(Args) >= Index)>>
	static constexpr decltype(auto) call(Arg &&arg, Args &&...args) {
		return argument_mapper<Index - 1>::call(
			std::forward<Args>(args)...);
	}

	template <
		typename ...Args,
		typename = std::enable_if_t<(sizeof...(Args) > Index)>>
	constexpr auto operator()(Args &&...args) const {
		return call(std::forward<Args>(args)...);
	}

};

template <>
struct argument_mapper<0> : base_mapper {
	template <
		typename Arg,
		typename ...Args>
	static constexpr decltype(auto) call(Arg &&arg, Args &&...args) {
		return std::forward<Arg>(arg);
	}

	template <
		typename Arg,
		typename ...Args>
	constexpr auto operator()(Arg &&arg, Args &&...args) const {
		return std::forward<Arg>(arg);
	}

};

template <typename Type>
class value_mapper : public base_mapper {
public:
	template <typename OtherType>
	constexpr value_mapper(OtherType &&value)
		: _value(std::forward<OtherType>(value)) {
	}

	template <typename ...Args>
	constexpr auto operator()(Args &&...args) const {
		return _value;
	}

private:
	Type _value;

};

template <typename Type>
inline value_mapper<std::decay_t<Type>> make_value_mapper(Type &&value) {
	return { std::forward<Type>(value) };
}

template <typename Type>
struct wrap_mapper {
	using type = std::conditional_t<
		is_mapper_v<Type>,
		Type,
		value_mapper<Type>>;
};

template <typename Type>
using wrap_mapper_t = typename wrap_mapper<Type>::type;

template <typename Type, typename Operator>
class unary_operator_mapper : public base_mapper {
	using TypeWrapper = wrap_mapper_t<std::decay_t<Type>>;

public:
	template <typename OtherType>
	constexpr unary_operator_mapper(OtherType &&value)
		: _value(std::forward<OtherType>(value)) {
	}

	template <
		typename ...Args,
		typename Result = decltype((Operator{})(
			std::declval<TypeWrapper>()(std::declval<Args>()...)))>
	constexpr std::decay_t<Result> operator()(Args &&...args) const {
		return (Operator{})(
			_value(std::forward<Args>(args)...));
	}

private:
	TypeWrapper _value;

};

template <typename Left, typename Right, typename Operator>
class binary_operator_mapper : public base_mapper {
	using LeftWrapper = wrap_mapper_t<std::decay_t<Left>>;
	using RightWrapper = wrap_mapper_t<std::decay_t<Right>>;

public:
	template <typename OtherLeft, typename OtherRight>
	constexpr binary_operator_mapper(OtherLeft &&left, OtherRight &&right)
		: _left(std::forward<OtherLeft>(left))
		, _right(std::forward<OtherRight>(right)) {
	}

	template <
		typename ...Args,
		typename Result = decltype((Operator{})(
			std::declval<LeftWrapper>()(std::declval<Args>()...),
			std::declval<RightWrapper>()(std::declval<Args>()...)))>
	constexpr std::decay_t<Result> operator()(Args &&...args) const {
		return (Operator{})(
			_left(std::forward<Args>(args)...),
			_right(std::forward<Args>(args)...));
	}

private:
	LeftWrapper _left;
	RightWrapper _right;

};

template <
	typename Left,
	typename Right,
	typename = std::enable_if_t<
		is_mapper_v<Left> || is_mapper_v<Right>
	>>
inline auto operator+(Left &&left, Right &&right) {
	return binary_operator_mapper<
		Left,
		Right,
		std::plus<>>(
			std::forward<Left>(left),
			std::forward<Right>(right));
}

template <
	typename Left,
	typename Right,
	typename = std::enable_if_t<
		is_mapper_v<Left> || is_mapper_v<Right>
	>>
inline auto operator-(Left &&left, Right &&right) {
	return binary_operator_mapper<
		Left,
		Right,
		std::minus<>>(
			std::forward<Left>(left),
			std::forward<Right>(right));
}

template <
	typename Left,
	typename Right,
	typename = std::enable_if_t<
		is_mapper_v<Left> || is_mapper_v<Right>
	>>
inline auto operator*(Left &&left, Right &&right) {
	return binary_operator_mapper<
		Left,
		Right,
		std::multiplies<>>(
			std::forward<Left>(left),
			std::forward<Right>(right));
}

template <
	typename Left,
	typename Right,
	typename = std::enable_if_t<
		is_mapper_v<Left> || is_mapper_v<Right>
	>>
inline auto operator/(Left &&left, Right &&right) {
	return binary_operator_mapper<
		Left,
		Right,
		std::divides<>>(
			std::forward<Left>(left),
			std::forward<Right>(right));
}

template <
	typename Left,
	typename Right,
	typename = std::enable_if_t<
		is_mapper_v<Left> || is_mapper_v<Right>
	>>
inline auto operator%(Left &&left, Right &&right) {
	return binary_operator_mapper<
		Left,
		Right,
		std::modulus<>>(
			std::forward<Left>(left),
			std::forward<Right>(right));
}

template <
	typename Type,
	typename = std::enable_if_t<
		is_mapper_v<Type>
	>>
inline auto operator-(Type &&value) {
	return unary_operator_mapper<
		Type,
		std::negate<>>(
			std::forward<Type>(value));
}

template <
	typename Left,
	typename Right,
	typename = std::enable_if_t<
		is_mapper_v<Left> || is_mapper_v<Right>
	>>
inline auto operator<(Left &&left, Right &&right) {
	return binary_operator_mapper<
		Left,
		Right,
		std::less<>>(
			std::forward<Left>(left),
			std::forward<Right>(right));
}

template <
	typename Left,
	typename Right,
	typename = std::enable_if_t<
		is_mapper_v<Left> || is_mapper_v<Right>
	>>
inline auto operator<=(Left &&left, Right &&right) {
	return binary_operator_mapper<
		Left,
		Right,
		std::less_equal<>>(
			std::forward<Left>(left),
			std::forward<Right>(right));
}

template <
	typename Left,
	typename Right,
	typename = std::enable_if_t<
		is_mapper_v<Left> || is_mapper_v<Right>
	>>
inline auto operator>(Left &&left, Right &&right) {
	return binary_operator_mapper<
		Left,
		Right,
		std::greater<>>(
			std::forward<Left>(left),
			std::forward<Right>(right));
}

template <
	typename Left,
	typename Right,
	typename = std::enable_if_t<
		is_mapper_v<Left> || is_mapper_v<Right>
	>>
inline auto operator>=(Left &&left, Right &&right) {
	return binary_operator_mapper<
		Left,
		Right,
		std::greater_equal<>>(
			std::forward<Left>(left),
			std::forward<Right>(right));
}

template <
	typename Left,
	typename Right,
	typename = std::enable_if_t<
		is_mapper_v<Left> || is_mapper_v<Right>
	>>
inline auto operator==(Left &&left, Right &&right) {
	return binary_operator_mapper<
		Left,
		Right,
		std::equal_to<>>(
			std::forward<Left>(left),
			std::forward<Right>(right));
}

template <
	typename Left,
	typename Right,
	typename = std::enable_if_t<
		is_mapper_v<Left> || is_mapper_v<Right>
	>>
inline auto operator!=(Left &&left, Right &&right) {
	return binary_operator_mapper<
		Left,
		Right,
		std::not_equal_to<>>(
			std::forward<Left>(left),
			std::forward<Right>(right));
}

template <
	typename Left,
	typename Right,
	typename = std::enable_if_t<
		is_mapper_v<Left> || is_mapper_v<Right>
	>>
inline auto operator&&(Left &&left, Right &&right) {
	return binary_operator_mapper<
		Left,
		Right,
		std::logical_and<>>(
			std::forward<Left>(left),
			std::forward<Right>(right));
}

template <
	typename Left,
	typename Right,
	typename = std::enable_if_t<
		is_mapper_v<Left> || is_mapper_v<Right>
	>>
inline auto operator||(Left &&left, Right &&right) {
	return binary_operator_mapper<
		Left,
		Right,
		std::logical_or<>>(
			std::forward<Left>(left),
			std::forward<Right>(right));
}

template <
	typename Type,
	typename = std::enable_if_t<
		is_mapper_v<Type>
	>>
inline auto operator!(Type &&value) {
	return unary_operator_mapper<
		Type,
		std::logical_not<>>(
			std::forward<Type>(value));
}

template <
	typename Left,
	typename Right,
	typename = std::enable_if_t<
		is_mapper_v<Left> || is_mapper_v<Right>
	>>
inline auto operator&(Left &&left, Right &&right) {
	return binary_operator_mapper<
		Left,
		Right,
		std::bit_and<>>(
			std::forward<Left>(left),
			std::forward<Right>(right));
}

template <
	typename Left,
	typename Right,
	typename = std::enable_if_t<
		is_mapper_v<Left> || is_mapper_v<Right>
	>>
inline auto operator|(Left &&left, Right &&right) {
	return binary_operator_mapper<
		Left,
		Right,
		std::bit_or<>>(
			std::forward<Left>(left),
			std::forward<Right>(right));
}

template <
	typename Left,
	typename Right,
	typename = std::enable_if_t<
		is_mapper_v<Left> || is_mapper_v<Right>
	>>
inline auto operator^(Left &&left, Right &&right) {
	return binary_operator_mapper<
		Left,
		Right,
		std::bit_xor<>>(
			std::forward<Left>(left),
			std::forward<Right>(right));
}

template <
	typename Type,
	typename = std::enable_if_t<
		is_mapper_v<Type>
	>>
inline auto operator~(Type &&value) {
	return unary_operator_mapper<
		Type,
		std::bit_not<>>(
			std::forward<Type>(value));
}

} // namespace details

namespace mappers {

constexpr const details::argument_mapper<0> $1;
constexpr const details::argument_mapper<1> $2;
constexpr const details::argument_mapper<2> $3;
constexpr const details::argument_mapper<3> $4;
constexpr const details::argument_mapper<4> $5;
constexpr const details::argument_mapper<5> $6;
constexpr const details::argument_mapper<6> $7;
constexpr const details::argument_mapper<7> $8;
constexpr const details::argument_mapper<8> $9;
constexpr const details::argument_mapper<9> $10;
constexpr const details::argument_mapper<10> $11;
constexpr const details::argument_mapper<11> $12;
constexpr const details::argument_mapper<12> $13;
constexpr const details::argument_mapper<13> $14;
constexpr const details::argument_mapper<14> $15;
constexpr const details::argument_mapper<15> $16;
constexpr const details::argument_mapper<16> $17;
constexpr const details::argument_mapper<17> $18;
constexpr const details::argument_mapper<18> $19;
constexpr const details::argument_mapper<19> $20;

template <typename Type>
inline auto $val(Type &&value) {
	return details::make_value_mapper(std::forward<Type>(value));
}

} // namespace mappers
} // namespace rpl
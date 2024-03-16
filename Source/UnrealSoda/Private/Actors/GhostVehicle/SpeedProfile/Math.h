// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include <cmath>
#include <vector>
#include <type_traits>
#include <Eigen/Core>

namespace SpeedProfile
{

namespace math 
{

constexpr double grav_acc = 9.80665; //**m/s^2 */

template<typename T>
constexpr T pi() 
{
	return (T)3.141592653589793;
}

template<typename T>
constexpr decltype(T() * T()) square(const T t) 
{
	static_assert(std::is_arithmetic<T>::value, "Type should be arithmetic");
	return t * t;
}

template<typename T>
constexpr int approx_cmp(const T a, const T b, const T prec = 5 * std::numeric_limits<T>::epsilon()) 
{
	static_assert(std::is_floating_point<T>::value, "Type should be floating point.");
	if (std::abs(a - b) < prec)
		return 0;
	else
		return a < b ? -1 : 1;
}

template<typename T, std::enable_if_t<std::is_arithmetic<T>::value, bool> = true>
constexpr int sign(T number, T epsilon = 0) 
{
	return number > epsilon ? 1 : number < -epsilon ? -1 : 0;
}

} // namespace math

} // namespace SpeedProfile


#pragma once

#include <array>
#include <climits>

namespace soda 
{
namespace sim 
{
namespace proto_v1 
{

template<typename T, typename = std::enable_if_t<std::is_unsigned<T>::value>>
class CrcTable 
{
public:
    explicit CrcTable(T const polynomial) noexcept 
    {
        calculate(polynomial);
    }

    auto begin() const noexcept { return table.begin(); }
    auto end() const noexcept { return table.end(); }
    auto cbegin() const noexcept { return begin(); }
    auto cend() const noexcept { return end(); }
    auto operator[](std::size_t index) const noexcept { return table[index]; }
    auto size() const noexcept { return table.size(); }

private:
    void calculate(T const polynomial) noexcept 
    {
        static auto constexpr bit_width = CHAR_BIT * sizeof(T);
        static T constexpr highest_bit = 1u << (bit_width - 1);

        for (std::size_t index = 0; index < table.size(); ++index) 
        {
            // NOTE: Substitute index in place of the highest byte of remainder's initial value.
            auto remainder = static_cast<T>(index << (bit_width - CHAR_BIT));

            for (auto bit = 0u; bit < CHAR_BIT; ++bit) 
            {
                bool const needs_xor_with_poly = remainder & highest_bit;
                remainder <<= 1;
                if (needs_xor_with_poly) 
                {
                    remainder ^= polynomial;
                }
            }
            table[index] = remainder;
        }
    }

    static std::size_t constexpr TableSize = 1u << CHAR_BIT;
    std::array<T, TableSize> table;
};

template<typename T, typename = std::enable_if_t<std::is_unsigned<T>::value>>
class Crc 
{
public:
    using result_type = T;

    explicit Crc(T const polynomial)
        : table(polynomial)
    {}

    template<typename ForwardIterator>
    T operator()(ForwardIterator begin, ForwardIterator const end, T remainder = T{0}) const noexcept 
    {
        static_assert(std::is_same<std::decay_t<decltype(*begin)>, std::uint8_t>::value, "Dereferred iterator should result in std::uint8_t");
        while (begin != end) 
        {
            static auto constexpr bit_width = CHAR_BIT * sizeof(T);
            auto const table_index = *begin ^ (remainder >> (bit_width - CHAR_BIT));
            remainder = table[table_index] ^ (remainder << CHAR_BIT);
            ++begin;
        }

        return remainder;
    }

    template<typename ContiguousIterator>
    T operator()(ContiguousIterator begin, std::size_t size, T remainder = T{0}) const noexcept 
    {
        return (*this)(begin, begin + size, remainder);
    }

private:
    CrcTable<T> table;
};

using Crc8 = Crc<std::uint8_t>;
using Crc16 = Crc<std::uint16_t>;
using Crc32 = Crc<std::uint32_t>;

} // namespace sim
} // namespace proto_v1 
} // namespace soda

#pragma once

#include <type_traits>
#include <string>
#include <algorithm>
#include <cassert>

// Explicit scoped enums to underlying type conversion
template <typename E>
constexpr auto e_to_type(E e) noexcept
{
    return static_cast<std::underlying_type_t<E>>(e);
}

// Constexpr map
template <typename Key, typename Val, std::size_t Size>
struct RegMap
{
    std::array<std::pair<Key, Val>, Size> arr_;

    [[nodiscard]] constexpr Val at(const Key &key) const
    {
        const auto it = std::find_if(std::begin(arr_), std::end(arr_),
                                     [&key](const auto &v) { return v.first == key; });
        // Since this map is supposed to be used with enums, value should definitely be found
        assert(it != std::end(arr_));
        return it->second;
    }
};

constexpr std::string_view ExNames [] = {
    "SYS",
    "PCI_CFG",
    "PCI_IDS_DB"
};

enum class ExType
{
    SYS,
    PCI_CFG,
    PCI_ID_DB,
};

struct CommonEx : public std::runtime_error
{
    CommonEx(const std::string &ex_msg) : std::runtime_error(ex_msg) {}
};


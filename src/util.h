#pragma once

#include <type_traits>
#include <string>

// Explicit scoped enums to underlying type conversion
template <typename E>
constexpr auto e_type(E e) noexcept
{
    return static_cast<std::underlying_type_t<E>>(e);
}

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


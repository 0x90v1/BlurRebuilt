// [Blur1Multi] The part of XenosRecomp's pch.h the recompiler itself uses
// (the DXC, smol-v, xxHash and zstd includes serve its own cache writer, which
// is not vendored). byteSwap uses MSVC's intrinsics, as the local build fix in
// Blur2Test's copy does.
#pragma once

#include <intrin.h>

#include <bit>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>

#include "fmt/core.h"

template<typename T>
static T byteSwap(T value)
{
    if constexpr (sizeof(T) == 1)
        return value;
    else if constexpr (sizeof(T) == 2)
        return static_cast<T>(_byteswap_ushort(static_cast<uint16_t>(value)));
    else if constexpr (sizeof(T) == 4)
        return static_cast<T>(_byteswap_ulong(static_cast<uint32_t>(value)));
    else if constexpr (sizeof(T) == 8)
        return static_cast<T>(_byteswap_uint64(static_cast<uint64_t>(value)));

    assert(false && "Unexpected byte size.");
    return value;
}

template<typename T>
struct be
{
    T value;

    T get() const
    {
        if constexpr (std::is_enum_v<T>)
            return T(byteSwap(std::underlying_type_t<T>(value)));
        else
            return byteSwap(value);
    }

    operator T() const
    {
        return get();
    }
};

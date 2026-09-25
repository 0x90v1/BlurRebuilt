// [Blur1Multi] The {fmt} calls XenosRecomp's recompiler makes, on C++20 std::format.
#pragma once

#include <format>
#include <iterator>
#include <string>

namespace fmt
{
    template<class... Args>
    using format_string = std::format_string<Args...>;

    using std::format;
    using std::make_format_args;
    using std::vformat_to;
}

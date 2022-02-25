#pragma once

#include <cstdint>
#include <cstdio>
#include <cmath>
#include <cinttypes>
#include <cstring>
#include <cerrno>

#include <string>
#include <vector>
#include <map>
#include <functional>

#include <filesystem>
#include <system_error>

#include <sys/types.h>
#include <sys/stat.h>

#include <ncurses.h>

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using s8 = std::int8_t;
using s16 = std::int16_t;
using s32 = std::int32_t;
using s64 = std::int64_t;
using f32 = float;
using f64 = double;
using usize = std::size_t;
using ssize = std::intmax_t;

using namespace std::literals;
namespace fs = std::filesystem;

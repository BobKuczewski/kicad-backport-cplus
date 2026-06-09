#ifndef KICAD_BACKPORT_COMPAT_H
#define KICAD_BACKPORT_COMPAT_H

#if defined( __has_include )
#if __has_include( <string_view> )
#include <string_view>
#elif __has_include( <experimental/string_view> )
#include <experimental/string_view>
namespace std
{
using experimental::basic_string_view;
using experimental::string_view;
}
#else
#error "A C++17 string_view implementation is required."
#endif

#if __has_include( <filesystem> )
#include <filesystem>
#elif __has_include( <experimental/filesystem> )
#include <experimental/filesystem>
namespace std
{
namespace filesystem = experimental::filesystem;
}
#else
#error "A C++17 filesystem implementation is required."
#endif

#if __has_include( <memory_resource> )
#include <memory_resource>
#elif __has_include( <experimental/memory_resource> )
#include <experimental/memory_resource>
namespace std
{
namespace pmr = experimental::pmr;
}
#else
#error "A C++17 polymorphic memory resource implementation is required."
#endif
#else
#include <string_view>
#include <filesystem>
#include <memory_resource>
#endif

#endif // KICAD_BACKPORT_COMPAT_H

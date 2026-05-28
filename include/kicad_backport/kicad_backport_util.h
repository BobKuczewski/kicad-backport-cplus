#pragma once

#include <filesystem>
#include <string>


namespace KICAD_BACKPORT
{

// String helpers keep parsing code locale-independent and explicit.
std::string Lower( std::string aValue );
std::string Trim( const std::string& aValue );

bool StartsWith( const std::string& aValue, const std::string& aPrefix );
bool EndsWith( const std::string& aValue, const std::string& aSuffix );
bool IsNumber( const std::string& aValue );

// Small IO and formatting helpers shared by CLI and report modules.
std::string JsonEscape( const std::string& aValue );
std::string ReadTextFile( const std::filesystem::path& aPath );
void WriteTextFile( const std::filesystem::path& aPath, const std::string& aText );
std::string ReplaceExtension( const std::filesystem::path& aPath, const std::string& aExt );

} // namespace KICAD_BACKPORT

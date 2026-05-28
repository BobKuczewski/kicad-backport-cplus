#include "kicad_backport/kicad_backport_util.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>


namespace KICAD_BACKPORT
{

std::string Lower( std::string aValue )
{
    // KiCad tokens are ASCII; avoid locale-sensitive case conversions.
    std::transform( aValue.begin(), aValue.end(), aValue.begin(),
            []( unsigned char c ) { return static_cast<char>( std::tolower( c ) ); } );
    return aValue;
}


std::string Trim( const std::string& aValue )
{
    size_t first = 0;

    while( first < aValue.size() && std::isspace( static_cast<unsigned char>( aValue[first] ) ) )
        ++first;

    size_t last = aValue.size();

    while( last > first && std::isspace( static_cast<unsigned char>( aValue[last - 1] ) ) )
        --last;

    return aValue.substr( first, last - first );
}


bool StartsWith( const std::string& aValue, const std::string& aPrefix )
{
    return aValue.size() >= aPrefix.size() && aValue.compare( 0, aPrefix.size(), aPrefix ) == 0;
}


bool EndsWith( const std::string& aValue, const std::string& aSuffix )
{
    return aValue.size() >= aSuffix.size()
           && aValue.compare( aValue.size() - aSuffix.size(), aSuffix.size(), aSuffix ) == 0;
}


bool IsNumber( const std::string& aValue )
{
    return !aValue.empty()
           && std::all_of( aValue.begin(), aValue.end(),
                   []( unsigned char c ) { return std::isdigit( c ) != 0; } );
}


std::string JsonEscape( const std::string& aValue )
{
    // Reports are simple JSON and only need string escaping.
    std::string out;

    for( char ch : aValue )
    {
        switch( ch )
        {
        case '\\': out += "\\\\"; break;
        case '"':  out += "\\\""; break;
        case '\n': out += "\\n";  break;
        case '\r': break;
        case '\t': out += "\\t";  break;
        default:   out.push_back( ch ); break;
        }
    }

    return out;
}


std::string ReadTextFile( const std::filesystem::path& aPath )
{
    std::ifstream file( aPath, std::ios::binary );

    if( !file )
        throw std::runtime_error( "cannot read file: " + aPath.string() );

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}


void WriteTextFile( const std::filesystem::path& aPath, const std::string& aText )
{
    // Create parent directories so report and converted file writes are atomic at call sites.
    std::filesystem::create_directories( aPath.parent_path().empty() ? "." : aPath.parent_path() );

    std::ofstream file( aPath, std::ios::binary | std::ios::trunc );

    if( !file )
        throw std::runtime_error( "cannot write file: " + aPath.string() );

    file << aText;
}


std::string ReplaceExtension( const std::filesystem::path& aPath, const std::string& aExt )
{
    std::filesystem::path copy = aPath;
    copy.replace_extension( aExt );
    return copy.string();
}

} // namespace KICAD_BACKPORT

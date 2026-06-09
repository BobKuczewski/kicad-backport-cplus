#include "kicad_backport/kicad_backport_util.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <limits>
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


std::string ReadTextFile( const FS::path& aPath )
{
    std::ifstream file( aPath, std::ios::binary );

    if( !file )
        throw std::runtime_error( "cannot read file: " + aPath.string() );

    file.seekg( 0, std::ios::end );
    std::streamoff size = file.tellg();

    if( size < 0 )
        throw std::runtime_error( "cannot read file size: " + aPath.string() );

    if( static_cast<unsigned long long>( size )
        > static_cast<unsigned long long>( std::numeric_limits<size_t>::max() ) )
    {
        throw std::runtime_error( "file is too large to read on this platform: " + aPath.string() );
    }

    std::string text( static_cast<size_t>( size ), '\0' );
    file.seekg( 0, std::ios::beg );

    size_t offset = 0;

    while( offset < text.size() )
    {
        size_t remaining = text.size() - offset;
        size_t chunk = std::min<size_t>( remaining, 1024 * 1024 );
        file.read( &text[offset], static_cast<std::streamsize>( chunk ) );

        if( file.gcount() <= 0 )
            throw std::runtime_error( "cannot read file contents: " + aPath.string() );

        offset += static_cast<size_t>( file.gcount() );
    }

    if( !file )
        throw std::runtime_error( "cannot read file contents: " + aPath.string() );

    return text;
}


void WriteTextFile( const FS::path& aPath, const std::string& aText )
{
    // Create parent directories so report and converted file writes are atomic at call sites.
    FS::create_directories( aPath.parent_path().empty() ? "." : aPath.parent_path() );

    std::ofstream file( aPath, std::ios::binary | std::ios::trunc );

    if( !file )
        throw std::runtime_error( "cannot write file: " + aPath.string() );

    if( !aText.empty() )
        file.write( aText.data(), static_cast<std::streamsize>( aText.size() ) );

    if( !file )
        throw std::runtime_error( "cannot write file contents: " + aPath.string() );
}


std::string ReplaceExtension( const FS::path& aPath, const std::string& aExt )
{
    FS::path copy = aPath;
    copy.replace_extension( aExt );
    return copy.string();
}

} // namespace KICAD_BACKPORT

#include "kicad_backport/filesystem.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <stdexcept>

#if defined( _WIN32 )
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <direct.h>
#include <windows.h>
#else
#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace KICAD_BACKPORT
{
namespace FS
{
namespace
{

bool isSeparator( char aChar )
{
    return aChar == '/' || aChar == '\\';
}

std::string normalizeSeparators( const std::string& aValue )
{
    std::string out = aValue;

    for( char& ch : out )
    {
        if( ch == '\\' )
            ch = '/';
    }

    return out;
}

bool isAbsolutePath( const std::string& aPath )
{
    if( aPath.empty() )
        return false;

    if( isSeparator( aPath[0] ) )
        return true;

    return aPath.size() > 2 && aPath[1] == ':' && isSeparator( aPath[2] );
}

size_t rootLength( const std::string& aPath )
{
    if( aPath.size() > 2 && aPath[1] == ':' && isSeparator( aPath[2] ) )
        return 3;

    if( !aPath.empty() && isSeparator( aPath[0] ) )
        return 1;

    return 0;
}

std::string trimTrailingSeparators( std::string aPath )
{
    size_t root = rootLength( aPath );

    while( aPath.size() > root && isSeparator( aPath.back() ) )
        aPath.pop_back();

    return aPath;
}

std::vector<directory_entry> listDirectory( const path& aPath, std::error_code* aError )
{
    std::vector<directory_entry> entries;
    std::string base = aPath.string();

#if defined( _WIN32 )
    std::string pattern = trimTrailingSeparators( base );
    if( pattern.empty() )
        pattern = ".";
    pattern += "/*";

    WIN32_FIND_DATAA data;
    HANDLE handle = FindFirstFileA( pattern.c_str(), &data );

    if( handle == INVALID_HANDLE_VALUE )
    {
        if( aError )
            *aError = std::error_code( static_cast<int>( GetLastError() ), std::system_category() );
        return entries;
    }

    do
    {
        std::string name = data.cFileName;
        if( name == "." || name == ".." )
            continue;

        bool isDir = ( data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) != 0;
        entries.emplace_back( path( base ) / name, isDir, !isDir );
    } while( FindNextFileA( handle, &data ) );

    FindClose( handle );
#else
    DIR* dir = opendir( base.empty() ? "." : base.c_str() );

    if( !dir )
    {
        if( aError )
            *aError = std::error_code( errno, std::generic_category() );
        return entries;
    }

    while( dirent* item = readdir( dir ) )
    {
        std::string name = item->d_name;
        if( name == "." || name == ".." )
            continue;

        path itemPath = path( base ) / name;
        struct stat st;
        bool statOk = stat( itemPath.string().c_str(), &st ) == 0;
        entries.emplace_back( itemPath, statOk && S_ISDIR( st.st_mode ),
                              statOk && S_ISREG( st.st_mode ) );
    }

    closedir( dir );
#endif

    if( aError )
        aError->clear();

    return entries;
}

int makeDirectory( const std::string& aPath )
{
#if defined( _WIN32 )
    return _mkdir( aPath.c_str() );
#else
    return mkdir( aPath.c_str(), 0777 );
#endif
}

} // namespace

path::path( const char* aValue ) :
        m_path( normalizeSeparators( aValue ? aValue : "" ) )
{
}


path::path( const std::string& aValue ) :
        m_path( normalizeSeparators( aValue ) )
{
}


path path::parent_path() const
{
    std::string value = trimTrailingSeparators( m_path );
    size_t root = rootLength( value );
    size_t pos = value.find_last_of( '/' );

    if( pos == std::string::npos || pos < root )
        return path();

    if( pos == 0 )
        return path( "/" );

    return path( value.substr( 0, pos ) );
}


path path::filename() const
{
    std::string value = trimTrailingSeparators( m_path );
    size_t root = rootLength( value );
    size_t pos = value.find_last_of( '/' );

    if( pos == std::string::npos || pos < root )
        return path( value.substr( root ) );

    return path( value.substr( pos + 1 ) );
}


path path::extension() const
{
    std::string name = filename().string();
    size_t pos = name.find_last_of( '.' );

    if( pos == std::string::npos || pos == 0 )
        return path();

    return path( name.substr( pos ) );
}


path path::stem() const
{
    std::string name = filename().string();
    size_t pos = name.find_last_of( '.' );

    if( pos == std::string::npos || pos == 0 )
        return path( name );

    return path( name.substr( 0, pos ) );
}


path& path::replace_extension( const std::string& aExtension )
{
    path parent = parent_path();
    std::string stemName = stem().string();
    std::string ext = aExtension;

    if( !ext.empty() && ext[0] != '.' )
        ext = "." + ext;

    *this = parent.empty() ? path( stemName + ext ) : parent / ( stemName + ext );
    return *this;
}


path path::lexically_normal() const
{
    std::string value = normalizeSeparators( m_path );
    std::string root = value.substr( 0, rootLength( value ) );
    std::vector<std::string> parts;
    size_t pos = root.size();

    while( pos <= value.size() )
    {
        size_t next = value.find( '/', pos );
        std::string part = value.substr( pos, next == std::string::npos ? next : next - pos );

        if( part.empty() || part == "." )
        {
        }
        else if( part == ".." && !parts.empty() && parts.back() != ".." )
        {
            parts.pop_back();
        }
        else if( part != ".." || root.empty() )
        {
            parts.push_back( part );
        }

        if( next == std::string::npos )
            break;

        pos = next + 1;
    }

    std::string out = root;
    for( const std::string& part : parts )
    {
        if( !out.empty() && !isSeparator( out.back() ) )
            out += '/';
        out += part;
    }

    if( out.empty() )
        out = ".";

    return path( out );
}


path& path::operator=( const std::string& aValue )
{
    m_path = normalizeSeparators( aValue );
    return *this;
}


path operator/( const path& aLeft, const path& aRight )
{
    if( aLeft.empty() )
        return aRight;

    if( aRight.empty() )
        return aLeft;

    std::string left = aLeft.string();
    std::string right = aRight.string();

    if( isAbsolutePath( right ) )
        return path( right );

    if( !left.empty() && !isSeparator( left.back() ) )
        left += '/';

    return path( left + right );
}


bool operator==( const path& aLeft, const path& aRight )
{
    return aLeft.lexically_normal().string() == aRight.lexically_normal().string();
}


bool operator!=( const path& aLeft, const path& aRight )
{
    return !( aLeft == aRight );
}


bool operator<( const path& aLeft, const path& aRight )
{
    return aLeft.lexically_normal().string() < aRight.lexically_normal().string();
}


directory_entry::directory_entry( FS::path aPath, bool aIsDirectory, bool aIsRegularFile ) :
        m_path( aPath ),
        m_isDirectory( aIsDirectory ),
        m_isRegularFile( aIsRegularFile )
{
}


directory_iterator::directory_iterator( const path& aPath )
{
    std::error_code error;
    m_entries = listDirectory( aPath, &error );
}


directory_iterator::directory_iterator( const path& aPath, std::error_code& aError )
{
    m_entries = listDirectory( aPath, &aError );
}


directory_iterator& directory_iterator::operator++()
{
    if( m_index < m_entries.size() )
        ++m_index;

    return *this;
}


bool directory_iterator::operator==( const directory_iterator& aOther ) const
{
    bool leftEnd = m_index >= m_entries.size();
    bool rightEnd = aOther.m_index >= aOther.m_entries.size();

    if( leftEnd || rightEnd )
        return leftEnd == rightEnd;

    return m_index == aOther.m_index && m_entries.data() == aOther.m_entries.data();
}


recursive_directory_iterator::recursive_directory_iterator( const path& aPath )
{
    FRAME frame;
    frame.Entries = listDirectory( aPath, nullptr );

    if( !frame.Entries.empty() )
    {
        m_stack.push_back( frame );
        m_atEnd = false;
        m_current = m_stack.back().Entries[0];
    }
}


recursive_directory_iterator& recursive_directory_iterator::operator++()
{
    if( m_atEnd )
        return *this;

    if( m_current.is_directory() && !m_skipCurrentDirectory )
    {
        FRAME child;
        child.Entries = listDirectory( m_current.path(), nullptr );

        if( !child.Entries.empty() )
        {
            m_stack.push_back( child );
            m_current = m_stack.back().Entries[0];
        }
        else
        {
            advanceToNext();
        }
    }
    else
    {
        advanceToNext();
    }

    m_skipCurrentDirectory = false;
    return *this;
}


bool recursive_directory_iterator::operator==( const recursive_directory_iterator& aOther ) const
{
    if( m_atEnd || aOther.m_atEnd )
        return m_atEnd == aOther.m_atEnd;

    return m_current.path() == aOther.m_current.path();
}


void recursive_directory_iterator::advanceToNext()
{
    while( !m_stack.empty() )
    {
        FRAME& frame = m_stack.back();
        ++frame.Index;

        if( frame.Index < frame.Entries.size() )
        {
            m_current = frame.Entries[frame.Index];
            return;
        }

        m_stack.pop_back();
    }

    m_atEnd = true;
}


bool exists( const path& aPath )
{
#if defined( _WIN32 )
    return GetFileAttributesA( aPath.string().c_str() ) != INVALID_FILE_ATTRIBUTES;
#else
    struct stat st;
    return stat( aPath.string().c_str(), &st ) == 0;
#endif
}


bool is_directory( const path& aPath )
{
#if defined( _WIN32 )
    DWORD attrs = GetFileAttributesA( aPath.string().c_str() );
    return attrs != INVALID_FILE_ATTRIBUTES && ( attrs & FILE_ATTRIBUTE_DIRECTORY ) != 0;
#else
    struct stat st;
    return stat( aPath.string().c_str(), &st ) == 0 && S_ISDIR( st.st_mode );
#endif
}


bool create_directories( const path& aPath )
{
    path normal = aPath.lexically_normal();
    std::string value = normal.string();

    if( value.empty() || value == "." || exists( normal ) )
        return true;

    path parent = normal.parent_path();
    if( !parent.empty() && !exists( parent ) )
        create_directories( parent );

    if( makeDirectory( value ) == 0 )
        return true;

    return errno == EEXIST || exists( normal );
}


bool copy_file( const path& aFrom, const path& aTo, copy_options::VALUE )
{
    create_directories( aTo.parent_path() );

#if defined( _WIN32 )
    return CopyFileA( aFrom.string().c_str(), aTo.string().c_str(), FALSE ) != 0;
#else
    std::ifstream input( aFrom.string(), std::ios::binary );
    std::ofstream output( aTo.string(), std::ios::binary | std::ios::trunc );

    if( !input || !output )
        return false;

    output << input.rdbuf();
    return static_cast<bool>( output );
#endif
}


path absolute( const path& aPath )
{
    std::string value = aPath.string();

    if( isAbsolutePath( value ) )
        return aPath.lexically_normal();

#if defined( _WIN32 )
    char buffer[MAX_PATH];
    DWORD len = GetFullPathNameA( value.c_str(), MAX_PATH, buffer, nullptr );
    if( len > 0 && len < MAX_PATH )
        return path( std::string( buffer ) ).lexically_normal();
#else
    char cwd[PATH_MAX];
    if( getcwd( cwd, sizeof( cwd ) ) )
        return ( path( cwd ) / value ).lexically_normal();
#endif

    return aPath.lexically_normal();
}


path relative( const path& aPath, const path& aBase )
{
    std::error_code error;
    return relative( aPath, aBase, error );
}


path relative( const path& aPath, const path& aBase, std::error_code& aError )
{
    std::string target = absolute( aPath ).lexically_normal().string();
    std::string base = absolute( aBase ).lexically_normal().string();

    if( !base.empty() && base.back() != '/' )
        base += '/';

    if( target.compare( 0, base.size(), base ) == 0 )
    {
        aError.clear();
        return path( target.substr( base.size() ) );
    }

    aError = std::make_error_code( std::errc::invalid_argument );
    return path();
}

} // namespace FS
} // namespace KICAD_BACKPORT

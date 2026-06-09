#ifndef KICAD_BACKPORT_FILESYSTEM_H
#define KICAD_BACKPORT_FILESYSTEM_H

#include <string>
#include <system_error>
#include <vector>

namespace KICAD_BACKPORT
{
namespace FS
{

class path
{
public:
    path() = default;
    path( const char* aValue );
    path( const std::string& aValue );

    bool empty() const { return m_path.empty(); }
    std::string string() const { return m_path; }
    std::string generic_string() const { return m_path; }
    const std::string& native() const { return m_path; }

    path parent_path() const;
    path filename() const;
    path extension() const;
    path stem() const;
    path& replace_extension( const std::string& aExtension );
    path lexically_normal() const;

    path& operator=( const std::string& aValue );
    operator std::string() const { return m_path; }

private:
    std::string m_path;
};

path operator/( const path& aLeft, const path& aRight );
bool operator==( const path& aLeft, const path& aRight );
bool operator!=( const path& aLeft, const path& aRight );
bool operator<( const path& aLeft, const path& aRight );

class directory_entry
{
public:
    directory_entry() = default;
    directory_entry( FS::path aPath, bool aIsDirectory, bool aIsRegularFile );

    const FS::path& path() const { return m_path; }
    bool is_directory() const { return m_isDirectory; }
    bool is_regular_file() const { return m_isRegularFile; }

private:
    FS::path m_path;
    bool     m_isDirectory = false;
    bool     m_isRegularFile = false;
};

class directory_iterator
{
public:
    directory_iterator() = default;
    explicit directory_iterator( const path& aPath );
    directory_iterator( const path& aPath, std::error_code& aError );

    const directory_entry& operator*() const { return m_entries[m_index]; }
    const directory_entry* operator->() const { return &m_entries[m_index]; }
    directory_iterator& operator++();
    bool operator==( const directory_iterator& aOther ) const;
    bool operator!=( const directory_iterator& aOther ) const { return !( *this == aOther ); }

private:
    std::vector<directory_entry> m_entries;
    size_t                       m_index = 0;
};

class recursive_directory_iterator
{
public:
    recursive_directory_iterator() = default;
    explicit recursive_directory_iterator( const path& aPath );

    const directory_entry& operator*() const { return m_current; }
    const directory_entry* operator->() const { return &m_current; }
    recursive_directory_iterator& operator++();
    bool operator==( const recursive_directory_iterator& aOther ) const;
    bool operator!=( const recursive_directory_iterator& aOther ) const { return !( *this == aOther ); }
    void disable_recursion_pending() { m_skipCurrentDirectory = true; }

private:
    struct FRAME
    {
        std::vector<directory_entry> Entries;
        size_t                       Index = 0;
    };

    void advanceToNext();

    std::vector<FRAME> m_stack;
    directory_entry    m_current;
    bool               m_atEnd = true;
    bool               m_skipCurrentDirectory = false;
};

namespace copy_options
{
enum VALUE
{
    overwrite_existing = 1
};
}

bool exists( const path& aPath );
bool is_directory( const path& aPath );
bool create_directories( const path& aPath );
bool copy_file( const path& aFrom, const path& aTo, copy_options::VALUE aOptions );
path absolute( const path& aPath );
path relative( const path& aPath, const path& aBase );
path relative( const path& aPath, const path& aBase, std::error_code& aError );

} // namespace FS
} // namespace KICAD_BACKPORT

#endif // KICAD_BACKPORT_FILESYSTEM_H

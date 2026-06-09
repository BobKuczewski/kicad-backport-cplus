#ifndef KICAD_BACKPORT_SEXPR_H
#define KICAD_BACKPORT_SEXPR_H

#include "kicad_backport/compat.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>


namespace SEXPR
{

// Minimal mutable S-expression tree used by KiCad file rewrites.
class NODE
{
public:
    NODE();

    std::shared_ptr<std::pmr::monotonic_buffer_resource> ArenaOwner;
    std::pmr::string                  Atom;
    bool                              Quoted = false;
    std::pmr::vector<std::unique_ptr<NODE>> Children;

    static void* operator new( size_t aSize );
    static void operator delete( void* aPtr ) noexcept;
    static void operator delete( void* aPtr, size_t aSize ) noexcept;

    bool IsAtom() const { return Children.empty(); }
    std::string Head() const;
    std::string_view HeadView() const;
    std::string AtomAt( size_t aIndex ) const;
    std::string_view AtomAtView( size_t aIndex ) const;
    bool SetAtomAt( size_t aIndex, const std::string& aValue, bool aQuoted );
    NODE* ChildList( const std::string& aHead );
    std::vector<NODE*> ChildLists( const std::string& aHead );

    static std::unique_ptr<NODE> MakeAtom( const std::string& aValue, bool aQuoted = false );
    static std::unique_ptr<NODE> MakeAtom( std::string&& aValue, bool aQuoted = false );
    static std::unique_ptr<NODE> MakeList();
};

// Parses and formats KiCad-style S-expression text.
std::unique_ptr<NODE> Parse( const std::string& aText );
std::string Format( const NODE* aRoot );
std::string Format( const NODE* aRoot, size_t aReserveBytes );

} // namespace SEXPR

#endif // KICAD_BACKPORT_SEXPR_H

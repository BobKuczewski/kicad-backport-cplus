#pragma once

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
    std::string                         Atom;
    bool                                Quoted = false;
    std::vector<std::unique_ptr<NODE>>  Children;

    bool IsAtom() const { return Children.empty(); }
    std::string Head() const;
    std::string AtomAt( size_t aIndex ) const;
    bool SetAtomAt( size_t aIndex, const std::string& aValue, bool aQuoted );
    NODE* ChildList( const std::string& aHead );
    std::vector<NODE*> ChildLists( const std::string& aHead );

    static std::unique_ptr<NODE> MakeAtom( const std::string& aValue, bool aQuoted = false );
    static std::unique_ptr<NODE> MakeList();
};

// Parses and formats KiCad-style S-expression text.
std::unique_ptr<NODE> Parse( const std::string& aText );
std::string Format( const NODE* aRoot );

} // namespace SEXPR

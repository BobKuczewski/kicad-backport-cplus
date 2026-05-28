#include "kicad_backport/kicad_backport_versions.h"
#include "kicad_backport/kicad_backport_util.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <stdexcept>


namespace KICAD_BACKPORT
{
namespace
{

using VERSION_MAP = std::map<KIND, std::string>;

std::string normalizedAlias( const std::string& aTarget )
{
    // Accept common user forms such as "v9", "kicad-9", and "9.0".
    std::string target = Lower( Trim( aTarget ) );

    if( target.empty() )
        throw std::runtime_error( "empty target version" );

    if( StartsWith( target, "kicad-" ) )
        target = target.substr( 6 );

    if( StartsWith( target, "v" ) )
        target = target.substr( 1 );

    if( target.find( '.' ) == std::string::npos )
        target += ".0";

    return target;
}

const std::map<std::string, VERSION_MAP>& targetVersions()
{
    // File format versions differ by document type for the same KiCad release.
    static const std::map<std::string, VERSION_MAP> targets = {
        { "6.0",  { { KIND::SYMBOL_LIBRARY, "20211014" }, { KIND::SCHEMATIC, "20211123" }, { KIND::BOARD, "20211014" }, { KIND::FOOTPRINT, "20211014" }, { KIND::WORKSHEET, "20210606" }, { KIND::DESIGN_RULES, "20200610" } } },
        { "7.0",  { { KIND::SYMBOL_LIBRARY, "20220914" }, { KIND::SCHEMATIC, "20230121" }, { KIND::BOARD, "20221018" }, { KIND::FOOTPRINT, "20221018" }, { KIND::WORKSHEET, "20220228" }, { KIND::DESIGN_RULES, "20200610" } } },
        { "8.0",  { { KIND::SYMBOL_LIBRARY, "20231120" }, { KIND::SCHEMATIC, "20231120" }, { KIND::BOARD, "20240108" }, { KIND::FOOTPRINT, "20240108" }, { KIND::WORKSHEET, "20231118" }, { KIND::DESIGN_RULES, "20200610" } } },
        { "9.0",  { { KIND::SYMBOL_LIBRARY, "20241209" }, { KIND::SCHEMATIC, "20250114" }, { KIND::BOARD, "20241229" }, { KIND::FOOTPRINT, "20241229" }, { KIND::WORKSHEET, "20231118" }, { KIND::DESIGN_RULES, "20200610" } } },
        { "10.0", { { KIND::SYMBOL_LIBRARY, "20251024" }, { KIND::SCHEMATIC, "20260306" }, { KIND::BOARD, "20260206" }, { KIND::FOOTPRINT, "20260206" }, { KIND::WORKSHEET, "20231118" }, { KIND::DESIGN_RULES, "20200610" } } },
    };

    return targets;
}

} // namespace


std::string ResolveTargetVersion( KIND aKind, const std::string& aTarget )
{
    std::string target = Lower( Trim( aTarget ) );

    if( target.empty() )
        throw std::runtime_error( "empty target version" );

    if( IsNumber( target ) )
        return target;

    target = normalizedAlias( target );
    const auto& targets = targetVersions();
    auto targetIt = targets.find( target );

    if( targetIt == targets.end() )
        throw std::runtime_error( "unsupported KiCad target version alias: " + aTarget );

    auto kindIt = targetIt->second.find( aKind );

    if( kindIt == targetIt->second.end() )
        throw std::runtime_error( "target version is not defined for this file type" );

    return kindIt->second;
}


std::string TargetVersionSuffix( const std::string& aTarget )
{
    // Keep output names stable: project -> project_V9, file -> file_V9.ext.
    std::string value = Lower( Trim( aTarget ) );

    if( StartsWith( value, "kicad-" ) )
        value = value.substr( 6 );

    if( StartsWith( value, "v" ) )
        value = value.substr( 1 );

    size_t sep = value.find_first_of( ".-_" );
    std::string major = sep == std::string::npos ? value : value.substr( 0, sep );

    if( major.empty() )
        return std::string();

    std::transform( major.begin(), major.end(), major.begin(),
            []( unsigned char c ) { return static_cast<char>( std::toupper( c ) ); } );

    return "V" + major;
}

} // namespace KICAD_BACKPORT

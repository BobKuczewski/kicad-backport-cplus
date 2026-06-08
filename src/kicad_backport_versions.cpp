#include "kicad_backport/kicad_backport_versions.h"
#include "kicad_backport/kicad_backport_util.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <stdexcept>
#include <vector>


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
        { "4.0",  { { KIND::BOARD, "4" }, { KIND::FOOTPRINT, "4" } } },
        { "5.0",  { { KIND::BOARD, "20171130" }, { KIND::FOOTPRINT, "20171130" } } },
        { "5.1",  { { KIND::BOARD, "20171130" }, { KIND::FOOTPRINT, "20171130" } } },
        { "6.0",  { { KIND::SYMBOL_LIBRARY, "20211014" }, { KIND::SCHEMATIC, "20211123" }, { KIND::BOARD, "20211014" }, { KIND::FOOTPRINT, "20211014" }, { KIND::WORKSHEET, "20210606" }, { KIND::DESIGN_RULES, "20200610" } } },
        { "7.0",  { { KIND::SYMBOL_LIBRARY, "20220914" }, { KIND::SCHEMATIC, "20230121" }, { KIND::BOARD, "20221018" }, { KIND::FOOTPRINT, "20221018" }, { KIND::WORKSHEET, "20220228" }, { KIND::DESIGN_RULES, "20200610" } } },
        { "8.0",  { { KIND::SYMBOL_LIBRARY, "20231120" }, { KIND::SCHEMATIC, "20231120" }, { KIND::BOARD, "20240108" }, { KIND::FOOTPRINT, "20240108" }, { KIND::WORKSHEET, "20231118" }, { KIND::DESIGN_RULES, "20200610" } } },
        { "9.0",  { { KIND::SYMBOL_LIBRARY, "20241209" }, { KIND::SCHEMATIC, "20250114" }, { KIND::BOARD, "20241229" }, { KIND::FOOTPRINT, "20241229" }, { KIND::WORKSHEET, "20231118" }, { KIND::DESIGN_RULES, "20200610" } } },
        { "10.0", { { KIND::SYMBOL_LIBRARY, "20251024" }, { KIND::SCHEMATIC, "20260306" }, { KIND::BOARD, "20260206" }, { KIND::FOOTPRINT, "20260206" }, { KIND::WORKSHEET, "20231118" }, { KIND::DESIGN_RULES, "20200610" } } },
        { "10.99", { { KIND::SYMBOL_LIBRARY, "20251024" }, { KIND::SCHEMATIC, "20260306" }, { KIND::BOARD, "20260603" }, { KIND::FOOTPRINT, "20260603" }, { KIND::WORKSHEET, "20231118" }, { KIND::DESIGN_RULES, "20200610" } } },
    };

    return targets;
}


const std::vector<std::string>& displayAliasOrder()
{
    static const std::vector<std::string> order = {
        "4.0", "5.0", "5.1", "6.0", "7.0", "8.0", "9.0", "10.0", "10.99",
    };

    return order;
}


std::string joinAliases( const std::vector<std::string>& aAliases )
{
    std::string text;

    for( const std::string& alias : aAliases )
    {
        if( !text.empty() )
            text += "/";

        text += alias;
    }

    return text;
}


std::string developmentAliasForVersion( KIND aKind, int aVersion )
{
    int previousVersion = -1;
    std::string previousAlias;
    const auto& targets = targetVersions();

    for( const std::string& alias : displayAliasOrder() )
    {
        auto targetIt = targets.find( alias );

        if( targetIt == targets.end() )
            continue;

        auto kindIt = targetIt->second.find( aKind );

        if( kindIt == targetIt->second.end() || !IsNumber( kindIt->second ) )
            continue;

        int aliasVersion = std::stoi( kindIt->second );

        if( aVersion < aliasVersion && previousVersion >= 0 && aVersion > previousVersion )
            return alias + "-dev";

        previousVersion = aliasVersion;
        previousAlias = alias;
    }

    if( previousVersion >= 0 && aVersion > previousVersion && !previousAlias.empty() )
        return previousAlias + "+";

    return std::string();
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

    if( value == "10.99" )
        return "V10_99";

    size_t sep = value.find_first_of( ".-_" );
    std::string major = sep == std::string::npos ? value : value.substr( 0, sep );

    if( major.empty() )
        return std::string();

    std::transform( major.begin(), major.end(), major.begin(),
            []( unsigned char c ) { return static_cast<char>( std::toupper( c ) ); } );

    return "V" + major;
}


std::string DisplayVersionAlias( KIND aKind, const std::string& aVersion )
{
    std::string version = Trim( aVersion );

    if( version.empty() )
        return version;

    std::vector<std::string> aliases;
    const auto& targets = targetVersions();

    for( const std::string& alias : displayAliasOrder() )
    {
        auto targetIt = targets.find( alias );

        if( targetIt == targets.end() )
            continue;

        auto kindIt = targetIt->second.find( aKind );

        if( kindIt != targetIt->second.end() && kindIt->second == version )
            aliases.push_back( alias );
    }

    if( aliases.empty() )
    {
        if( IsNumber( version ) )
        {
            std::string devAlias = developmentAliasForVersion( aKind, std::stoi( version ) );

            if( !devAlias.empty() )
                return devAlias + " (" + version + ")";
        }

        return version;
    }

    return joinAliases( aliases ) + " (" + version + ")";
}


int TargetMajorVersion( const std::string& aTarget )
{
    std::string value = Lower( Trim( aTarget ) );

    if( value.empty() )
        throw std::runtime_error( "empty target version" );

    if( StartsWith( value, "kicad-" ) )
        value = value.substr( 6 );

    if( StartsWith( value, "v" ) )
        value = value.substr( 1 );

    size_t sep = value.find_first_of( ".-_" );
    std::string major = sep == std::string::npos ? value : value.substr( 0, sep );

    if( !IsNumber( major ) )
        throw std::runtime_error( "unsupported KiCad target version alias: " + aTarget );

    return std::stoi( major );
}

} // namespace KICAD_BACKPORT

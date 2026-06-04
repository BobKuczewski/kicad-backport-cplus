#include "kicad_backport/kicad_backport.h"
#include "kicad_backport/kicad_backport_util.h"


namespace KICAD_BACKPORT
{

std::string KindName( KIND aKind )
{
    switch( aKind )
    {
    case KIND::SYMBOL_LIBRARY: return "symbol-library";
    case KIND::SCHEMATIC:      return "schematic";
    case KIND::BOARD:          return "board";
    case KIND::FOOTPRINT:      return "footprint";
    case KIND::DESIGN_RULES:   return "design-rules";
    case KIND::WORKSHEET:      return "worksheet";
    case KIND::LEGACY_SCHEMATIC:             return "legacy-schematic";
    case KIND::LEGACY_SYMBOL_LIBRARY:        return "legacy-symbol-library";
    case KIND::LEGACY_SYMBOL_DOCUMENTATION:  return "legacy-symbol-documentation";
    case KIND::LEGACY_PROJECT:               return "legacy-project";
    default:                   return "unknown";
    }
}


KIND DetectKind( const std::filesystem::path& aPath, const std::string& aTopLevel )
{
    // Prefer the root S-expression head because copied files may have odd names.
    if( aTopLevel == "kicad_symbol_lib" )
        return KIND::SYMBOL_LIBRARY;
    if( aTopLevel == "kicad_sch" )
        return KIND::SCHEMATIC;
    if( aTopLevel == "kicad_pcb" )
        return KIND::BOARD;
    if( aTopLevel == "footprint" )
        return KIND::FOOTPRINT;
    if( aTopLevel == "kicad_dru" )
        return KIND::DESIGN_RULES;
    if( aTopLevel == "kicad_wks" || aTopLevel == "drawing_sheet" )
        return KIND::WORKSHEET;

    // Fall back to extension for empty or unknown root heads.
    std::string ext = Lower( aPath.extension().string() );

    if( ext == ".kicad_sym" )
        return KIND::SYMBOL_LIBRARY;
    if( ext == ".kicad_sch" )
        return KIND::SCHEMATIC;
    if( ext == ".kicad_pcb" )
        return KIND::BOARD;
    if( ext == ".kicad_mod" )
        return KIND::FOOTPRINT;
    if( ext == ".kicad_dru" )
        return KIND::DESIGN_RULES;
    if( ext == ".kicad_wks" )
        return KIND::WORKSHEET;
    if( ext == ".sch" )
        return KIND::LEGACY_SCHEMATIC;
    if( ext == ".lib" )
        return KIND::LEGACY_SYMBOL_LIBRARY;
    if( ext == ".dcm" )
        return KIND::LEGACY_SYMBOL_DOCUMENTATION;
    if( ext == ".pro" )
        return KIND::LEGACY_PROJECT;

    return KIND::UNKNOWN;
}

} // namespace KICAD_BACKPORT

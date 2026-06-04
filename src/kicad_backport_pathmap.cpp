#include "kicad_backport/kicad_backport_pathmap.h"


namespace KICAD_BACKPORT
{

std::string TargetExtensionForKind( KIND aKind )
{
    switch( aKind )
    {
    case KIND::SYMBOL_LIBRARY: return ".kicad_sym";
    case KIND::SCHEMATIC:      return ".kicad_sch";
    case KIND::BOARD:          return ".kicad_pcb";
    case KIND::FOOTPRINT:      return ".kicad_mod";
    case KIND::DESIGN_RULES:   return ".kicad_dru";
    case KIND::WORKSHEET:      return ".kicad_wks";
    case KIND::LEGACY_SCHEMATIC:             return ".sch";
    case KIND::LEGACY_SYMBOL_LIBRARY:        return ".lib";
    case KIND::LEGACY_SYMBOL_DOCUMENTATION:  return ".dcm";
    case KIND::LEGACY_PROJECT:               return ".pro";
    default:                   return std::string();
    }
}


std::filesystem::path WithTargetExtension( const std::filesystem::path& aPath, KIND aTargetKind )
{
    std::string ext = TargetExtensionForKind( aTargetKind );

    if( ext.empty() )
        return aPath;

    return aPath.parent_path() / ( aPath.stem().string() + ext );
}

} // namespace KICAD_BACKPORT

#include "kicad_backport/kicad_backport_legacy.h"

#include <stdexcept>
#include <utility>


namespace KICAD_BACKPORT
{

bool IsLegacyKind( KIND aKind )
{
    return aKind == KIND::LEGACY_SCHEMATIC || aKind == KIND::LEGACY_SYMBOL_LIBRARY
           || aKind == KIND::LEGACY_SYMBOL_DOCUMENTATION || aKind == KIND::LEGACY_PROJECT;
}


DOCUMENT LoadLegacyDocument( const std::filesystem::path& aPath, std::string aText )
{
    DOCUMENT doc;
    doc.Path = aPath;
    doc.SourceBytes = aText.size();
    doc.RawText = std::move( aText );
    doc.Kind = DetectKind( aPath, std::string() );

    switch( doc.Kind )
    {
    case KIND::LEGACY_SCHEMATIC:
        doc.Version = "legacy-sch";
        break;
    case KIND::LEGACY_SYMBOL_LIBRARY:
        doc.Version = "legacy-lib";
        break;
    case KIND::LEGACY_SYMBOL_DOCUMENTATION:
        doc.Version = "legacy-dcm";
        break;
    case KIND::LEGACY_PROJECT:
        doc.Version = "legacy-pro";
        break;
    default:
        throw std::runtime_error( "not a KiCad legacy document" );
    }

    return doc;
}

} // namespace KICAD_BACKPORT

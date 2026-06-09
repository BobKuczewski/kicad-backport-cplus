#ifndef KICAD_BACKPORT_KICAD_BACKPORT_PATHMAP_H
#define KICAD_BACKPORT_KICAD_BACKPORT_PATHMAP_H

#include "kicad_backport/kicad_backport.h"

#include <string>


namespace KICAD_BACKPORT
{

std::string TargetExtensionForKind( KIND aKind );
FS::path WithTargetExtension( const FS::path& aPath, KIND aTargetKind );

} // namespace KICAD_BACKPORT

#endif // KICAD_BACKPORT_KICAD_BACKPORT_PATHMAP_H

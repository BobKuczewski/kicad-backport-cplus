#ifndef KICAD_BACKPORT_KICAD_BACKPORT_VERSIONS_H
#define KICAD_BACKPORT_KICAD_BACKPORT_VERSIONS_H

#include "kicad_backport/kicad_backport.h"

#include <string>


namespace KICAD_BACKPORT
{

// Resolves user-facing KiCad aliases to per-file format versions.
std::string ResolveTargetVersion( KIND aKind, const std::string& aTarget );

// Returns the output suffix used for converted project/file names.
std::string TargetVersionSuffix( const std::string& aTarget );

// Formats a raw file format version as a KiCad release alias when one is known.
std::string DisplayVersionAlias( KIND aKind, const std::string& aVersion );

// Returns the KiCad major release number for aliases such as "5.0" or "kicad-7".
int TargetMajorVersion( const std::string& aTarget );

} // namespace KICAD_BACKPORT

#endif // KICAD_BACKPORT_KICAD_BACKPORT_VERSIONS_H

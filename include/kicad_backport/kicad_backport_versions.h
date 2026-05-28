#pragma once

#include "kicad_backport/kicad_backport.h"

#include <string>


namespace KICAD_BACKPORT
{

// Resolves user-facing KiCad aliases to per-file format versions.
std::string ResolveTargetVersion( KIND aKind, const std::string& aTarget );

// Returns the output suffix used for converted project/file names.
std::string TargetVersionSuffix( const std::string& aTarget );

} // namespace KICAD_BACKPORT

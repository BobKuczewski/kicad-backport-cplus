#pragma once

#include "kicad_backport/kicad_backport.h"

#include <string>
#include <vector>


namespace KICAD_BACKPORT
{

// Applies syntax rewrites before the target file version is written.
std::vector<std::string> ApplyDowngradeRules( DOCUMENT& aDocument, int aTarget );

} // namespace KICAD_BACKPORT

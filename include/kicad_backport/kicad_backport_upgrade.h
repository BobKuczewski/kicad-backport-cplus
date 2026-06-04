#pragma once

#include "kicad_backport/kicad_backport.h"

#include <string>
#include <vector>


namespace KICAD_BACKPORT
{

// Applies forward-compatible S-expression rewrites before the target version is written.
std::vector<std::string> ApplyUpgradeRules( DOCUMENT& aDocument, int aTarget );

} // namespace KICAD_BACKPORT

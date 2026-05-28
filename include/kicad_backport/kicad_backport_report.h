#pragma once

#include "kicad_backport/kicad_backport.h"

#include <string>
#include <vector>


namespace KICAD_BACKPORT
{

// Formats machine-readable inspect/convert reports.
std::string FormatReportsJson( const std::vector<FILE_REPORT>& aReports );

} // namespace KICAD_BACKPORT

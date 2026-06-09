#ifndef KICAD_BACKPORT_KICAD_BACKPORT_REPORT_H
#define KICAD_BACKPORT_KICAD_BACKPORT_REPORT_H

#include "kicad_backport/kicad_backport.h"

#include <string>
#include <vector>


namespace KICAD_BACKPORT
{

// Formats machine-readable inspect/convert reports.
std::string FormatReportsJson( const std::vector<FILE_REPORT>& aReports );

} // namespace KICAD_BACKPORT

#endif // KICAD_BACKPORT_KICAD_BACKPORT_REPORT_H

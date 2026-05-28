#include "kicad_backport/kicad_backport_report.h"
#include "kicad_backport/kicad_backport_util.h"

#include <sstream>


namespace KICAD_BACKPORT
{

std::string FormatReportsJson( const std::vector<FILE_REPORT>& aReports )
{
    std::ostringstream out;
    out << "[\n";

    for( size_t i = 0; i < aReports.size(); ++i )
    {
        const FILE_REPORT& report = aReports[i];
        out << "  {\n";
        out << "    \"path\": \"" << JsonEscape( report.Path ) << "\",\n";
        out << "    \"kind\": \"" << JsonEscape( report.Kind ) << "\",\n";
        out << "    \"source_version\": \"" << JsonEscape( report.SourceVersion ) << "\",\n";

        if( !report.TargetVersion.empty() )
            out << "    \"target_version\": \"" << JsonEscape( report.TargetVersion ) << "\",\n";

        out << "    \"changed\": " << ( report.Changed ? "true" : "false" );

        if( !report.Warnings.empty() )
        {
            out << ",\n    \"warnings\": [";

            for( size_t j = 0; j < report.Warnings.size(); ++j )
            {
                if( j > 0 )
                    out << ", ";

                out << "\"" << JsonEscape( report.Warnings[j] ) << "\"";
            }

            out << "]";
        }

        out << "\n  }";

        if( i + 1 < aReports.size() )
            out << ",";

        out << "\n";
    }

    out << "]\n";
    return out.str();
}

} // namespace KICAD_BACKPORT

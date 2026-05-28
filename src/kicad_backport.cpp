#include "kicad_backport/kicad_backport.h"
#include "kicad_backport/kicad_backport_report.h"
#include "kicad_backport/kicad_backport_rules.h"
#include "kicad_backport/kicad_backport_util.h"
#include "kicad_backport/kicad_backport_versions.h"

#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>


namespace KICAD_BACKPORT
{

// Dispatches subcommands and keeps all fatal errors on stderr for script use.
int CONVERTER::Run( int aArgc, char** aArgv )
{
    try
    {
        if( aArgc < 2 )
        {
            printUsage();
            return 2;
        }

        std::string command = aArgv[1];

        if( command == "version" || command == "--version" || command == "-version" )
        {
            std::cout << VERSION << '\n';
            return 0;
        }

        std::vector<std::string> args;

        for( int i = 2; i < aArgc; ++i )
            args.emplace_back( aArgv[i] );

        if( command == "convert" )
            return runConvert( args );

        if( command == "inspect" )
            return runInspect( args );

        printUsage();
        return 2;
    }
    catch( const std::exception& e )
    {
        std::cerr << "error: " << e.what() << '\n';
        return 1;
    }
}


void CONVERTER::printUsage() const
{
    std::cerr << "usage:\n";
    std::cerr << "  kicad-backport convert --target-version <7.0|8.0|9.0|10.0|number> <input> <output>\n";
    std::cerr << "  kicad-backport inspect <input>\n";
    std::cerr << "  kicad-backport version\n";
}


DOCUMENT CONVERTER::loadDocument( const std::filesystem::path& aPath ) const
{
    DOCUMENT doc;
    doc.Path = aPath;
    doc.Root = SEXPR::Parse( ReadTextFile( aPath ) );
    doc.Kind = DetectKind( aPath, doc.Root->Head() );

    if( SEXPR::NODE* version = doc.Root->ChildList( "version" ) )
        doc.Version = version->AtomAt( 1 );

    return doc;
}


void CONVERTER::ensureVersion( DOCUMENT& aDocument, const std::string& aVersion ) const
{
    // KiCad expects the version field near the root head when it is present.
    if( SEXPR::NODE* version = aDocument.Root->ChildList( "version" ) )
    {
        if( !version->SetAtomAt( 1, aVersion, false ) )
            throw std::runtime_error( "document has an invalid top-level version field" );
    }
    else
    {
        std::unique_ptr<SEXPR::NODE> versionNode = SEXPR::NODE::MakeList();
        versionNode->Children.push_back( SEXPR::NODE::MakeAtom( "version" ) );
        versionNode->Children.push_back( SEXPR::NODE::MakeAtom( aVersion ) );

        if( aDocument.Root->Children.empty() )
            aDocument.Root->Children.push_back( std::move( versionNode ) );
        else
            aDocument.Root->Children.insert( aDocument.Root->Children.begin() + 1, std::move( versionNode ) );
    }

    aDocument.Version = aVersion;
}


FILE_REPORT CONVERTER::normalizeFile( const std::filesystem::path& aInput,
                                      const std::filesystem::path& aOutput,
                                      const std::string& aTarget )
{
    DOCUMENT doc = loadDocument( aInput );
    FILE_REPORT report;
    report.Path = aOutput.string();
    report.Kind = KindName( doc.Kind );
    report.SourceVersion = doc.Version;

    std::string resolved = ResolveTargetVersion( doc.Kind, aTarget );
    int source = IsNumber( doc.Version ) ? std::stoi( doc.Version ) : 0;
    int target = std::stoi( resolved );

    // Do not upgrade older source files when the requested target is newer.
    if( source > 0 && source < target )
    {
        if( aInput != aOutput )
            std::filesystem::copy_file( aInput, aOutput, std::filesystem::copy_options::overwrite_existing );

        report.TargetVersion = doc.Version;
        return report;
    }

    report.Warnings = ApplyDowngradeRules( doc, target );
    ensureVersion( doc, resolved );
    report.TargetVersion = doc.Version;
    report.Changed = true;

    for( const std::string& warning : report.Warnings )
        std::cerr << "warning: " << aInput.string() << ": " << warning << '\n';

    WriteTextFile( aOutput, SEXPR::Format( doc.Root.get() ) );
    return report;
}


bool CONVERTER::isKiCadDocumentPath( const std::filesystem::path& aPath ) const
{
    std::string ext = Lower( aPath.extension().string() );
    return ext == ".kicad_sch" || ext == ".kicad_pcb" || ext == ".kicad_sym"
           || ext == ".kicad_mod" || ext == ".kicad_dru" || ext == ".kicad_wks";
}


bool CONVERTER::isKiCadProjectFilePath( const std::filesystem::path& aPath ) const
{
    std::string name = Lower( aPath.filename().string() );

    // Keep only source files needed to reopen and edit the converted KiCad project.
    if( name.empty() || StartsWith( name, ".#" ) || EndsWith( name, "~" ) )
        return false;

    std::string ext = Lower( aPath.extension().string() );

    if( ext == ".bak" || ext == ".backup" || ext == ".bck" || ext == ".orig"
        || ext == ".tmp" || ext == ".temp" )
    {
        return false;
    }

    static const std::set<std::string> modelExts = {
        ".step", ".stp", ".wrl", ".iges", ".igs", ".stl", ".obj"
    };

    return name == "fp-lib-table" || name == "sym-lib-table" || ext == ".kicad_pro"
           || ext == ".kicad_prl" || isKiCadDocumentPath( aPath )
           || modelExts.count( ext ) != 0;
}


bool CONVERTER::isExcludedProjectDirName( const std::string& aName ) const
{
    std::string name = Lower( aName );

    static const std::set<std::string> excluded = {
        ".git", ".svn", ".hg", ".history", ".backup", "__pycache__", "history",
        "histories", "backup", "backups", "archive", "archives", "old", "gerber",
        "gerbers", "gerberfiles", "gerber_files", "fab", "fabrication", "outputs",
        "production", "plot", "plots", "export", "exports", "bom", "ibom",
        "assembly", "jlcpcb", "oshpark"
    };

    return excluded.count( name ) || name.find( "backup" ) != std::string::npos
           || name.find( "history" ) != std::string::npos
           || name.find( "gerber" ) != std::string::npos;
}


std::vector<std::filesystem::path> CONVERTER::copyProjectTree( const std::filesystem::path& aInput,
                                                               const std::filesystem::path& aOutput ) const
{
    std::filesystem::path src = std::filesystem::absolute( aInput );
    std::filesystem::path dest = std::filesystem::absolute( aOutput );

    if( src == dest )
        throw std::runtime_error( "output directory must differ from input directory" );

    // Copy only editable KiCad project inputs, not generated manufacturing outputs.
    std::vector<std::filesystem::path> copied;

    std::filesystem::recursive_directory_iterator it( src );
    const std::filesystem::recursive_directory_iterator end;

    for( ; it != end; ++it )
    {
        const std::filesystem::directory_entry& entry = *it;
        std::filesystem::path rel = std::filesystem::relative( entry.path(), src );

        if( entry.is_directory() )
        {
            if( isExcludedProjectDirName( entry.path().filename().string() ) )
            {
                it.disable_recursion_pending();
                continue;
            }

            continue;
        }

        if( !entry.is_regular_file() || !isKiCadProjectFilePath( entry.path() ) )
            continue;

        std::filesystem::path out = dest / rel;
        std::filesystem::create_directories( out.parent_path() );
        std::filesystem::copy_file( entry.path(), out, std::filesystem::copy_options::overwrite_existing );
        copied.push_back( out );
    }

    return copied;
}


std::filesystem::path CONVERTER::versionedOutputPath( const std::filesystem::path& aPath,
                                                      const std::string& aTarget ) const
{
    std::string label = TargetVersionSuffix( aTarget );

    if( label.empty() )
        return aPath;

    std::string stem = aPath.stem().string();

    if( EndsWith( Lower( stem ), Lower( "_" + label ) ) )
        return aPath;

    return aPath.parent_path() / ( stem + "_" + label + aPath.extension().string() );
}


void CONVERTER::writeReport( const std::filesystem::path& aPath,
                             const std::vector<FILE_REPORT>& aReports ) const
{
    WriteTextFile( aPath, FormatReportsJson( aReports ) );
}


void CONVERTER::ensureLegacyProjectLocalSettings( const std::filesystem::path& aPath,
                                                  const std::string& aTargetSuffix ) const
{
    // KiCad 7/8 may hide converted through-hole pads without legacy PRL visibility data.
    int metaVersion = aTargetSuffix == "V8" ? 3 : 4;

    std::ostringstream out;
    out << "{\n";
    out << "  \"board\": {\n";
    out << "    \"visible_items\": [\n";

    const int items[] = {
        0, 1, 2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 15, 16, 17, 18, 19,
        20, 21, 23, 24, 25, 26, 27, 28, 29, 30, 32, 33, 34, 35, 36, 39, 40, 41
    };

    for( size_t i = 0; i < sizeof( items ) / sizeof( items[0] ); ++i )
    {
        out << "      " << items[i];

        if( i + 1 < sizeof( items ) / sizeof( items[0] ) )
            out << ",";

        out << "\n";
    }

    out << "    ],\n";
    out << "    \"visible_layers\": \"ffffffff_ffffffff_ffffffff_ffffffff\"\n";
    out << "  },\n";
    out << "  \"meta\": {\n";
    out << "    \"filename\": \"" << JsonEscape( aPath.filename().string() ) << "\",\n";
    out << "    \"version\": " << metaVersion << "\n";
    out << "  }\n";
    out << "}\n";

    WriteTextFile( aPath, out.str() );
}


int CONVERTER::runConvert( const std::vector<std::string>& aArgs )
{
    std::string target;
    std::string reportPath;
    std::vector<std::string> positional;

    for( size_t i = 0; i < aArgs.size(); ++i )
    {
        if( aArgs[i] == "--target-version" && i + 1 < aArgs.size() )
            target = aArgs[++i];
        else if( aArgs[i] == "--report" && i + 1 < aArgs.size() )
            reportPath = aArgs[++i];
        else
            positional.push_back( aArgs[i] );
    }

    if( target.empty() )
        throw std::runtime_error( "--target-version is required" );

    if( positional.size() != 2 )
        throw std::runtime_error( "convert requires input and output paths" );

    std::filesystem::path input = positional[0];
    std::filesystem::path output = versionedOutputPath( positional[1], target );

    std::vector<FILE_REPORT> reports;

    if( std::filesystem::is_directory( input ) || Lower( input.extension().string() ) == ".kicad_pro" )
    {
        std::filesystem::path srcDir = std::filesystem::is_directory( input ) ? input : input.parent_path();
        std::vector<std::filesystem::path> copied = copyProjectTree( srcDir, output );

        for( const std::filesystem::path& path : copied )
        {
            if( isKiCadDocumentPath( path ) )
                reports.push_back( normalizeFile( path, path, target ) );
        }

        std::string suffix = TargetVersionSuffix( target );

        if( suffix == "V7" || suffix == "V8" )
        {
            for( const std::filesystem::path& path : copied )
            {
                if( Lower( path.extension().string() ) == ".kicad_pcb" )
                    ensureLegacyProjectLocalSettings( ReplaceExtension( path, ".kicad_prl" ), suffix );
            }
        }
    }
    else
    {
        if( std::filesystem::absolute( input ) == std::filesystem::absolute( output ) )
            throw std::runtime_error( "output file must differ from input file" );

        reports.push_back( normalizeFile( input, output, target ) );

        std::string suffix = TargetVersionSuffix( target );

        if( ( suffix == "V7" || suffix == "V8" ) && Lower( output.extension().string() ) == ".kicad_pcb" )
            ensureLegacyProjectLocalSettings( ReplaceExtension( output, ".kicad_prl" ), suffix );
    }

    if( !reportPath.empty() )
        writeReport( reportPath, reports );

    size_t changed = 0;

    for( const FILE_REPORT& report : reports )
    {
        if( report.Changed )
            ++changed;
    }

    std::cout << "wrote " << output.string() << "; normalized " << changed << " KiCad file(s)\n";
    return 0;
}


std::vector<FILE_REPORT> CONVERTER::inspectPath( const std::filesystem::path& aPath ) const
{
    std::vector<FILE_REPORT> reports;

    if( !std::filesystem::is_directory( aPath ) && Lower( aPath.extension().string() ) != ".kicad_pro" )
    {
        DOCUMENT doc = loadDocument( aPath );
        reports.push_back( FILE_REPORT{ aPath.string(), KindName( doc.Kind ), doc.Version, "", false, {} } );
        return reports;
    }

    std::filesystem::path root = std::filesystem::is_directory( aPath ) ? aPath : aPath.parent_path();

    for( const std::filesystem::directory_entry& entry :
            std::filesystem::recursive_directory_iterator( root ) )
    {
        if( !entry.is_regular_file() || !isKiCadDocumentPath( entry.path() ) )
            continue;

        DOCUMENT doc = loadDocument( entry.path() );
        reports.push_back( FILE_REPORT{ entry.path().string(), KindName( doc.Kind ), doc.Version, "", false, {} } );
    }

    return reports;
}


int CONVERTER::runInspect( const std::vector<std::string>& aArgs )
{
    if( aArgs.size() != 1 )
        throw std::runtime_error( "inspect requires one input path" );

    std::cout << FormatReportsJson( inspectPath( aArgs[0] ) );
    return 0;
}

} // namespace KICAD_BACKPORT

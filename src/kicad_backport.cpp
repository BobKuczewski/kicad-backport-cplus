#include "kicad_backport/kicad_backport.h"
#include "kicad_backport/kicad_backport_report.h"
#include "kicad_backport/kicad_backport_rules.h"
#include "kicad_backport/kicad_backport_util.h"
#include "kicad_backport/kicad_backport_versions.h"

#include <algorithm>
#include <chrono>
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <thread>


namespace KICAD_BACKPORT
{

namespace
{

using CLOCK = std::chrono::steady_clock;

long long elapsedMicros( CLOCK::time_point aStart, CLOCK::time_point aEnd )
{
    return std::chrono::duration_cast<std::chrono::microseconds>( aEnd - aStart ).count();
}


bool timingEnabled()
{
#if defined( _MSC_VER )
    char* value = nullptr;
    size_t len = 0;
    _dupenv_s( &value, &len, "KICAD_BACKPORT_TIMING" );
    bool enabled = value && value[0] && std::string( value ) != "0";
    std::free( value );
    return enabled;
#else
    const char* value = std::getenv( "KICAD_BACKPORT_TIMING" );
    return value && value[0] && std::string( value ) != "0";
#endif
}


DOCUMENT loadDocumentImpl( const std::filesystem::path& aPath, long long* aReadUs,
                           long long* aParseUs )
{
    DOCUMENT doc;
    doc.Path = aPath;

    CLOCK::time_point readStart = CLOCK::now();
    std::string text = ReadTextFile( aPath );
    CLOCK::time_point readEnd = CLOCK::now();

    doc.SourceBytes = text.size();

    CLOCK::time_point parseStart = CLOCK::now();
    doc.Root = SEXPR::Parse( text );
    CLOCK::time_point parseEnd = CLOCK::now();

    doc.Kind = DetectKind( aPath, doc.Root->Head() );

    if( SEXPR::NODE* version = doc.Root->ChildList( "version" ) )
        doc.Version = version->AtomAt( 1 );

    if( aReadUs )
        *aReadUs = elapsedMicros( readStart, readEnd );

    if( aParseUs )
        *aParseUs = elapsedMicros( parseStart, parseEnd );

    return doc;
}


void printTiming( const std::filesystem::path& aPath, long long aReadUs, long long aParseUs,
                  long long aRulesUs, long long aFormatUs, long long aWriteUs,
                  long long aTotalUs )
{
    auto ms = []( long long us )
    {
        return static_cast<double>( us ) / 1000.0;
    };

    std::cerr << "timing: " << aPath.string()
              << " read=" << ms( aReadUs )
              << "ms parse=" << ms( aParseUs )
              << "ms rules=" << ms( aRulesUs )
              << "ms format=" << ms( aFormatUs )
              << "ms write=" << ms( aWriteUs )
              << "ms total=" << ms( aTotalUs ) << "ms\n";
}

} // namespace

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
    std::cerr << "  kicad-backport convert [--quiet] --target-version <7.0|8.0|9.0|10.0|number> <input> <output>\n";
    std::cerr << "  kicad-backport inspect <input>\n";
    std::cerr << "  kicad-backport version\n";
}


DOCUMENT CONVERTER::loadDocument( const std::filesystem::path& aPath ) const
{
    return loadDocumentImpl( aPath, nullptr, nullptr );
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
                                      const std::string& aTarget,
                                      bool aPrintWarnings )
{
    const bool emitTiming = timingEnabled();
    CLOCK::time_point totalStart = CLOCK::now();
    long long readUs = 0;
    long long parseUs = 0;
    long long rulesUs = 0;
    long long formatUs = 0;
    long long writeUs = 0;

    DOCUMENT doc = loadDocumentImpl( aInput, &readUs, &parseUs );
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

    CLOCK::time_point rulesStart = CLOCK::now();
    report.Warnings = ApplyDowngradeRules( doc, target );
    CLOCK::time_point rulesEnd = CLOCK::now();
    rulesUs = elapsedMicros( rulesStart, rulesEnd );

    ensureVersion( doc, resolved );
    report.TargetVersion = doc.Version;
    report.Changed = true;

    if( aPrintWarnings )
    {
        for( const std::string& warning : report.Warnings )
            std::cerr << "warning: " << aInput.string() << ": " << warning << '\n';
    }

    size_t reserveBytes = doc.SourceBytes + doc.SourceBytes / 10 + 1024;

    CLOCK::time_point formatStart = CLOCK::now();
    std::string formatted = SEXPR::Format( doc.Root.get(), reserveBytes );
    CLOCK::time_point formatEnd = CLOCK::now();
    formatUs = elapsedMicros( formatStart, formatEnd );

    CLOCK::time_point writeStart = CLOCK::now();
    WriteTextFile( aOutput, formatted );
    CLOCK::time_point writeEnd = CLOCK::now();
    writeUs = elapsedMicros( writeStart, writeEnd );

    if( emitTiming )
        printTiming( aInput, readUs, parseUs, rulesUs, formatUs, writeUs,
                     elapsedMicros( totalStart, CLOCK::now() ) );

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


std::vector<PROJECT_COPY_ENTRY> CONVERTER::copyProjectTree( const std::filesystem::path& aInput,
                                                            const std::filesystem::path& aOutput ) const
{
    std::filesystem::path src = std::filesystem::absolute( aInput );
    std::filesystem::path dest = std::filesystem::absolute( aOutput );

    if( src == dest )
        throw std::runtime_error( "output directory must differ from input directory" );

    // Copy only editable KiCad project inputs, not generated manufacturing outputs.
    std::vector<PROJECT_COPY_ENTRY> copied;

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

        bool isDocument = isKiCadDocumentPath( entry.path() );

        if( !isDocument )
            std::filesystem::copy_file( entry.path(), out, std::filesystem::copy_options::overwrite_existing );

        copied.push_back( PROJECT_COPY_ENTRY{ entry.path(), out, isDocument } );
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


void printReportWarnings( const std::vector<FILE_REPORT>& aReports )
{
    for( const FILE_REPORT& report : aReports )
    {
        for( const std::string& warning : report.Warnings )
            std::cerr << "warning: " << report.Path << ": " << warning << '\n';
    }
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
    CLOCK::time_point convertStart = CLOCK::now();
    std::string target;
    std::string reportPath;
    bool quiet = false;
    std::vector<std::string> positional;

    for( size_t i = 0; i < aArgs.size(); ++i )
    {
        if( aArgs[i] == "--target-version" && i + 1 < aArgs.size() )
            target = aArgs[++i];
        else if( aArgs[i] == "--report" && i + 1 < aArgs.size() )
            reportPath = aArgs[++i];
        else if( aArgs[i] == "--quiet" || aArgs[i] == "-q" )
            quiet = true;
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
        std::vector<PROJECT_COPY_ENTRY> copied = copyProjectTree( srcDir, output );
        std::vector<PROJECT_COPY_ENTRY> documents;

        for( const PROJECT_COPY_ENTRY& entry : copied )
        {
            if( entry.IsDocument )
                documents.push_back( entry );
        }

        if( documents.size() <= 1 )
        {
            for( const PROJECT_COPY_ENTRY& entry : documents )
                reports.push_back( normalizeFile( entry.Source, entry.Output, target, false ) );
        }
        else
        {
            reports.resize( documents.size() );
            std::vector<size_t> processOrder( documents.size() );

            for( size_t i = 0; i < processOrder.size(); ++i )
                processOrder[i] = i;

            std::sort( processOrder.begin(), processOrder.end(),
                       [&]( size_t aLeft, size_t aRight )
                       {
                           std::error_code leftError;
                           std::error_code rightError;
                           uintmax_t leftSize = std::filesystem::file_size( documents[aLeft].Source,
                                                                             leftError );
                           uintmax_t rightSize = std::filesystem::file_size( documents[aRight].Source,
                                                                              rightError );

                           if( leftError )
                               leftSize = 0;

                           if( rightError )
                               rightSize = 0;

                           return leftSize > rightSize;
                       } );

            std::atomic<size_t> nextIndex{ 0 };
            std::mutex errorMutex;
            std::exception_ptr firstError;
            unsigned int hardwareThreads = std::thread::hardware_concurrency();
            size_t preferredThreads = hardwareThreads == 0 ? 4 : std::max<size_t>( 2, hardwareThreads / 2 );
            size_t workerCount = std::min<size_t>( documents.size(),
                    std::min<size_t>( 4, preferredThreads ) );

            std::vector<std::thread> workers;
            workers.reserve( workerCount );

            for( size_t worker = 0; worker < workerCount; ++worker )
            {
                workers.emplace_back(
                        [&, target]()
                        {
                            while( true )
                            {
                                size_t orderIndex = nextIndex.fetch_add( 1 );

                                if( orderIndex >= processOrder.size() )
                                    return;

                                size_t index = processOrder[orderIndex];

                                try
                                {
                                    const PROJECT_COPY_ENTRY& entry = documents[index];
                                    reports[index] = normalizeFile( entry.Source, entry.Output,
                                                                    target, false );
                                }
                                catch( ... )
                                {
                                    std::lock_guard<std::mutex> lock( errorMutex );

                                    if( !firstError )
                                        firstError = std::current_exception();

                                    return;
                                }
                            }
                        } );
            }

            for( std::thread& worker : workers )
                worker.join();

            if( firstError )
                std::rethrow_exception( firstError );
        }

        if( !quiet )
            printReportWarnings( reports );

        std::string suffix = TargetVersionSuffix( target );

        if( suffix == "V7" || suffix == "V8" )
        {
            for( const PROJECT_COPY_ENTRY& entry : copied )
            {
                if( Lower( entry.Output.extension().string() ) == ".kicad_pcb" )
                    ensureLegacyProjectLocalSettings( ReplaceExtension( entry.Output, ".kicad_prl" ), suffix );
            }
        }
    }
    else
    {
        if( std::filesystem::absolute( input ) == std::filesystem::absolute( output ) )
            throw std::runtime_error( "output file must differ from input file" );

        reports.push_back( normalizeFile( input, output, target, !quiet ) );

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

    if( timingEnabled() )
        std::cerr << "timing: convert total="
                  << static_cast<double>( elapsedMicros( convertStart, CLOCK::now() ) ) / 1000.0
                  << "ms\n";

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

#include "kicad_backport/kicad_backport_upgrade.h"
#include "kicad_backport/kicad_backport_util.h"
#include "internal/kicad_backport_rule_rewriters.h"

#include <map>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <set>
#include <sstream>
#include <string_view>


namespace KICAD_BACKPORT
{
namespace
{

bool containsString( const std::set<std::string>& aSet, std::string_view aValue )
{
    for( const std::string& item : aSet )
    {
        if( item == aValue )
            return true;
    }

    return false;
}


bool parseKiCadBool( const std::string& aValue, bool aDefault )
{
    std::string value = Lower( aValue );

    if( value == "yes" || value == "true" || value == "1" )
        return true;

    if( value == "no" || value == "false" || value == "0" )
        return false;

    return aDefault;
}


std::unique_ptr<SEXPR::NODE> listNode( const std::string& aHead )
{
    std::unique_ptr<SEXPR::NODE> node = SEXPR::NODE::MakeList();
    node->Children.push_back( SEXPR::NODE::MakeAtom( aHead ) );
    return node;
}


double toDouble( const std::string& aValue, double aDefault = 0.0 )
{
    char* end = nullptr;
    double value = std::strtod( aValue.c_str(), &end );

    if( end == aValue.c_str() || ( end && *end != '\0' ) )
        return aDefault;

    return value;
}


std::string formatDouble( double aValue )
{
    if( std::abs( aValue ) < 0.0000000005 )
        aValue = 0.0;

    std::ostringstream out;
    out << std::fixed << std::setprecision( 9 ) << aValue;
    std::string text = out.str();

    while( text.size() > 1 && text.back() == '0' )
        text.pop_back();

    if( !text.empty() && text.back() == '.' )
        text.pop_back();

    return text.empty() ? "0" : text;
}


int renameChildHeadInParents( SEXPR::NODE* aRoot, const std::set<std::string>& aParents,
                              const std::string& aFrom, const std::string& aTo )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int changed = 0;

    if( containsString( aParents, aRoot->HeadView() ) )
    {
        for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        {
            if( child && !child->IsAtom() && child->HeadView() == aFrom
                && child->SetAtomAt( 0, aTo, false ) )
            {
                ++changed;
            }
        }
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        changed += renameChildHeadInParents( child.get(), aParents, aFrom, aTo );

    return changed;
}


int upgradeTextBoxStartEnd( SEXPR::NODE* aRoot )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int changed = 0;

    if( aRoot->HeadView() == "text_box" || aRoot->HeadView() == "textbox" )
    {
        SEXPR::NODE* start = aRoot->ChildList( "start" );
        SEXPR::NODE* end = aRoot->ChildList( "end" );

        if( start && end && !aRoot->ChildList( "at" ) && !aRoot->ChildList( "size" ) )
        {
            double x1 = toDouble( start->AtomAt( 1 ) );
            double y1 = toDouble( start->AtomAt( 2 ) );
            double x2 = toDouble( end->AtomAt( 1 ) );
            double y2 = toDouble( end->AtomAt( 2 ) );

            start->SetAtomAt( 0, "at", false );
            end->SetAtomAt( 0, "size", false );
            end->SetAtomAt( 1, formatDouble( x2 - x1 ), false );
            end->SetAtomAt( 2, formatDouble( y2 - y1 ), false );
            ++changed;
        }
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        changed += upgradeTextBoxStartEnd( child.get() );

    return changed;
}


int expandPresenceAtomsInParents( SEXPR::NODE* aRoot, const std::set<std::string>& aParents,
                                  const std::set<std::string>& aAtoms )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int changed = 0;

    if( containsString( aParents, aRoot->HeadView() ) )
    {
        for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        {
            if( child && child->IsAtom() && containsString( aAtoms, child->Atom ) )
            {
                std::string head( child->Atom );
                std::unique_ptr<SEXPR::NODE> list = listNode( head );
                list->Children.push_back( SEXPR::NODE::MakeAtom( "yes" ) );
                child = std::move( list );
                ++changed;
            }
        }
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        changed += expandPresenceAtomsInParents( child.get(), aParents, aAtoms );

    return changed;
}


int expandFontStyleAtoms( SEXPR::NODE* aRoot )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int changed = 0;

    if( aRoot->HeadView() == "font" )
    {
        for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        {
            if( child && child->IsAtom() && ( child->Atom == "bold" || child->Atom == "italic" ) )
            {
                std::string head( child->Atom );
                std::unique_ptr<SEXPR::NODE> list = listNode( head );
                list->Children.push_back( SEXPR::NODE::MakeAtom( "yes" ) );
                child = std::move( list );
                ++changed;
            }
        }
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        changed += expandFontStyleAtoms( child.get() );

    return changed;
}


int normalizeBoolValues( SEXPR::NODE* aRoot, const std::set<std::string>& aHeads )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int changed = 0;

    if( containsString( aHeads, aRoot->HeadView() ) && aRoot->Children.size() > 1 )
    {
        std::string value = Lower( aRoot->AtomAt( 1 ) );

        if( value == "true" || value == "1" )
        {
            if( aRoot->SetAtomAt( 1, "yes", false ) )
                ++changed;
        }
        else if( value == "false" || value == "0" )
        {
            if( aRoot->SetAtomAt( 1, "no", false ) )
                ++changed;
        }
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        changed += normalizeBoolValues( child.get(), aHeads );

    return changed;
}


int removeChildrenFromParents( SEXPR::NODE* aRoot, const std::set<std::string>& aParents,
                               const std::set<std::string>& aChildren )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int removed = 0;

    if( containsString( aParents, aRoot->HeadView() ) )
    {
        std::pmr::vector<std::unique_ptr<SEXPR::NODE>> kept( aRoot->Children.get_allocator() );
        kept.reserve( aRoot->Children.size() );

        for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        {
            if( child && !child->IsAtom() && containsString( aChildren, child->HeadView() ) )
            {
                ++removed;
                continue;
            }

            kept.push_back( std::move( child ) );
        }

        aRoot->Children = std::move( kept );
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        removed += removeChildrenFromParents( child.get(), aParents, aChildren );

    return removed;
}


int removeIdFromStandardProperties( SEXPR::NODE* aRoot )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int changed = 0;

    if( aRoot->HeadView() == "property" )
    {
        std::pmr::vector<std::unique_ptr<SEXPR::NODE>> kept( aRoot->Children.get_allocator() );
        kept.reserve( aRoot->Children.size() );

        for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        {
            if( child && !child->IsAtom() && child->HeadView() == "id" )
            {
                ++changed;
                continue;
            }

            kept.push_back( std::move( child ) );
        }

        aRoot->Children = std::move( kept );
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        changed += removeIdFromStandardProperties( child.get() );

    return changed;
}


int moveEffectsHideToProperty( SEXPR::NODE* aRoot )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int changed = 0;

    if( aRoot->HeadView() == "property" && !aRoot->ChildList( "hide" ) )
    {
        SEXPR::NODE* effects = aRoot->ChildList( "effects" );

        if( effects )
        {
            std::pmr::vector<std::unique_ptr<SEXPR::NODE>> kept( effects->Children.get_allocator() );
            kept.reserve( effects->Children.size() );
            bool hidden = false;

            for( std::unique_ptr<SEXPR::NODE>& child : effects->Children )
            {
                if( child && child->IsAtom() && child->Atom == "hide" )
                {
                    hidden = true;
                    ++changed;
                    continue;
                }

                kept.push_back( std::move( child ) );
            }

            effects->Children = std::move( kept );

            if( hidden )
            {
                std::unique_ptr<SEXPR::NODE> hide = listNode( "hide" );
                hide->Children.push_back( SEXPR::NODE::MakeAtom( "yes" ) );
                aRoot->Children.push_back( std::move( hide ) );
                ++changed;
            }
        }
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        changed += moveEffectsHideToProperty( child.get() );

    return changed;
}


bool isBoardNetDeclaration( SEXPR::NODE* aNode, std::string* aName )
{
    if( !aNode || aNode->IsAtom() || aNode->HeadView() != "net" )
        return false;

    if( !aNode->AtomAtView( 1 ).empty() && !aNode->AtomAtView( 2 ).empty() )
    {
        *aName = aNode->AtomAt( 2 );
        return true;
    }

    return false;
}


int upgradeBoardNetCodesToNames( SEXPR::NODE* aRoot )
{
    if( !aRoot || aRoot->IsAtom() || aRoot->HeadView() != "kicad_pcb" )
        return 0;

    std::map<std::string, std::string> codeToName;

    for( const std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
    {
        std::string name;

        if( isBoardNetDeclaration( child.get(), &name ) )
            codeToName[child->AtomAt( 1 )] = name;
    }

    if( codeToName.empty() )
        return 0;

    auto visit = [&]( auto&& self, SEXPR::NODE* node, std::string_view parentHead ) -> int
    {
        if( !node || node->IsAtom() )
            return 0;

        int changed = 0;

        if( node->HeadView() == "net" && !node->AtomAtView( 1 ).empty() )
        {
            auto it = codeToName.find( node->AtomAt( 1 ) );

            if( it != codeToName.end() && parentHead != "kicad_pcb" )
            {
                node->SetAtomAt( 1, it->second, true );

                if( node->Children.size() > 2 )
                    node->Children.resize( 2 );

                ++changed;
            }
        }

        std::string head = node->Head();

        for( std::unique_ptr<SEXPR::NODE>& child : node->Children )
            changed += self( self, child.get(), head );

        return changed;
    };

    return visit( visit, aRoot, std::string_view() );
}


int upgradePcbPageToPaper( SEXPR::NODE* aRoot )
{
    if( !aRoot || aRoot->IsAtom() || aRoot->HeadView() != "kicad_pcb" )
        return 0;

    int changed = 0;

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
    {
        if( !child || child->IsAtom() || child->HeadView() != "page" )
            continue;

        if( child->SetAtomAt( 0, "paper", false ) )
            ++changed;

        if( !child->AtomAtView( 1 ).empty() && child->SetAtomAt( 1, child->AtomAt( 1 ), true ) )
            ++changed;
    }

    return changed;
}


std::unique_ptr<SEXPR::NODE> xyListNode( const std::string& aHead, double aX, double aY )
{
    std::unique_ptr<SEXPR::NODE> node = listNode( aHead );
    node->Children.push_back( SEXPR::NODE::MakeAtom( formatDouble( aX ) ) );
    node->Children.push_back( SEXPR::NODE::MakeAtom( formatDouble( aY ) ) );
    return node;
}


int upgradeLegacyArcAngles( SEXPR::NODE* aRoot )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int changed = 0;

    if( aRoot->HeadView() == "fp_arc" || aRoot->HeadView() == "gr_arc" )
    {
        SEXPR::NODE* center = aRoot->ChildList( "start" );
        SEXPR::NODE* start = aRoot->ChildList( "end" );
        SEXPR::NODE* angle = aRoot->ChildList( "angle" );

        if( center && start && angle && !angle->AtomAtView( 1 ).empty() )
        {
            double cx = toDouble( center->AtomAt( 1 ) );
            double cy = toDouble( center->AtomAt( 2 ) );
            double sx = toDouble( start->AtomAt( 1 ) );
            double sy = toDouble( start->AtomAt( 2 ) );
            double radians = toDouble( angle->AtomAt( 1 ) ) * 3.14159265358979323846 / 180.0;
            double vx = sx - cx;
            double vy = sy - cy;

            auto rotateX = [&]( double a ) { return cx + vx * std::cos( a ) - vy * std::sin( a ); };
            auto rotateY = [&]( double a ) { return cy + vx * std::sin( a ) + vy * std::cos( a ); };

            center->SetAtomAt( 1, formatDouble( sx ), false );
            center->SetAtomAt( 2, formatDouble( sy ), false );
            start->SetAtomAt( 1, formatDouble( rotateX( radians ) ), false );
            start->SetAtomAt( 2, formatDouble( rotateY( radians ) ), false );

            std::unique_ptr<SEXPR::NODE> mid = xyListNode( "mid", rotateX( radians / 2.0 ),
                                                           rotateY( radians / 2.0 ) );

            for( auto it = aRoot->Children.begin(); it != aRoot->Children.end(); ++it )
            {
                if( it->get() == center )
                {
                    aRoot->Children.insert( it + 1, std::move( mid ) );
                    break;
                }
            }

            for( auto it = aRoot->Children.begin(); it != aRoot->Children.end(); )
            {
                if( it->get() == angle )
                    it = aRoot->Children.erase( it );
                else
                    ++it;
            }

            ++changed;
        }
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        changed += upgradeLegacyArcAngles( child.get() );

    return changed;
}


int removeLegacyGraphicLineAngles( SEXPR::NODE* aRoot )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int changed = 0;

    if( aRoot->HeadView() == "fp_line" || aRoot->HeadView() == "gr_line" )
    {
        for( auto it = aRoot->Children.begin(); it != aRoot->Children.end(); )
        {
            if( *it && !( *it )->IsAtom() && ( *it )->HeadView() == "angle" )
            {
                it = aRoot->Children.erase( it );
                ++changed;
            }
            else
            {
                ++it;
            }
        }
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        changed += removeLegacyGraphicLineAngles( child.get() );

    return changed;
}


void appendWarning( std::vector<std::string>& aWarnings, int aCount, const std::string& aText )
{
    if( aCount > 0 )
        aWarnings.push_back( aText );
}

} // namespace


std::vector<std::string> ApplyUpgradeRules( DOCUMENT& aDocument, int aTarget )
{
    std::vector<std::string> warnings;

    if( !aDocument.Root )
        return warnings;

    if( aTarget < 20211014 )
        return warnings;

    static const std::set<std::string> schematicTstampParents = {
        "symbol", "sheet", "junction", "no_connect", "wire", "bus", "polyline", "text",
        "text_box", "label", "global_label", "hierarchical_label", "directive_label",
        "image", "sheet_instances", "path", "instance", "property"
    };

    static const std::set<std::string> boardTstampParents = {
        "footprint", "module", "pad", "via", "segment", "arc", "zone", "group",
        "generated", "gr_line", "gr_arc", "gr_circle", "gr_rect", "gr_poly", "gr_curve",
        "gr_text", "fp_line", "fp_arc", "fp_circle", "fp_rect", "fp_poly", "fp_curve",
        "fp_text", "dimension"
    };

    static const std::set<std::string> boolHeads = {
        "hide", "bold", "italic", "locked", "free", "remove_unused_layers",
        "keep_end_layers", "suppress_zeroes", "keep_text_aligned"
    };

    switch( aDocument.Kind )
    {
    case KIND::SYMBOL_LIBRARY:
    {
        int n = 0;

        if( aTarget >= 20240108 )
        {
            n = expandFontStyleAtoms( aDocument.Root.get() );
            appendWarning( warnings, n, "upgraded symbol library font style atoms to boolean lists" );
        }

        if( aTarget >= 20241004 )
        {
            n = expandPresenceAtomsInParents( aDocument.Root.get(),
                                              { "pin_names", "pin_numbers" }, { "hide" } );
            appendWarning( warnings, n, "upgraded symbol pin visibility atoms to boolean lists" );
        }

        if( aTarget >= 20241209 )
        {
            n = moveEffectsHideToProperty( aDocument.Root.get() );
            appendWarning( warnings, n, "moved symbol property hide flags out of effects" );
        }

        n = removeIdFromStandardProperties( aDocument.Root.get() );
        appendWarning( warnings, n, "removed legacy symbol property ids" );
        break;
    }

    case KIND::SCHEMATIC:
    {
        int n = renameChildHeadInParents( aDocument.Root.get(), schematicTstampParents,
                                          "tstamp", "uuid" );
        appendWarning( warnings, n, "renamed schematic tstamp fields to uuid" );

        n = renameChildHeadInParents( aDocument.Root.get(), { "kicad_sch" },
                                      "netclass_flag", "directive_label" );
        appendWarning( warnings, n, "renamed schematic netclass flags to directive labels" );

        n = upgradeTextBoxStartEnd( aDocument.Root.get() );
        appendWarning( warnings, n, "upgraded schematic text box start/end fields to at/size" );

        if( aTarget >= 20240108 )
        {
            n = expandFontStyleAtoms( aDocument.Root.get() );
            appendWarning( warnings, n, "upgraded schematic font style atoms to boolean lists" );
        }

        if( aTarget >= 20241004 )
        {
            n = expandPresenceAtomsInParents( aDocument.Root.get(),
                                              { "pin_names", "pin_numbers" }, { "hide" } );
            appendWarning( warnings, n, "upgraded schematic symbol pin visibility atoms to boolean lists" );
        }

        if( aTarget >= 20241209 )
        {
            n = moveEffectsHideToProperty( aDocument.Root.get() );
            appendWarning( warnings, n, "moved schematic property hide flags out of effects" );
        }

        n = removeIdFromStandardProperties( aDocument.Root.get() );
        appendWarning( warnings, n, "removed legacy schematic property ids" );
        break;
    }

    case KIND::BOARD:
    case KIND::FOOTPRINT:
    {
        int n = 0;

        n = removeChildrenFromParents( aDocument.Root.get(), { "kicad_pcb" }, { "host" } );
        appendWarning( warnings, n, "removed legacy PCB host metadata during upgrade" );

        n = upgradePcbPageToPaper( aDocument.Root.get() );
        appendWarning( warnings, n, "renamed legacy PCB page settings to paper" );

        n = upgradeLegacyArcAngles( aDocument.Root.get() );
        appendWarning( warnings, n, "upgraded legacy PCB arc angle fields to midpoint arcs" );

        n = removeLegacyGraphicLineAngles( aDocument.Root.get() );
        appendWarning( warnings, n, "removed legacy PCB line angle fields" );

        n = RULE_REWRITERS::ensureZoneFilledAreasThickness( aDocument.Root.get() );
        appendWarning( warnings, n, "tagged legacy cached zone fills as polygon fills" );

        n = RULE_REWRITERS::ensureZoneFilledPolygonLayers( aDocument.Root.get() );
        appendWarning( warnings, n, "added zone layers to legacy cached polygon fills" );

        if( aTarget >= 20231231 )
        {
            n = renameChildHeadInParents( aDocument.Root.get(), boardTstampParents,
                                          "tstamp", "uuid" );
            appendWarning( warnings, n, "renamed PCB tstamp fields to uuid" );
        }

        n = expandFontStyleAtoms( aDocument.Root.get() );
        appendWarning( warnings, n, "upgraded PCB font style atoms to boolean lists" );

        if( aTarget >= 20230410 )
        {
            n = expandPresenceAtomsInParents( aDocument.Root.get(), { "attr" }, { "dnp" } );
            appendWarning( warnings, n, "upgraded footprint dnp atoms to boolean lists" );
        }

        n = normalizeBoolValues( aDocument.Root.get(), boolHeads );
        appendWarning( warnings, n, "normalized PCB boolean values for KiCad 7 syntax" );

        n = removeChildrenFromParents( aDocument.Root.get(), { "footprint", "module" },
                                       { "tedit" } );
        appendWarning( warnings, n, "removed obsolete footprint tedit fields during upgrade" );

        if( aTarget >= 20251028 )
        {
            n = upgradeBoardNetCodesToNames( aDocument.Root.get() );
            appendWarning( warnings, n, "upgraded legacy numeric board net references to net names" );
        }
        break;
    }

    case KIND::WORKSHEET:
        break;

    default:
        break;
    }

    return warnings;
}

} // namespace KICAD_BACKPORT

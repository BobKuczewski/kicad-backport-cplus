#include "internal/kicad_backport_rule_rewriters.h"
#include "kicad_backport/kicad_backport_util.h"

#include <algorithm>
#include <cctype>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>


namespace KICAD_BACKPORT::RULE_REWRITERS
{

// Removes any subtree whose head token is not accepted by the target parser.
int removeDescendantsByHead( SEXPR::NODE* aRoot, const std::set<std::string>& aHeads )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int removed = 0;
    std::vector<std::unique_ptr<SEXPR::NODE>> kept;

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
    {
        if( !child )
            continue;

        if( !child->IsAtom() && aHeads.count( child->Head() ) )
        {
            ++removed;
            continue;
        }

        removed += removeDescendantsByHead( child.get(), aHeads );
        kept.push_back( std::move( child ) );
    }

    aRoot->Children = std::move( kept );
    return removed;
}


int removeChildrenFromParents( SEXPR::NODE* aRoot, const std::set<std::string>& aParents,
                               const std::set<std::string>& aChildren )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int removed = 0;

    if( aParents.count( aRoot->Head() ) )
    {
        std::vector<std::unique_ptr<SEXPR::NODE>> kept;

        for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        {
            if( child && !child->IsAtom() && aChildren.count( child->Head() ) )
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


bool hasChild( SEXPR::NODE* aNode, const std::string& aHead )
{
    return aNode && aNode->ChildList( aHead ) != nullptr;
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


bool isIntegerAtom( const std::string& aValue )
{
    if( aValue.empty() )
        return false;

    size_t pos = 0;

    if( aValue[0] == '-' || aValue[0] == '+' )
        pos = 1;

    if( pos == aValue.size() )
        return false;

    for( ; pos < aValue.size(); ++pos )
    {
        if( !std::isdigit( static_cast<unsigned char>( aValue[pos] ) ) )
            return false;
    }

    return true;
}


int atomToInt( const std::string& aValue, int aDefault = 0 )
{
    try
    {
        return std::stoi( aValue );
    }
    catch( const std::exception& )
    {
        return aDefault;
    }
}


std::unique_ptr<SEXPR::NODE> propertyNode( const std::string& aName, const std::string& aValue )
{
    std::unique_ptr<SEXPR::NODE> node = SEXPR::NODE::MakeList();
    node->Children.push_back( SEXPR::NODE::MakeAtom( "property" ) );
    node->Children.push_back( SEXPR::NODE::MakeAtom( aName, true ) );
    node->Children.push_back( SEXPR::NODE::MakeAtom( aValue, true ) );
    return node;
}


bool isStandardSchProperty( const std::string& aName )
{
    static const std::set<std::string> standard = {
        "Reference", "Value", "Footprint", "Datasheet", "ki_keywords", "ki_fp_filters"
    };

    return standard.count( aName ) != 0;
}


// Older schematic formats require explicit ids on non-standard fields.
int ensureLegacyPropertyIds( SEXPR::NODE* aRoot )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int changed = 0;

    if( aRoot->Head() == "symbol" || aRoot->Head() == "sheet" )
    {
        int nextId = 5;

        for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        {
            if( !child || child->IsAtom() || child->Head() != "property" )
                continue;

            if( isStandardSchProperty( child->AtomAt( 1 ) ) || child->ChildList( "id" ) )
                continue;

            std::unique_ptr<SEXPR::NODE> id = SEXPR::NODE::MakeList();
            id->Children.push_back( SEXPR::NODE::MakeAtom( "id" ) );
            id->Children.push_back( SEXPR::NODE::MakeAtom( std::to_string( nextId++ ) ) );

            size_t insertAt = std::min<size_t>( 3, child->Children.size() );
            child->Children.insert( child->Children.begin() + insertAt, std::move( id ) );
            ++changed;
        }
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        changed += ensureLegacyPropertyIds( child.get() );

    return changed;
}


// KiCad 8/7 store property visibility inside the effects list.
int movePropertyHideToEffects( SEXPR::NODE* aRoot )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int changed = 0;

    if( aRoot->Head() == "property" )
    {
        std::vector<std::unique_ptr<SEXPR::NODE>> kept;
        bool hidden = false;

        for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        {
            if( child && child->IsAtom() && child->Atom == "hide" )
            {
                hidden = true;
                ++changed;
                continue;
            }

            if( child && !child->IsAtom() && child->Head() == "hide" )
            {
                hidden = parseKiCadBool( child->AtomAt( 1 ), true );
                ++changed;
                continue;
            }

            kept.push_back( std::move( child ) );
        }

        aRoot->Children = std::move( kept );

        if( hidden )
        {
            SEXPR::NODE* effects = aRoot->ChildList( "effects" );

            if( effects && !effects->ChildList( "hide" ) )
            {
                effects->Children.push_back( SEXPR::NODE::MakeAtom( "hide" ) );
                ++changed;
            }
        }
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        changed += movePropertyHideToEffects( child.get() );

    return changed;
}


int removeDirectChildrenByHead( SEXPR::NODE* aRoot, const std::string& aHead )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int removed = 0;
    std::vector<std::unique_ptr<SEXPR::NODE>> kept;

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
    {
        if( child && !child->IsAtom() && child->Head() == aHead )
        {
            ++removed;
            continue;
        }

        kept.push_back( std::move( child ) );
    }

    aRoot->Children = std::move( kept );
    return removed;
}


int renameChildHeadInParents( SEXPR::NODE* aRoot, const std::set<std::string>& aParents,
                              const std::string& aFrom, const std::string& aTo )
{
    // Used for UUID-to-tstamp/id compatibility without touching unrelated UUIDs.
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int changed = 0;

    if( aParents.count( aRoot->Head() ) )
    {
        for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        {
            if( child && !child->IsAtom() && child->Head() == aFrom && child->SetAtomAt( 0, aTo, false ) )
                ++changed;
        }
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        changed += renameChildHeadInParents( child.get(), aParents, aFrom, aTo );

    return changed;
}


int removeAtomsFromHeadedLists( SEXPR::NODE* aRoot, const std::set<std::string>& aParents,
                                const std::set<std::string>& aAtoms )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int removed = 0;

    if( aParents.count( aRoot->Head() ) )
    {
        std::vector<std::unique_ptr<SEXPR::NODE>> kept;

        for( size_t i = 0; i < aRoot->Children.size(); ++i )
        {
            std::unique_ptr<SEXPR::NODE>& child = aRoot->Children[i];

            if( i > 0 && child && child->IsAtom() && aAtoms.count( child->Atom ) )
            {
                ++removed;
                continue;
            }

            kept.push_back( std::move( child ) );
        }

        aRoot->Children = std::move( kept );
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        removed += removeAtomsFromHeadedLists( child.get(), aParents, aAtoms );

    return removed;
}


int flattenChildListsToAtomsInParents( SEXPR::NODE* aRoot, const std::set<std::string>& aParents,
                                       const std::set<std::string>& aChildren )
{
    // Older KiCad often stores flags as bare atoms instead of boolean child lists.
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int changed = 0;

    if( aParents.count( aRoot->Head() ) )
    {
        for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        {
            if( child && !child->IsAtom() && aChildren.count( child->Head() ) )
            {
                child = SEXPR::NODE::MakeAtom( child->Head() );
                ++changed;
            }
        }
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        changed += flattenChildListsToAtomsInParents( child.get(), aParents, aChildren );

    return changed;
}


bool tentingBool( SEXPR::NODE* aNode, bool* aSeen )
{
    if( !aNode )
    {
        *aSeen = false;
        return false;
    }

    *aSeen = true;
    std::string value = Lower( aNode->AtomAt( 1 ) );
    return value.empty() || value == "yes" || value == "true" || value == "1";
}


int downgradeTentingToLegacyAtoms( SEXPR::NODE* aRoot )
{
    // Preserve front/back tenting intent using the older atom list form.
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int changed = 0;

    if( aRoot->Head() == "tenting" )
    {
        bool sawFront = false;
        bool sawBack = false;
        bool front = tentingBool( aRoot->ChildList( "front" ), &sawFront );
        bool back = tentingBool( aRoot->ChildList( "back" ), &sawBack );

        if( sawFront || sawBack )
        {
            std::vector<std::unique_ptr<SEXPR::NODE>> children;
            children.push_back( SEXPR::NODE::MakeAtom( "tenting" ) );

            if( front )
                children.push_back( SEXPR::NODE::MakeAtom( "front" ) );

            if( back )
                children.push_back( SEXPR::NODE::MakeAtom( "back" ) );

            if( children.size() == 1 )
                children.push_back( SEXPR::NODE::MakeAtom( "none" ) );

            aRoot->Children = std::move( children );
            return 1;
        }
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        changed += downgradeTentingToLegacyAtoms( child.get() );

    return changed;
}


// New boolean list forms become presence atoms in older file formats.
int downgradeBooleanPresenceNodes( SEXPR::NODE* aRoot, const std::set<std::string>& aHeads )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int changed = 0;
    std::vector<std::unique_ptr<SEXPR::NODE>> kept;

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
    {
        if( !child )
            continue;

        if( !child->IsAtom() && aHeads.count( child->Head() ) && child->Children.size() > 1 )
        {
            std::string value = Lower( child->AtomAt( 1 ) );

            if( value == "yes" || value == "true" || value == "1" )
            {
                child->Children.resize( 1 );
                ++changed;
            }
            else if( value == "no" || value == "false" || value == "0" )
            {
                ++changed;
                continue;
            }
        }

        changed += downgradeBooleanPresenceNodes( child.get(), aHeads );
        kept.push_back( std::move( child ) );
    }

    aRoot->Children = std::move( kept );
    return changed;
}


int downgradeFontStyleListsToAtoms( SEXPR::NODE* aRoot )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int changed = 0;

    if( aRoot->Head() == "font" )
    {
        std::vector<std::unique_ptr<SEXPR::NODE>> kept;

        for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        {
            if( child && !child->IsAtom()
                && ( child->Head() == "bold" || child->Head() == "italic" )
                && child->Children.size() > 1 )
            {
                std::string head = child->Head();
                bool enabled = parseKiCadBool( child->AtomAt( 1 ), true );

                if( enabled )
                    kept.push_back( SEXPR::NODE::MakeAtom( head ) );

                ++changed;
                continue;
            }

            kept.push_back( std::move( child ) );
        }

        aRoot->Children = std::move( kept );
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        changed += downgradeFontStyleListsToAtoms( child.get() );

    return changed;
}


int downgradeBoolListsToAtoms( SEXPR::NODE* aRoot, const std::set<std::string>& aHeads )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int changed = 0;
    std::vector<std::unique_ptr<SEXPR::NODE>> kept;

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
    {
        if( !child )
            continue;

        if( !child->IsAtom() && aHeads.count( child->Head() ) )
        {
            bool enabled = parseKiCadBool( child->AtomAt( 1 ), true );

            if( enabled )
                kept.push_back( SEXPR::NODE::MakeAtom( child->Head() ) );

            ++changed;
            continue;
        }

        changed += downgradeBoolListsToAtoms( child.get(), aHeads );
        kept.push_back( std::move( child ) );
    }

    aRoot->Children = std::move( kept );
    return changed;
}


// KiCad 7 stores reference/value as fp_text instead of footprint properties.
std::unique_ptr<SEXPR::NODE> pcbPropertyToLegacyFPText( std::unique_ptr<SEXPR::NODE> aProperty,
                                                        const std::string& aKind )
{
    std::unique_ptr<SEXPR::NODE> text = SEXPR::NODE::MakeList();
    text->Children.push_back( SEXPR::NODE::MakeAtom( "fp_text" ) );
    text->Children.push_back( SEXPR::NODE::MakeAtom( aKind ) );
    text->Children.push_back( SEXPR::NODE::MakeAtom( aProperty->AtomAt( 2 ), true ) );

    for( size_t i = 3; i < aProperty->Children.size(); ++i )
    {
        std::unique_ptr<SEXPR::NODE>& child = aProperty->Children[i];

        if( !child )
            continue;

        if( !child->IsAtom() && child->Head() == "hide" )
        {
            if( parseKiCadBool( child->AtomAt( 1 ), true ) )
                text->Children.push_back( SEXPR::NODE::MakeAtom( "hide" ) );

            continue;
        }

        text->Children.push_back( std::move( child ) );
    }

    return text;
}


int downgradePCBFootprintFields( SEXPR::NODE* aRoot )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int changed = 0;

    if( aRoot->Head() == "footprint" || aRoot->Head() == "module" )
    {
        std::vector<std::unique_ptr<SEXPR::NODE>> kept;

        for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        {
            if( !child )
                continue;

            if( !child->IsAtom() )
            {
                if( child->Head() == "property" )
                {
                    if( child->AtomAt( 1 ) == "Reference" )
                    {
                        kept.push_back( pcbPropertyToLegacyFPText( std::move( child ), "reference" ) );
                        ++changed;
                        continue;
                    }

                    if( child->AtomAt( 1 ) == "Value" )
                    {
                        kept.push_back( pcbPropertyToLegacyFPText( std::move( child ), "value" ) );
                        ++changed;
                        continue;
                    }

                    if( child->AtomAt( 1 ) == "Description" && child->SetAtomAt( 1, "ki_description", true ) )
                        ++changed;

                    if( child->Children.size() > 3 )
                    {
                        child->Children.resize( 3 );
                        ++changed;
                    }
                }
                else if( child->Head() == "sheetname" )
                {
                    if( !child->AtomAt( 1 ).empty() )
                    {
                        kept.push_back( propertyNode( "Sheetname", child->AtomAt( 1 ) ) );
                        ++changed;
                    }

                    continue;
                }
                else if( child->Head() == "sheetfile" )
                {
                    if( !child->AtomAt( 1 ).empty() )
                    {
                        kept.push_back( propertyNode( "Sheetfile", child->AtomAt( 1 ) ) );
                        ++changed;
                    }

                    continue;
                }
            }

            kept.push_back( std::move( child ) );
        }

        aRoot->Children = std::move( kept );
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        changed += downgradePCBFootprintFields( child.get() );

    return changed;
}


int downgradeUserLayerTypes( SEXPR::NODE* aRoot )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int changed = 0;

    if( aRoot->Head() == "layers" )
    {
        for( std::unique_ptr<SEXPR::NODE>& layer : aRoot->Children )
        {
            if( !layer || layer->IsAtom() || layer->Children.size() < 4 )
                continue;

            std::string name = layer->AtomAt( 1 );
            std::string type = layer->AtomAt( 2 );

            if( StartsWith( name, "User." )
                && ( type == "front" || type == "back" || type == "auxiliary" ) )
            {
                layer->SetAtomAt( 2, "user", false );
                ++changed;
            }
        }

        return changed;
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        changed += downgradeUserLayerTypes( child.get() );

    return changed;
}


int downgradePCBPlotParamsBoolsToTrueFalse( SEXPR::NODE* aRoot )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int changed = 0;

    if( aRoot->Head() == "pcbplotparams" )
    {
        for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        {
            if( !child || child->IsAtom() || child->Children.size() < 2 )
                continue;

            std::string value = Lower( child->AtomAt( 1 ) );

            if( value == "yes" && child->SetAtomAt( 1, "true", false ) )
                ++changed;
            else if( value == "no" && child->SetAtomAt( 1, "false", false ) )
                ++changed;
        }

        return changed;
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        changed += downgradePCBPlotParamsBoolsToTrueFalse( child.get() );

    return changed;
}


int downgradePCBShapeFillNoToNone( SEXPR::NODE* aRoot )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    static const std::set<std::string> shapeHeads = {
        "gr_rect", "gr_circle", "gr_poly", "fp_rect", "fp_circle", "fp_poly"
    };

    int changed = 0;

    if( shapeHeads.count( aRoot->Head() ) )
    {
        SEXPR::NODE* fill = aRoot->ChildList( "fill" );

        if( fill && Lower( fill->AtomAt( 1 ) ) == "no" && fill->SetAtomAt( 1, "none", false ) )
            ++changed;
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        changed += downgradePCBShapeFillNoToNone( child.get() );

    return changed;
}


int downgradePCBShapeHatchFills( SEXPR::NODE* aRoot )
{
    // KiCad versions without hatch fill support still display/manufacture solid fills.
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    static const std::set<std::string> shapeHeads = {
        "gr_rect", "gr_circle", "gr_poly", "fp_rect", "fp_circle", "fp_poly"
    };

    int changed = 0;

    if( shapeHeads.count( aRoot->Head() ) )
    {
        SEXPR::NODE* fill = aRoot->ChildList( "fill" );

        if( fill )
        {
            std::string value = fill->AtomAt( 1 );

            if( value == "hatch" || value == "reverse_hatch" || value == "cross_hatch" )
            {
                fill->SetAtomAt( 1, "yes", false );
                ++changed;
            }
        }
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        changed += downgradePCBShapeHatchFills( child.get() );

    return changed;
}


int ensureZoneFilledAreasThickness( SEXPR::NODE* aRoot )
{
    // KiCad 6+ uses this flag to distinguish polygon fill caches from old stroked fills.
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int changed = 0;

    if( aRoot->Head() == "zone" && aRoot->ChildList( "filled_polygon" )
        && !aRoot->ChildList( "filled_areas_thickness" ) )
    {
        std::unique_ptr<SEXPR::NODE> thickness = SEXPR::NODE::MakeList();
        thickness->Children.push_back( SEXPR::NODE::MakeAtom( "filled_areas_thickness" ) );
        thickness->Children.push_back( SEXPR::NODE::MakeAtom( "no" ) );

        size_t insertAt = aRoot->Children.size();

        for( size_t i = 1; i < aRoot->Children.size(); ++i )
        {
            if( aRoot->Children[i] && !aRoot->Children[i]->IsAtom()
                && aRoot->Children[i]->Head() == "fill" )
            {
                insertAt = i;
                break;
            }
        }

        aRoot->Children.insert( aRoot->Children.begin() + insertAt, std::move( thickness ) );
        ++changed;
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        changed += ensureZoneFilledAreasThickness( child.get() );

    return changed;
}


// Dimensions cannot be represented in KiCad 7 PCB syntax, so keep visible text.
std::unique_ptr<SEXPR::NODE> legacyTextFromDimension( std::unique_ptr<SEXPR::NODE> aDimension,
                                                       bool aInsideFootprint )
{
    SEXPR::NODE* source = aDimension ? aDimension->ChildList( "gr_text" ) : nullptr;

    if( !source )
        return nullptr;

    std::unique_ptr<SEXPR::NODE> text = SEXPR::NODE::MakeList();

    if( aInsideFootprint )
    {
        text->Children.push_back( SEXPR::NODE::MakeAtom( "fp_text" ) );
        text->Children.push_back( SEXPR::NODE::MakeAtom( "user" ) );
        text->Children.push_back( SEXPR::NODE::MakeAtom( source->AtomAt( 1 ), true ) );
    }
    else
    {
        text->Children.push_back( SEXPR::NODE::MakeAtom( "gr_text" ) );
        text->Children.push_back( SEXPR::NODE::MakeAtom( source->AtomAt( 1 ), true ) );
    }

    for( size_t i = 2; i < source->Children.size(); ++i )
        text->Children.push_back( std::move( source->Children[i] ) );

    return text;
}


int downgradeDimensionsToTextImpl( SEXPR::NODE* aRoot, bool aInsideFootprint )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    bool insideFootprint = aInsideFootprint || aRoot->Head() == "footprint" || aRoot->Head() == "module";
    int changed = 0;
    std::vector<std::unique_ptr<SEXPR::NODE>> kept;

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
    {
        if( child && !child->IsAtom() && child->Head() == "dimension" )
        {
            std::unique_ptr<SEXPR::NODE> text = legacyTextFromDimension( std::move( child ),
                                                                         insideFootprint );

            if( text )
                kept.push_back( std::move( text ) );

            ++changed;
            continue;
        }

        changed += downgradeDimensionsToTextImpl( child.get(), insideFootprint );
        kept.push_back( std::move( child ) );
    }

    aRoot->Children = std::move( kept );
    return changed;
}


int downgradeDimensionsToText( SEXPR::NODE* aRoot )
{
    return downgradeDimensionsToTextImpl( aRoot, false );
}


int removeNodesContainingChild( SEXPR::NODE* aRoot, const std::string& aParentHead,
                                const std::string& aChildHead )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int removed = 0;
    std::vector<std::unique_ptr<SEXPR::NODE>> kept;

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
    {
        if( child && !child->IsAtom() && child->Head() == aParentHead && hasChild( child.get(), aChildHead ) )
        {
            ++removed;
            continue;
        }

        removed += removeNodesContainingChild( child.get(), aParentHead, aChildHead );
        kept.push_back( std::move( child ) );
    }

    aRoot->Children = std::move( kept );
    return removed;
}


int replaceAtomValuesInParents( SEXPR::NODE* aRoot, const std::set<std::string>& aParents,
                                const std::string& aFrom, const std::string& aTo )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int changed = 0;

    if( aParents.count( aRoot->Head() ) )
    {
        for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        {
            if( child && child->IsAtom() && child->Atom == aFrom )
            {
                child->Atom = aTo;
                ++changed;
            }
        }
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        changed += replaceAtomValuesInParents( child.get(), aParents, aFrom, aTo );

    return changed;
}


std::string zoneNetName( SEXPR::NODE* aZone )
{
    if( !aZone )
        return std::string();

    SEXPR::NODE* net = aZone->ChildList( "net" );

    if( !net || isIntegerAtom( net->AtomAt( 1 ) ) )
        return std::string();

    return net->AtomAt( 1 );
}


void collectBoardNetRefs( SEXPR::NODE* aRoot, std::map<std::string, int>& aCodes, int& aNext )
{
    if( !aRoot || aRoot->IsAtom() )
        return;

    if( aRoot->Head() == "net" && !isIntegerAtom( aRoot->AtomAt( 1 ) ) )
    {
        std::string name = aRoot->AtomAt( 1 );

        if( !aCodes.count( name ) )
            aCodes[name] = aNext++;
    }

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        collectBoardNetRefs( child.get(), aCodes, aNext );
}


std::map<std::string, int> collectBoardNetCodes( SEXPR::NODE* aRoot )
{
    std::map<std::string, int> codes;
    codes[""] = 0;
    int next = 1;

    if( aRoot )
    {
        for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        {
            if( !child || child->IsAtom() || child->Head() != "net" )
                continue;

            if( isIntegerAtom( child->AtomAt( 1 ) ) )
            {
                int code = atomToInt( child->AtomAt( 1 ) );
                std::string name = child->AtomAt( 2 );

                if( !codes.count( name ) )
                    codes[name] = code;

                if( code >= next )
                    next = code + 1;
            }
            else
            {
                std::string name = child->AtomAt( 1 );

                if( !codes.count( name ) )
                    codes[name] = next++;
            }
        }
    }

    collectBoardNetRefs( aRoot, codes, next );
    return codes;
}


int rewriteBoardNetNamesToCodes( SEXPR::NODE* aRoot, const std::string& aParentHead,
                                 const std::map<std::string, int>& aCodes )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    int changed = 0;

    if( aRoot->Head() == "net" && !isIntegerAtom( aRoot->AtomAt( 1 ) ) )
    {
        std::string name = aRoot->AtomAt( 1 );
        int code = 0;
        auto it = aCodes.find( name );

        if( it != aCodes.end() )
            code = it->second;

        std::unique_ptr<SEXPR::NODE> head = std::move( aRoot->Children[0] );
        std::vector<std::unique_ptr<SEXPR::NODE>> children;
        children.push_back( std::move( head ) );
        children.push_back( SEXPR::NODE::MakeAtom( std::to_string( code ) ) );

        if( aParentHead == "kicad_pcb" || aParentHead == "pad" )
        {
            for( size_t i = 1; i < aRoot->Children.size(); ++i )
                children.push_back( std::move( aRoot->Children[i] ) );
        }

        aRoot->Children = std::move( children );
        ++changed;
    }

    if( aRoot->Head() == "zone" )
    {
        std::string name = zoneNetName( aRoot );

        if( !name.empty() && !aRoot->ChildList( "net_name" ) )
        {
            std::unique_ptr<SEXPR::NODE> netName = SEXPR::NODE::MakeList();
            netName->Children.push_back( SEXPR::NODE::MakeAtom( "net_name" ) );
            netName->Children.push_back( SEXPR::NODE::MakeAtom( name, true ) );
            aRoot->Children.push_back( std::move( netName ) );
            ++changed;
        }
    }

    std::string parent = aRoot->Head();

    for( std::unique_ptr<SEXPR::NODE>& child : aRoot->Children )
        changed += rewriteBoardNetNamesToCodes( child.get(), parent, aCodes );

    return changed;
}


int firstBoardItemIndex( SEXPR::NODE* aRoot )
{
    if( !aRoot || aRoot->IsAtom() )
        return 0;

    static const std::set<std::string> itemHeads = {
        "arc", "dimension", "footprint", "gr_arc", "gr_circle", "gr_curve", "gr_line",
        "gr_poly", "gr_rect", "gr_text", "group", "image", "segment", "via", "zone"
    };

    for( size_t i = 0; i < aRoot->Children.size(); ++i )
    {
        SEXPR::NODE* child = aRoot->Children[i].get();

        if( child && !child->IsAtom() && itemHeads.count( child->Head() ) )
            return static_cast<int>( i );
    }

    return static_cast<int>( aRoot->Children.size() );
}


int ensureRootBoardNetCodeTable( SEXPR::NODE* aRoot, const std::map<std::string, int>& aCodes )
{
    // Legacy boards require root-level numeric net declarations before items reference them.
    if( !aRoot || aRoot->IsAtom() || aRoot->Head() != "kicad_pcb" )
        return 0;

    std::set<int> existingCodes;
    int lastRootNet = -1;

    for( size_t i = 0; i < aRoot->Children.size(); ++i )
    {
        SEXPR::NODE* child = aRoot->Children[i].get();

        if( !child || child->IsAtom() || child->Head() != "net" )
            continue;

        if( isIntegerAtom( child->AtomAt( 1 ) ) )
            existingCodes.insert( atomToInt( child->AtomAt( 1 ) ) );

        lastRootNet = static_cast<int>( i );
    }

    std::vector<std::pair<std::string, int>> entries;

    for( const auto& item : aCodes )
    {
        if( !existingCodes.count( item.second ) )
            entries.push_back( item );
    }

    std::sort( entries.begin(), entries.end(),
            []( const auto& a, const auto& b )
            {
                if( a.second == b.second )
                    return a.first < b.first;

                return a.second < b.second;
            } );

    if( entries.empty() )
        return 0;

    size_t insertAt = lastRootNet >= 0 ? static_cast<size_t>( lastRootNet + 1 )
                                      : static_cast<size_t>( firstBoardItemIndex( aRoot ) );

    std::vector<std::unique_ptr<SEXPR::NODE>> nodes;

    for( const auto& entry : entries )
    {
        std::unique_ptr<SEXPR::NODE> net = SEXPR::NODE::MakeList();
        net->Children.push_back( SEXPR::NODE::MakeAtom( "net" ) );
        net->Children.push_back( SEXPR::NODE::MakeAtom( std::to_string( entry.second ) ) );
        net->Children.push_back( SEXPR::NODE::MakeAtom( entry.first, true ) );
        nodes.push_back( std::move( net ) );
    }

    aRoot->Children.insert( aRoot->Children.begin() + insertAt,
                            std::make_move_iterator( nodes.begin() ),
                            std::make_move_iterator( nodes.end() ) );

    return static_cast<int>( entries.size() );
}


// Rebuilds legacy numeric net ids while keeping net names where required.
int downgradeBoardNetNamesToCodes( SEXPR::NODE* aRoot )
{
    std::map<std::string, int> codes = collectBoardNetCodes( aRoot );
    int changed = rewriteBoardNetNamesToCodes( aRoot, std::string(), codes );
    changed += ensureRootBoardNetCodeTable( aRoot, codes );
    return changed;
}


std::vector<std::string> removeIntroduced( SEXPR::NODE* aRoot, int aTarget,
                                           const std::vector<FEATURE_RULE>& aRules )
{
    // Feature gates are deliberately conservative: remove only when the target parser cannot read it.
    std::vector<std::string> warnings;

    for( const FEATURE_RULE& rule : aRules )
    {
        if( aTarget >= rule.MinVersion )
            continue;

        std::set<std::string> heads( rule.Heads.begin(), rule.Heads.end() );
        int removed = removeDescendantsByHead( aRoot, heads );

        if( removed > 0 )
        {
            std::ostringstream msg;
            msg << "removed " << removed << " node(s) introduced in " << rule.MinVersion
                << ": " << rule.Reason;
            warnings.push_back( msg.str() );
        }
    }

    return warnings;
}


} // namespace KICAD_BACKPORT::RULE_REWRITERS

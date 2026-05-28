#include "kicad_backport/kicad_backport_rules.h"
#include "internal/kicad_backport_rule_rewriters.h"

namespace KICAD_BACKPORT
{

using namespace RULE_REWRITERS;

namespace
{

// Feature gates remove syntax that older KiCad parsers cannot read.
const std::vector<FEATURE_RULE>& symbolRules()
{
    static const std::vector<FEATURE_RULE> rules = {
        { 20220126, { "text_box", "textbox" }, "symbol text boxes are not available" },
        { 20240529, { "embedded_files", "embedded_file" }, "embedded files are not available" },
        { 20241209, { "private" }, "private SCH_FIELD flags are not available" },
        { 20250324, { "pin_group", "pin_groups" }, "jumper pin groups are not available" },
        { 20250829, { "rounded_rectangle", "roundrect" }, "rounded rectangles are not available" },
        { 20260508, { "ellipse", "ellipse_arc" }, "native ellipse primitives are not available" },
    };

    return rules;
}


const std::vector<FEATURE_RULE>& schematicRules()
{
    static const std::vector<FEATURE_RULE> rules = {
        { 20220126, { "text_box", "textbox" }, "schematic text boxes are not available" },
        { 20220622, { "simulation_model", "sim_model" }, "new simulation model format is not available" },
        { 20240101, { "table" }, "schematic tables are not available" },
        { 20240417, { "rule_area" }, "schematic rule areas are not available" },
        { 20240620, { "embedded_files", "embedded_file" }, "embedded files are not available" },
        { 20241209, { "private" }, "private SCH_FIELD flags are not available" },
        { 20250829, { "rounded_rectangle", "roundrect" }, "rounded rectangles are not available" },
        { 20250922, { "variants", "variant" }, "schematic variants are not available" },
        { 20260508, { "ellipse", "ellipse_arc" }, "native ellipse primitives are not available" },
        { 20260512, { "net_chain", "net_chains" }, "schematic net chains are not available" },
    };

    return rules;
}


const std::vector<FEATURE_RULE>& boardRules()
{
    static const std::vector<FEATURE_RULE> rules = {
        { 20220131, { "gr_text_box", "fp_text_box", "text_box", "textbox" }, "PCB textboxes are not available" },
        { 20220621, { "image" }, "PCB image objects are not available" },
        { 20220818, { "net_tie", "net_ties" }, "first-class net-tie storage is not available" },
        { 20231007, { "generated" }, "PCB generative objects are not available" },
        { 20240108, { "teardrop", "teardrops" }, "teardrop parameters are not available" },
        { 20240202, { "table" }, "PCB tables are not available" },
        { 20240609, { "tenting" }, "tenting keyword is not available" },
        { 20240706, { "embedded_files", "embedded_file", "embedded_fonts" }, "embedded files are not available" },
        { 20240928, { "component_class", "component_classes" }, "component classes are not available" },
        { 20240929, { "padstack" }, "complex padstacks are not available" },
        { 20241006, { "via_stack", "viastack" }, "via stacks are not available" },
        { 20241009, { "rule_area" }, "placement/rule areas are not available" },
        { 20250228, { "via_protection", "covering", "plugging", "filling", "capping" }, "IPC-4761 via protection is not available" },
        { 20250818, { "custom_layer_count", "custom_layer_counts" }, "custom footprint layer counts are not available" },
        { 20250829, { "rounded_rectangle", "roundrect" }, "rounded rectangles are not available" },
        { 20250901, { "point" }, "PCB point objects are not available" },
        { 20250914, { "barcode", "pcb_barcode", "gr_barcode", "fp_barcode" }, "PCB barcode objects are not available" },
        { 20251101, { "backdrill", "tertiary_drill", "front_post_machining", "back_post_machining" }, "backdrill and tertiary drill fields are not available" },
        { 20260101, { "variants", "variant" }, "PCB variants are not available" },
        { 20260410, { "extruded" }, "extruded footprint 3D body models are not available" },
        { 20260508, { "gr_ellipse", "gr_ellipse_arc", "fp_ellipse", "fp_ellipse_arc" }, "native PCB ellipse primitives are not available" },
        { 20260511, { "spec_frequency", "dielectric_model" }, "dielectric frequency-dependent stackup fields are not available" },
        { 20260512, { "net_chains", "net_chain" }, "PCB net chains are not available" },
        { 20260513, { "thieving" }, "copper thieving zone fill mode is not available" },
    };

    return rules;
}

} // namespace


std::vector<std::string> ApplyDowngradeRules( DOCUMENT& aDocument, int aTarget )
{
    std::vector<std::string> warnings;

    // Each branch is ordered from broad parser compatibility to narrow fixes.
    auto append = [&]( std::vector<std::string> aMore )
    {
        warnings.insert( warnings.end(), aMore.begin(), aMore.end() );
    };

    auto warnIfChanged = [&]( int aChanged, const std::string& aWarning )
    {
        if( aChanged > 0 )
            warnings.push_back( aWarning );
    };

    // Compact single-action rules without std::function or heap allocation.
    auto applyWhen = [&]( bool aCondition, auto&& aRewrite, const std::string& aWarning )
    {
        if( aCondition )
            warnIfChanged( aRewrite(), aWarning );
    };

    switch( aDocument.Kind )
    {
    case KIND::SYMBOL_LIBRARY:
        // Symbol libraries share much of the schematic symbol syntax.
        append( removeIntroduced( aDocument.Root.get(), aTarget, symbolRules() ) );

        applyWhen( aTarget < 20231120, [&]() { return removeDirectChildrenByHead( aDocument.Root.get(), "generator_version" ); },
                   "removed symbol library generator_version fields" );

        applyWhen( aTarget < 20241209, [&]() { return removeDescendantsByHead( aDocument.Root.get(), { "embedded_fonts" } ); },
                   "removed symbol library embedded_fonts fields" );

        applyWhen( aTarget < 20240108, [&]() { return downgradeFontStyleListsToAtoms( aDocument.Root.get() ); },
                   "downgraded symbol library font bold/italic bool fields" );

        applyWhen( aTarget <= 20241209,
                   [&]()
                   {
                       return removeChildrenFromParents( aDocument.Root.get(), { "font" }, { "face" } );
                   },
                   "removed symbol library font face fields" );

        // Multi-action gates stay grouped when ordering matters.
        if( aTarget < 20241004 )
        {
            int n = downgradeBoolListsToAtoms( aDocument.Root.get(), { "hide" } );
            warnIfChanged( n, "downgraded symbol library boolean hide fields" );

            n = flattenChildListsToAtomsInParents( aDocument.Root.get(),
                                                       { "pin_names", "pin_numbers" },
                                                       { "hide" } );
            warnIfChanged( n, "downgraded symbol pin visibility fields" );
        }

        // Add missing ids before moving property visibility into effects.
        if( aTarget < 20241209 )
        {
            int n = ensureLegacyPropertyIds( aDocument.Root.get() );
            warnIfChanged( n, "added legacy symbol property ids" );

            n = movePropertyHideToEffects( aDocument.Root.get() );
            warnIfChanged( n, "moved symbol property hide flags to effects" );
        }

        applyWhen( aTarget < 20251024,
                   [&]()
                   {
                       return removeChildrenFromParents( aDocument.Root.get(), { "symbol" },
                                               { "in_pos_files" } );
                   },
                   "removed symbol library position file flags" );

        applyWhen( aTarget < 20250324,
                   [&]()
                   {
                       return removeChildrenFromParents( aDocument.Root.get(), { "symbol" },
                                               { "duplicate_pin_numbers_are_jumpers" } );
                   },
                   "removed symbol library jumper pin-number flags" );

        applyWhen( aTarget < 20250227, [&]() { return removeChildrenFromParents( aDocument.Root.get(), { "symbol" }, { "power" } ); },
                   "removed symbol library power class flags" );

        applyWhen( aTarget < 20251024,
                   [&]()
                   {
                       return removeChildrenFromParents( aDocument.Root.get(), { "property" },
                                               { "show_name", "do_not_autoplace" } );
                   },
                   "removed symbol property formatting fields" );
        break;

    case KIND::SCHEMATIC:
        // Schematic downgrades preserve visible fields whenever old syntax can express them.
        append( removeIntroduced( aDocument.Root.get(), aTarget, schematicRules() ) );

        applyWhen( aTarget < 20231120, [&]() { return removeDirectChildrenByHead( aDocument.Root.get(), "generator_version" ); },
                   "removed schematic generator_version fields" );

        applyWhen( aTarget < 20260326, [&]() { return removeDescendantsByHead( aDocument.Root.get(), { "locked" } ); },
                   "removed schematic locked fields introduced after target version" );

        applyWhen( aTarget < 20260306, [&]() { return removeDescendantsByHead( aDocument.Root.get(), { "embedded_fonts" } ); },
                   "removed schematic embedded_fonts fields" );

        applyWhen( aTarget < 20250827, [&]() { return removeDescendantsByHead( aDocument.Root.get(), { "body_styles", "body_style" } ); },
                   "removed schematic custom body style fields" );

        applyWhen( aTarget < 20250114,
                   [&]()
                   {
                       return removeChildrenFromParents( aDocument.Root.get(), { "text", "text_box", "textbox" },
                                               { "exclude_from_sim" } );
                   },
                   "removed schematic text simulation flags" );

        applyWhen( aTarget < 20260306,
                   [&]()
                   {
                       return removeChildrenFromParents( aDocument.Root.get(), { "sheet" },
                                               { "exclude_from_sim", "in_bom", "on_board", "dnp" } );
                   },
                   "removed schematic sheet assembly/simulation flags" );

        applyWhen( aTarget <= 20230121, [&]() { return removeDescendantsByHead( aDocument.Root.get(), { "exclude_from_sim" } ); },
                   "removed schematic simulation exclusion flags" );

        applyWhen( aTarget < 20251024,
                   [&]()
                   {
                       return removeChildrenFromParents( aDocument.Root.get(), { "symbol" },
                                               { "in_pos_files" } );
                   },
                   "removed schematic symbol position file flags" );

        applyWhen( aTarget < 20250324,
                   [&]()
                   {
                       return removeChildrenFromParents( aDocument.Root.get(), { "symbol" },
                                               { "duplicate_pin_numbers_are_jumpers" } );
                   },
                   "removed schematic library symbol jumper pin-number flags" );

        applyWhen( aTarget < 20250227, [&]() { return removeChildrenFromParents( aDocument.Root.get(), { "symbol" }, { "power" } ); },
                   "removed schematic library symbol power class flags" );

        // Pin-name and pin-number visibility use the same legacy atom syntax.
        if( aTarget < 20241004 )
        {
            int n = downgradeBoolListsToAtoms( aDocument.Root.get(), { "hide" } );
            warnIfChanged( n, "downgraded schematic boolean hide fields" );

            n = flattenChildListsToAtomsInParents( aDocument.Root.get(),
                                                   { "pin_names", "pin_numbers" },
                                                   { "hide" } );
            warnIfChanged( n, "downgraded schematic symbol pin visibility fields" );
        }

        applyWhen( aTarget < 20240108, [&]() { return downgradeFontStyleListsToAtoms( aDocument.Root.get() ); },
                   "downgraded schematic font bold/italic bool fields" );

        applyWhen( aTarget <= 20250114,
                   [&]()
                   {
                       return removeChildrenFromParents( aDocument.Root.get(), { "font" }, { "face" } );
                   },
                   "removed schematic font face fields" );

        if( aTarget < 20241209 )
        {
            int n = ensureLegacyPropertyIds( aDocument.Root.get() );
            warnIfChanged( n, "added legacy schematic property ids" );

            n = movePropertyHideToEffects( aDocument.Root.get() );
            warnIfChanged( n, "moved schematic property hide flags to effects" );
        }

        applyWhen( aTarget < 20231120,
                   [&]()
                   {
                       return removeChildrenFromParents( aDocument.Root.get(), { "symbol", "sheet" },
                                               { "fields_autoplaced" } );
                   },
                   "removed schematic symbol/sheet fields_autoplaced fields" );

        applyWhen( aTarget < 20251028,
                   [&]()
                   {
                       return removeChildrenFromParents( aDocument.Root.get(), { "property" },
                                               { "show_name", "do_not_autoplace" } );
                   },
                   "removed schematic property formatting fields" );

        applyWhen( aTarget < 20260306, [&]() { return removeDirectChildrenByHead( aDocument.Root.get(), "group" ); },
                   "removed schematic group nodes" );
        break;

    case KIND::BOARD:
    case KIND::FOOTPRINT:
        // PCB and footprint syntax use the same graphical and pad-level rewrites.
        append( removeIntroduced( aDocument.Root.get(), aTarget, boardRules() ) );

        applyWhen( aTarget < 20260410, [&]() { return removeNodesContainingChild( aDocument.Root.get(), "model", "type" ); },
                   "removed typed/extruded 3D model blocks" );

        applyWhen( aTarget < 20260513, [&]() { return replaceAtomValuesInParents( aDocument.Root.get(), { "mode" }, "thieving", "polygon" ); },
                   "downgraded copper thieving fill modes to polygon fill" );

        applyWhen( aTarget >= 20220225, [&]() { return removeChildrenFromParents( aDocument.Root.get(), { "footprint", "module" }, { "tedit" } ); },
                   "removed obsolete footprint tedit fields" );

        applyWhen( aTarget >= 20200628, [&]() { return removeChildrenFromParents( aDocument.Root.get(), { "setup" }, { "visible_elements" } ); },
                   "removed obsolete board visible_elements settings" );

        applyWhen( aTarget < 20240703, [&]() { return downgradeUserLayerTypes( aDocument.Root.get() ); },
                   "removed user-layer type qualifiers" );

        // Static parent lists are built only when the target needs this rewrite.
        if( aTarget < 20241010 )
        {
            static const std::set<std::string> graphicParents = {
                "gr_line", "gr_arc", "gr_circle", "gr_rect", "gr_poly",
                "fp_line", "fp_arc", "fp_circle", "fp_rect", "fp_poly"
            };

            int n = removeChildrenFromParents( aDocument.Root.get(), graphicParents,
                                               { "solder_mask_margin" } );
            warnIfChanged( n, "removed graphic solder_mask_margin fields" );
        }

        // Dimension style changes share one format cutoff.
        if( aTarget < 20241030 )
        {
            int n = downgradeBoolListsToAtoms( aDocument.Root.get(),
                                               { "suppress_zeroes", "keep_text_aligned" } );
            warnIfChanged( n, "downgraded dimension boolean fields to legacy atom syntax" );

            n = removeChildrenFromParents( aDocument.Root.get(), { "style" }, { "arrow_direction" } );
            warnIfChanged( n, "removed dimension arrow direction fields" );
        }

        applyWhen( aTarget < 20241009, [&]() { return removeChildrenFromParents( aDocument.Root.get(), { "zone" }, { "placement" } ); },
                   "removed zone placement fields" );

        applyWhen( aTarget < 20241007,
                   [&]()
                   {
                       return removeChildrenFromParents( aDocument.Root.get(), { "segment", "arc" },
                                               { "solder_mask_margin", "solder_mask_layer" } );
                   },
                   "removed track soldermask layer/margin fields" );

        applyWhen( aTarget < 20240617, [&]() { return removeChildrenFromParents( aDocument.Root.get(), { "table_cell" }, { "angle" } ); },
                   "removed PCB table cell angle fields" );

        if( aTarget < 20250228 )
        {
            int n = downgradeTentingToLegacyAtoms( aDocument.Root.get() );
            warnIfChanged( n, "downgraded tenting front/back bool lists to legacy atom syntax" );

            n = removeDescendantsByHead( aDocument.Root.get(), { "covering", "plugging", "filling", "capping" } );
            warnIfChanged( n, "removed IPC-4761 via protection fields" );
        }

        // Older PCB syntax represents these booleans as bare presence atoms.
        if( aTarget < 20231212 )
        {
            int n = downgradeBooleanPresenceNodes( aDocument.Root.get(), { "locked", "hide" } );
            warnIfChanged( n, "downgraded board/footprint boolean locked/hide fields" );

            n = removeDescendantsByHead( aDocument.Root.get(), { "unlocked" } );
            warnIfChanged( n, "removed PCB text keep-upright unlock fields" );

            n = removeChildrenFromParents( aDocument.Root.get(), { "model" }, { "hide" } );
            warnIfChanged( n, "removed legacy-incompatible 3D model hide fields" );
        }

        applyWhen( aTarget < 20231014, [&]() { return removeDirectChildrenByHead( aDocument.Root.get(), "generator_version" ); },
                   "removed board/footprint generator_version fields" );

        if( aTarget < 20230924 )
        {
            int n = downgradePCBPlotParamsBoolsToTrueFalse( aDocument.Root.get() );
            warnIfChanged( n, "downgraded pcbplotparams boolean values" );

            n = downgradePCBShapeFillNoToNone( aDocument.Root.get() );
            warnIfChanged( n, "downgraded PCB shape fill no values to none" );
        }

        // Graphic net ownership is unsupported before this format version.
        if( aTarget < 20230730 )
        {
            static const std::set<std::string> graphicParents = {
                "gr_line", "gr_arc", "gr_circle", "gr_rect", "gr_poly", "gr_curve",
                "fp_line", "fp_arc", "fp_circle", "fp_rect", "fp_poly", "fp_curve"
            };

            int n = removeChildrenFromParents( aDocument.Root.get(), graphicParents, { "net" } );
            warnIfChanged( n, "removed PCB graphic shape net connectivity fields" );
        }

        if( aTarget < 20240108 )
        {
            int n = removeChildrenFromParents( aDocument.Root.get(), { "group" }, { "locked" } );
            warnIfChanged( n, "removed group locked fields" );

            n = downgradeFontStyleListsToAtoms( aDocument.Root.get() );
            warnIfChanged( n, "downgraded PCB font bold/italic bool fields" );
        }

        applyWhen( aTarget < 20230620, [&]() { return downgradePCBFootprintFields( aDocument.Root.get() ); },
                   "downgraded PCB footprint fields to legacy storage" );

        // UUID rename is scoped so unrelated ids are not touched.
        if( aTarget < 20231231 )
        {
            static const std::set<std::string> tstampParents = {
                "footprint", "module", "pad", "via", "segment", "arc", "zone",
                "gr_line", "gr_arc", "gr_circle", "gr_rect", "gr_poly", "gr_curve", "gr_text",
                "fp_line", "fp_arc", "fp_circle", "fp_rect", "fp_poly", "fp_curve", "fp_text"
            };

            int n = renameChildHeadInParents( aDocument.Root.get(), tstampParents, "uuid", "tstamp" );
            warnIfChanged( n, "renamed footprint uuid fields back to legacy tstamp" );

            n = renameChildHeadInParents( aDocument.Root.get(), { "group", "generated" }, "uuid", "id" );
            warnIfChanged( n, "renamed board group/generated uuid fields back to id" );
        }

        applyWhen( aTarget < 20250324,
                   [&]()
                   {
                       return removeChildrenFromParents( aDocument.Root.get(), { "footprint" },
                                               { "duplicate_pad_numbers_are_jumpers", "jumper_pad_groups" } );
                   },
                   "removed footprint jumper pad fields" );

        applyWhen( aTarget <= 20221018, [&]() { return removeAtomsFromHeadedLists( aDocument.Root.get(), { "attr" }, { "dnp" } ); },
                   "removed footprint dnp attributes" );

        applyWhen( aTarget < 20250309,
                   [&]()
                   {
                       return removeChildrenFromParents( aDocument.Root.get(), { "placement" },
                                               { "component_class" } );
                   },
                   "removed rule_area component_class placement sources" );

        applyWhen( aTarget < 20250222, [&]() { return downgradePCBShapeHatchFills( aDocument.Root.get() ); },
                   "downgraded PCB shape hatch fills" );

        applyWhen( aTarget < 20250210,
                   [&]()
                   {
                       return removeChildrenFromParents( aDocument.Root.get(), { "gr_text_box", "fp_text_box" },
                                               { "knockout" } );
                   },
                   "removed PCB text box knockout fields" );

        applyWhen( aTarget < 20250210, [&]() { return ensureZoneFilledAreasThickness( aDocument.Root.get() ); },
                   "tagged cached zone fills as polygon fills" );

        applyWhen( aTarget <= 20241229,
                   [&]()
                   {
                       return removeChildrenFromParents( aDocument.Root.get(), { "font" }, { "face" } );
                   },
                   "removed PCB font face fields" );

        // KiCad 7 PCB syntax needs several legacy cleanups at the same cutoff.
        if( aTarget <= 20221018 )
        {
            int n = removeChildrenFromParents( aDocument.Root.get(), { "pad", "via" },
                                               { "remove_unused_layers" } );
            warnIfChanged( n, "removed pad/via remove_unused_layers fields" );

            n = downgradeDimensionsToText( aDocument.Root.get() );
            warnIfChanged( n, "downgraded PCB dimensions to legacy text annotations" );

            n = removeDescendantsByHead( aDocument.Root.get(), { "locked" } );
            warnIfChanged( n, "removed legacy-incompatible locked fields" );

            n = downgradeBooleanPresenceNodes( aDocument.Root.get(), { "free" } );
            warnIfChanged( n, "downgraded free via fields" );
        }

        applyWhen( aTarget < 20251101,
                   [&]()
                   {
                       return removeChildrenFromParents( aDocument.Root.get(), { "pad", "via" },
                                               { "front_post_machining", "back_post_machining" } );
                   },
                   "removed pad/via post-machining fields" );

        applyWhen( aTarget < 20251028, [&]() { return downgradeBoardNetNamesToCodes( aDocument.Root.get() ); },
                   "added legacy netcodes to board net references" );
        break;

    case KIND::WORKSHEET:
        if( aTarget < 20220228 )
            append( removeIntroduced( aDocument.Root.get(), aTarget, { { 20220228, { "font" }, "worksheet font blocks are not available" } } ) );
        break;

    default:
        break;
    }

    return warnings;
}

} // namespace KICAD_BACKPORT

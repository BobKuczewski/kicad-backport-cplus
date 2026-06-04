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

        applyWhen( aTarget < 20230409,
                   [&]()
                   {
                       return removeChildrenFromParents( aDocument.Root.get(), { "symbol" },
                                               { "exclude_from_sim" } );
                   },
                   "removed symbol library simulation exclusion flags" );

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

            if( aTarget <= 20211014 )
            {
                n = ensureKiCad6StandardPropertyIds( aDocument.Root.get() );
                warnIfChanged( n, "added KiCad 6 standard symbol property ids" );
            }

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

        applyWhen( aTarget < 20220822,
                   [&]()
                   {
                       return removeChildrenFromParents( aDocument.Root.get(),
                                               { "text", "text_box", "textbox", "label",
                                                 "global_label", "hierarchical_label",
                                                 "directive_label", "netclass_flag" },
                                               { "hyperlink" } );
                   },
                   "removed schematic text hyperlink fields" );

        applyWhen( aTarget < 20220914,
                   [&]()
                   {
                       return removeChildrenFromParents( aDocument.Root.get(), { "symbol" },
                                               { "dnp" } );
                   },
                   "removed schematic DNP flags" );

        applyWhen( aTarget < 20220124,
                   [&]()
                   {
                       return renameChildHeadInParents( aDocument.Root.get(), { "kicad_sch" },
                                                        "directive_label", "netclass_flag" );
                   },
                   "renamed schematic directive labels to legacy netclass flags" );

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

        applyWhen( aTarget <= 20211123,
                   [&]()
                   {
                       return removeChildrenFromParents( aDocument.Root.get(), { "pin" },
                                               { "alternate" } );
                   },
                   "removed schematic pin alternate-function fields" );

        applyWhen( aTarget < 20240108, [&]() { return downgradeFontStyleListsToAtoms( aDocument.Root.get() ); },
                   "downgraded schematic font bold/italic bool fields" );

        applyWhen( aTarget <= 20250114,
                   [&]()
                   {
                       return removeChildrenFromParents( aDocument.Root.get(), { "font" }, { "face" } );
                   },
                   "removed schematic font face fields" );

        applyWhen( aTarget <= 20230121,
                   [&]()
                   {
                       return unquoteAtomsInHeadedLists( aDocument.Root.get(), { "uuid" }, 1 );
                   },
                   "normalized schematic UUID atoms for KiCad 6/7 parsers" );

        applyWhen( aTarget <= 20211123,
                   [&]()
                   {
                       return ensureLegacySchematicSymbolInstances( aDocument.Root.get() );
                   },
                   "generated schematic symbol instance table for KiCad 6 parsers" );

        applyWhen( aTarget <= 20211123,
                   [&]()
                   {
                       return ensureKiCad6StandardPropertyIds( aDocument.Root.get() );
                   },
                   "added KiCad 6 standard schematic property ids" );

        applyWhen( aTarget <= 20211123,
                   [&]()
                   {
                       return normalizeKiCad6SheetProperties( aDocument.Root.get() );
                   },
                   "normalized KiCad 6 sheet property names and ids" );

        applyWhen( aTarget <= 20211123,
                   [&]()
                   {
                       return removeDescendantsByHead( aDocument.Root.get(), { "instances" } );
                   },
                   "removed schematic symbol instance data" );

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
    {
        // PCB and footprint syntax use the same graphical and pad-level rewrites.
        if( aTarget < 20260603 )
        {
            int n = removeChildrenFromParents( aDocument.Root.get(), { "table_cell" },
                                               { "knockout" } );
            warnIfChanged( n, "removed PCB table-cell knockout flags" );
        }

        append( removeIntroduced( aDocument.Root.get(), aTarget, boardRules() ) );

        std::vector<CHILD_REMOVAL_RULE> childRemovalRules;
        std::vector<std::string> childRemovalWarnings;

        auto addChildRemoval = [&]( bool aCondition, std::set<std::string> aParents,
                                    std::set<std::string> aChildren,
                                    const std::string& aWarning )
        {
            if( !aCondition )
                return;

            childRemovalRules.push_back( CHILD_REMOVAL_RULE{ std::move( aParents ),
                                                              std::move( aChildren ) } );
            childRemovalWarnings.push_back( aWarning );
        };

        static const std::set<std::string> graphicParents = {
            "gr_line", "gr_arc", "gr_circle", "gr_rect", "gr_poly",
            "fp_line", "fp_arc", "fp_circle", "fp_rect", "fp_poly"
        };

        static const std::set<std::string> connectedGraphicParents = {
            "gr_line", "gr_arc", "gr_circle", "gr_rect", "gr_poly", "gr_curve",
            "fp_line", "fp_arc", "fp_circle", "fp_rect", "fp_poly", "fp_curve"
        };

        addChildRemoval( aTarget >= 20220225, { "footprint", "module" }, { "tedit" },
                         "removed obsolete footprint tedit fields" );
        addChildRemoval( aTarget >= 20200628, { "setup" }, { "visible_elements" },
                         "removed obsolete board visible_elements settings" );
        addChildRemoval( aTarget < 20241010, graphicParents, { "solder_mask_margin" },
                         "removed graphic solder_mask_margin fields" );
        addChildRemoval( aTarget < 20241030, { "style" }, { "arrow_direction" },
                         "removed dimension arrow direction fields" );
        addChildRemoval( aTarget < 20250210, { "gr_text", "fp_text" },
                         { "render_cache" },
                         "removed PCB text render caches" );
        addChildRemoval( aTarget < 20241009, { "zone" }, { "placement" },
                         "removed zone placement fields" );
        addChildRemoval( aTarget <= 20221018, { "zone" }, { "attr" },
                         "removed zone attr fields" );
        addChildRemoval( aTarget < 20240108, { "setup" },
                         { "allow_soldermask_bridges_in_footprints" },
                         "removed board soldermask bridge setup fields" );
        addChildRemoval( aTarget < 20241007, { "segment", "arc" },
                         { "solder_mask_margin", "solder_mask_layer" },
                         "removed track soldermask layer/margin fields" );
        addChildRemoval( aTarget < 20240617, { "table_cell" }, { "angle" },
                         "removed PCB table cell angle fields" );
        addChildRemoval( aTarget < 20260521, { "pad" }, { "sim_electrical_type" },
                         "removed pad simulation electrical type fields" );
        addChildRemoval( aTarget < 20231212, { "model" }, { "hide" },
                         "removed legacy-incompatible 3D model hide fields" );
        addChildRemoval( aTarget < 20230730, connectedGraphicParents, { "net" },
                         "removed PCB graphic shape net connectivity fields" );
        addChildRemoval( aTarget < 20240108, { "group" }, { "locked" },
                         "removed group locked fields" );
        addChildRemoval( aTarget < 20240108, { "via" },
                         { "remove_unused_layers", "keep_end_layers", "start_end_only",
                           "zone_layer_connections" },
                         "removed legacy via layer-connection fields" );
        addChildRemoval( aTarget < 20250324, { "footprint" },
                         { "duplicate_pad_numbers_are_jumpers", "jumper_pad_groups" },
                         "removed footprint jumper pad fields" );
        addChildRemoval( aTarget <= 20221018, { "footprint", "module" },
                         { "net_tie_pad_groups" },
                         "removed footprint net-tie pad group fields" );
        addChildRemoval( aTarget <= 20221018, { "footprint", "module" }, { "units" },
                         "removed footprint unit pin grouping fields" );
        addChildRemoval( aTarget < 20250210, { "gr_text_box", "fp_text_box" },
                         { "knockout" }, "removed PCB text box knockout fields" );
        addChildRemoval( aTarget <= 20241229, { "font" }, { "face" },
                         "removed PCB font face fields" );
        addChildRemoval( aTarget <= 20221018, { "pad", "via" },
                         { "remove_unused_layers" },
                         "removed pad/via remove_unused_layers fields" );
        addChildRemoval( aTarget <= 20221018, { "pad", "zone" },
                         { "thermal_bridge_angle" },
                         "removed pad/zone thermal bridge angle fields" );

        BOARD_FAST_COUNTS boardFastCounts;

        {
            BOARD_FAST_OPTIONS options;
            options.DimensionBoolFields = aTarget < 20241030;
            options.BoardPresenceBoolFields = aTarget < 20231212;
            options.RemoveUnlocked = aTarget < 20231212;
            options.PlotParamBools = aTarget < 20230924;
            options.ShapeFillNoToNone = aTarget < 20230924;
            options.FontStyleLists = aTarget < 20240108;
            options.AttrDnpAtoms = aTarget <= 20221018;
            options.ShapeHatchFills = aTarget < 20250222;
            options.ZoneFilledAreasThickness = aTarget < 20250210;
            options.RemoveLocked = aTarget <= 20221018;
            options.FreeViaPresence = aTarget <= 20221018;
            options.RemoveTypedModels = aTarget < 20260410;
            options.ReplaceThievingMode = aTarget < 20260513;
            options.UserLayerTypes = aTarget < 20240703;
            options.RemoveRootGeneratorVersion = aTarget < 20231014;
            options.RenameUuidToTstamp = aTarget < 20231231;
            options.RenameGroupGeneratedUuidToId = aTarget < 20231231;
            options.DowngradePCBFootprintFields = aTarget < 20230620;

            boardFastCounts = applyBoardFastVisitor( aDocument.Root.get(), options,
                                                     childRemovalRules );

            for( size_t i = 0; i < boardFastCounts.ChildRemovalRules.size(); ++i )
                warnIfChanged( boardFastCounts.ChildRemovalRules[i], childRemovalWarnings[i] );

            warnIfChanged( boardFastCounts.DimensionBoolFields,
                           "downgraded dimension boolean fields to legacy atom syntax" );
            warnIfChanged( boardFastCounts.BoardPresenceBoolFields,
                           "downgraded board/footprint boolean locked/hide fields" );
            warnIfChanged( boardFastCounts.RemovedUnlocked,
                           "removed PCB text keep-upright unlock fields" );
            warnIfChanged( boardFastCounts.PlotParamBools,
                           "downgraded pcbplotparams boolean values" );
            warnIfChanged( boardFastCounts.ShapeFillNoToNone,
                           "downgraded PCB shape fill no values to none" );
            warnIfChanged( boardFastCounts.FontStyleLists,
                           "downgraded PCB font bold/italic bool fields" );
            warnIfChanged( boardFastCounts.RemovedAttrDnpAtoms,
                           "removed footprint dnp attributes" );
            warnIfChanged( boardFastCounts.ShapeHatchFills,
                           "downgraded PCB shape hatch fills" );
            warnIfChanged( boardFastCounts.ZoneFilledAreasThickness,
                           "tagged cached zone fills as polygon fills" );
            warnIfChanged( boardFastCounts.RemovedLocked,
                           "removed legacy-incompatible locked fields" );
            warnIfChanged( boardFastCounts.FreeViaPresence,
                           "downgraded free via fields" );
        }

        warnIfChanged( boardFastCounts.RemovedTypedModels,
                       "removed typed/extruded 3D model blocks" );

        warnIfChanged( boardFastCounts.ReplacedThievingMode,
                       "downgraded copper thieving fill modes to polygon fill" );

        warnIfChanged( boardFastCounts.UserLayerTypes,
                       "removed user-layer type qualifiers" );

        if( aTarget < 20250228 )
        {
            if( aTarget >= 20240609 )
            {
                int n = downgradeTentingToLegacyAtoms( aDocument.Root.get() );
                warnIfChanged( n, "downgraded tenting front/back bool lists to legacy atom syntax" );
            }

            // IPC-4761 child fields are already removed by the broad feature gate above.
        }

        warnIfChanged( boardFastCounts.RemovedRootGeneratorVersion,
                       "removed board/footprint generator_version fields" );

        warnIfChanged( boardFastCounts.PCBFootprintFields,
                       "downgraded PCB footprint fields to legacy storage" );

        warnIfChanged( boardFastCounts.RenamedUuidToTstamp,
                       "renamed footprint uuid fields back to legacy tstamp" );
        warnIfChanged( boardFastCounts.RenamedGroupGeneratedUuidToId,
                       "renamed board group/generated uuid fields back to id" );

        applyWhen( aTarget <= 20221018,
                   [&]()
                   {
                       return removeAtomsFromHeadedLists( aDocument.Root.get(), { "attr" },
                                                          { "allow_missing_courtyard" } );
                   },
                   "removed legacy-incompatible footprint attr flags" );

        applyWhen( aTarget < 20250210,
                   [&]()
                   {
                       return removeAtomsFromHeadedLists( aDocument.Root.get(), { "layer" },
                                                          { "knockout" } );
                   },
                   "removed PCB layer knockout flags" );

        applyWhen( aTarget < 20231231,
                   [&]()
                   {
                       return unquoteAtomsInHeadedLists( aDocument.Root.get(),
                                                        { "uuid", "tstamp", "id" }, 1 );
                   },
                   "normalized PCB UUID/tstamp/id atoms for legacy parsers" );

        applyWhen( aTarget < 20240225,
                   [&]()
                   {
                       return renameChildHeadInParents( aDocument.Root.get(),
                                                        { "footprint", "module", "pad" },
                                                        "solder_paste_margin_ratio",
                                                        "solder_paste_ratio" );
                   },
                   "renamed solder_paste_margin_ratio fields to legacy solder_paste_ratio" );

        applyWhen( aTarget <= 20221018,
                   [&]()
                   {
                       return renameChildHeadInParents( aDocument.Root.get(),
                                                        { "pad", "zone" },
                                                        "thermal_bridge_width",
                                                        "thermal_width" );
                   },
                   "renamed thermal_bridge_width fields to legacy thermal_width" );

        applyWhen( aTarget <= 20221018,
                   [&]()
                   {
                       return downgradePCBStrokeToLegacyWidth( aDocument.Root.get() );
                   },
                   "downgraded PCB stroke blocks to legacy width fields" );

        applyWhen( aTarget < 20250309 && aTarget >= 20240928,
                   [&]()
                   {
                       return removeChildrenFromParents( aDocument.Root.get(), { "placement" },
                                               { "component_class" } );
                   },
                   "removed rule_area component_class placement sources" );

        // KiCad 7 PCB syntax needs several legacy cleanups at the same cutoff.
        if( aTarget <= 20221018 )
        {
            int n = downgradeDimensionsToGraphics( aDocument.Root.get() );
            warnIfChanged( n, "downgraded PCB dimensions to legacy graphic annotations" );

            n = downgradePCBStrokeToLegacyWidth( aDocument.Root.get() );
            warnIfChanged( n, "downgraded generated PCB dimension strokes to legacy width fields" );
        }

        // Pad/via post-machining fields are already removed by the broad feature gate above.

        applyWhen( aTarget < 20251028, [&]() { return downgradeBoardNetNamesToCodes( aDocument.Root.get() ); },
                   "added legacy netcodes to board net references" );
        break;
    }

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

#pragma once

#include "kicad_backport/sexpr.h"

#include <set>
#include <string>
#include <vector>


namespace KICAD_BACKPORT::RULE_REWRITERS
{

// Feature gate for syntax introduced after a target file format version.
struct FEATURE_RULE
{
    int                      MinVersion; // First file format version that accepts these heads.
    std::vector<std::string> Heads;      // S-expression head tokens guarded by this rule.
    std::string              Reason;     // Warning text when older targets require removal.
};

struct CHILD_REMOVAL_RULE
{
    std::set<std::string> Parents;
    std::set<std::string> Children;
};

struct BOARD_FAST_OPTIONS
{
    bool DimensionBoolFields = false;
    bool BoardPresenceBoolFields = false;
    bool RemoveUnlocked = false;
    bool PlotParamBools = false;
    bool ShapeFillNoToNone = false;
    bool FontStyleLists = false;
    bool AttrDnpAtoms = false;
    bool ShapeHatchFills = false;
    bool ZoneFilledAreasThickness = false;
    bool RemoveLocked = false;
    bool FreeViaPresence = false;
    bool RemoveTypedModels = false;
    bool ReplaceThievingMode = false;
    bool UserLayerTypes = false;
    bool RemoveRootGeneratorVersion = false;
    bool RenameUuidToTstamp = false;
    bool RenameGroupGeneratedUuidToId = false;
    bool DowngradePCBFootprintFields = false;
};

struct BOARD_FAST_COUNTS
{
    std::vector<int> ChildRemovalRules;
    int DimensionBoolFields = 0;
    int BoardPresenceBoolFields = 0;
    int RemovedUnlocked = 0;
    int PlotParamBools = 0;
    int ShapeFillNoToNone = 0;
    int FontStyleLists = 0;
    int RemovedAttrDnpAtoms = 0;
    int ShapeHatchFills = 0;
    int ZoneFilledAreasThickness = 0;
    int RemovedLocked = 0;
    int FreeViaPresence = 0;
    int RemovedTypedModels = 0;
    int ReplacedThievingMode = 0;
    int UserLayerTypes = 0;
    int RemovedRootGeneratorVersion = 0;
    int RenamedUuidToTstamp = 0;
    int RenamedGroupGeneratedUuidToId = 0;
    int PCBFootprintFields = 0;
};

// Generic tree rewrites used by version-specific downgrade rules.
int removeDescendantsByHead( SEXPR::NODE* aRoot, const std::set<std::string>& aHeads );
int removeChildrenFromParents( SEXPR::NODE* aRoot, const std::set<std::string>& aParents,
                               const std::set<std::string>& aChildren );
std::vector<int> removeChildrenFromParentsBatch( SEXPR::NODE* aRoot,
                                                 const std::vector<CHILD_REMOVAL_RULE>& aRules );
BOARD_FAST_COUNTS applyBoardFastVisitor( SEXPR::NODE* aRoot, const BOARD_FAST_OPTIONS& aOptions,
                                         const std::vector<CHILD_REMOVAL_RULE>& aChildRemovalRules = {} );
int removeDirectChildrenByHead( SEXPR::NODE* aRoot, const std::string& aHead );
int renameChildHeadInParents( SEXPR::NODE* aRoot, const std::set<std::string>& aParents,
                              const std::string& aFrom, const std::string& aTo );
int removeAtomsFromHeadedLists( SEXPR::NODE* aRoot, const std::set<std::string>& aParents,
                                const std::set<std::string>& aAtoms );
int flattenChildListsToAtomsInParents( SEXPR::NODE* aRoot, const std::set<std::string>& aParents,
                                       const std::set<std::string>& aChildren );
int removeNodesContainingChild( SEXPR::NODE* aRoot, const std::string& aParentHead,
                                const std::string& aChildHead );
int replaceAtomValuesInParents( SEXPR::NODE* aRoot, const std::set<std::string>& aParents,
                                const std::string& aFrom, const std::string& aTo );
int unquoteAtomsInHeadedLists( SEXPR::NODE* aRoot, const std::set<std::string>& aHeads,
                               size_t aAtomIndex );
int ensureLegacySchematicSymbolInstances( SEXPR::NODE* aRoot );
int removePlacedSymbolPinUuidBlocks( SEXPR::NODE* aRoot );
int downgradePCBStrokeToLegacyWidth( SEXPR::NODE* aRoot );

// KiCad-specific equivalent downgrades.
int ensureLegacyPropertyIds( SEXPR::NODE* aRoot );
int ensureKiCad6StandardPropertyIds( SEXPR::NODE* aRoot );
int normalizeKiCad6SheetProperties( SEXPR::NODE* aRoot );
int movePropertyHideToEffects( SEXPR::NODE* aRoot );
int downgradeTentingToLegacyAtoms( SEXPR::NODE* aRoot );
int downgradeBooleanPresenceNodes( SEXPR::NODE* aRoot, const std::set<std::string>& aHeads );
int downgradeFontStyleListsToAtoms( SEXPR::NODE* aRoot );
int downgradeBoolListsToAtoms( SEXPR::NODE* aRoot, const std::set<std::string>& aHeads );
int downgradePCBFootprintFields( SEXPR::NODE* aRoot );
int downgradeUserLayerTypes( SEXPR::NODE* aRoot );
int downgradePCBPlotParamsBoolsToTrueFalse( SEXPR::NODE* aRoot );
int downgradePCBShapeFillNoToNone( SEXPR::NODE* aRoot );
int downgradePCBShapeHatchFills( SEXPR::NODE* aRoot );
int ensureZoneFilledAreasThickness( SEXPR::NODE* aRoot );
int downgradeDimensionsToText( SEXPR::NODE* aRoot );
int downgradeDimensionsToGraphics( SEXPR::NODE* aRoot );
int downgradeBoardNetNamesToCodes( SEXPR::NODE* aRoot );

std::vector<std::string> removeIntroduced( SEXPR::NODE* aRoot, int aTarget,
                                           const std::vector<FEATURE_RULE>& aRules );

} // namespace KICAD_BACKPORT::RULE_REWRITERS

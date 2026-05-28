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

// Generic tree rewrites used by version-specific downgrade rules.
int removeDescendantsByHead( SEXPR::NODE* aRoot, const std::set<std::string>& aHeads );
int removeChildrenFromParents( SEXPR::NODE* aRoot, const std::set<std::string>& aParents,
                               const std::set<std::string>& aChildren );
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

// KiCad-specific equivalent downgrades.
int ensureLegacyPropertyIds( SEXPR::NODE* aRoot );
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
int downgradeBoardNetNamesToCodes( SEXPR::NODE* aRoot );

std::vector<std::string> removeIntroduced( SEXPR::NODE* aRoot, int aTarget,
                                           const std::vector<FEATURE_RULE>& aRules );

} // namespace KICAD_BACKPORT::RULE_REWRITERS

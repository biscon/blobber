template <class A> void visit(A&, CellSurface&, const CellSurface&);
template <class A> void visit(A&, Decoration&, const Decoration&);
template <class A> void visit(A&, WorldDefinition&, const WorldDefinition&);
template <class A> void visit(A&, AmbientFire&, const AmbientFire&);
template <class A> void visit(A&, SceneryPlacement&, const SceneryPlacement&);
template <class A> void visit(A& a, NpcAiConfig& v, const NpcAiConfig& base) {
    a("waypointCount", v.waypointCount, base.waypointCount);
    a("routine", v.routine, base.routine);
    a.counted("waypoints", v.waypoints, v.waypointCount, base.waypoints);
}
// Internal explicit schema field list. No padding, pointers, derived caches or runtime epochs.
template <class A> void visit(A&, Cell&, const Cell&);
template <class A> void visit(A&, ActorPosition&, const ActorPosition&);
template <class A> void visit(A&, ActorId&, const ActorId&);
template <class A> void visit(A&, NpcInstance&, const NpcInstance&);
template <class A> void visit(A&, ActorState&, const ActorState&);
template <class A> void visit(A&, ActorSnapshot&, const ActorSnapshot&);
template <class A> void visit(A&, FloorGeometry::Saved&, const FloorGeometry::Saved&);
template <class A> void visit(A&, FloorLoot&, const FloorLoot&);
template <class A> void visit(A&, LootSnapshot&, const LootSnapshot&);
template <class A> void visit(A&, LootWorld::Saved&, const LootWorld::Saved&);
template <class A> void visit(A&, NpcAiState&, const NpcAiState&);
template <class A> void visit(A&, NpcAiSnapshot&, const NpcAiSnapshot&);
template <class A> void visit(A&, NpcAiController::Saved&, const NpcAiController::Saved&);
template <class A> void visit(A&, EffectTarget&, const EffectTarget&);
template <class A> void visit(A&, EffectInstance&, const EffectInstance&);
template <class A> void visit(A&, ConditionState&, const ConditionState&);
template <class A> void visit(A&, GroundArea&, const GroundArea&);
template <class A> void visit(A&, SpectralWeapon&, const SpectralWeapon&);
template <class A> void visit(A&, GroundSnapshot&, const GroundSnapshot&);
template <class A> void visit(A&, Restraint&, const Restraint&);
template <class A> void visit(A&, EffectController::Saved&, const EffectController::Saved&);
template <class A> void visit(A&, CombatParticipant&, const CombatParticipant&);
template <class A> void visit(A&, CombatSnapshot&, const CombatSnapshot&);
template <class A> void visit(A&, CombatController::Saved&, const CombatController::Saved&);
template <class A> void visit(A&, FloorObjectiveSnapshot&, const FloorObjectiveSnapshot&);
template <class A> void visit(A&, FloorState&, const FloorState&);
template <class A> void visit(A&, Player&, const Player&);
template <class A> void visit(A&, rules::CharacterIdentity&, const rules::CharacterIdentity&);
template <class A> void visit(A&, PartyCharacter&, const PartyCharacter&);
template <class A> void visit(A&, PartySnapshot&, const PartySnapshot&);
template <class A> void visit(A&, ItemStack&, const ItemStack&);
template <class A> void visit(A&, Inventory::Saved&, const Inventory::Saved&);
template <class A> void visit(A&, Party::Saved&, const Party::Saved&);
template <class A> void visit(A&, GroundPlacement&, const GroundPlacement&);
template <class A> void visit(A&, RecoveryLevel&, const RecoveryLevel&);
template <class A> void visit(A&, FloorObjectiveDefinition&, const FloorObjectiveDefinition&);
template <class A> void visit(A&, FixtureDefinition&, const FixtureDefinition&);
template <class A> void visit(A&, FloorFixtures&, const FloorFixtures&);
template <class A> void visit(A&, generation::GenerationRequest&, const generation::GenerationRequest&);
template <class A> void visit(A&, generation::Room&, const generation::Room&);
template <class A> void visit(A&, generation::Connection&, const generation::Connection&);
template <class A> void visit(A&, generation::PopulationPlacement&, const generation::PopulationPlacement&);
template <class A> void visit(A&, generation::FloorLayout&, const generation::FloorLayout&);
template <class A> void visit(A&, generation::Failure&, const generation::Failure&);
template <class A>
void visit(A&, generation::GenerationDiagnostics&, const generation::GenerationDiagnostics&);
template <class A> void visit(A&, Navigation::Request&, const Navigation::Request&);
template <class A> void visit(A&, Navigation::Result&, const Navigation::Result&);
template <class A> void visit(A&, Navigation::Saved&, const Navigation::Saved&);
template <class A> void visit(A&, CampaignLocationState&, const CampaignLocationState&);
template <class A> void visit(A&, CampaignCheckpoint&, const CampaignCheckpoint&);
template <class A> void visit(A&, EffectId&, const EffectId&);
template <class A> void visit(A&, AreaId&, const AreaId&);
template <class A> void visit(A&, rules::CharacterId&, const rules::CharacterId&);
template <class A> void visit(A& a, CellSurface& v, const CellSurface& base) {
    a("floor", v.floor, base.floor);
    a("ceiling", v.ceiling, base.ceiling);
    a("wallMaterial", v.wallMaterial, base.wallMaterial);
    a("floorMaterial", v.floorMaterial, base.floorMaterial);
    a("ceilingMaterial", v.ceilingMaterial, base.ceilingMaterial);
    a("environment", v.environment, base.environment);
}
template <class A> void visit(A& a, Decoration& v, const Decoration& base) {
    a("container", v.container, base.container);
    a("sealed", v.sealed, base.sealed);
    a("cell", v.cell, base.cell);
    a("face", v.face, base.face);
    a("kind", v.kind, base.kind);
    a("material", v.material, base.material);
    a("offset", v.offset, base.offset);
    a("elevation", v.elevation, base.elevation);
    a("width", v.width, base.width);
    a("height", v.height, base.height);
    a("depth", v.depth, base.depth);
}
template <class A> void visit(A& a, BuildingPlacement& v, const BuildingPlacement& base) {
    a("model", v.model, base.model);
    a("anchor", v.anchor, base.anchor);
    a("footprint", v.footprint, base.footprint);
    a("facing", v.facing, base.facing);
    a("width", v.width, base.width);
    a("depth", v.depth, base.depth);
}
template <class A> void visit(A& a, AmbientFire& v, const AmbientFire& base) {
    a("x", v.x, base.x);
    a("y", v.y, base.y);
    a("z", v.z, base.z);
}
template <class A> void visit(A& a, WorldDefinition& v, const WorldDefinition& base) {
    a("hearth", v.hearth, base.hearth);
    a("assetPack", v.assetPack, base.assetPack);
    a("surfaces", v.surfaces, base.surfaces);
    a("decorations", v.decorations, base.decorations);
    a("buildings", v.buildings, base.buildings);
    a("scenery", v.scenery, base.scenery);
}
template <class A> void visit(A& a, SceneryPlacement& v, const SceneryPlacement& base) {
    a("model", v.model, base.model);
    a("anchor", v.anchor, base.anchor);
    a("footprint", v.footprint, base.footprint);
    a("facing", v.facing, base.facing);
    a("width", v.width, base.width);
    a("depth", v.depth, base.depth);
}
template <class A> void visit(A& a, Cell& v, const Cell& base) {
    a("x", v.x, base.x);
    a("y", v.y, base.y);
}
template <class A> void visit(A& a, ActorPosition& v, const ActorPosition& base) {
    a("x", v.x, base.x);
    a("y", v.y, base.y);
}
template <class A> void visit(A& a, ActorId& v, const ActorId& base) {
    a("slot", v.slot, base.slot);
}
template <class A> void visit(A& a, NpcInstance& v, const NpcInstance& base) {
    a("type", v.type, base.type);
    a("cell", v.cell, base.cell);
    a("subcell", v.subcell, base.subcell);
    a("facing", v.facing, base.facing);
    a("animation", v.animation, base.animation);
    a("phaseMs", v.phaseMs, base.phaseMs);
    a("ai", v.ai, base.ai);
    a("monster", v.monster, base.monster);
    a("role", v.role, base.role);
    a("residentId", v.residentId, base.residentId);
}
template <class A> void visit(A& a, ActorState& v, const ActorState& base) {
    a("npc", v.npc, base.npc);
    a("monster", v.monster, base.monster);
    a("hitPoints", v.hitPoints, base.hitPoints);
    a("maxHitPoints", v.maxHitPoints, base.maxHitPoints);
    a("life", v.life, base.life);
    a("hasDeath", v.hasDeath, base.hasDeath);
    a("hasDamage", v.hasDamage, base.hasDamage);
    a("damageTimeMs", v.damageTimeMs, base.damageTimeMs);
    a("hasAttack", v.hasAttack, base.hasAttack);
    a("rangedAttack", v.rangedAttack, base.rangedAttack);
    a("attackTimeMs", v.attackTimeMs, base.attackTimeMs);
    a("resumableAttack", v.resumableAttack, base.resumableAttack);
    a("attackPausedMs", v.attackPausedMs, base.attackPausedMs);
    a("attackPauseFromMs", v.attackPauseFromMs, base.attackPauseFromMs);
    a("attackPauseUntilMs", v.attackPauseUntilMs, base.attackPauseUntilMs);
    a("hasWalk", v.hasWalk, base.hasWalk);
    a("walkTimeMs", v.walkTimeMs, base.walkTimeMs);
    a("walkDurationMs", v.walkDurationMs, base.walkDurationMs);
    a("hasTravel", v.hasTravel, base.hasTravel);
    a("walkFrom", v.walkFrom, base.walkFrom);
    a("walkFromElevation", v.walkFromElevation, base.walkFromElevation);
}
template <class A> void visit(A& a, ActorSnapshot& v, const ActorSnapshot& base) {
    a("count", v.count, base.count);
    a.counted("actors", v.actors, v.count, base.actors);
    a("typeCount", v.typeCount, base.typeCount);
    a("revision", v.revision, base.revision);
}
template <class A> void visit(A& a, FloorGeometry::Saved& v, const FloorGeometry::Saved& base) {
    a("completed", v.completed, base.completed);
    a("open", v.open, base.open);
    a("rememberedOpen", v.rememberedOpen, base.rememberedOpen);
    a("known", v.known, base.known);
    a("remembered", v.remembered, base.remembered);
    a("discovered", v.discovered, base.discovered);
    a("revision", v.revision, base.revision);
    a("geometryRevision", v.geometryRevision, base.geometryRevision);
    a("discoveryRevision", v.discoveryRevision, base.discoveryRevision);
}
template <class A> void visit(A& a, LootContainer& v, const LootContainer& base) {
    a("cell", v.cell, base.cell);
    a("facing", v.facing, base.facing);
    a("opened", v.opened, base.opened);
    a("sealed", v.sealed, base.sealed);
}
template <class A> void visit(A& a, FloorLoot& v, const FloorLoot& base) {
    a("generation", v.generation, base.generation);
    a("container", v.container, base.container);
    a("quantity", v.quantity, base.quantity);
    a("active", v.active, base.active);
    if (v.container != noContainer)
        a("cell", v.cell, base.cell);
    if (!v.quantity && !v.active)
        return;
    if (v.container == noContainer)
        a("cell", v.cell, base.cell);
    a("item", v.item, base.item);
}
template <class A> void visit(A& a, LootSnapshot& v, const LootSnapshot& base) {
    a("containerCount", v.containerCount, base.containerCount);
    a.counted("containers", v.containers, v.containerCount, base.containers);
    a("stacks", v.stacks, base.stacks);
    a("revision", v.revision, base.revision);
}
template <class A> void visit(A& a, LootWorld::Saved& v, const LootWorld::Saved& base) {
    a("state", v.state, base.state);
    a("deaths", v.deaths, base.deaths);
    a("actorRevision", v.actorRevision, base.actorRevision);
}
template <class A> void visit(A& a, NpcAiState& v, const NpcAiState& base) {
    a("awareness", v.awareness, base.awareness);
    a("home", v.home, base.home);
    a("lastSeen", v.lastSeen, base.lastSeen);
    a("homeFacing", v.homeFacing, base.homeFacing);
    a("config", v.config, base.config);
    a("waypoint", v.waypoint, base.waypoint);
    a("searchTurns", v.searchTurns, base.searchTurns);
    a("lastSeenMs", v.lastSeenMs, base.lastSeenMs);
    a("searchStartMs", v.searchStartMs, base.searchStartMs);
    a("moveDueMs", v.moveDueMs, base.moveDueMs);
}
template <class A> void visit(A& a, NpcAiSnapshot& v, const NpcAiSnapshot& base) {
    a("count", v.count, base.count);
    a.counted("actors", v.actors, v.count, base.actors);
    a("searchWork", v.searchWork, base.searchWork);
    a("completedSearches", v.completedSearches, base.completedSearches);
    a("timeMs", v.timeMs, base.timeMs);
}
template <class A> void visit(A& a, NpcAiController::Saved& v, const NpcAiController::Saved& base) {
    a("state", v.state, base.state);
    a("prepared", v.prepared, base.prepared);
    a("rng", v.rng, base.rng);
    a("tickDue", v.tickDue, base.tickDue);
    a("priority", v.priority, base.priority);
}
template <class A> void visit(A& a, EffectTarget& v, const EffectTarget& base) {
    a("npc", v.npc, base.npc);
    a("character", v.character, base.character);
    a("enemy", v.enemy, base.enemy);
}
template <class A> void visit(A& a, EffectSource& v, const EffectSource& base) {
    a("npc", v.npc, base.npc);
    a("character", v.character, base.character);
    a("enemy", v.enemy, base.enemy);
}
template <class A> void visit(A& a, EffectInstance& v, const EffectInstance& base) {
    a("active", v.active, base.active);
    a("reserved", v.reserved, base.reserved);
    a("pendingBless", v.pendingBless, base.pendingBless);
    if (!v.active && !v.reserved && !v.pendingBless)
        return;
    a("count", v.count, base.count);
    a("id", v.id, base.id);
    a("kind", v.kind, base.kind);
    a("caster", v.caster, base.caster);
    a.counted("targets", v.targets, v.count, base.targets);
    a("difficulty", v.difficulty, base.difficulty);
    a("expiresMs", v.expiresMs, base.expiresMs);
    a("retryMs", v.retryMs, base.retryMs);
    a("duplicates", v.duplicates, base.duplicates);
    a("cell", v.cell, base.cell);
    a("readyMs", v.readyMs, base.readyMs);
}
template <class A> void visit(A& a, ConditionState& v, const ConditionState& base) {
    a("active", v.active, base.active);
    if (!v.active)
        return;
    a("caster", v.caster, base.caster);
    a("untilMs", v.untilMs, base.untilMs);
    a("remainingMs", v.remainingMs, base.remainingMs);
    a("casterTurn", v.casterTurn, base.casterTurn);
    a("turnBound", v.turnBound, base.turnBound);
}
template <class A> void visit(A& a, GroundArea& v, const GroundArea& base) {
    a("active", v.active, base.active);
    if (!v.active)
        return;
    a("id", v.id, base.id);
    a("cell", v.cell, base.cell);
    a("caster", v.caster, base.caster);
    a("difficulty", v.difficulty, base.difficulty);
    a("untilMs", v.untilMs, base.untilMs);
    a("web", v.web, base.web);
    a("effect", v.effect, base.effect);
    a("burnMs", v.burnMs, base.burnMs);
    a("footprintOrigin", v.footprint.origin, base.footprint.origin);
    a("footprintMask", v.footprint.mask, base.footprint.mask);
    a("burnDue", v.burnDue, base.burnDue);
}
template <class A> void visit(A& a, SpectralWeapon& v, const SpectralWeapon& base) {
    a("active", v.active, base.active);
    if (!v.active)
        return;
    a("cell", v.cell, base.cell);
}
template <class A> void visit(A& a, GroundSnapshot& v, const GroundSnapshot& base) {
    a("weapons", v.weapons, base.weapons);
    a("areas", v.areas, base.areas);
    a("revision", v.revision, base.revision);
}
template <class A> void visit(A& a, Restraint& v, const Restraint& base) {
    a("area", v.area, base.area);
    if (!v.area.serial)
        return;
    a("cell", v.cell, base.cell);
    a("difficulty", v.difficulty, base.difficulty);
    a("caster", v.caster, base.caster);
}
template <class A> void visit(A& a, EffectController::Saved& v, const EffectController::Saved& base) {
    a("instances", v.instances, base.instances);
    a("guidingUntil", v.guidingUntil, base.guidingUntil);
    a("nextSerial", v.nextSerial, base.nextSerial);
    a("gameplayMs", v.gameplayMs, base.gameplayMs);
    a("conditions", v.conditions, base.conditions);
    a("casterTurns", v.casterTurns, base.casterTurns);
    a("ground", v.ground, base.ground);
    a("restrained", v.restrained, base.restrained);
    a("nextArea", v.nextArea, base.nextArea);
    a("groundDueMs", v.groundDueMs, base.groundDueMs);
    a("turns", v.turns, base.turns);
}
template <class A> void visit(A& a, CombatParticipant& v, const CombatParticipant& base) {
    a("character", v.character, base.character);
    a("enemy", v.enemy, base.enemy);
    a("deathSave", v.deathSave, base.deathSave);
}
template <class A> void visit(A& a, CombatSnapshot& v, const CombatSnapshot& base) {
    a("upcomingCount", v.upcomingCount, base.upcomingCount);
    a("enemyCount", v.enemyCount, base.enemyCount);
    a("phase", v.phase, base.phase);
    a("turn", v.turn, base.turn);
    a("round", v.round, base.round);
    a("activeCharacter", v.activeCharacter, base.activeCharacter);
    a("activeEnemy", v.activeEnemy, base.activeEnemy);
    a("awaitingAction", v.awaitingAction, base.awaitingAction);
    a("resolvingAction", v.resolvingAction, base.resolvingAction);
    a("canEndTurn", v.canEndTurn, base.canEndTurn);
    a("defending", v.defending, base.defending);
    a.counted("enemies", v.enemies, v.enemyCount, base.enemies);
    a.counted("upcoming", v.upcoming, v.upcomingCount, base.upcoming);
    a("cooldownMs", v.cooldownMs, base.cooldownMs);
    a("spent", v.spent, base.spent);
    a("facing", v.facing, base.facing);
    a("knownDirections", v.knownDirections, base.knownDirections);
    a("pendingCount", v.pendingCount, base.pendingCount);
    a("gameplayMs", v.gameplayMs, base.gameplayMs);
}
template <class A> void visit(A& a, CombatController::Saved& v, const CombatController::Saved& base) {
    a("state", v.state, base.state);
    a("sneakReady", v.sneakReady, base.sneakReady);
    a("cunningReady", v.cunningReady, base.cunningReady);
    a("extraUntil", v.extraUntil, base.extraUntil);
    a("partyReady", v.partyReady, base.partyReady);
    a("saveDue", v.saveDue, base.saveDue);
    a("defendDue", v.defendDue, base.defendDue);
    a("processedCasterRound", v.processedCasterRound, base.processedCasterRound);
    a("extraAction", v.extraAction, base.extraAction);
    a("known", v.known, base.known);
    a("episodeActors", v.episodeActors, base.episodeActors);
    a("knownUntil", v.knownUntil, base.knownUntil);
    a("enemyReady", v.enemyReady, base.enemyReady);
    a("enemyBusy", v.enemyBusy, base.enemyBusy);
    a("nextAttack", v.nextAttack, base.nextAttack);
    a("autoCursor", v.autoCursor, base.autoCursor);
    a("enemyMana", v.enemyMana, base.enemyMana);
    a("enemyTarget", v.enemyTarget, base.enemyTarget);
}
template <class A> void visit(A& a, FloorObjectiveSnapshot& v, const FloorObjectiveSnapshot& base) {
    a("count", v.count, base.count);
    a.counted("members", v.members, v.count, base.members);
    a("known", v.known, base.known);
    a("completed", v.completed, base.completed);
    a("rewardIssued", v.rewardIssued, base.rewardIssued);
    a("capacityFailure", v.capacityFailure, base.capacityFailure);
    a("revision", v.revision, base.revision);
}
template <class A> void visit(A& a, FloorState& v, const FloorState& base) {
    a("fixtures", v.fixtures, base.fixtures);
    a("actors", v.actors, base.actors);
    a("loot", v.loot, base.loot);
    a("ai", v.ai, base.ai);
    a("effects", v.effects, base.effects);
    a("combat", v.combat, base.combat);
    a("objective", v.objective, base.objective);
    a("creditedDeaths", v.creditedDeaths, base.creditedDeaths);
    a("rewardRevision", v.rewardRevision, base.rewardRevision);
    a("timeMs", v.timeMs, base.timeMs);
    a("restToken", v.restToken, base.restToken);
    a("remainderMs", v.remainderMs, base.remainderMs);
    a("rng", v.rng, base.rng);
}
template <class A> void visit(A& a, Player& v, const Player& base) {
    a("cell", v.cell, base.cell);
    a("heading", v.heading, base.heading);
}
template <class A> void visit(A& a, rules::CharacterIdentity& v, const rules::CharacterIdentity& base) {
    a("id", v.id, base.id);
    a("name", v.name, base.name);
    a("race", v.race, base.race);
    a("characterClass", v.characterClass, base.characterClass);
    a("baseAbilities", v.baseAbilities, base.baseAbilities);
    a("level", v.level, base.level);
    a("defense", v.defense, base.defense);
    a("lifeDomain", v.lifeDomain, base.lifeDomain);
    a("portrait", v.portrait, base.portrait);
    a("spells", v.spells, base.spells);
}
template <class A> void visit(A& a, PartyCharacter& v, const PartyCharacter& base) {
    a("hitPoints", v.hitPoints, base.hitPoints);
    a("life", v.life, base.life);
    a("successes", v.successes, base.successes);
    a("failures", v.failures, base.failures);
    a("temporaryHitPoints", v.temporaryHitPoints, base.temporaryHitPoints);
    a("build", v.build, base.build);
    a("availableHitDice", v.availableHitDice, base.availableHitDice);
    a("mana", v.mana, base.mana);
    a("secondWind", v.secondWind, base.secondWind);
    a("arcaneRecovery", v.arcaneRecovery, base.arcaneRecovery);
    a("experience", v.experience, base.experience);
    a("aidBonus", v.aidBonus, base.aidBonus);
    a("experienceFraction", v.experienceFraction, base.experienceFraction);
    a("actionSurge", v.actionSurge, base.actionSurge);
    a("channelDivinity", v.channelDivinity, base.channelDivinity);
    a("temporaryUntilMs", v.temporaryUntilMs, base.temporaryUntilMs);
}
template <class A> void visit(A& a, PartySnapshot& v, const PartySnapshot& base) {
    a("characters", v.characters, base.characters);
    a("formation", v.formation, base.formation);
}
template <class A> void visit(A& a, ItemStack& v, const ItemStack& base) {
    a("generation", v.generation, base.generation);
    a("quantity", v.quantity, base.quantity);
    if (!v.quantity)
        return;
    a("item", v.item, base.item);
}
template <class A> void visit(A& a, Inventory::Saved& v, const Inventory::Saved& base) {
    a("stacks", v.stacks, base.stacks);
    a("locations", v.locations, base.locations);
    a("revision", v.revision, base.revision);
}
template <class A> void visit(A& a, Party::Saved& v, const Party::Saved& base) {
    a("state", v.state, base.state);
    a("inventory", v.inventory, base.inventory);
    a.required("gold", v.gold, base.gold);
    a("questKeys", v.questKeys, base.questKeys);
    a("preparationRevision", v.preparationRevision, base.preparationRevision);
    a("progressionRevision", v.progressionRevision, base.progressionRevision);
}
template <class A> void visit(A& a, GroundPlacement& v, const GroundPlacement& base) {
    a("container", v.container, base.container);
    a("cell", v.cell, base.cell);
    a("item", v.item, base.item);
    a("quantity", v.quantity, base.quantity);
}
template <class A> void visit(A& a, RecoveryLevel& v, const RecoveryLevel& base) {
    a("groundCount", v.groundCount, base.groundCount);
    a("sanctuaryCount", v.sanctuaryCount, base.sanctuaryCount);
    a("freeRestEverywhere", v.freeRestEverywhere, base.freeRestEverywhere);
    a.counted("sanctuaries", v.sanctuaries, v.sanctuaryCount, base.sanctuaries);
    a.counted("ground", v.ground, v.groundCount, base.ground);
}
template <class A> void visit(A& a, FloorObjectiveDefinition& v, const FloorObjectiveDefinition& base) {
    a("rewardCount", v.rewardCount, base.rewardCount);
    a("count", v.count, base.count);
    a.counted("members", v.members, v.count, base.members);
    a("approachDoor", v.approachDoor, base.approachDoor);
    a("gate", v.gate, base.gate);
    a.counted("rewards", v.rewards, v.rewardCount, base.rewards);
    a("lootSeed", v.lootSeed, base.lootSeed);
}
template <class A> void visit(A& a, FixtureDefinition& v, const FixtureDefinition& base) {
    a("cell", v.cell, base.cell);
    a("facing", v.facing, base.facing);
    a("kind", v.kind, base.kind);
    a("key", v.key, base.key);
}
template <class A> void visit(A& a, FloorFixtures& v, const FloorFixtures& base) {
    a("landingCount", v.landingCount, base.landingCount);
    a("count", v.count, base.count);
    a.counted("entries", v.entries, v.count, base.entries);
    a("depth", v.depth, base.depth);
    a("discovery", v.discovery, base.discovery);
    a.counted("landings", v.landings, v.landingCount, base.landings);
    a("npcExcluded", v.npcExcluded, base.npcExcluded);
}
template <class A>
void visit(A& a, generation::GenerationRequest& v, const generation::GenerationRequest& base) {
    a("seed", v.seed, base.seed);
    a("depth", v.depth, base.depth);
    a("generator", v.generator, base.generator);
    a("templates", v.templates, base.templates);
}
template <class A> void visit(A& a, generation::Room& v, const generation::Room& base) {
    a("origin", v.origin, base.origin);
    a("size", v.size, base.size);
    a("anchor", v.anchor, base.anchor);
    a("role", v.role, base.role);
    a("id", v.id, base.id);
    a("shape", v.shape, base.shape);
    a("rotation", v.rotation, base.rotation);
    a("clearance", v.clearance, base.clearance);
    a("facing", v.facing, base.facing);
}
template <class A> void visit(A& a, generation::Connection& v, const generation::Connection& base) {
    a("a", v.a, base.a);
    a("b", v.b, base.b);
    a("socketA", v.socketA, base.socketA);
    a("socketB", v.socketB, base.socketB);
    a("facingA", v.facingA, base.facingA);
    a("facingB", v.facingB, base.facingB);
    a("first", v.first, base.first);
    a("count", v.count, base.count);
}
template <class A>
void visit(A& a, generation::PopulationPlacement& v, const generation::PopulationPlacement& base) {
    a("cell", v.cell, base.cell);
    a("subcell", v.subcell, base.subcell);
    a("facing", v.facing, base.facing);
    a("recipe", v.recipe, base.recipe);
    a("id", v.id, base.id);
    a("guardian", v.guardian, base.guardian);
}
template <class A> void visit(A& a, generation::Encounter& v, const generation::Encounter& base) {
    a("recipe", v.recipe, base.recipe);
    a("role", v.role, base.role);
    a("room", v.room, base.room);
    a("first", v.first, base.first);
    a("count", v.count, base.count);
}
template <class A> void visit(A& a, generation::CacheSite& v, const generation::CacheSite& base) {
    a("cell", v.cell, base.cell);
    a("facing", v.facing, base.facing);
}
template <class A> void visit(A& a, generation::FloorLayout& v, const generation::FloorLayout& base) {
    a("routeCount", v.routeCount, base.routeCount);
    a("encounterCount", v.encounterCount, base.encounterCount);
    a("populationCount", v.populationCount, base.populationCount);
    a("connectionCount", v.connectionCount, base.connectionCount);
    a("roomCount", v.roomCount, base.roomCount);
    a("pathCount", v.pathCount, base.pathCount);
    a("cacheCount", v.cacheCount, base.cacheCount);
    a.counted("caches", v.caches, v.cacheCount, base.caches);
    a("dressingFingerprint", v.dressingFingerprint, base.dressingFingerprint);
    a("width", v.width, base.width);
    a("height", v.height, base.height);
    a("floor", v.floor, base.floor);
    a("membership", v.membership, base.membership);
    a.counted("paths", v.paths, v.pathCount, base.paths);
    a("biome", v.biome, base.biome);
    a("algorithm", v.algorithm, base.algorithm);
    a("profile", v.profile, base.profile);
    a.counted("rooms", v.rooms, v.roomCount, base.rooms);
    a.counted("connections", v.connections, v.connectionCount, base.connections);
    a("family", v.family, base.family);
    a("rotation", v.rotation, base.rotation);
    a("loopConnection", v.loopConnection, base.loopConnection);
    a("spawn", v.spawn, base.spawn);
    a("depth", v.depth, base.depth);
    a("fixtures", v.fixtures, base.fixtures);
    a.counted("population", v.population, v.populationCount, base.population);
    a.counted("encounters", v.encounters, v.encounterCount, base.encounters);
    a.counted("route", v.route, v.routeCount, base.route);
    a("supply", v.supply, base.supply);
    a("optionalSupply", v.optionalSupply, base.optionalSupply);
    a("guardianReward", v.guardianReward, base.guardianReward);
    a("guardianApproach", v.guardianApproach, base.guardianApproach);
    a("approachDoor", v.approachDoor, base.approachDoor);
    a("guardianGate", v.guardianGate, base.guardianGate);
}
template <class A> void visit(A& a, generation::Failure& v, const generation::Failure& base) {
    a("code", v.code, base.code);
    a("room", v.room, base.room);
    a("connection", v.connection, base.connection);
}
template <class A>
void visit(A& a, generation::GenerationDiagnostics& v, const generation::GenerationDiagnostics& base) {
    a("attempts", v.attempts, base.attempts);
    a("rejected", v.rejected, base.rejected);
    a("fallback", v.fallback, base.fallback);
    a("fallbackId", v.fallbackId, base.fallbackId);
    a("failure", v.failure, base.failure);
}
template <class A> void visit(A& a, Navigation::Request& v, const Navigation::Request& base) {
    a("pending", v.pending, base.pending);
    if (!v.pending)
        return;
    a("from", v.from, base.from);
    a("target", v.target, base.target);
    a("adjacent", v.adjacent, base.adjacent);
    a("preferred", v.preferred, base.preferred);
}
template <class A> void visit(A& a, Navigation::Result& v, const Navigation::Result& base) {
    a("ready", v.ready, base.ready);
    if (!v.ready)
        return;
    a("found", v.found, base.found);
    a("from", v.from, base.from);
    a("target", v.target, base.target);
    a("step", v.step, base.step);
    a("adjacent", v.adjacent, base.adjacent);
    a("preferred", v.preferred, base.preferred);
}
template <class A> void visit(A& a, Navigation::Saved& v, const Navigation::Saved& base) {
    a("tail", v.tail, base.tail);
    a.counted("queue", v.queue, v.tail, base.queue);
    a("seen", v.seen, base.seen);
    a("first", v.first, base.first);
    a("requests", v.requests, base.requests);
    a("results", v.results, base.results);
    a("active", v.active, base.active);
    a("geometryRevision", v.geometryRevision, base.geometryRevision);
    a("owner", v.owner, base.owner);
    a("cursor", v.cursor, base.cursor);
    a("head", v.head, base.head);
    a("cleanup", v.cleanup, base.cleanup);
    a("work", v.work, base.work);
    a("completed", v.completed, base.completed);
    a("cleaning", v.cleaning, base.cleaning);
    a("canceled", v.canceled, base.canceled);
    a("preferAvailable", v.preferAvailable, base.preferAvailable);
    a("centerAvailable", v.centerAvailable, base.centerAvailable);
    a("fallbackStep", v.fallbackStep, base.fallbackStep);
    a("frontStep", v.frontStep, base.frontStep);
}
template <class A> void visit(A& a, CampaignLocationState& v, const CampaignLocationState& base) {
    a("generated", v.generated, base.generated);
    a("visited", v.visited, base.visited);
    a("generation", v.generation, base.generation);
    a("layout", v.layout, base.layout);
    a("fingerprint", v.fingerprint, base.fingerprint);
    a("world", v.world, base.world);
    a("worldFingerprint", v.worldFingerprint, base.worldFingerprint);
    a("rows", v.rows, base.rows);
    a("types", v.types, base.types);
    a("placements", v.placements, base.placements);
    a("recovery", v.recovery, base.recovery);
    a("fixtures", v.fixtures, base.fixtures);
    a("objective", v.objective, base.objective);
    a.floor("state", v.state, base);
    a("entry", v.entry, base.entry);
    a("exit", v.exit, base.exit);
    a("seed", v.seed, base.seed);
}
template <class A> void visit(A& a, DialogueHistoryRecord& v, const DialogueHistoryRecord& base) {
    a("factCount", v.factCount, base.factCount);
    a("completedCount", v.completedCount, base.completedCount);
    a("id", v.id, base.id);
    a("introduced", v.introduced, base.introduced);
    a.counted("completed", v.completed, v.completedCount, base.completed);
    a.counted("facts", v.facts, v.factCount, base.facts);
}
template <class A> void visit(A& a, DialogueHistory& v, const DialogueHistory& base) {
    a("count", v.count, base.count);
    a.counted("records", v.records, v.count, base.records);
}
template <class A> void visit(A& a, BuybackStack& v, const BuybackStack& base) {
    a("quantity", v.quantity, base.quantity);
    if (!v.quantity)
        return;
    a("item", v.item, base.item);
    a("price", v.price, base.price);
    a("expiresMs", v.expiresMs, base.expiresMs);
}
template <class A> void visit(A& a, MerchantState& v, const MerchantState& base) {
    a("resident", v.resident, base.resident);
    a("shop", v.shop, base.shop);
    a("revision", v.revision, base.revision);
    a("restockMs", v.restockMs, base.restockMs);
    a("stock", v.stock, base.stock);
    a("buyback", v.buyback, base.buyback);
}
template <class A> void visit(A& a, CampaignCheckpoint& v, const CampaignCheckpoint& base) {
    a("dialogue", v.dialogue, base.dialogue);
    a("merchants", v.merchants, base.merchants);
    a("party", v.party, base.party);
    a("locations", v.locations, base.locations);
    a("navigation", v.navigation, base.navigation);
    a("pose", v.pose, base.pose);
    a("animated", v.animated, base.animated);
    a("activeLocation", v.activeLocation, base.activeLocation);
    a("gameplayMs", v.gameplayMs, base.gameplayMs);
    a("revision", v.revision, base.revision);
    a("runId", v.runId, base.runId);
}
template <class A> void visit(A& a, EffectId& v, const EffectId& base) {
    a("serial", v.serial, base.serial);
}
template <class A> void visit(A& a, AreaId& v, const AreaId& base) {
    a("serial", v.serial, base.serial);
}
template <class A> void visit(A& a, rules::CharacterId& v, const rules::CharacterId& base) {
    a("index", v.index, base.index);
}

template <class A> void visit(A& a, CampaignSaveData& v, const CampaignSaveData& base) {
    a("dialogue", v.dialogue, base.dialogue);
    a("merchants", v.merchants, base.merchants);
    a("party", v.party, base.party);
    a("locations", v.locations, base.locations);
    a("navigation", v.navigation, base.navigation);
    a("pose", v.pose, base.pose);
    a("animated", v.animated, base.animated);
    a("activeLocation", v.activeLocation, base.activeLocation);
    a("gameplayMs", v.gameplayMs, base.gameplayMs);
    a("revision", v.revision, base.revision);
    a("runId", v.runId, base.runId);
}

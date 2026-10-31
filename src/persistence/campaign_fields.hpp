template <class A> void visit(A&, CellSurface&);
template <class A> void visit(A&, Decoration&);
template <class A> void visit(A&, WorldDefinition&);
template <class A> void visit(A& a, NpcAiConfig& v) {
    a("routine", v.routine);
    a("waypoints", v.waypoints);
    a("waypointCount", v.waypointCount);
}
// Internal explicit schema field list. No padding, pointers, derived caches or runtime epochs.
template <class A> void visit(A&, Cell&);
template <class A> void visit(A&, ActorPosition&);
template <class A> void visit(A&, ActorId&);
template <class A> void visit(A&, NpcInstance&);
template <class A> void visit(A&, ActorState&);
template <class A> void visit(A&, ActorSnapshot&);
template <class A> void visit(A&, FloorGeometry::Saved&);
template <class A> void visit(A&, FloorLoot&);
template <class A> void visit(A&, LootSnapshot&);
template <class A> void visit(A&, LootWorld::Saved&);
template <class A> void visit(A&, NpcAiState&);
template <class A> void visit(A&, NpcAiSnapshot&);
template <class A> void visit(A&, NpcAiController::Saved&);
template <class A> void visit(A&, EffectTarget&);
template <class A> void visit(A&, EffectInstance&);
template <class A> void visit(A&, ConditionState&);
template <class A> void visit(A&, GroundArea&);
template <class A> void visit(A&, SpectralWeapon&);
template <class A> void visit(A&, GroundSnapshot&);
template <class A> void visit(A&, Restraint&);
template <class A> void visit(A&, EffectController::Saved&);
template <class A> void visit(A&, CombatParticipant&);
template <class A> void visit(A&, CombatSnapshot&);
template <class A> void visit(A&, CombatController::Saved&);
template <class A> void visit(A&, FloorObjectiveSnapshot&);
template <class A> void visit(A&, FloorState&);
template <class A> void visit(A&, Player&);
template <class A> void visit(A&, rules::CharacterIdentity&);
template <class A> void visit(A&, PartyCharacter&);
template <class A> void visit(A&, PartySnapshot&);
template <class A> void visit(A&, ItemStack&);
template <class A> void visit(A&, Inventory::Saved&);
template <class A> void visit(A&, Party::Saved&);
template <class A> void visit(A&, GroundPlacement&);
template <class A> void visit(A&, RecoveryLevel&);
template <class A> void visit(A&, FloorObjectiveDefinition&);
template <class A> void visit(A&, FixtureDefinition&);
template <class A> void visit(A&, FloorFixtures&);
template <class A> void visit(A&, generation::GenerationRequest&);
template <class A> void visit(A&, generation::Room&);
template <class A> void visit(A&, generation::Connection&);
template <class A> void visit(A&, generation::PopulationPlacement&);
template <class A> void visit(A&, generation::FloorLayout&);
template <class A> void visit(A&, generation::Failure&);
template <class A> void visit(A&, generation::GenerationDiagnostics&);
template <class A> void visit(A&, Navigation::Request&);
template <class A> void visit(A&, Navigation::Result&);
template <class A> void visit(A&, Navigation::Saved&);
template <class A> void visit(A&, CampaignLocationState&);
template <class A> void visit(A&, CampaignCheckpoint&);
template <class A> void visit(A&, EffectId&);
template <class A> void visit(A&, AreaId&);
template <class A> void visit(A&, rules::CharacterId&);
template <class A> void visit(A& a, CellSurface& v) {
    a("floor", v.floor);
    a("ceiling", v.ceiling);
    a("wallMaterial", v.wallMaterial);
    a("floorMaterial", v.floorMaterial);
    a("ceilingMaterial", v.ceilingMaterial);
    a("environment", v.environment);
}
template <class A> void visit(A& a, Decoration& v) {
    a("cell", v.cell);
    a("face", v.face);
    a("kind", v.kind);
    a("material", v.material);
    a("offset", v.offset);
    a("elevation", v.elevation);
    a("width", v.width);
    a("height", v.height);
    a("depth", v.depth);
}
template <class A> void visit(A& a, WorldDefinition& v) {
    a("surfaces", v.surfaces);
    a("decorations", v.decorations);
}
template <class A> void visit(A& a, Cell& v) {
    a("x", v.x);
    a("y", v.y);
}
template <class A> void visit(A& a, ActorPosition& v) {
    a("x", v.x);
    a("y", v.y);
}
template <class A> void visit(A& a, ActorId& v) {
    a("slot", v.slot);
}
template <class A> void visit(A& a, NpcInstance& v) {
    a("type", v.type);
    a("cell", v.cell);
    a("subcell", v.subcell);
    a("facing", v.facing);
    a("animation", v.animation);
    a("phaseMs", v.phaseMs);
    a("ai", v.ai);
    a("monster", v.monster);
}
template <class A> void visit(A& a, ActorState& v) {
    a("npc", v.npc);
    a("monster", v.monster);
    a("hitPoints", v.hitPoints);
    a("maxHitPoints", v.maxHitPoints);
    a("life", v.life);
    a("hasDeath", v.hasDeath);
    a("hasDamage", v.hasDamage);
    a("damageTimeMs", v.damageTimeMs);
    a("hasAttack", v.hasAttack);
    a("rangedAttack", v.rangedAttack);
    a("attackTimeMs", v.attackTimeMs);
    a("resumableAttack", v.resumableAttack);
    a("attackPausedMs", v.attackPausedMs);
    a("attackPauseFromMs", v.attackPauseFromMs);
    a("attackPauseUntilMs", v.attackPauseUntilMs);
    a("hasWalk", v.hasWalk);
    a("walkTimeMs", v.walkTimeMs);
    a("walkDurationMs", v.walkDurationMs);
    a("hasTravel", v.hasTravel);
    a("walkFrom", v.walkFrom);
    a("walkFromElevation", v.walkFromElevation);
}
template <class A> void visit(A& a, ActorSnapshot& v) {
    a("actors", v.actors);
    a("count", v.count);
    a("typeCount", v.typeCount);
    a("revision", v.revision);
}
template <class A> void visit(A& a, FloorGeometry::Saved& v) {
    a("completed", v.completed);
    a("known", v.known);
    a("remembered", v.remembered);
    a("discovered", v.discovered);
    a("revision", v.revision);
    a("geometryRevision", v.geometryRevision);
    a("discoveryRevision", v.discoveryRevision);
}
template <class A> void visit(A& a, FloorLoot& v) {
    a("cell", v.cell);
    a("item", v.item);
    a("quantity", v.quantity);
    a("active", v.active);
    a("generation", v.generation);
}
template <class A> void visit(A& a, LootSnapshot& v) {
    a("stacks", v.stacks);
    a("revision", v.revision);
}
template <class A> void visit(A& a, LootWorld::Saved& v) {
    a("state", v.state);
    a("deaths", v.deaths);
    a("actorRevision", v.actorRevision);
}
template <class A> void visit(A& a, NpcAiState& v) {
    a("awareness", v.awareness);
    a("home", v.home);
    a("lastSeen", v.lastSeen);
    a("homeFacing", v.homeFacing);
    a("config", v.config);
    a("waypoint", v.waypoint);
    a("searchTurns", v.searchTurns);
    a("lastSeenMs", v.lastSeenMs);
    a("searchStartMs", v.searchStartMs);
    a("moveDueMs", v.moveDueMs);
}
template <class A> void visit(A& a, NpcAiSnapshot& v) {
    a("actors", v.actors);
    a("count", v.count);
    a("searchWork", v.searchWork);
    a("completedSearches", v.completedSearches);
    a("timeMs", v.timeMs);
}
template <class A> void visit(A& a, NpcAiController::Saved& v) {
    a("state", v.state);
    a("prepared", v.prepared);
    a("rng", v.rng);
    a("tickDue", v.tickDue);
    a("priority", v.priority);
}
template <class A> void visit(A& a, EffectTarget& v) {
    a("npc", v.npc);
    a("character", v.character);
    a("enemy", v.enemy);
}
template <class A> void visit(A& a, EffectInstance& v) {
    a("id", v.id);
    a("kind", v.kind);
    a("caster", v.caster);
    a("targets", v.targets);
    a("count", v.count);
    a("difficulty", v.difficulty);
    a("expiresMs", v.expiresMs);
    a("retryMs", v.retryMs);
    a("active", v.active);
    a("duplicates", v.duplicates);
    a("reserved", v.reserved);
    a("cell", v.cell);
    a("readyMs", v.readyMs);
}
template <class A> void visit(A& a, ConditionState& v) {
    a("caster", v.caster);
    a("untilMs", v.untilMs);
    a("remainingMs", v.remainingMs);
    a("casterTurn", v.casterTurn);
    a("active", v.active);
    a("turnBound", v.turnBound);
}
template <class A> void visit(A& a, GroundArea& v) {
    a("id", v.id);
    a("cell", v.cell);
    a("caster", v.caster);
    a("difficulty", v.difficulty);
    a("untilMs", v.untilMs);
    a("active", v.active);
    a("web", v.web);
    a("effect", v.effect);
    a("burnMs", v.burnMs);
}
template <class A> void visit(A& a, SpectralWeapon& v) {
    a("cell", v.cell);
    a("active", v.active);
}
template <class A> void visit(A& a, GroundSnapshot& v) {
    a("weapons", v.weapons);
    a("areas", v.areas);
    a("revision", v.revision);
}
template <class A> void visit(A& a, Restraint& v) {
    a("area", v.area);
    a("cell", v.cell);
    a("difficulty", v.difficulty);
    a("caster", v.caster);
}
template <class A> void visit(A& a, EffectController::Saved& v) {
    a("instances", v.instances);
    a("guidingUntil", v.guidingUntil);
    a("nextSerial", v.nextSerial);
    a("gameplayMs", v.gameplayMs);
    a("conditions", v.conditions);
    a("casterTurns", v.casterTurns);
    a("ground", v.ground);
    a("restrained", v.restrained);
    a("nextArea", v.nextArea);
    a("groundDueMs", v.groundDueMs);
    a("turns", v.turns);
}
template <class A> void visit(A& a, CombatParticipant& v) {
    a("character", v.character);
    a("enemy", v.enemy);
    a("deathSave", v.deathSave);
}
template <class A> void visit(A& a, CombatSnapshot& v) {
    a("phase", v.phase);
    a("turn", v.turn);
    a("round", v.round);
    a("activeCharacter", v.activeCharacter);
    a("activeEnemy", v.activeEnemy);
    a("awaitingAction", v.awaitingAction);
    a("resolvingAction", v.resolvingAction);
    a("canEndTurn", v.canEndTurn);
    a("defending", v.defending);
    a("enemies", v.enemies);
    a("enemyCount", v.enemyCount);
    a("upcoming", v.upcoming);
    a("upcomingCount", v.upcomingCount);
    a("cooldownMs", v.cooldownMs);
    a("spent", v.spent);
    a("facing", v.facing);
    a("knownDirections", v.knownDirections);
    a("pendingCount", v.pendingCount);
    a("gameplayMs", v.gameplayMs);
}
template <class A> void visit(A& a, CombatController::Saved& v) {
    a("state", v.state);
    a("sneakReady", v.sneakReady);
    a("cunningReady", v.cunningReady);
    a("extraUntil", v.extraUntil);
    a("partyReady", v.partyReady);
    a("saveDue", v.saveDue);
    a("defendDue", v.defendDue);
    a("processedCasterRound", v.processedCasterRound);
    a("extraAction", v.extraAction);
    a("known", v.known);
    a("episodeActors", v.episodeActors);
    a("knownUntil", v.knownUntil);
    a("enemyReady", v.enemyReady);
    a("enemyBusy", v.enemyBusy);
    a("nextAttack", v.nextAttack);
    a("autoCursor", v.autoCursor);
}
template <class A> void visit(A& a, FloorObjectiveSnapshot& v) {
    a("members", v.members);
    a("count", v.count);
    a("known", v.known);
    a("completed", v.completed);
    a("rewardIssued", v.rewardIssued);
    a("capacityFailure", v.capacityFailure);
    a("revision", v.revision);
}
template <class A> void visit(A& a, FloorState& v) {
    a("fixtures", v.fixtures);
    a("actors", v.actors);
    a("loot", v.loot);
    a("ai", v.ai);
    a("effects", v.effects);
    a("combat", v.combat);
    a("objective", v.objective);
    a("creditedDeaths", v.creditedDeaths);
    a("rewardRevision", v.rewardRevision);
    a("timeMs", v.timeMs);
    a("restToken", v.restToken);
    a("remainderMs", v.remainderMs);
    a("rng", v.rng);
}
template <class A> void visit(A& a, Player& v) {
    a("cell", v.cell);
    a("heading", v.heading);
}
template <class A> void visit(A& a, rules::CharacterIdentity& v) {
    a("id", v.id);
    a("name", v.name);
    a("race", v.race);
    a("characterClass", v.characterClass);
    a("baseAbilities", v.baseAbilities);
    a("level", v.level);
    a("defense", v.defense);
    a("lifeDomain", v.lifeDomain);
    a("portrait", v.portrait);
    a("spells", v.spells);
}
template <class A> void visit(A& a, PartyCharacter& v) {
    a("hitPoints", v.hitPoints);
    a("life", v.life);
    a("successes", v.successes);
    a("failures", v.failures);
    a("temporaryHitPoints", v.temporaryHitPoints);
    a("build", v.build);
    a("availableHitDice", v.availableHitDice);
    a("mana", v.mana);
    a("secondWind", v.secondWind);
    a("arcaneRecovery", v.arcaneRecovery);
    a("experience", v.experience);
    a("aidBonus", v.aidBonus);
    a("experienceFraction", v.experienceFraction);
    a("actionSurge", v.actionSurge);
    a("channelDivinity", v.channelDivinity);
    a("temporaryUntilMs", v.temporaryUntilMs);
}
template <class A> void visit(A& a, PartySnapshot& v) {
    a("characters", v.characters);
    a("formation", v.formation);
}
template <class A> void visit(A& a, ItemStack& v) {
    a("item", v.item);
    a("quantity", v.quantity);
    a("generation", v.generation);
}
template <class A> void visit(A& a, Inventory::Saved& v) {
    a("stacks", v.stacks);
    a("locations", v.locations);
    a("revision", v.revision);
}
template <class A> void visit(A& a, Party::Saved& v) {
    a("state", v.state);
    a("inventory", v.inventory);
    a("questKeys", v.questKeys);
    a("preparationRevision", v.preparationRevision);
    a("progressionRevision", v.progressionRevision);
}
template <class A> void visit(A& a, GroundPlacement& v) {
    a("cell", v.cell);
    a("item", v.item);
    a("quantity", v.quantity);
}
template <class A> void visit(A& a, RecoveryLevel& v) {
    a("sanctuaries", v.sanctuaries);
    a("sanctuaryCount", v.sanctuaryCount);
    a("ground", v.ground);
    a("groundCount", v.groundCount);
}
template <class A> void visit(A& a, FloorObjectiveDefinition& v) {
    a("members", v.members);
    a("count", v.count);
    a("approachDoor", v.approachDoor);
    a("gate", v.gate);
    a("rewards", v.rewards);
    a("rewardCount", v.rewardCount);
    a("lootSeed", v.lootSeed);
}
template <class A> void visit(A& a, FixtureDefinition& v) {
    a("cell", v.cell);
    a("facing", v.facing);
    a("kind", v.kind);
    a("key", v.key);
}
template <class A> void visit(A& a, FloorFixtures& v) {
    a("entries", v.entries);
    a("count", v.count);
    a("depth", v.depth);
    a("discovery", v.discovery);
    a("landings", v.landings);
    a("landingCount", v.landingCount);
    a("npcExcluded", v.npcExcluded);
}
template <class A> void visit(A& a, generation::GenerationRequest& v) {
    a("seed", v.seed);
    a("depth", v.depth);
    a("generator", v.generator);
    a("templates", v.templates);
}
template <class A> void visit(A& a, generation::Room& v) {
    a("site", v.site);
    a("anchor", v.anchor);
    a("role", v.role);
    a("id", v.id);
    a("shape", v.shape);
    a("rotation", v.rotation);
    a("ports", v.ports);
    a("facing", v.facing);
}
template <class A> void visit(A& a, generation::Connection& v) {
    a("a", v.a);
    a("b", v.b);
}
template <class A> void visit(A& a, generation::PopulationPlacement& v) {
    a("cell", v.cell);
    a("subcell", v.subcell);
    a("facing", v.facing);
    a("recipe", v.recipe);
    a("id", v.id);
    a("guardian", v.guardian);
}
template <class A> void visit(A& a, generation::FloorLayout& v) {
    a("width", v.width);
    a("height", v.height);
    a("floor", v.floor);
    a("rooms", v.rooms);
    a("connections", v.connections);
    a("roomCount", v.roomCount);
    a("connectionCount", v.connectionCount);
    a("family", v.family);
    a("rotation", v.rotation);
    a("loopConnection", v.loopConnection);
    a("spawn", v.spawn);
    a("depth", v.depth);
    a("fixtures", v.fixtures);
    a("population", v.population);
    a("populationCount", v.populationCount);
    a("optionalSupply", v.optionalSupply);
    a("guardianReward", v.guardianReward);
    a("guardianApproach", v.guardianApproach);
    a("approachDoor", v.approachDoor);
    a("guardianGate", v.guardianGate);
}
template <class A> void visit(A& a, generation::Failure& v) {
    a("code", v.code);
    a("room", v.room);
    a("connection", v.connection);
}
template <class A> void visit(A& a, generation::GenerationDiagnostics& v) {
    a("attempts", v.attempts);
    a("rejected", v.rejected);
    a("fallback", v.fallback);
    a("fallbackId", v.fallbackId);
    a("failure", v.failure);
}
template <class A> void visit(A& a, Navigation::Request& v) {
    a("pending", v.pending);
    a("from", v.from);
    a("target", v.target);
    a("adjacent", v.adjacent);
    a("preferred", v.preferred);
}
template <class A> void visit(A& a, Navigation::Result& v) {
    a("ready", v.ready);
    a("found", v.found);
    a("from", v.from);
    a("target", v.target);
    a("step", v.step);
    a("adjacent", v.adjacent);
    a("preferred", v.preferred);
}
template <class A> void visit(A& a, Navigation::Saved& v) {
    a("queue", v.queue);
    a("seen", v.seen);
    a("first", v.first);
    a("requests", v.requests);
    a("results", v.results);
    a("active", v.active);
    a("geometryRevision", v.geometryRevision);
    a("owner", v.owner);
    a("cursor", v.cursor);
    a("head", v.head);
    a("tail", v.tail);
    a("cleanup", v.cleanup);
    a("work", v.work);
    a("completed", v.completed);
    a("cleaning", v.cleaning);
    a("canceled", v.canceled);
    a("preferAvailable", v.preferAvailable);
    a("centerAvailable", v.centerAvailable);
    a("fallbackStep", v.fallbackStep);
    a("frontStep", v.frontStep);
}
template <class A> void visit(A& a, CampaignLocationState& v) {
    a("generated", v.generated);
    a("visited", v.visited);
    a("generation", v.generation);
    a("layout", v.layout);
    a("diagnostics", v.diagnostics);
    a("fingerprint", v.fingerprint);
    a("world", v.world);
    a("worldFingerprint", v.worldFingerprint);
    a("rows", v.rows);
    a("types", v.types);
    a("placements", v.placements);
    a("recovery", v.recovery);
    a("fixtures", v.fixtures);
    a("objective", v.objective);
    a("state", v.state);
    a("entry", v.entry);
    a("exit", v.exit);
    a("seed", v.seed);
}
template <class A> void visit(A& a, CampaignCheckpoint& v) {
    a("party", v.party);
    a("locations", v.locations);
    a("navigation", v.navigation);
    a("pose", v.pose);
    a("animated", v.animated);
    a("activeLocation", v.activeLocation);
    a("gameplayMs", v.gameplayMs);
    a("revision", v.revision);
    a("runId", v.runId);
}
template <class A> void visit(A& a, EffectId& v) {
    a("serial", v.serial);
}
template <class A> void visit(A& a, AreaId& v) {
    a("serial", v.serial);
}
template <class A> void visit(A& a, rules::CharacterId& v) {
    a("index", v.index);
}

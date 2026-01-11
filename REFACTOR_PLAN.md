# Serial Killers Reforged - Refactoring Plan

## Overview

This document outlines the plan to refactor Serial Killers Reforged to:
1. **Integrate Reforger Lobby** for player slotting and spectator capability
2. **Replace CallQueue/CallLater** with proper GameSystem architecture for game logic

Reference implementation: **DefenseInDepth** (`C:\Users\nielu\Projects\armaforces\DefenseInDepth`)

---

## Current Architecture Analysis

### Existing Components

| File | Purpose | Uses CallLater? |
|------|---------|-----------------|
| `SK_SerialKillersGameMode.c` | Main game mode, scoring, kill handling | ✅ Heavy usage |
| `SK_CivilianManagerComponent.c` | Spawns civilians and vehicles | ✅ Yes |
| `SK_PrisonManagerComponent.c` | Prison system for captured killers | ❌ No |
| `SK_PrisonerComponent.c` | Player prisoner state | ❌ No |
| `SK_XPHandlerComponent.c` | XP rewards for Blufor | ❌ No |
| `SK_StorageCacheComponent.c` | Killer resupply caches | ✅ Yes |
| `SK_MapDescriptorComponent.c` | Map markers for locations | ✅ Yes |
| `SK_ScoreInfoDisplay.c` | HUD display | ✅ Yes (UI timing) |
| `SK_RespawnSystemComponent.c` | Empty override | ❌ No |
| `SK_SerialKillersConfigComponent.c` | Configuration storage | ❌ No |

### Current CallLater Usage

```
SK_SerialKillersGameMode.c:
├── StartGame (delayed game start)
├── TimeoutGameEnd (game timer)
├── StartPenaltyClock (killer inactivity penalty)
└── PenaltyClockCheck (recurring penalty checks)

SK_CivilianManagerComponent.c:
└── SpawnVehicles (delayed vehicle spawning)

SK_StorageCacheComponent.c:
└── InitializeCache (staggered cache initialization)

SK_MapDescriptorComponent.c:
└── CreateMapMarker (staggered marker creation)

SK_ScoreInfoDisplay.c:
└── HideHUD (UI auto-hide timer)
```

### Current Dependencies (addon.gproj)
- Multiple mod dependencies (10 total)
- Missing: `5EAF2B0473DB5A99` (Reforger Lobby - PS_)

---

## Phase 1: Reforger Lobby Integration

### 1.1 Add Reforger Lobby Dependency

**File:** `addon.gproj`

Add Reforger Lobby GUID `5EAF2B0473DB5A99` to dependencies:
```
Dependencies {
  "58D0FB3206B6F859" "5EAF2B0473DB5A99" "65AD7C249E4ECDFB" ...
}
```

### 1.2 Update Game Mode Base Class

**Current:** `SCR_BaseGameMode`
**Target:** `PS_GameModeCoop` (from Reforger Lobby)

**File:** `SK_SerialKillersGameMode.c`

```c
// BEFORE
class SK_SerialKillersGameModeClass: SCR_BaseGameModeClass {}
class SK_SerialKillersGameMode : SCR_BaseGameMode

// AFTER
class SK_SerialKillersGameModeClass: PS_GameModeCoopClass {}
class SK_SerialKillersGameMode : PS_GameModeCoop
```

### 1.3 Implement Player Slotting System

Create new component for managing Serial Killers-specific slotting:

**New File:** `Scripts/Game/SK/GameMode/SK_PlayableManagerComponent.c`

```c
class SK_PlayableManagerComponent : ScriptComponent
{
    // Integration with PS_PlayableManager for:
    // - Killer slot assignment (limited slots)
    // - Cop slot assignment (unlimited)
    // - Spectator capability when captured/dead
}
```

### 1.4 Implement Spectator Support

Use `PS_PlayableManager` for spectator transitions:

**Changes in** `SK_SerialKillersGameMode.c`:
- When killer is captured → switch to spectator
- When killer is freed → switch back to playable unit
- Leverage `PS_RespawnData` for respawn handling

**Reference from DiD:**
```c
protected void RespawnPlayer(int playerId, PS_PlayableComponent playableComponent, AFM_PlayerSpawnPointEntity sp)
{
    if (playableComponent)
    {
        ResourceName prefabToSpawn = playableComponent.GetNextRespawn(false);
        if (prefabToSpawn != "")
        {
            PS_RespawnData respawnData = new PS_RespawnData(playableComponent, prefabToSpawn);
            if (sp)
                respawnData.m_aSpawnTransform[3] = sp.GetOrigin();
            Respawn(playerId, respawnData);
            return;
        }
    }
    SwitchToInitialEntity(playerId);
}
```

### 1.5 Update Respawn System

**File:** `SK_RespawnSystemComponent.c`

Replace empty class with proper PS_ integration or remove entirely if base class handles it.

---

## Phase 2: GameSystem Architecture

### 2.1 Create SK_GameStateSystem

**New File:** `Scripts/Game/SK/GameMode/Systems/SK_GameStateSystem.c`

Manages all time-based game logic currently handled by CallLater:

```c
class SK_GameStateSystem : GameSystem
{
    protected float m_fUpdateInterval = 1.0;
    protected float m_fUpdateTimer = 0;
    
    // Timers (in seconds)
    protected float m_fGameStartTimer;
    protected float m_fVictoryTimer;
    protected float m_fPenaltyTimer;
    
    // States
    protected bool m_bGameStartPending = true;
    protected bool m_bPenaltyClockActive = false;
    
    //--------------------------------------------------------
    override static void InitInfo(WorldSystemInfo outInfo)
    {
        outInfo
            .SetAbstract(false)
            .SetUnique(true)
            .SetLocation(ESystemLocation.Server)
            .AddPoint(ESystemPoint.FixedFrame);
    }
    
    //--------------------------------------------------------
    static SK_GameStateSystem GetInstance()
    {
        World world = GetGame().GetWorld();
        if (!world)
            return null;
        return SK_GameStateSystem.Cast(world.FindSystem(SK_GameStateSystem));
    }
    
    //--------------------------------------------------------
    override event protected void OnUpdatePoint(WorldUpdatePointArgs args)
    {
        m_fUpdateTimer += args.GetTimeSliceSeconds();
        if (m_fUpdateTimer < m_fUpdateInterval)
            return;
        m_fUpdateTimer = 0;
        
        ProcessTimers();
    }
    
    //--------------------------------------------------------
    protected void ProcessTimers()
    {
        // Game start countdown
        if (m_bGameStartPending)
        {
            m_fGameStartTimer -= m_fUpdateInterval;
            if (m_fGameStartTimer <= 0)
                StartGame();
        }
        
        // Victory countdown
        m_fVictoryTimer -= m_fUpdateInterval;
        if (m_fVictoryTimer <= 0)
            TimeoutGameEnd();
        
        // Penalty clock
        if (m_bPenaltyClockActive)
        {
            m_fPenaltyTimer -= m_fUpdateInterval;
            if (m_fPenaltyTimer <= 0)
                ProcessPenalty();
        }
    }
}
```

### 2.2 Create SK_CivilianSpawnSystem

**New File:** `Scripts/Game/SK/GameMode/Systems/SK_CivilianSpawnSystem.c`

Handles civilian and vehicle spawning with proper frame-distributed spawning:

```c
class SK_CivilianSpawnSystem : GameSystem
{
    // Pending spawn queues
    protected ref array<ref SK_CivilianSpawnData> m_aPendingCivilians;
    protected ref array<ref SK_VehicleSpawnData> m_aPendingVehicles;
    
    // Spawn rate limiting
    protected int m_iSpawnsPerFrame = 2;
    
    override event protected void OnUpdatePoint(WorldUpdatePointArgs args)
    {
        ProcessPendingSpawns();
    }
}
```

### 2.3 Refactor SK_CivilianManagerComponent

**File:** `SK_CivilianManagerComponent.c`

Changes:
- Remove `GetGame().GetCallqueue().CallLater(SpawnVehicles, 1500);`
- Queue spawns to `SK_CivilianSpawnSystem` instead
- Keep configuration and spawn logic, delegate timing to system

### 2.4 Refactor SK_StorageCacheComponent

**File:** `SK_StorageCacheComponent.c`

```c
// BEFORE
GetGame().GetCallqueue().CallLater(InitializeCache, Math.RandomInt(1,15) * 250);

// AFTER - Use EOnInit with system registration
override void EOnInit(IEntity owner)
{
    super.EOnInit(owner);
    SK_CacheInitSystem system = SK_CacheInitSystem.GetInstance();
    if (system)
        system.RegisterCache(this);
}
```

### 2.5 Create SK_CacheInitSystem

**New File:** `Scripts/Game/SK/GameMode/Systems/SK_CacheInitSystem.c`

Handles staggered cache initialization:

```c
class SK_CacheInitSystem : GameSystem
{
    protected ref array<SK_StorageCacheComponent> m_aPendingCaches;
    protected int m_iInitPerFrame = 1;
    
    void RegisterCache(SK_StorageCacheComponent cache)
    {
        m_aPendingCaches.Insert(cache);
    }
    
    override event protected void OnUpdatePoint(WorldUpdatePointArgs args)
    {
        for (int i = 0; i < m_iInitPerFrame && m_aPendingCaches.Count() > 0; i++)
        {
            SK_StorageCacheComponent cache = m_aPendingCaches[0];
            m_aPendingCaches.Remove(0);
            cache.InitializeCache();
        }
        
        if (m_aPendingCaches.Count() == 0)
            Enable(false);
    }
}
```

---

## Phase 3: Game Mode Refactoring

### 3.1 Split SK_SerialKillersGameMode Responsibilities

The current game mode is too monolithic. Split into:

| Component | Responsibility |
|-----------|----------------|
| `SK_SerialKillersGameMode` | Core game mode, state machine, victory conditions |
| `SK_GameStateSystem` | Timer management, periodic checks |
| `SK_ScoreManagerComponent` | Score tracking, XP rewards |
| `SK_KillHandlerComponent` | Kill event processing, marker creation |

### 3.2 Updated SK_SerialKillersGameMode

**File:** `SK_SerialKillersGameMode.c`

Key changes:
1. Inherit from `PS_GameModeCoop`
2. Remove all `CallLater` calls
3. Subscribe to `SK_GameStateSystem` events
4. Delegate timer logic to system

```c
class SK_SerialKillersGameModeClass: PS_GameModeCoopClass {}

class SK_SerialKillersGameMode : PS_GameModeCoop
{
    protected SK_GameStateSystem m_GameStateSystem;
    
    override void EOnInit(IEntity owner)
    {
        super.EOnInit(owner);
        
        m_GameStateSystem = SK_GameStateSystem.GetInstance();
        if (m_GameStateSystem)
        {
            m_GameStateSystem.GetOnGameStart().Insert(OnGameStarted);
            m_GameStateSystem.GetOnVictoryTimeout().Insert(TimeoutGameEnd);
            m_GameStateSystem.GetOnPenalty().Insert(ProcessPenalty);
        }
    }
    
    override void OnGameStateChanged()
    {
        super.OnGameStateChanged();
        
        if (GetState() == SCR_EGameModeState.GAME)
        {
            // Activate systems
            if (m_GameStateSystem)
                m_GameStateSystem.StartGameCountdown(m_iGameStartDelaySeconds);
        }
    }
}
```

### 3.3 Create ScriptInvoker Events

For communication between systems and game mode:

```c
// In SK_GameStateSystem
protected ref ScriptInvoker m_OnGameStart;
protected ref ScriptInvoker m_OnVictoryTimeout;
protected ref ScriptInvoker m_OnPenalty;

ScriptInvoker GetOnGameStart()
{
    if (!m_OnGameStart)
        m_OnGameStart = new ScriptInvoker();
    return m_OnGameStart;
}
```

---

## Phase 4: UI Updates

### 4.1 SK_ScoreInfoDisplay CallLater Handling

**File:** `SK_ScoreInfoDisplay.c`

The HUD hide timer is acceptable to keep as CallLater since it's:
- Client-side only
- Non-critical timing
- Standard UI pattern

However, consider using `DisplayUpdate` for periodic refresh:

```c
protected float m_fHideTimer = 0;
protected bool m_bHideScheduled = false;

override void DisplayUpdate(IEntity owner, float timeSlice)
{
    if (m_bHideScheduled)
    {
        m_fHideTimer -= timeSlice * 1000; // Convert to ms
        if (m_fHideTimer <= 0)
        {
            HideHUD();
            m_bHideScheduled = false;
        }
    }
    
    if (m_bPeriodicRefresh)
        UpdateHUD();
}
```

---

## Phase 5: Migration Steps

### Step-by-Step Implementation Order

1. **Preparation**
   - [ ] Create backup branch
   - [ ] Add Reforger Lobby dependency to `addon.gproj`
   - [ ] Verify dependency loads correctly

2. **Create New Systems (no breaking changes)**
   - [ ] Create `SK_GameStateSystem.c`
   - [ ] Create `SK_CivilianSpawnSystem.c`
   - [ ] Create `SK_CacheInitSystem.c`
   - [ ] Test systems in isolation

3. **Update Game Mode**
   - [ ] Change base class to `PS_GameModeCoop`
   - [ ] Add system references and event handlers
   - [ ] Migrate timer logic to `SK_GameStateSystem`
   - [ ] Test game start/end flows

4. **Migrate Components**
   - [ ] Update `SK_CivilianManagerComponent` to use spawn system
   - [ ] Update `SK_StorageCacheComponent` to use cache system
   - [ ] Update `SK_MapDescriptorComponent` marker creation
   - [ ] Test each component individually

5. **Implement Spectator/Slotting**
   - [ ] Create `SK_PlayableManagerComponent`
   - [ ] Implement killer capture → spectator transition
   - [ ] Implement prison release → respawn flow
   - [ ] Test multiplayer scenarios

6. **UI Updates**
   - [ ] Update `SK_ScoreInfoDisplay` (optional)
   - [ ] Test HUD with new game mode

7. **Cleanup**
   - [ ] Remove deprecated code
   - [ ] Update prefabs to include new components
   - [ ] Update mission files if needed
   - [ ] Final integration testing

---

## New File Structure

```
Scripts/Game/SK/
├── Components/
│   ├── SK_PrisonerComponent.c
│   ├── SK_PrisonManagerComponent.c
│   ├── SK_StorageCacheComponent.c
│   └── SK_XPHandlerComponent.c
├── Configs/
│   └── SK_SerialKillersConfigComponent.c
├── GameMode/
│   ├── SK_SerialKillersGameMode.c (MODIFIED)
│   ├── SK_RespawnSystemComponent.c (POSSIBLY REMOVED)
│   ├── SK_PlayableManagerComponent.c (NEW)
│   ├── Managers/
│   │   ├── SK_CivilianManagerComponent.c (MODIFIED)
│   │   └── SK_ScoreManagerComponent.c (NEW - optional)
│   └── Systems/                          (NEW FOLDER)
│       ├── SK_GameStateSystem.c (NEW)
│       ├── SK_CivilianSpawnSystem.c (NEW)
│       └── SK_CacheInitSystem.c (NEW)
├── Global/
│   └── SK_Global.c
├── Map/
│   └── Components/
│       └── SK_MapDescriptorComponent.c (MODIFIED)
└── UI/
    └── HUD/
        └── SK_ScoreInfoDisplay.c (MODIFIED - optional)
```

---

## Key API References

### GameSystem Pattern (from DefenseInDepth)

```c
class MySystem : GameSystem
{
    // Required: Define system properties
    override static void InitInfo(WorldSystemInfo outInfo)
    {
        outInfo
            .SetAbstract(false)
            .SetUnique(true)
            .SetLocation(ESystemLocation.Server) // or Client, Both
            .AddPoint(ESystemPoint.FixedFrame);  // or SimulateFrame
    }
    
    // Get singleton instance
    static MySystem GetInstance()
    {
        World world = GetGame().GetWorld();
        if (!world) return null;
        return MySystem.Cast(world.FindSystem(MySystem));
    }
    
    // Called every frame (based on AddPoint)
    override event protected void OnUpdatePoint(WorldUpdatePointArgs args)
    {
        float dt = args.GetTimeSliceSeconds();
        // Process logic
    }
    
    // Control system execution
    override event bool ShouldBePaused()
    {
        return true; // Pauses when game is paused
    }
}
```

### PS_GameModeCoop Integration

```c
// Respawn through Reforger Lobby
PS_PlayableManager playableManager = PS_PlayableManager.GetInstance();
PS_RespawnData respawnData = new PS_RespawnData(playableComponent, prefabToSpawn);
Respawn(playerId, respawnData);

// Switch to spectator
SwitchToInitialEntity(playerId);
```

---

## Testing Checklist

- [ ] Game starts correctly with countdown
- [ ] Killers can be captured and sent to prison
- [ ] Killers can be freed and respawn correctly
- [ ] Civilian spawning works across the map
- [ ] Vehicle spawning near buildings works
- [ ] Score tracking and XP rewards work
- [ ] Victory conditions trigger correctly (score/time/capture)
- [ ] Penalty system activates after inactivity
- [ ] HUD displays correctly
- [ ] Multiplayer synchronization works
- [ ] Spectator mode works for dead/captured players

---

## Notes

1. **CallLater for UI** - It's acceptable to keep CallLater for client-side UI timing as it doesn't affect game state synchronization.

2. **System Location** - Use `ESystemLocation.Server` for game logic, `ESystemLocation.Both` for synchronized state.

3. **Performance** - GameSystems are more efficient than CallLater for recurring checks as they don't create callback overhead.

4. **Backwards Compatibility** - The spawn systems can be introduced gradually without breaking existing functionality.

---

*Last Updated: January 11, 2026*
*Reference: DefenseInDepth mod implementation*

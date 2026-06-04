set(SRC_FILES
    # ---- Core ----
    Core/App.cpp
    Core/GameSession.cpp
    Core/GameWorldContext.cpp
    Core/DefenderWeaponSystem.cpp
    # ---- Audio ----
    Audio/MusicPlayer.cpp
    # ---- States ----
    States/MenuCommon.cpp
    States/TitleScreenState.cpp
    States/MainMenuState.cpp
    States/SettingsState.cpp
    States/BattleSetupState.cpp
    States/RulesState.cpp
    States/TeamSelectState.cpp
    States/WeaponSelectState.cpp
    States/LevelSelectState.cpp
    States/GameplayState.cpp
    States/GamePausedState.cpp
    States/ResultsState.cpp
    # ---- Config ----
    Config/SaveData.cpp
    Config/KeyBindings.cpp
    # ---- Entities ----
    Entities/Player.cpp
    Entities/PlayerBounce.cpp
    Entities/Bomb.cpp
    Entities/Explosion.cpp
    Entities/Spirit.cpp
    Entities/Interactable.cpp
    # ---- Managers ----
    Managers/AIManager.cpp
    Managers/BombManager.cpp
    Managers/InteractableManager.cpp
    Managers/LevelManager.cpp
    Managers/CheatManager.cpp
    # ---- Map ----
    Map/MapTiles.cpp
    # ---- Bot ----
    Bot/BotNavigator.cpp
    Bot/DangerMap.cpp
    Bot/Pathfinder.cpp
    # ---- UI ----
    UI/SelectableList.cpp
    UI/ButtonRow.cpp
    UI/UIGroup.cpp
    UI/UIManager.cpp
    UI/UIImage.cpp
    UI/UIText.cpp
    UI/UIButtonList.cpp
    UI/DebugConsole.cpp
    UI/PauseMenu.cpp
    UI/Slider.cpp
    UI/DebugOverlay.cpp
    UI/ConfirmDialog.cpp
    # ---- Turret ----
    Turret/Projectile.cpp
    Turret/TurretManager.cpp
    Turret/Turret.cpp
    # ---- Weapons (防守方武器) ----
    Weapons/SwordWeapon.cpp
    Weapons/LaserWeapon.cpp
    Weapons/BarrierWeapon.cpp
)

set(INCLUDE_FILES
    # ---- Core ----
    Core/App.hpp
    Core/GameSession.hpp
    Core/GameWorldContext.hpp
    Core/WorldContext.hpp
    Core/DefenderWeaponSystem.hpp
    # ---- States ----
    States/IGameState.hpp
    States/MenuCommon.hpp
    States/TitleScreenState.hpp
    States/MainMenuState.hpp
    States/SettingsState.hpp
    States/BattleSetupState.hpp
    States/RulesState.hpp
    States/TeamSelectState.hpp
    States/WeaponSelectState.hpp
    States/LevelSelectState.hpp
    States/GameplayState.hpp
    States/GamePausedState.hpp
    States/ResultsState.hpp
    # ---- Config ----
    Config/GameConstants.hpp
    Config/GameTypes.hpp
    Config/GridCoord.hpp
    Config/MatchConfig.hpp
    Config/SaveData.hpp
    Config/KeyBindings.hpp
    # ---- Entities ----
    Entities/Player.hpp
    Entities/PlayerBounce.hpp
    Entities/Bomb.hpp
    Entities/Explosion.hpp
    Entities/Spirit.hpp
    Entities/Interactable.hpp
    # ---- Managers ----
    Managers/AIManager.hpp
    Managers/BombManager.hpp
    Managers/InteractableManager.hpp
    Managers/LevelManager.hpp
    Managers/CheatManager.hpp
    # ---- Map ----
    Map/MapTiles.hpp
    Map/TileSet.hpp
    # ---- Audio ----
    Audio/MusicPlayer.hpp
    # ---- Bot ----
    Bot/BotNavigator.hpp
    Bot/BotProfile.hpp
    Bot/DangerMap.hpp
    Bot/Pathfinder.hpp
    # ---- Controller ----
    Controller/InputController.hpp
    Controller/HumanController.hpp
    Controller/BotController.hpp
    Controller/IProgrammableController.hpp
    Controller/CooldownResetGuard.hpp
    # ---- Effects ----
    Effects/IPlayerEffect.hpp
    Effects/PlayerEffects.hpp
    # ---- UI ----
    UI/SelectableList.hpp
    UI/ButtonRow.hpp
    UI/UIGroup.hpp
    UI/UIManager.hpp
    UI/UIImage.hpp
    UI/UIText.hpp
    UI/UIButtonList.hpp
    UI/DebugConsole.hpp
    UI/PauseMenu.hpp
    UI/Slider.hpp
    UI/DebugOverlay.hpp
    UI/ConfirmDialog.hpp
    # ---- Turret ----
    Turret/Projectile.hpp
    Turret/TurretManager.hpp
    Turret/Turret.hpp
    # ---- Weapons (防守方武器) ----
    Weapons/IWeaponEffects.hpp
    Weapons/IDefenderWeapon.hpp
    Weapons/SwordWeapon.hpp
    Weapons/LaserWeapon.hpp
    Weapons/BarrierWeapon.hpp
    Weapons/WeaponFactory.hpp
)

set(TEST_FILES
)

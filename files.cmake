set(SRC_FILES
    # ---- Core ----
    Core/App.cpp
    Core/GameSession.cpp
    Core/GameWorldContext.cpp
    # ---- States ----
    States/MenuCommon.cpp
    States/TitleScreenState.cpp
    States/MainMenuState.cpp
    States/SettingsState.cpp
    States/BattleSetupState.cpp
    States/RulesState.cpp
    States/TeamSelectState.cpp
    States/LevelSelectState.cpp
    States/GameplayState.cpp
    States/GamePausedState.cpp
    States/ResultsState.cpp
    # ---- Config ----
    Config/SaveData.cpp
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
    # ---- UI ----
    UI/UIManager.cpp
    UI/UIImage.cpp
    UI/UIText.cpp
    UI/UIButtonList.cpp
    UI/Menu.cpp
    UI/PauseMenu.cpp
    UI/DebugOverlay.cpp
    # ---- Turret ----
    Turret/Projectile.cpp
    Turret/TurretManager.cpp
    Turret/Turret.cpp
)

set(INCLUDE_FILES
    # ---- Core ----
    Core/App.hpp
    Core/GameSession.hpp
    Core/GameWorldContext.hpp
    Core/WorldContext.hpp
    # ---- States ----
    States/IGameState.hpp
    States/MenuCommon.hpp
    States/TitleScreenState.hpp
    States/MainMenuState.hpp
    States/SettingsState.hpp
    States/BattleSetupState.hpp
    States/RulesState.hpp
    States/TeamSelectState.hpp
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
    # ---- Bot ----
    Bot/BotNavigator.hpp
    Bot/DangerMap.hpp
    # ---- Controller ----
    Controller/InputController.hpp
    Controller/HumanController.hpp
    Controller/BotController.hpp
    Controller/IProgrammableController.hpp
    # ---- Effects ----
    Effects/IPlayerEffect.hpp
    Effects/PlayerEffects.hpp
    # ---- UI ----
    UI/UIManager.hpp
    UI/UIImage.hpp
    UI/UIText.hpp
    UI/UIButtonList.hpp
    UI/Menu.hpp
    UI/PauseMenu.hpp
    UI/DebugOverlay.hpp
    # ---- Turret ----
    Turret/Projectile.hpp
    Turret/TurretManager.hpp
    Turret/Turret.hpp
)

set(TEST_FILES
)

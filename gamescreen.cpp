#include "gamescreen.h"


#include <prism/soundeffect.h>
#include <cmath>

#include "storyscreen.h"
#include "gameoverscreen.h"

static void gotoWinStory(void* tCaller) {
    (void)tCaller;
    setCurrentStoryDefinitionFile("game/OUTRO.def", 1);
    setNewScreen(getStoryScreen());
}

static void gotoGameOver(void* tCaller) {
    (void)tCaller;
    setNewScreen(getGameOverScreen());
}

static struct 
{
    int mLevel = 0;
    CollisionListData* mPlayerCollisionList;
    CollisionListData* mPlayerShotCollisionList;
    CollisionListData* mGoliathCollisionList;
    CollisionListData* mGoliathShotCollisionList;

    int mHasShownTutorial1 = 0;
    int mHasShownTutorial2 = 0;
} gGameScreenData;

static void playerHitCB(void* tCaller, void* tCollisionData);
static void playerShotHitCB(void* tCaller, void* tCollisionData);
static void goliathHitCB(void* tCaller, void* tCollisionData);
static void goliathShotHitCB(void* tCaller, void* tCollisionData);

class GameScreen
{
    public:
    GameScreen() {
        load();
        //setVolume(0);
        //setSoundEffectVolume(0);
        
        //activateCollisionHandlerDebugMode();
    }

    MugenSpriteFile mSprites;
    MugenAnimations mAnimations;
    MugenSounds mSounds;

    void loadFiles() {
        mSprites = loadMugenSpriteFileWithoutPalette("game/GAME.sff");
        mAnimations = loadMugenAnimationFile("game/GAME.air");
        if(isOnDreamcast())
        {
            mSounds = loadMugenSoundFile("game/dreamcast/GAME.snd");
        }
        else
        {
            mSounds = loadMugenSoundFile("game/GAME.snd");
        }
    }

    void load() {
        loadFiles();
        loadCollisions();
        loadBG();
        loadPlayer();
        loadPlayerShots();
        loadGoliath();
        loadGoliathShots();
        loadUI();
        loadGoliathFadeAndUI();
        loadTutorials();
        loadPause();
        if (isOnDreamcast())
        {
            setSoundEffectVolume(1.0);
        }
        else
        {
            setVolume(0.5);
            setSoundEffectVolume(0.5);
        }
        if(isOnDreamcast())
        {
            streamMusicFile("game/dreamcast/GAME.ogg");
        }
        else
        {
            streamMusicFile("game/GAME.ogg");
        }
    }

    void loadCollisions() {
        gGameScreenData.mPlayerCollisionList = addCollisionListToHandler();
        gGameScreenData.mPlayerShotCollisionList = addCollisionListToHandler();
        gGameScreenData.mGoliathCollisionList = addCollisionListToHandler();
        gGameScreenData.mGoliathShotCollisionList = addCollisionListToHandler();

        addCollisionHandlerCheck(gGameScreenData.mGoliathCollisionList, gGameScreenData.mPlayerShotCollisionList);
        addCollisionHandlerCheck(gGameScreenData.mPlayerCollisionList, gGameScreenData.mGoliathShotCollisionList);
    }

    int screenshakeTicks = 0;
    void setScreenShake(int tDuration) {
        screenshakeTicks = tDuration;
    }
    void updateScreenshake() {
        Vector2D cameraOffset = Vector2D(0, 0);
        if(screenshakeTicks) {
            screenshakeTicks--;
            cameraOffset = Vector2D(randfrom(-5, 5), randfrom(-5, 5));
        } else {
            cameraOffset = Vector2D(0, 0);
        }
        setBlitzCameraHandlerPosition(cameraOffset.xyz(0));
    }

    int gameTicks = 0;
    void update() {
        if(!isPaused)
        {
            if(!hitPauseTicks)
            {
                updatePlayer();
                updatePlayerShots();
                updateGoliath();
                updateGoliathShots();
                updateUI();
                updateFloatingNumbers();
                updateGoliathFadeAndUI();
                updateScreenshake();
                gameTicks++;
            }
            updateTutorials();
            updateHitPause();
            showTutorial1();
        }
        updatePause();
    }

    int isPaused = 0;
    MugenAnimationHandlerElement* pauseAnim;
    void loadPause() {
        pauseAnim = addMugenAnimation(getMugenAnimation(&mAnimations, 3020), &mSprites, Vector3D(0, 0, 75));
        setMugenAnimationVisibility(pauseAnim, 0);
    }
    void updatePause()
    {
        if(hasPressedAbortFlank())
        {
            isPaused = !isPaused;
            setMugenAnimationVisibility(pauseAnim, isPaused);
            if(isPaused)
            {
                setVolume(0);
                setSoundEffectVolume(0);
                pauseBlitzMugenAnimation(playerEntity);
                pauseBlitzMugenAnimation(rectangleEntity);
                pauseBlitzMugenAnimation(gunEntity);
                pauseBlitzMugenAnimation(goliathEntity);
                pauseBlitzMugenAnimation(goliathShadowEntity);
                for (auto& shot : playerShots)
                {
                    pauseBlitzMugenAnimation(shot.second.entity);
                }
                for (auto& shot : goliathShots)
                {
                    pauseBlitzMugenAnimation(shot.second.entity);
                }
            }
            else
            {
                if (isOnDreamcast())
                {
                    setSoundEffectVolume(1.0);
                }
                else
                {
                    setVolume(0.5);
                    setSoundEffectVolume(0.5);
                }
                unpauseBlitzMugenAnimation(playerEntity);
                unpauseBlitzMugenAnimation(rectangleEntity);
                unpauseBlitzMugenAnimation(gunEntity);
                unpauseBlitzMugenAnimation(goliathEntity);
                unpauseBlitzMugenAnimation(goliathShadowEntity);
                for (auto& shot : playerShots)
                {
                    unpauseBlitzMugenAnimation(shot.second.entity);
                }
                for (auto& shot : goliathShots)
                {
                    unpauseBlitzMugenAnimation(shot.second.entity);
                }
            }
        }
    }

    // Tutorials
    MugenAnimationHandlerElement* tutorial1;
    MugenAnimationHandlerElement* tutorial2;
    void loadTutorials()
    {
        tutorial1 = addMugenAnimation(getMugenAnimation(&mAnimations, isOnDreamcast() ? 3012 : 3011), &mSprites, Vector3D(0, 0, 70));
        setMugenAnimationVisibility(tutorial1, 0);
        tutorial2 = addMugenAnimation(getMugenAnimation(&mAnimations, 3010), &mSprites, Vector3D(0, 0, 70));
        setMugenAnimationVisibility(tutorial2, 0);
    }

    void updateTutorials()
    {
        if(getMugenAnimationVisibility(tutorial1) || getMugenAnimationVisibility(tutorial2))
        {
            if(hasPressedAttackFlank() || hasPressedStartFlank())
            {
                hitPauseTicks = 1;
            }
        }
        if(hitPauseTicks == 1)
        {
            setMugenAnimationVisibility(tutorial1, 0);
            setMugenAnimationVisibility(tutorial2, 0);
        }
    }

    void showTutorial1()
    {
        if(gGameScreenData.mHasShownTutorial1) return;
        setMugenAnimationVisibility(tutorial1, 1);
        gGameScreenData.mHasShownTutorial1 = 1;
        setHitPause(200);
    }

    void showTutorial2()
    {
        if(gGameScreenData.mHasShownTutorial2) return;
        setMugenAnimationVisibility(tutorial2, 1);
        gGameScreenData.mHasShownTutorial2 = 1;
        setHitPause(200);
    }

    //BG
    // BG: 1
    int bgEntity;
    void loadBG() {
        bgEntity = addBlitzEntity(Vector3D(0, 0, 1));
        addBlitzMugenAnimationComponent(bgEntity, &mSprites, &mAnimations, 1);
    }

    // Player
    // Idle: 100
    // Walk: 120
    // Melee: 130
    // Dodge: 140
    // Gun: 150
    // Rectangle: 160
    int playerEntity;
    int rectangleEntity;
    int gunEntity;
    int playerShadowEntity;
    int playerLife = 100;
    void loadPlayer() {
        playerEntity = addBlitzEntity(Vector3D(80, 120, 20));
        addBlitzMugenAnimationComponent(playerEntity, &mSprites, &mAnimations, 100);
        addBlitzCollisionComponent(playerEntity);
        int collisionId = addBlitzCollisionRect(playerEntity, gGameScreenData.mPlayerCollisionList, CollisionRect(-1, -10, 3, 3));
        addBlitzCollisionCB(playerEntity, collisionId, playerHitCB, NULL);
        rectangleEntity = addBlitzEntity(Vector3D(100, 120, 36));
        addBlitzMugenAnimationComponent(rectangleEntity, &mSprites, &mAnimations, 160);
        gunEntity = addBlitzEntity(Vector3D(80, 120, 35));
        addBlitzMugenAnimationComponent(gunEntity, &mSprites, &mAnimations, 150);
        playerShadowEntity = addBlitzEntity(Vector3D(80, 120, 10));
        addBlitzMugenAnimationComponentStatic(playerShadowEntity, &mSprites, 170, 0);
        setBlitzMugenAnimationTransparency(playerShadowEntity, 0.5);
    }
    void updatePlayer() {
        if(goliathState == GoliathState::DEAD) return;
        updatePlayerMovement();
        updatePlayerRectangle();
        updatePlayerGun();
        updatePlayerShooting();
        updatePlayerMeleeAttack();
        //updatePlayerDodge();
        updatePlayerShadow();
        updatePlayerInvincibility();
        updatePlayerDying();
        updatePlayerZ();
    }

    void updatePlayerZ() {
        auto* pos = getBlitzEntityPositionReference(playerShadowEntity);
        auto y = pos->y;
        auto z = 20 + (y / 240) * 10;
        setBlitzEntityPositionZ(playerEntity, z);   
    }

    int playerInvincibilityTicks = 0;
    void updatePlayerInvincibility() {
        if(playerInvincibilityTicks)
        {
            playerInvincibilityTicks--;
            if(!playerInvincibilityTicks)
            {
                setBlitzMugenAnimationTransparency(playerEntity, 1);
            }
        }
    }

    void setPlayerInvincibility() {
        playerInvincibilityTicks = 120;
        setBlitzMugenAnimationTransparency(playerEntity, 0.5);
    }

    int isPlayerDying = 0;
    int playerDyingTicks = 0;
    void updatePlayerDying() {

        if(!isPlayerDying && playerLife <= 0)
        {
            isPlayerDying = 1;
            changeBlitzMugenAnimation(playerEntity, 180);
            setBlitzMugenAnimationVisibility(rectangleEntity, 0);
            setBlitzMugenAnimationVisibility(gunEntity, 0);
            playerDyingTicks = 120;
        }

        if(playerDyingTicks)
        {
            playerDyingTicks--;
            if(!playerDyingTicks)
            {
                addFadeOut(30, gotoGameOver, NULL);
            }
        }
    }

    void updatePlayerShadow() {
        auto* pos = getBlitzEntityPositionReference(playerShadowEntity);
        *pos = getBlitzEntityPosition(playerEntity) + Vector3D(-1, -1, -1);
        pos->z = 10;
    }

    void updatePlayerGun() {
        if(isPlayerDying) return;
        auto direction = getDirectionFromPlayerToMouse();
        auto* pos = getBlitzEntityPositionReference(gunEntity);
        *pos = getBlitzEntityPosition(playerEntity) + Vector2D(0, -10) + Vector3D(direction.x * 5, direction.y * 5, 0);
        setBlitzMugenAnimationFaceDirection(gunEntity, direction.x > 0);
    }

    int dodgeTicks = 0;
    void updatePlayerDodge() {
        if(isPlayerDying) return;
        if(dodgeTicks)
        {
            dodgeTicks--;
            if(!dodgeTicks)
            {
                changeBlitzMugenAnimation(playerEntity, 100);
            }
            return;
        }

        if (hasPressedB())
        {
            auto* pos = getBlitzEntityPositionReference(playerEntity);
            *pos += vecNormalize(Vector2D(1, 1)) * 4.f;
            changeBlitzMugenAnimation(playerEntity, 140);
            dodgeTicks = 60;
        }
    }

    int hasPressedAttackFlank() {
        if(isOnDreamcast())
        {
            return hasPressedRFlank();
        }
        else
        {
            return hasPressedMouseLeftFlank();
        }
    }

    int hasPressedMelee() {
        if(isOnDreamcast())
        {
            return hasPressedLFlank();
        }
        else
        {
            return hasPressedMouseRight();
        }
    }

    int hasPressedAttack() {
        if(isOnDreamcast())
        {
            return hasPressedR();
        }
        else
        {
            return hasPressedMouseLeft();
        }
    }

    int hasPressedMoveLeft() {
        if(isOnDreamcast())
        {
            return hasPressedX();
        }
        else
        {
            return hasPressedRawKeyboardKey(KEYBOARD_A_PRISM);
        }
    }

    int hasPressedMoveRight() {
        if(isOnDreamcast())
        {
            return hasPressedB();
        }
        else
        {
            return hasPressedRawKeyboardKey(KEYBOARD_D_PRISM);
        }
    }

    int hasPressedMoveUp() {
        if(isOnDreamcast())
        {
            return hasPressedY();
        }
        else
        {
            return hasPressedRawKeyboardKey(KEYBOARD_W_PRISM);
        }
    }

    int hasPressedMoveDown() {
        if(isOnDreamcast())
        {
            return hasPressedA();
        }
        else
        {
            return hasPressedRawKeyboardKey(KEYBOARD_S_PRISM);
        }
    }

    int shootCooldown = 0;
    void updatePlayerShooting() {
        if(isPlayerDying) return;
        if(shootCooldown)
        {
            shootCooldown--;
            return;
        }
        if (hasPressedAttack())
        {
            shootCooldown = 15;
            auto direction = getDirectionFromPlayerToMouse();
            tryPlayMugenSound(&mSounds, 1, 5);
            changeBlitzMugenAnimation(gunEntity, 150);
            addPlayerShot(direction);
        }
        else if(hasPressedMelee())
        {
            shootCooldown = 30;
            tryPlayMugenSound(&mSounds, 1, 6);
            performPlayerMeleeAttack();
        }
    }

    void performPlayerMeleeAttack() {
        auto playerPos = getBlitzEntityPosition(playerEntity);
        auto mousePos = getRectanglePointerPosition();
        auto direction = vecNormalize(Vector2D(mousePos.x - playerPos.x, mousePos.y - playerPos.y));
        changeBlitzMugenAnimation(playerEntity, 130);
        changeBlitzMugenAnimation(gunEntity, 135);
    }

    int getPlayerDamageMultiplier() {
        return goliathState == GoliathState::DOWN ? 4 : 1;
    }

    void updatePlayerMeleeAttack()
    {
        if(getBlitzMugenAnimationAnimationNumber(playerEntity) == 130)
        {
            auto playerPos = getBlitzEntityPosition(playerEntity);
            auto mousePos = getRectanglePointerPosition();
            auto direction = vecNormalize(Vector2D(mousePos.x - playerPos.x, mousePos.y - playerPos.y));
            auto startPos = (getBlitzEntityPosition(playerEntity) + Vector2D(0, -10) + Vector3D(direction.x * 10, direction.y * 10, 0)).xy();
            auto goliathCenter = (getBlitzEntityPosition(goliathEntity) - Vector2D(0, 45)).xy();
            if(shootCooldown == 20 && vecLength(goliathCenter - startPos) < 45)
            {
                auto damage = min(2 * getPlayerDamageMultiplier(), goliathLife);
                goliathLife = max(0, goliathLife - damage);
                if(damage > 0)
                {
                    tryPlayMugenSound(&mSounds, 1, 0);
                    addFloatingNumber(getBlitzEntityPosition(goliathEntity).xy() + Vector2DI(0, -70), damage);
                    setHitPause(damage / 2); 
                    setGoliathFlashing();      
                }   
                if(goliathState == GoliathState::EXPOSED)
                {
                    goliathState = GoliathState::DOWN;
                    changeBlitzMugenAnimation(goliathEntity, 1050);
                }
            }
            if(shootCooldown == 10) {
                changeBlitzMugenAnimation(playerEntity, 100);
                changeBlitzMugenAnimation(gunEntity, 136);
            }
        }
    }

    Vector2D getDirectionFromPlayerToMouse() {
        auto playerPos = getBlitzEntityPosition(playerEntity);
        auto mousePos = getRectanglePointerPosition();
        return vecNormalize(Vector2D(mousePos.x - playerPos.x, mousePos.y - playerPos.y));
    }

    Vector2D getRectanglePointerPosition() {
        auto mousePos = getBlitzEntityPosition(rectangleEntity);
        return Vector2D(mousePos.x, mousePos.y);
    }

    void updatePlayerRectangle() {
        if(isPlayerDying) return;
        if(isOnDreamcast())
        {
            auto x = getLeftStickNormalizedX();
            auto y = getLeftStickNormalizedY();
            
            Vector2D movement = Vector2D(x, y);
            if(vecLength(movement) < 0.2) return;
            movement = vecNormalize(movement);

            auto* rectanglePosition = getBlitzEntityPositionReference(rectangleEntity);
            *rectanglePosition += movement * 4.f;
            *rectanglePosition = clampPositionToGeoRectangle(*rectanglePosition, GeoRectangle2D(0, 0, 320, 240));
        }
        else
        {
            auto mousePos = getMousePointerPosition();
            auto* rectanglePosition = getBlitzEntityPositionReference(rectangleEntity);
            *rectanglePosition = Vector3D(mousePos.x, mousePos.y, rectanglePosition->z);
        }
    }

    void updatePlayerMovement() {
        if(isPlayerDying) return;
        Vector2DI movement = Vector2DI(0, 0);
        if(hasPressedMoveLeft())
        {
            movement.x = -1;
        }
        else if(hasPressedMoveRight())
        {
            movement.x = 1;
        }
        if(hasPressedMoveUp())
        {
            movement.y = -1;
        }
        else if(hasPressedMoveDown())
        {
            movement.y = 1;
        }

        if(getBlitzMugenAnimationAnimationNumber(playerEntity) == 100 && vecLength(movement) > 0)
        {
            changeBlitzMugenAnimation(playerEntity, 120);
        }
        else if(getBlitzMugenAnimationAnimationNumber(playerEntity) == 120 && vecLength(movement) == 0)
        {
            changeBlitzMugenAnimation(playerEntity, 100);
        }

        auto* pos = getBlitzEntityPositionReference(playerEntity);
        *pos += vecNormalize(movement) * 4.f;
        // constraint to screen
        *pos = clampPositionToGeoRectangle(*pos, GeoRectangle2D(0, 25, 320, 240 - 25));
    }

    // Player shots
    // Shot: 200
    // Shot Hit: 210
    struct PlayerShot
    {
        int entity;
        Vector2D direction;
        int toBeDeleted = 0;
    };
    std::map<int, PlayerShot> playerShots;
    void loadPlayerShots() {
        playerShots.clear();
    }

    void addPlayerShot(Vector2D tDirection)
    {
        auto startPos = (getBlitzEntityPosition(playerEntity) + Vector2D(0, -10) + Vector3D(tDirection.x * 10, tDirection.y * 10, 0)).xy();
        auto entity = addBlitzEntity(startPos.xyz(40));
        addBlitzMugenAnimationComponent(entity, &mSprites, &mAnimations, 200);
        addBlitzCollisionComponent(entity);
        setBlitzMugenAnimationAngle(entity, getAngleFromDirection(tDirection) + M_PI);
        int collisionId = addBlitzCollisionCirc(entity, gGameScreenData.mPlayerShotCollisionList, CollisionCirc{Vector2D(0, 0), 5});
        playerShots[entity] = { entity, tDirection, 0 };
        addBlitzCollisionCB(entity, collisionId, playerShotHitCB, &playerShots[entity]);
    }

    void updateSinglePlayerShot(PlayerShot& tShot)
    {
        auto* pos = getBlitzEntityPositionReference(tShot.entity);
        *pos += tShot.direction * 4.f;

        if (pos->x < -10 || pos->x > 330 || pos->y < -10 || pos->y > 250)
        {
            tShot.toBeDeleted = 1;
        }
    }

    void updatePlayerShots()
    {
        updatePlayerShotDeletion();
        for (auto& shot : playerShots)
        {
            updateSinglePlayerShot(shot.second);
        }        
        updatePlayerShotDeletion();
    }

    void updatePlayerShotDeletion()
    {
        for(auto iter = playerShots.begin(); iter !=  playerShots.end(); ) {
            if (iter->second.toBeDeleted) {
                if(iter->second.toBeDeleted == 2)
                {
                    auto anim = addMugenAnimation(getMugenAnimation(&mAnimations, 210), &mSprites, getBlitzEntityPosition(iter->second.entity));
                    setMugenAnimationDrawAngle(anim, getBlitzMugenAnimationDrawAngle(iter->second.entity));
                    setMugenAnimationNoLoop(anim);
                    auto damage = min(1 * getPlayerDamageMultiplier(), goliathLife);
                    goliathLife = max(0, goliathLife - damage);
                    if(damage > 0)
                    {
                        tryPlayMugenSound(&mSounds, 1, 0);
                        addFloatingNumber(getBlitzEntityPosition(goliathEntity).xy() + Vector2DI(0, -70), damage); 
                        setHitPause(damage / 2);
                        setGoliathFlashing();    
                    }            
                }
                removeBlitzEntity(iter->second.entity);
                iter = playerShots.erase(iter);
            } else {
                ++iter;
            }
        }
    }


    // Goliath
    // Idle: 1000
    int goliathEntity;
    int goliathShadowEntity;
    int goliathLife = 100;
    enum class GoliathState
    {
        IDLE,
        MOVING,
        ATTACKING,
        EXPOSED,
        DOWN,
        EXPLODING,
        DEAD
    };
    GoliathState goliathState = GoliathState::IDLE;
    enum class GoliathShotType :uint32_t
    {
        ROUND = 0,
        RANDOM = 1,
    };
    GoliathShotType goliathShotType = GoliathShotType::RANDOM;
    void loadGoliath() {
        goliathEntity = addBlitzEntity(Vector3D(160, 120, 20));
        addBlitzMugenAnimationComponent(goliathEntity, &mSprites, &mAnimations, 1000);
        addBlitzCollisionComponent(goliathEntity);
        int collisionId = addBlitzCollisionRect(goliathEntity, gGameScreenData.mGoliathCollisionList, CollisionRect(-15, -90, 30, 90));
        addBlitzCollisionCB(goliathEntity, collisionId, goliathHitCB, NULL);
        goliathShadowEntity = addBlitzEntity(Vector3D(160, 120, 10));
        addBlitzMugenAnimationComponent(goliathShadowEntity, &mSprites, &mAnimations, 1060);
        setBlitzMugenAnimationTransparency(goliathShadowEntity, 0.5);
    }

    void updateGoliath() {
        updateGoliathExplodingStart();
        updateGoliathExploding();
        updateGoliathMovement();
        updateGoliathShooting();
        updateGoliathExposed();
        updateGoliathDown();
        updateGoliathFlashing();
        updateGoliathShadow();
        updateGoliathZ();
    }

    void updateGoliathZ() {
        auto* pos = getBlitzEntityPositionReference(goliathShadowEntity);
        auto y = pos->y;
        auto z = 20 + (y / 240) * 10;
        setBlitzEntityPositionZ(goliathEntity, z);   
    }

    int goliathFlashingTicks = 0;
    void setGoliathFlashing() {
        goliathFlashingTicks = 2;
        setBlitzMugenAnimationColor(goliathEntity, 1, 0, 0);
    }

    void updateGoliathFlashing() {
        if(goliathFlashingTicks)
        {
            goliathFlashingTicks--;
            if(!goliathFlashingTicks)
            {
                setBlitzMugenAnimationColor(goliathEntity, 1, 1, 1);
            }
        }
    }

    void updateGoliathShadow() {
        auto* pos = getBlitzEntityPositionReference(goliathShadowEntity);
        *pos = getBlitzEntityPosition(goliathEntity) + Vector3D(0, goliathHeight, -1);
    }

    int goliathExplodingTicks = 0;

    void updateGoliathExplodingStart() {
        if (goliathState != GoliathState::IDLE) return;
        if(goliathLife <= 0)
        {
            startGoliathExploding();
        }
    }

    void startGoliathExploding() {
        goliathState = GoliathState::EXPLODING;
        changeBlitzMugenAnimation(goliathEntity, 1050);
        goliathExplodingTicks = 0;
        stopStreamingMusicFile();
    }

    void addExplosionAtPosition(const Vector2D& tPosition) {
        auto anim = addMugenAnimation(getMugenAnimation(&mAnimations, 8000), &mSprites, tPosition.xyz(50));
        setMugenAnimationNoLoop(anim);
        tryPlayMugenSound(&mSounds, 1, 7);
    }

    void updateGoliathExploding() {
        if (goliathState != GoliathState::EXPLODING && goliathState != GoliathState::DEAD) return;

        goliathExplodingTicks++;
        int frequency = 10;
        int steps = 120;
        if(goliathExplodingTicks <= steps)
        {
            frequency = 60;
        }
        else if(goliathExplodingTicks <= steps * 2)
        {
            frequency = 30;
        }
        else if (goliathExplodingTicks <= steps * 3)
        {
            frequency = 15;
        }
        else
        {
            frequency = 7;
        }
        if(goliathExplodingTicks % frequency == 0 && goliathExplodingTicks < steps * 5 + 60)
        {
            addExplosionAtPosition(getBlitzEntityPosition(goliathEntity).xy() + Vector2D(randfromInteger(-20, 20), randfromInteger(-50, 0)));
        }
        if (goliathExplodingTicks == steps * 5)
        {
            goliathState = GoliathState::DEAD;
            changeBlitzMugenAnimation(goliathEntity, 1080);
            setScreenShake(60);
            fadeGoliathTicks = 540;
        }
    }

    int fadeGoliathTicks = 0;
    MugenAnimationHandlerElement* whiteFadeBG;
    MugenAnimationHandlerElement* defeatedUI;
    int timerText = 0;
    void loadGoliathFadeAndUI()
    {
        whiteFadeBG = addMugenAnimation(getMugenAnimation(&mAnimations, 3001), &mSprites, Vector3D(0, 0, 65));
        setMugenAnimationTransparency(whiteFadeBG, 0);
        setMugenAnimationDrawScale(whiteFadeBG, Vector2D(30, 30));
        defeatedUI = addMugenAnimation(getMugenAnimation(&mAnimations, 3000), &mSprites, Vector3D(0, 0, 66));
        setMugenAnimationTransparency(defeatedUI, 0);
        {
            double timeElapsed = gameTicks / 60.0;
            std::string timeString = std::string("TIME: ") + std::to_string(timeElapsed);
            timerText = addMugenTextMugenStyle(timeString.c_str(), Vector3D(160, 150, 66), Vector3DI(2, 0, 0));
            setMugenTextVisibility(timerText, 0);
        }

    }

    void updateGoliathFadeAndUI()
    {
        if(!fadeGoliathTicks) return;

        fadeGoliathTicks--;

        if(fadeGoliathTicks == 539)
        {
            setMugenAnimationTransparency(whiteFadeBG, 0);
            setMugenAnimationTransparency(defeatedUI, 0);
        }
       
       if(fadeGoliathTicks <= 540 && fadeGoliathTicks >= 480)
        {
            double t = (540 - fadeGoliathTicks) / 60.0;
            setMugenAnimationTransparency(whiteFadeBG, t);
        }

        if(fadeGoliathTicks <= 480 && fadeGoliathTicks >= 420)
        {
            setMugenAnimationTransparency(defeatedUI, 1 - (fadeGoliathTicks - 420) / 60.0);
            
        }
        if(fadeGoliathTicks == 420)
        {
            stopStreamingMusicFile();
            tryPlayMugenSound(&mSounds, 2, 1);
            setMugenTextVisibility(timerText, 1);
            int timeElapsed1 = gameTicks / 60;
            int timeElapsed2 = gameTicks % 60;
            std::string timeString = std::string("TIME: ") + std::to_string(timeElapsed1) + ":" + std::to_string(int(timeElapsed2 * 100 / 60.0)) + " SECONDS";
            changeMugenText(timerText, timeString.c_str());
        }

        if(!fadeGoliathTicks)
        {
            gotoWinStory(NULL);
        }
    }


    int goliathDownTicks = 0;
    void updateGoliathDown() {
        updateGoliathDownFinish();
    }

    void setGoliathDown() {
        goliathState = GoliathState::DOWN;
        changeBlitzMugenAnimation(goliathEntity, 1005);
        goliathDownTicks = 0;
    }

    void updateGoliathDownFinish() {
        if (goliathState != GoliathState::DOWN) return;

        goliathDownTicks++;
        if (goliathDownTicks == 240 || goliathLife <= 0)
        {
            goliathState = GoliathState::IDLE;
            changeBlitzMugenAnimation(goliathEntity, 1000);
        }
    }

    int goliathExposedTicks = 0;
    void updateGoliathExposed() {
        updateGoliathExposedStart();
        updateGoliathExposedFinish();
    }

    int otherActions = 0;
    void updateGoliathExposedStart(){
        if (goliathState != GoliathState::IDLE) return;

        if (otherActions >= 5)
        {
            otherActions = 0;
            goliathState = GoliathState::EXPOSED;
            changeBlitzMugenAnimation(goliathEntity, 1040);
            goliathExposedTicks = 0;
            showTutorial2();
        }
    }

    void updateGoliathExposedFinish() {
        if (goliathState != GoliathState::EXPOSED) return;

        goliathExposedTicks++;
        if (goliathExposedTicks == 180 || goliathLife <= 0)
        {
            goliathState = GoliathState::IDLE;
            changeBlitzMugenAnimation(goliathEntity, 1000);
        }
    }

    int goliathAttackTicks = 0;
    void updateGoliathShooting() {
        updateGoliathShootingStart();
        updateGoliathShootingFinish();
    }

    int goliathShotFrequency = 10;
    double randOffset = 0;
    void updateGoliathShootingStart() {
        if (goliathState != GoliathState::IDLE) return;

        auto randVal = randfromInteger(0, 30);
        if (randVal == 0)
        {
            otherActions++;
            goliathState = GoliathState::ATTACKING;
            changeBlitzMugenAnimation(goliathEntity, 1030);
            goliathAttackTicks = 0;
            goliathShotType = GoliathShotType(int(randfrom(0, 1.99)));
            goliathShotFrequency = randfromInteger(3, 7);
            randOffset = randfrom(0, 2*M_PI);
        }
    }

    void updateGoliathShootingFinish() {
        if (goliathState != GoliathState::ATTACKING) return;

        goliathAttackTicks++;
        if(goliathAttackTicks % goliathShotFrequency == 0)
        {
            Vector2D direction = Vector2D(0, 0);
            if(goliathShotType == GoliathShotType::RANDOM)
            {
                while(vecLength(direction) < 0.1)
                {
                    direction = vecNormalize(Vector2D(randfromInteger(-10, 10), randfromInteger(-10, 10)));
                }
            }
            else
            {
                direction = vecRotateZ2D(Vector2D(1, 0), (((goliathAttackTicks % 60) / 60.0) * 2 * M_PI) + randOffset);
            }
            addGoliathShot(direction);

        }
        if (goliathAttackTicks == 180 || goliathLife <= 0)
        {
            goliathState = GoliathState::IDLE;
            changeBlitzMugenAnimation(goliathEntity, 1000);
        }
    }

    int goliathMovementTicks = 0;
    double goliathHeight = 0.f;
    void updateGoliathMovement() {
        updateGoliathMovementStart();
        updateGoliathMovementFinish();
    }

    void updateGoliathMovementStart() {
        if (goliathState != GoliathState::IDLE) return;

        auto randVal = randfromInteger(0, 60);
        if (randVal == 0)
        {
            otherActions++;
            goliathState = GoliathState::MOVING;
            changeBlitzMugenAnimation(goliathEntity, 1020);
            goliathMovementTicks = 0;
        }
    }

    void updateGoliathMovementFinish() {
        if (goliathState != GoliathState::MOVING) return;

        goliathMovementTicks++;
        if(goliathMovementTicks < 60)
        {
            auto* pos = getBlitzEntityPositionReference(goliathEntity);
            pos->y -= 8.0;
            goliathHeight += 8.0;
        }
        else if (goliathMovementTicks == 60)
        {
            tryPlayMugenSound(&mSounds, 1, 1);
            auto newPos = Vector3D(randfromInteger(20, 300), randfromInteger(50, 240), 20);
            auto* pos = getBlitzEntityPositionReference(goliathEntity);
            *pos = newPos;
            pos->y -= 480.0;
            goliathHeight = 480.0;
            changeBlitzMugenAnimation(goliathEntity, 1020);
        }
        else if(goliathMovementTicks < 121)
        {
            auto* pos = getBlitzEntityPositionReference(goliathEntity);
            pos->y += 8.0;
            goliathHeight -= 8.0;
        }
        else if (goliathMovementTicks == 121)
        {
            tryPlayMugenSound(&mSounds, 1, 2);
            goliathState = GoliathState::IDLE;
            goliathHeight = 0;
            changeBlitzMugenAnimation(goliathEntity, 1025);
            auto dustAnim = addMugenAnimation(getMugenAnimation(&mAnimations, 1070), &mSprites, getBlitzEntityPosition(goliathEntity).xy().xyz(31));
            setMugenAnimationNoLoop(dustAnim);
            setMugenAnimationTransparency(dustAnim, 0.5);
            setScreenShake(10);

            
            {
                auto playerPos = getBlitzEntityPosition(playerEntity).xy();
                auto goliathPos = getBlitzEntityPosition(goliathEntity).xy();
                if (vecLength(playerPos - goliathPos) < 20)
                {
                    if(!playerInvincibilityTicks)
                    {
                        tryPlayMugenSound(&mSounds, 1, 4);
                        playerLife = max(0, playerLife - 10);
                    }
                    if(playerLife > 0)
                    {
                        setHitPause(20);
                        setPlayerInvincibility();
                    }
                }
            }
        }
    }


    // Goliath shots
    // Shot: 2000
    struct GoliathShot
    {
        int entity;
        Vector2D direction;
        int toBeDeleted = 0;
    };
    std::map<int, GoliathShot> goliathShots;

    void loadGoliathShots()
    {
        goliathShots.clear();
    }

    void addGoliathShot(Vector2D tDirection)
    {
        tryPlayMugenSound(&mSounds, 1, 3);
        auto goliathPosition = getBlitzEntityPosition(goliathEntity).xy();
        auto entity = addBlitzEntity((goliathPosition + Vector2D(0, -47)).xyz(45) + Vector3D(tDirection.x * 10, tDirection.y * 10, 0));
        addBlitzMugenAnimationComponent(entity, &mSprites, &mAnimations, 2000);
        addBlitzCollisionComponent(entity);
        int collisionId = addBlitzCollisionCirc(entity, gGameScreenData.mGoliathShotCollisionList, CollisionCirc{Vector2D(0, 0), 4});
        goliathShots[entity] = { entity, tDirection, 0 };
        addBlitzCollisionCB(entity, collisionId, goliathShotHitCB, &goliathShots[entity]);
    }

    void updateSingleGoliathShot(GoliathShot& tShot)
    {
        auto* pos = getBlitzEntityPositionReference(tShot.entity);
        *pos += tShot.direction * 4.f;

        if (pos->x < -10 || pos->x > 330 || pos->y < -10 || pos->y > 250)
        {
            tShot.toBeDeleted = 1;
        }
    }

    void updateGoliathShots()
    {
        updateGoliathShotDeletion();
        for (auto& shot : goliathShots)
        {
            updateSingleGoliathShot(shot.second);
        }
        updateGoliathShotDeletion();
    }

    void updateGoliathShotDeletion()
    {
        for(auto iter = goliathShots.begin(); iter !=  goliathShots.end(); ) {
            if (iter->second.toBeDeleted) {
                if(iter->second.toBeDeleted == 2)
                {
                    auto anim = addMugenAnimation(getMugenAnimation(&mAnimations, 2010), &mSprites, getBlitzEntityPosition(iter->second.entity));
                    setMugenAnimationNoLoop(anim);
                    auto playerPos = getBlitzEntityPosition(playerEntity).xy();
                    auto shotPos = getBlitzEntityPosition(iter->second.entity).xy();
                    auto dist = vecLength(playerPos - shotPos);
                    if(dist < 20)
                    {
                        if(!playerInvincibilityTicks)
                        {
                            tryPlayMugenSound(&mSounds, 1, 4);
                            playerLife = max(0, playerLife - 10);
                        }
                        if(playerLife > 0)
                        {
                            setHitPause(20);
                            setPlayerInvincibility();
                        }
                    }
                }
                removeBlitzEntity(iter->second.entity);
                iter = goliathShots.erase(iter);
            } else {
                ++iter;
            }
        }
    }

    void playerHitCBInternal(void* tCaller, void* tCollisionData)
    {
    }
    void playerShotHitCBInternal(void* tCaller, void* tCollisionData)
    {
        auto shot = (PlayerShot*)tCaller;
        if(playerShots.find(shot->entity) != playerShots.end())
        {
            shot->toBeDeleted = 2;
        }
    }
    void goliathHitCBInternal(void* tCaller, void* tCollisionData)
     {
     }
    void goliathShotHitCBInternal(void* tCaller, void* tCollisionData)
    {
        auto shot = (GoliathShot*)tCaller;
        if(goliathShots.find(shot->entity) != goliathShots.end())
        {
            shot->toBeDeleted = 2;
        }
    }

    // UI
    // 3000 anim range
    MugenAnimationHandlerElement* goliathLifebarBG;
    MugenAnimationHandlerElement* goliathLifebarFG;
    MugenAnimationHandlerElement* playerLifebarBG;
    MugenAnimationHandlerElement* playerLifebarFG;
    void loadUI()
    {
        goliathLifebarBG = addMugenAnimation(getMugenAnimation(&mAnimations, 510), &mSprites, Vector3D(16, 10, 60));
        goliathLifebarFG = addMugenAnimation(getMugenAnimation(&mAnimations, 511), &mSprites, Vector3D(16, 10, 61));
        playerLifebarBG = addMugenAnimation(getMugenAnimation(&mAnimations, 500), &mSprites, Vector3D(238, 230, 60));
        playerLifebarFG = addMugenAnimation(getMugenAnimation(&mAnimations, 501), &mSprites, Vector3D(238, 230, 61));
    }

    void updateUI()
    {
        updateGoliathLifebar();
        updatePlayerLifebar();
    }

    void updateGoliathLifebar()
    {
        setMugenAnimationDrawScale(goliathLifebarFG, Vector2D((goliathLife / 100.0) * 291, 1));
    }

    void updatePlayerLifebar()
    {
        setMugenAnimationDrawScale(playerLifebarFG, Vector2D((playerLife / 100.0) * 76, 1));
    }

    struct FloatingNumber
    {
        int textId;
        int ticks;
    };
    std::vector<FloatingNumber> floatingNumbers;

    void addFloatingNumber(const Vector2D& tPosition, int number)
    {
        int color = 0;
        if(number > 5)
        {
            color = 1;
        }
        else if(number > 3)
        {
            color = 6;
        }
        else if(number > 1)
        {
            color = 5;
        }

        std::string text = std::to_string(number);
        auto textId = addMugenTextMugenStyle(text.c_str(), tPosition.xyz(40), Vector3DI(2, color, 1));
        floatingNumbers.push_back({ textId, 0 });
    }

    void updateFloatingNumbers()
    {
        for (auto& number : floatingNumbers)
        {
            number.ticks++;
            addMugenTextPosition(number.textId, Vector3D(0, -1, 0));
            if (number.ticks == 30)
            {
                removeMugenText(number.textId);
            }
        }
        floatingNumbers.erase(std::remove_if(floatingNumbers.begin(), floatingNumbers.end(), [](const FloatingNumber& tNumber) { return tNumber.ticks == 30; }), floatingNumbers.end());
    }

    int hitPauseTicks = 0;
    void setHitPause(int tDuration)
    {
        if(!tDuration) return;
        hitPauseTicks = tDuration;
        pauseBlitzMugenAnimation(playerEntity);
        pauseBlitzMugenAnimation(rectangleEntity);
        pauseBlitzMugenAnimation(gunEntity);
        pauseBlitzMugenAnimation(goliathEntity);
        pauseBlitzMugenAnimation(goliathShadowEntity);
        for (auto& shot : playerShots)
        {
            pauseBlitzMugenAnimation(shot.second.entity);
        }
        for (auto& shot : goliathShots)
        {
            pauseBlitzMugenAnimation(shot.second.entity);
        }
    }

    void updateHitPause()
    {
        if (!hitPauseTicks) return;

        hitPauseTicks--;
        if (!hitPauseTicks)
        {
            unpauseBlitzMugenAnimation(playerEntity);
            unpauseBlitzMugenAnimation(rectangleEntity);
            unpauseBlitzMugenAnimation(gunEntity);
            unpauseBlitzMugenAnimation(goliathEntity);
            unpauseBlitzMugenAnimation(goliathShadowEntity);
            for (auto& shot : playerShots)
            {
                unpauseBlitzMugenAnimation(shot.second.entity);
            }
            for (auto& shot : goliathShots)
            {
                unpauseBlitzMugenAnimation(shot.second.entity);
            }
        }
    }
};

EXPORT_SCREEN_CLASS(GameScreen);

void resetGame()
{
    gGameScreenData.mLevel = 0;
}

static void playerHitCB(void* tCaller, void* tCollisionData)
{
    gGameScreen->playerHitCBInternal(tCaller, tCollisionData);
}
static void playerShotHitCB(void* tCaller, void* tCollisionData)
{
    gGameScreen->playerShotHitCBInternal(tCaller, tCollisionData);
}
static void goliathHitCB(void* tCaller, void* tCollisionData)
{
    gGameScreen->goliathHitCBInternal(tCaller, tCollisionData);
}
static void goliathShotHitCB(void* tCaller, void* tCollisionData)
{
    gGameScreen->goliathShotHitCBInternal(tCaller, tCollisionData);
}
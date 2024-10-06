#include "titlescreen.h"

#include <prism/blitz.h>
#include <prism/soundeffect.h>

#include "gamescreen.h"

class GameOverScreen
{
public:
    MugenSpriteFile mSprites;
    MugenAnimations mAnimations;
    MugenSounds mSounds;

    MugenAnimationHandlerElement* titleAnimation;

    GameOverScreen() {
        mSprites = loadMugenSpriteFileWithoutPalette("game/GAMEOVER.sff");
        mAnimations = loadMugenAnimationFile("game/GAMEOVER.air");
        if(isOnDreamcast())
        {
            mSounds = loadMugenSoundFile("game/dreamcast/GAME.snd");
        }
        else
        {
            mSounds = loadMugenSoundFile("game/GAME.snd");
        }

        titleAnimation = addMugenAnimation(getMugenAnimation(&mAnimations, 1), &mSprites, Vector3D(0, 0, 1));
        if (isOnDreamcast())
        {
            setSoundEffectVolume(1.0);
        }
        else
        {
            setVolume(0.5);
            setSoundEffectVolume(0.5);
        }
        tryPlayMugenSound(&mSounds, 2, 0);
    }
    void update() {
        updateInput();

    }

    void updateInput()
    {
        if(hasPressedStartFlank())
        {
            setNewScreen(getGameScreen());
        }
    }
};

EXPORT_SCREEN_CLASS(GameOverScreen);
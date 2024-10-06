#include "titlescreen.h"

#include <prism/blitz.h>
#include <prism/soundeffect.h>

#include "storyscreen.h"

class TitleScreen
{
public:
    MugenSpriteFile mSprites;
    MugenAnimations mAnimations;
    MugenSounds mSounds;

    MugenAnimationHandlerElement* titleAnimation;

    TitleScreen() {
        mSprites = loadMugenSpriteFileWithoutPalette("game/TITLE.sff");
        mAnimations = loadMugenAnimationFile("game/TITLE.air");
        if(isOnDreamcast())
        {
            mSounds = loadMugenSoundFile("game/dreamcast/TITLE.snd");
        }
        else
        {
            mSounds = loadMugenSoundFile("game/TITLE.snd");
        }

        titleAnimation = addMugenAnimation(getMugenAnimation(&mAnimations, 1), &mSprites, Vector3D(0, 0, 1));
        if (isOnDreamcast())
        {
            setSoundEffectVolume(1.0);
        }
        else
        {
            setVolume(0.25);
            setSoundEffectVolume(0.5);
        }
        if(isOnDreamcast())
        {
            streamMusicFile("game/dreamcast/TITLE.ogg");
        }
        else
        {
            streamMusicFile("game/TITLE.ogg");
        }
        tryPlayMugenSound(&mSounds, 1, 0);
    }
    void update() {
        updateInput();

    }

    void updateInput()
    {
        if(hasPressedStartFlank())
        {
            setCurrentStoryDefinitionFile("game/INTRO.def", 1);
            setNewScreen(getStoryScreen());
        }
    }
};

EXPORT_SCREEN_CLASS(TitleScreen);
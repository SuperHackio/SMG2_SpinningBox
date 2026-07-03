#pragma once

#include "Game/LiveActor/LiveActor.h"

class SpinningBox : public LiveActor {
public:
    SpinningBox(const char*);

    virtual void init(const JMapInfoIter& rIter);
    virtual void initAfterPlacement();
    virtual void kill();
    virtual void control();
    virtual void calcAndSetBaseMtx();
    virtual void attackSensor(HitSensor* pSender, HitSensor* pReceiver);
    virtual bool receiveMsgPlayerAttack(u32 msg, HitSensor* pSender, HitSensor* pReceiver);
    virtual bool receiveMsgEnemyAttack(u32 msg, HitSensor* pSender, HitSensor* pReceiver);
    virtual bool receiveOtherMsg(u32 msg, HitSensor* pSender, HitSensor* pReceiver);

    void exeFall();
    void exeWait();
    void exeOnEndWait();
    void exeSliding();
    void exeOnEndSliding();
    void exeSpinning();
    void exeOnEndSpinning();
    void exePointed();
    void exeOnEndPointed();

    void exeWaitRespawn();
    void exeOnEndWaitRespawn();

    void calcPlanarProjectedVec(TVec3f*, const TVec3f&, const TVec3f&);
    void calcHitDirection(const TVec3f&);
    void generateIceBox(HitSensor* pSender, HitSensor* pReceiver);
    void hitReflection(HitSensor* pSender, HitSensor* pReceiver);

    f32 getScale() const {
        return 1.0f;
    }

    TQuat4f mRotate;
    f32 mRotateSpeed;
    f32 mSlideSpeed;
    TVec3f mSlideDir;
    bool mIsOnGround;
    bool mIsIceBox;
    f32 mSpinAngle;  // NOTE: This keeps track of when the spin sfx should be emitted again
    TVec3f mInitPosition; // NEW
    s32 mRespawnTime; // NEW
    s32 mAutoKillTime; // NEW
    f32 mAutoKillRadius; // NEW
};

namespace MR {
    void addToAttributeGroupReflectSpinningBox(const LiveActor* pActor);
    bool isExistInAttributeGroupReflectSpinningBox(const LiveActor* pActor);


    bool isBindedWallOrSlopeGround(const LiveActor*, f32, TVec3f*);
}
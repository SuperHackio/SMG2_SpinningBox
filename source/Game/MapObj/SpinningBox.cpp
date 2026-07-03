#include "Game/MapObj/SpinningBox.h"
#include "Game/LiveActor/HitSensor.h"
#include "Game/Util/ActorMovementUtil.h"
#include "Game/Util/ActorSensorUtil.h"
#include "Game/Util/ActorShadowUtil.h"
#include "Game/Util/EffectUtil.h"
#include "Game/Util/JMapUtil.h"
#include "Game/Util/LiveActorUtil.h"
#include "Game/Util/MapUtil.h"
#include "Game/Util/MathUtil.h"
#include "Game/Util/ObjUtil.h"
#include "Game/Util/PlayerUtil.h"
#include "Game/Util/SoundUtil.h"
#include "Game/Util/StarPointerUtil.h"
#include "JSystem/JGeometry/TUtil.h"

#include "Game/Util.h"
#include "Game/Scene/SceneObjHolder.h"
#include "Game/Player/GroupChecker.h"
#include "JSystem/JMATrigonometric.h"

namespace {
    static const f32 hForceBindOffset = 65.0f;
    static const f32 hForceBindMul = 0.5f;
    static const f32 sShadowSize = 160.0f;
    // static const _32 hSpinningTime = ;
    static const f32 hGravityVel = 3.0f;
    static const f32 hVelHMin = 10.0f;
    static const f32 hReflectRate = 0.5f;
    static const f32 hSpinningAttackStrongVel = 10.0f;
    static const f32 hStopCheckVel = 0.1f;
    static const f32 hGroundCosine = 0.4f;
    static const s32 hWaitTime = 30;
    // static const _32 hKickableFrontDotCheck =
    static const f32 hPlayerIsSideDotCheck = 0.25f;
    static const f32 hSpinningAngle = 2.0f;
    static const f32 hDampVelHWait = 0.6f;
    static const f32 hDampVelVWait = 0.6f;
    static const f32 hTurnYDirRatio = 0.125f;
    // static const _32 hHitVelocity =
    // static const _32 hSpinningKind =
    // static const _32 hScVelCheck =
    static const f32 hPointedRotDamp = 0.8f;
    // static const _32 hWalkAngle =
    // static const _32 hWalkVelocity =
    static const f32 hSlidingVelocitySub = 0.08f;
    static const f32 hSlidingVelMax = 17.0f;
    static const f32 hSlidingRotateSub = 0.01f;
    static const f32 hSlidingRotVelMin = 0.02f;
    static const f32 hSlidingRotVelMax = 0.02f;
    static const s32 hKickableTime = 30;
    static const f32 hSpinningVelocitySub = 0.01f;
    static const f32 hSpinningVelMin = 0.0f;
    static const f32 hSpinningVelMax = 25.0f;
    static const f32 hSpinningRotateSub = 0.001f;
    static const f32 hSpinningRotVelMin = 0.1f;
    static const f32 hSpinningRotVelMax = 0.5f;
    static const f32 hPlayerVelocityCheck = 10.0f;
    static const f32 hIceInitBlowVelV = 100.0f;
    static const f32 hKickCheckOffsetUp = 0.0f;
    static const f32 hIceInitBlowVelH = 0.0f;

    inline f32 toRadian(f32 angle) {
        f32 x = (JMath::PI / 180.0f);
        return angle * x;
    }
}  // namespace

namespace NrvSpinningBox {
    FULL_NERVE(HostTypeNrvFall, SpinningBox, Fall);
    FULL_NERVE_ONEND(HostTypeNrvWait, SpinningBox, Wait, Wait);
    FULL_NERVE_ONEND(HostTypeNrvSliding, SpinningBox, Sliding, Sliding);
    FULL_NERVE_ONEND(HostTypeNrvSpinning, SpinningBox, Spinning, Spinning);
    FULL_NERVE_ONEND(HostTypeNrvPointed, SpinningBox, Pointed, Pointed);

    FULL_NERVE(HostTypeNrvWaitRespawn, SpinningBox, WaitRespawn);
}  // namespace NrvSpinningBox

SpinningBox::SpinningBox(const char* pName)
    : LiveActor(pName), mRotate(0.0f, 0.0f, 0.0f, 1.0f), mRotateSpeed(0.0f), mSlideSpeed(0.0f), mSlideDir(1.0f, 0.0f, 0.0f), mIsOnGround(false),
      mIsIceBox(false), mSpinAngle(0.0f), mRespawnTime(-1), mAutoKillTime(-1), mAutoKillRadius(0.0f) {
}

void SpinningBox::init(const JMapInfoIter& rIter) {
    MR::processInitFunction(this, rIter, false);

    initNerve(&NrvSpinningBox::HostTypeNrvFall::sInstance, 0);
    MR::setBinderExceptActor(this, this);
    MR::onCalcGravity(this);
    //MR::initShadowVolumeBox(this, TVec3f(sShadowSize, -70.0f, sShadowSize).scaleInline(scale), getBaseMtx());
    {
        initShadowControllerList(1);
        MR::addShadowVolumeBox(this, "ボリューム影(ボックス)", TVec3f(sShadowSize, -70.0f, sShadowSize), getBaseMtx());
    }

    MR::excludeCalcShadowToMyCollision(this, nullptr);
    MR::addToAttributeGroupReflectSpinningBox(this);
    MR::declareStarPiece(this, 3);
    makeActorAppeared();
    
    mRotate.setEuler(toRadian(mRotation.x), toRadian(mRotation.y), toRadian(mRotation.z));

    mInitPosition.set(mTranslation);
    MR::getJMapInfoArg0NoInit(rIter, &mRespawnTime);
    MR::getJMapInfoArg1NoInit(rIter, &mAutoKillTime);
    MR::getJMapInfoArg2NoInit(rIter, &mAutoKillRadius);
}

void SpinningBox::initAfterPlacement() {
    bool isvalid = MR::isValidCollisionParts(this);
    MR::invalidateCollisionParts(this);
    MR::trySetMoveLimitCollision(this);
    if (isvalid)
        MR::validateCollisionParts(this);
}

void SpinningBox::kill() {
    MR::startActionSound(this, "OjSpinBoxBreak", -1, -1, -1);
    //MR::startSound(this, "SE_OJ_STAR_PIECE_BURST");
    if (mRespawnTime < 0) {
        LiveActor::kill();
        setNerve(&NrvSpinningBox::HostTypeNrvWait::sInstance);
    }
    else {
        setNerve(&NrvSpinningBox::HostTypeNrvWaitRespawn::sInstance);
    }

    MR::appearStarPiece(this, mTranslation - (mGravity * 50.0f), MR::getDeclareRemnantStarPieceCount(this), 10.0f, 40.0f, false);
    MR::tryEmitEffect(this, "Break");
}

void SpinningBox::control() {
    if (isNerve(&NrvSpinningBox::HostTypeNrvWaitRespawn::sInstance))
        return;

    if (MR::isInDeath(this, TVec3f(0.0f, 0.0f, 0.0f)) || MR::isInWater(mTranslation)) {
        kill();
        return;
    }

    //if (MR::isBindedDamageElectric(this)) {
    //    kill();
    //    return;
    //}

    TVec3f slideDir = mSlideDir;
    TVec3f norm;
    if (MR::isBindedGround(this)) {
        norm.set(MR::getGroundNormal(this));
        calcPlanarProjectedVec(&slideDir, norm, mGravity);
    } else {
        norm.set(-mGravity);
        MR::vecKillElement(slideDir, norm, &slideDir);
    }

    MR::normalizeOrZero(&slideDir);
    if (!MR::isNearZero(slideDir, 0.001f)) {
        mSlideDir.set(slideDir);
    }

    TVec3f up;
    mRotate.getYDir(up);
    TQuat4f rotY;
    rotY.setRotate(up, norm, hTurnYDirRatio);
    PSQUATMultiply(rotY, mRotate, mRotate);
    mRotate.getYDir(up);
    rotY.setRotate(up, -mRotateSpeed);
    PSQUATMultiply(rotY, mRotate, mRotate);
    mRotate.normalize();

    if (MR::isOnPlayer(getSensor("body")) && !isNerve(&NrvSpinningBox::HostTypeNrvWait::sInstance)) {
        setNerve(&NrvSpinningBox::HostTypeNrvWait::sInstance);
    }
}

void SpinningBox::exeFall() {
    if (MR::isFirstStep(this)) {
        MR::invalidateClipping(this);
    }

    MR::reboundVelocityFromCollision(this, 0.0f, 0.0f, 1.0f);
    MR::applyVelocityDampAndGravity(this, hGravityVel, hDampVelHWait, hDampVelHWait, hDampVelVWait, hVelHMin);

    if (MR::isOnGround(this) && MR::isGreaterStep(this, hWaitTime)) {
        setNerve(&NrvSpinningBox::HostTypeNrvWait::sInstance);
    }
}

void SpinningBox::exeWait() {
    if (MR::isFirstStep(this)) {
        mIsOnGround = false;
        mRotateSpeed = 0.0f;
        MR::validateClipping(this);
    }

    if (MR::isStarPointerPointing2POnPressButton(this, "弱", true, false)) {
        setNerve(&NrvSpinningBox::HostTypeNrvPointed::sInstance);
        return;
    }

    TVec3f originalGravity = mGravity;
    if (MR::isOnGround(this)) {
        mIsOnGround = true;
        mGravity.set(-MR::getGroundNormal(this));
    }

    MR::reboundVelocityFromCollision(this, 0.0f, 0.0f, 1.0f);

    if (mIsOnGround) {
        MR::applyVelocityDampAndGravity(this, hGravityVel, 0.0f, 0.0f, hDampVelVWait, hVelHMin);
    } else {
        MR::applyVelocityDampAndGravity(this, hGravityVel, hDampVelHWait, hDampVelHWait, hDampVelVWait, hVelHMin);
    }

    mGravity.set(originalGravity);  // "gravity" was only used to update velocity, revert to initial

    if (mAutoKillTime >= 0) {
        if (PSVECDistance(mTranslation.toVecPtr(), mInitPosition.toVecPtr()) > 100.0f) {
            if (MR::isGreaterEqualStep(this, mAutoKillTime)) {
                if (mAutoKillRadius > 0.0f) {
                    if (PSVECDistance(mTranslation.toVecPtr(), mInitPosition.toVecPtr()) > mAutoKillRadius) {
                        kill();
                    }
                }
                else
                    kill();
            }
        }
    }
}

void SpinningBox::exeOnEndWait(){};

void SpinningBox::exeSliding() {
    if (MR::isFirstStep(this)) {
        mRotateSpeed = hSlidingRotVelMax;
        MR::invalidateClipping(this);
    }

    if (MR::isStarPointerPointing2POnPressButton(this, "弱", true, false)) {
        setNerve(&NrvSpinningBox::HostTypeNrvPointed::sInstance);
        return;
    }

    mRotateSpeed = MR::clamp(mRotateSpeed - hSlidingRotateSub, hSlidingRotVelMin, hSlidingRotVelMax);
    mSlideSpeed = MR::clamp(mSlideSpeed - hSlidingVelocitySub, 0.0f, hSlidingVelMax);
    f32 slideSoundLvl = (mSlideSpeed / hSlidingVelMax) * 100.0f;
    if (slideSoundLvl > 100.0f) {
        slideSoundLvl = 100.0f;
    }
    MR::startActionSound(this, "OjLvSpinBoxSlide", slideSoundLvl, -1, -1);

    if (mSlideSpeed <= hStopCheckVel) {
        setNerve(&NrvSpinningBox::HostTypeNrvWait::sInstance);
        return;
    }

    MR::reboundVelocityFromCollision(this, 0.0f, 0.0f, 1.0f);

    MR::vecKillElement(mVelocity, mSlideDir, &mVelocity);
    mVelocity = mSlideDir;
    mVelocity.scale(mSlideSpeed);

    if (!MR::isOnGround(this)) {
        MR::forceBindOnGround(this, hForceBindOffset, hForceBindMul);
    }

    TVec3f norm;
    if (MR::isBindedWallOrSlopeGround(this, hGroundCosine, &norm) || MR::isBindedWallOfMoveLimit(this)) {
        MR::startActionSound(this, "OjSpinBoxBound", -1, -1, -1);
        MR::calcReflectionVector(&mSlideDir, norm, 1.0f, 0.0f);
        mSlideSpeed *= hReflectRate;
        MR::normalizeOrZero(&mSlideDir);
    }
}

void SpinningBox::exeOnEndSliding() {
    MR::tryDeleteEffect(this, "Blur");
}

void SpinningBox::exeSpinning() {
    if (MR::isFirstStep(this)) {
        MR::tryEmitEffect(this, "Blur");
        mSpinAngle = 0.0f;
        MR::invalidateClipping(this);
    }

    if (MR::isStarPointerPointing2POnPressButton(this, "弱", true, false)) {
        setNerve(&NrvSpinningBox::HostTypeNrvPointed::sInstance);
        return;
    }

    mRotateSpeed = MR::clamp(mRotateSpeed - hSpinningRotateSub, hSpinningRotVelMin, hSpinningRotVelMax);
    mSlideSpeed = MR::clamp(mSlideSpeed - hSpinningVelocitySub, hSpinningVelMin, hSpinningVelMax);
    mSpinAngle += mRotateSpeed;

    if (mSpinAngle > hSpinningAngle) {
        while (mSpinAngle > hSpinningAngle) {
            mSpinAngle -= hSpinningAngle;
        }

        f32 spinSoundLvl = (mRotateSpeed / hSpinningRotVelMax) * 100.0f;
        if (spinSoundLvl > 100.0f) {
            spinSoundLvl = 100.0f;
        }
        MR::startActionSound(this, "OjSpinBoxSpin", spinSoundLvl, -1, -1);
    }

    f32 slideSoundLvl = (mSlideSpeed / hSlidingVelMax) * 100.0f;
    if (slideSoundLvl > 100.0f) {
        slideSoundLvl = 100.0f;
    }
    MR::startActionSound(this, "OjLvSpinBoxSlide", slideSoundLvl, -1, -1);

    if (mSlideSpeed <= hStopCheckVel) {
        setNerve(&NrvSpinningBox::HostTypeNrvWait::sInstance);
        return;
    }

    MR::reboundVelocityFromCollision(this, 0.0f, 0.0f, 1.0f);

    MR::vecKillElement(mVelocity, mSlideDir, &mVelocity);
    mVelocity = mSlideDir;
    mVelocity.scale(mSlideSpeed);

    if (!MR::isOnGround(this)) {
        MR::forceBindOnGround(this, hForceBindOffset, hForceBindMul);
    }

    if (MR::isBindedWallOrSlopeGround(this, hGroundCosine, nullptr) || MR::isBindedWallOfMoveLimit(this)) {
        kill();
    }
}

void SpinningBox::exeOnEndSpinning() {
    MR::tryDeleteEffect(this, "Blur");
}

void SpinningBox::exePointed() {
    if (MR::isFirstStep(this)) {
        mVelocity.zero();
        MR::startDPDHitSound();
        MR::tryEmitEffect(this, "Touch");
        MR::invalidateClipping(this);
    }

    mRotateSpeed *= hPointedRotDamp;
    MR::startDPDFreezeLevelSound(this);

    if (!MR::isStarPointerPointing2POnPressButton(this, "弱", true, false)) {
        setNerve(&NrvSpinningBox::HostTypeNrvWait::sInstance);
    }
}

void SpinningBox::exeOnEndPointed() {
    MR::tryDeleteEffect(this, "Touch");
}

void SpinningBox::exeWaitRespawn() {
    if (MR::isFirstStep(this)) {
        MR::hideModel(this);
        MR::invalidateClipping(this);
        MR::invalidateHitSensors(this);
        MR::offBind(this);
        MR::invalidateShadowAll(this);
        MR::invalidateCollisionParts(this);
        mVelocity.zero();
    }

    if (MR::isGreaterEqualStep(this, mRespawnTime)) {
        MR::showModel(this);
        MR::validateClipping(this);
        MR::validateHitSensors(this);
        MR::onBind(this);
        MR::validateShadowAll(this);
        MR::validateCollisionParts(this);
        mTranslation.set(mInitPosition);
        mRotateSpeed = 0.0f;
        mSlideSpeed = 0.0f;
        mSpinAngle = 0.0f;
        mSlideDir = TVec3f(1.0f, 0.0f, 0.0f);
        mRotate.setEuler(toRadian(mRotation.x), toRadian(mRotation.y), toRadian(mRotation.z));

        MR::tryEmitEffect(this, "Appear");
        MR::startActionSound(this, "OjSpinBoxAppear", -1, -1, -1);

        setNerve(&NrvSpinningBox::HostTypeNrvWait::sInstance);
    }
}

void SpinningBox::calcPlanarProjectedVec(TVec3f* pProj, const TVec3f& rNorm, const TVec3f& rGrav) {
    TVec3f vec(rGrav);
    vec.scale(-rNorm.dot(*pProj));
    pProj->add(vec / rNorm.dot(rGrav));
}

void SpinningBox::calcHitDirection(const TVec3f& rPos) {
    TVec3f hitDir = mTranslation - rPos;
    MR::vecKillElement(hitDir, mGravity, &hitDir);
    MR::normalizeOrZero(&hitDir);

    if (MR::isNearZero(hitDir, 0.001f)) {
        return;
    }

    TVec3f velDir = mVelocity;
    MR::normalizeOrZero(&velDir);
    velDir.add(hitDir);
    MR::normalizeOrZero(&velDir);
    if (MR::isNearZero(velDir, 0.001f)) {
        mSlideDir = hitDir;
    } else {
        mSlideDir = velDir;
    }
}

void SpinningBox::generateIceBox(HitSensor* pSender, HitSensor* pReceiver) {
    appear();
    mTranslation.set(pReceiver->mActor->mTranslation);
    TPos3f mtx = *(TPos3f*)pReceiver->mActor->getBaseMtx();
    TVec3f up, front;
    mtx.getYDir(up);
    mtx.getZDir(front);

    MR::makeQuatFromVec(&mRotate, front, up);

    TVec3f grav = pReceiver->mActor->mGravity;
    TVec3f dir = pReceiver->mPosition - pSender->mPosition;
    MR::vecKillElement(dir, grav, &dir);
    dir.setLength(hIceInitBlowVelH);
    grav.negate();
    grav.scale(hIceInitBlowVelV);
    dir.add(grav);
    mVelocity.set(dir);

    if (hPlayerVelocityCheck < PSVECMag(MR::getPlayerVelocity()->toVecPtr())) {
        calcHitDirection(pSender->mPosition);
        setNerve(&NrvSpinningBox::HostTypeNrvSpinning::sInstance);
    } else {
        setNerve(&NrvSpinningBox::HostTypeNrvFall::sInstance);
    }
}

void SpinningBox::attackSensor(HitSensor* pSender, HitSensor* pReceiver) {
    if (pReceiver->isType(ATYPE_BEGOMAN)) {
        MR::sendMsgEnemyAttack(pReceiver, pSender);
        return;
    }

    if (MR::isSensorPlayer(pReceiver)) {
        if (MR::isPlayerExistSide(this, hKickCheckOffsetUp, hPlayerIsSideDotCheck) && !MR::isOnPlayer(pSender)) {
            if (isNerve(&NrvSpinningBox::HostTypeNrvSpinning::sInstance)) {
                if (MR::isPlayerSwingAction() || !MR::isOnGroundPlayer()) {
                    return;
                }

                bool hit;
                if (hSpinningAttackStrongVel < mSlideSpeed) {
                    hit = MR::sendMsgEnemyAttackStrong(pReceiver, pSender);
                } else {
                    hit = MR::sendMsgEnemyAttackFlipWeak(pReceiver, pSender);
                }

                if (hit) {
                    hitReflection(pReceiver, pSender);
                } else {
                    MR::sendMsgPush(pReceiver, pSender);
                }

            } else if (isNerve(&NrvSpinningBox::HostTypeNrvSliding::sInstance)) {
                MR::sendMsgPush(pReceiver, pSender);
            }
        }
        return;
    }

    if (MR::isSensorMapObj(pReceiver) || MR::isSensorEnemy(pReceiver)) {
        if (isNerve(&NrvSpinningBox::HostTypeNrvSpinning::sInstance) || isNerve(&NrvSpinningBox::HostTypeNrvSliding::sInstance)) {
            if (MR::sendMsgToEnemyAttackBlow(pReceiver, pSender)) {
                if (!MR::isExistInAttributeGroupReflectSpinningBox(pReceiver->mActor)) {
                    kill();
                }
            }
        }
    }
}

bool SpinningBox::receiveOtherMsg(u32 msg, HitSensor* pSender, HitSensor* pReceiver) {
    if (MR::isMsgInhaleBlackHole(msg)) {
        kill();
        return true;
    }

    if (MR::isMsgPlayerKick(msg) && MR::isSensorMapObj(pReceiver)) {
        if (isNerve(&NrvSpinningBox::HostTypeNrvSpinning::sInstance)) {
            return false;
        }

        if (isNerve(&NrvSpinningBox::HostTypeNrvSliding::sInstance) && MR::isLessStep(this, hKickableTime)) {
            return false;
        }

        if (MR::isOnPlayer(getSensor("body"))) {
            return false;
        }

        if (MR::isPlayerExistSide(this, hKickCheckOffsetUp, hPlayerIsSideDotCheck)) {
            if (isNerve(&NrvSpinningBox::HostTypeNrvPointed::sInstance)) {
                return false;
            }

            MR::getPlayerFrontVec(&mSlideDir);
            mSlideSpeed = hSlidingVelMax;
            setNerve(&NrvSpinningBox::HostTypeNrvSliding::sInstance);
            return true;
        }
    }

    return false;
}

bool SpinningBox::receiveMsgPlayerAttack(u32 msg, HitSensor* pSender, HitSensor* pReceiver) {
    if (MR::isSensorEnemyAttack(pReceiver)) {
        return false;
    }

    if (MR::isMsgPlayerHipDrop(msg)) {
        kill();
        return true;
    }

    if (MR::isMsgPlayerHitAll(msg)) {
        if (isNerve(&NrvSpinningBox::HostTypeNrvFall::sInstance) || MR::isOnPlayer(getSensor("body"))) {
            return false;
        }

        MR::getPlayerFrontVec(&mSlideDir);
        MR::vecKillElement(mSlideDir, mGravity, &mSlideDir);
        MR::normalizeOrZero(&mSlideDir);
        mRotateSpeed = hSpinningRotVelMax;
        mSlideSpeed = hSpinningVelMax;
        MR::startSpinHitSound(this);
        MR::startBlowHitSound(this);
        setNerve(&NrvSpinningBox::HostTypeNrvSpinning::sInstance);
        return true;
    }

    return false;
}

bool SpinningBox::receiveMsgEnemyAttack(u32 msg, HitSensor* pSender, HitSensor* pReceiver) {
    if (MR::isExistInAttributeGroupReflectSpinningBox(pSender->mActor)) {
        if (!isNerve(&NrvSpinningBox::HostTypeNrvSpinning::sInstance) && !isNerve(&NrvSpinningBox::HostTypeNrvSliding::sInstance)) {
            TVec3f dir = mTranslation - pSender->mPosition;
            MR::normalizeOrZero(&dir);
            MR::vecKillElement(pSender->mActor->mVelocity, dir, &dir);
            MR::separateScalarAndDirection(&mSlideSpeed, &mSlideDir, dir);
            mSlideSpeed *= hReflectRate;
            setNerve(&NrvSpinningBox::HostTypeNrvSliding::sInstance);
        } else {
            hitReflection(pSender, pReceiver);
        }
        return true;
    }

    return false;
}

void SpinningBox::hitReflection(HitSensor* pSender, HitSensor* pReceiver) {
    TVec3f dir = pReceiver->mPosition - pSender->mPosition;
    MR::normalizeOrZero(&dir);
    if (MR::isNearZero(dir, 0.001f)) {
        return;
    }

    MR::calcReflectionVector(&mSlideDir, dir, 1.0f, 0.0f);
    mSlideSpeed *= hReflectRate; // Bug Fixed
    MR::normalizeOrZero(&mSlideDir);
}

void SpinningBox::calcAndSetBaseMtx() {
    TPos3f mtx;
    mtx.makeQuat(mRotate);
    mtx.setTrans(mTranslation);
    MR::setBaseTRMtx(this, mtx);
}




namespace MR {
    void addToAttributeGroupReflectSpinningBox(const LiveActor* pActor) {
        if (GroupCheckManager* pGroupChecker = (GroupCheckManager*)MR::getSceneObjHolder()->getObj(SCENE_OBJ_GROUP_CHECK_MANAGER))
            pGroupChecker->add(pActor, 1);
    }

    bool isExistInAttributeGroupReflectSpinningBox(const LiveActor* pActor) {
        if (GroupCheckManager* pGroupChecker = (GroupCheckManager*)MR::getSceneObjHolder()->getObj(SCENE_OBJ_GROUP_CHECK_MANAGER))
            return pGroupChecker->isExist(pActor, 1);
        return false;
    }





    bool isBindedWallOrSlopeGround(const LiveActor* pActor, f32 cos, TVec3f* pNormal) {
        if (isBindedWall(pActor)) {
            if (pNormal != nullptr) {
                const TVec3f* normal = pActor->mBinder->_CC.mParentTriangle.getNormal(0);
                pNormal->set(*normal);
            }

            return true;
        }

        if (isOnGround(pActor)) {
            const TVec3f* normal = pActor->mBinder->_3C.mParentTriangle.getNormal(0);
            if (!MR::isFloorPolygonCos(*normal, pActor->mGravity, cos)) {
                if (pNormal != nullptr) {
                    const TVec3f* groundNormal = pActor->mBinder->_3C.mParentTriangle.getNormal(0);
                    pNormal->set(*groundNormal);
                }

                return true;
            }
        }

        return false;
    }
}
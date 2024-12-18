#ifndef ANIMATOR_HPP
#define ANIMATOR_HPP

#include "animation.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>  // slerp 所需的函式

// 動畫管理器類別
class Animator
{
private:
    std::vector<glm::mat4> finalBoneMatrices; // 最終骨骼矩陣，儲存每個骨骼的最終變換
    Animation* currentAnimation;             // 當前播放的動畫
    Animation* nextAnimation;                // 下一個要過渡的動畫
    Animation* queueAnimation;               // 等待佇列中的動畫
    float currentTime;                       // 當前動畫的時間戳
    bool interpolating;                      // 是否正在進行動畫過渡
    float haltTime;                          // 動畫暫停時間點（過渡用）
    float interTime;                         // 動畫過渡的當前時間

public:
    // 構造函式，初始化變數
    Animator()
    {
        currentTime = 0.0;
        interpolating = false;
        haltTime = 0.0;
        interTime = 0.0;

        currentAnimation = nullptr;
        nextAnimation = nullptr;
        queueAnimation = nullptr;

        finalBoneMatrices.reserve(100); // 預留空間給 100 個骨骼矩陣

        for (int i = 0; i < 100; i++)
            finalBoneMatrices.push_back(glm::mat4(1.0f)); // 預設骨骼矩陣為單位矩陣
    }

    // 更新動畫，每幀呼叫一次
    void updateAnimation(float dt)
    {
        if (currentAnimation) {
            // 更新當前時間，根據動畫速度和時間增量進行更新
            currentTime = fmod(currentTime + currentAnimation->getTicksPerSecond() * dt, currentAnimation->getDuration());
            float transitionTime = currentAnimation->getTicksPerSecond() * 0.2f; // 過渡時間為 0.2 秒

            // 如果正在過渡動畫
            if (interpolating && interTime <= transitionTime) {
                interTime += currentAnimation->getTicksPerSecond() * dt; // 增加過渡時間
                // 計算動畫過渡的骨骼變換
                calculateBoneTransition(currentAnimation->getRootNode(), glm::mat4(1.0f), currentAnimation, nextAnimation, haltTime, interTime, transitionTime);
                return; // 過渡狀態直接返回 不用計算自己的變化, 而是計算當下跟下個動畫間各部位的角度差  
            }
            else if (interpolating) { // 過渡結束 interpolating == ture 但inner time 已經超過過渡時間
                if (queueAnimation) {
                    currentAnimation = nextAnimation; // 將下一個動畫設為當前動畫
                    haltTime = 0.0f;
                    nextAnimation = queueAnimation; // 將等待佇列的動畫設為下一個動畫
                    queueAnimation = nullptr;
                    currentTime = 0.0f;
                    interTime = 0.0;
                    return;
                }

                // 結束過渡狀態
                interpolating = false;
                currentAnimation = nextAnimation;
                currentTime = 0.0;
                interTime = 0.0;
            }

            // 計算骨骼變換()
            calculateBoneTransform(currentAnimation->getRootNode(), glm::mat4(1.0f), currentAnimation, currentTime);
        }
    }

    // 播放指定動畫
    void playAnimation(Animation* pAnimation, bool repeat = true)
    {
        if (!currentAnimation) {
            currentAnimation = pAnimation; // 如果沒有當前動畫，直接設置
            return;
        }

        if (interpolating) {
            // 如果正在過渡動畫，將新動畫加入等待佇列
            if (pAnimation != nextAnimation)
                queueAnimation = pAnimation;
        }
        else {
            // 直接啟動動畫過渡
            if (pAnimation != nextAnimation) {
                interpolating = true;
                haltTime = fmod(currentTime, currentAnimation->getDuration()); // 設置暫停時間點
                nextAnimation = pAnimation; // 設置下一個動畫
                currentTime = 0.0f;
                interTime = 0.0;
            }
        }
    }

    // 計算動畫過渡的骨骼變換
    void calculateBoneTransition(const AssimpNodeData* curNode, glm::mat4 parentTransform, Animation* prevAnimation, Animation* nextAnimation, float haltTime, float currentTime, float transitionTime)
    {
        std::string nodeName = curNode->name;
        glm::mat4 transform = curNode->transformation;

        Bone* prevBone = prevAnimation->findBone(nodeName);
        Bone* nextBone = nextAnimation->findBone(nodeName);

        if (prevBone && nextBone)
        {
            // 獲取前一動畫和下一動畫的骨骼位置、旋轉和縮放
            KeyPosition prevPos = prevBone->getPositions(haltTime);
            KeyRotation prevRot = prevBone->getRotations(haltTime);
            KeyScale prevScl = prevBone->getScalings(haltTime);

            KeyPosition nextPos = nextBone->getPositions(0.0f);
            KeyRotation nextRot = nextBone->getRotations(0.0f);
            KeyScale nextScl = nextBone->getScalings(0.0f);



            prevPos.timeStamp = 0.0f;
            prevRot.timeStamp = 0.0f;
            prevScl.timeStamp = 0.0f;

            nextPos.timeStamp = transitionTime;
            nextRot.timeStamp = transitionTime;
            nextScl.timeStamp = transitionTime;

            // 插值計算骨骼位置、旋轉和縮放
            glm::mat4 p = interpolatePosition(currentTime, prevPos, nextPos);
            glm::mat4 r = interpolateRotation(currentTime, prevRot, nextRot);
            glm::mat4 s = interpolateScaling(currentTime, prevScl, nextScl);

            transform = p * r * s;
        }

        // 計算全局變換矩陣
        glm::mat4 globalTransformation = parentTransform * transform;

        auto boneProps = nextAnimation->getBoneProps();
        for (unsigned int i = 0; i < boneProps.size(); i++) {
            if (boneProps[i].name == nodeName) {
                glm::mat4 offset = boneProps[i].offset;
                finalBoneMatrices[i] = globalTransformation * offset; // 設置最終骨骼矩陣
                break;
            }
        }

        for (int i = 0; i < curNode->childrenCount; i++)
            calculateBoneTransition(&curNode->children[i], globalTransformation, prevAnimation, nextAnimation, haltTime, currentTime, transitionTime);
    }

    // 計算骨骼變換（正常播放）
    void calculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform, Animation* animation, float currentTime)
    {
        std::string nodeName = node->name;
        glm::mat4 boneTransform = node->transformation;

        Bone* bone = animation->findBone(nodeName);

        if (bone)
        {
            bone->update(currentTime); // 更新骨骼狀態
            boneTransform = bone->getTransform(); // 獲取骨骼變換矩陣
        }

        glm::mat4 globalTransformation = parentTransform * boneTransform;

        auto boneProps = animation->getBoneProps();
        for (unsigned int i = 0; i < boneProps.size(); i++) {
            if (boneProps[i].name == nodeName) {
                glm::mat4 offset = boneProps[i].offset;
                finalBoneMatrices[i] = globalTransformation * offset; // 設置最終骨骼矩陣
                break;
            }
        }

        for (int i = 0; i < node->childrenCount; i++)
            calculateBoneTransform(&node->children[i], globalTransformation, animation, currentTime);
    }

    // 獲取最終骨骼矩陣
    std::vector<glm::mat4> getFinalBoneMatrices()
    {
        return finalBoneMatrices;
    }

    Animation* getNextAnimation() {
        return nextAnimation;
    }

    Animation* getCurAnimation() {
        return currentAnimation;
    }

    void calculateBlendedBoneTransform(const AssimpNodeData* curNode, glm::mat4 parentTransform,
        Animation* animA, Animation* animB, float currentTimeA, float currentTimeB, float blendFactor)
    {
        std::string nodeName = curNode->name;

        glm::mat4 transform = curNode->transformation;

        // 初始化骨骼變換
        glm::vec3 interpolatedPosition(0.0f);
        glm::quat interpolatedRotation(1.0, 0.0, 0.0, 0.0);
        glm::vec3 interpolatedScale(1.0f);

        // 查找骨骼
        Bone* boneA = animA->findBone(nodeName);
        Bone* boneB = animB->findBone(nodeName);

        if (boneA && boneB)
        {
            // 獲取骨骼的插值數據
            KeyPosition posA = boneA->getPositions(currentTimeA);
            KeyRotation rotA = boneA->getRotations(currentTimeA);
            KeyScale sclA = boneA->getScalings(currentTimeA);

            KeyPosition posB = boneB->getPositions(currentTimeB);
            KeyRotation rotB = boneB->getRotations(currentTimeB);
            KeyScale sclB = boneB->getScalings(currentTimeB);

            // 混合位置、旋轉和縮放
            interpolatedPosition = glm::mix(posA.position, posB.position, blendFactor);
            // 使用 Assimp 進行四元數插值
            aiQuaternion assimpRotA(rotA.orientation.w, rotA.orientation.x, rotA.orientation.y, rotA.orientation.z);
            aiQuaternion assimpRotB(rotB.orientation.w, rotB.orientation.x, rotB.orientation.y, rotB.orientation.z);
            aiQuaternion interpolatedRotation;
            aiQuaternion::Interpolate(interpolatedRotation, assimpRotA, assimpRotB, blendFactor);

            interpolatedScale = glm::mix(sclA.scale, sclB.scale, blendFactor);

            // 組合混合後的變換矩陣
           // 組合混合後的變換矩陣
            glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), interpolatedPosition);
            glm::mat4 rotationMatrix = glm::toMat4(glm::normalize(glm::quat(interpolatedRotation.w, interpolatedRotation.x, interpolatedRotation.y, interpolatedRotation.z)));
            glm::mat4 scalingMatrix = glm::scale(glm::mat4(1.0f), interpolatedScale);

            transform = translationMatrix * rotationMatrix * scalingMatrix;
        }

        // 計算全局變換矩陣
        glm::mat4 globalTransformation = parentTransform * transform;

        // 更新骨骼最終變換矩陣
        auto boneProps = animA->getBoneProps();
        for (unsigned int i = 0; i < boneProps.size(); i++) {
            if (boneProps[i].name == nodeName) {
                glm::mat4 offset = boneProps[i].offset;
                finalBoneMatrices[i] = globalTransformation * offset;
                break;
            }
        }

        // 遞迴處理子節點
        for (int i = 0; i < curNode->childrenCount; i++)
        {
            calculateBlendedBoneTransform(&curNode->children[i], globalTransformation, animA, animB, currentTimeA, currentTimeB, blendFactor);
        }
    }

    void blendAnimations(float dt, Animation* animA, Animation* animB, float blendFactor)
    {
        if (animA && animB) {
            // 計算兩個動畫各自的時間點
            float currentTimeA = fmod(currentTime, animA->getDuration());
            float currentTimeB = fmod(currentTime, animB->getDuration());

            // 計算骨骼混合變換
            calculateBlendedBoneTransform(animA->getRootNode(), glm::mat4(1.0f), animA, animB, currentTimeA, currentTimeB, blendFactor);

            // 更新時間（基於動畫 A 的時長）
            currentTime += animA->getTicksPerSecond() * dt;
            currentTime = fmod(currentTime, animA->getDuration());
        }
    }

};

#endif

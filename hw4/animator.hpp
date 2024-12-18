#ifndef ANIMATOR_HPP
#define ANIMATOR_HPP

#include "animation.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>  // slerp �һݪ��禡

// �ʵe�޲z�����O
class Animator
{
private:
    std::vector<glm::mat4> finalBoneMatrices; // �̲װ��f�x�}�A�x�s�C�Ӱ��f���̲��ܴ�
    Animation* currentAnimation;             // ��e���񪺰ʵe
    Animation* nextAnimation;                // �U�@�ӭn�L�窺�ʵe
    Animation* queueAnimation;               // ���ݦ�C�����ʵe
    float currentTime;                       // ��e�ʵe���ɶ��W
    bool interpolating;                      // �O�_���b�i��ʵe�L��
    float haltTime;                          // �ʵe�Ȱ��ɶ��I�]�L��Ρ^
    float interTime;                         // �ʵe�L�窺��e�ɶ�

public:
    // �c�y�禡�A��l���ܼ�
    Animator()
    {
        currentTime = 0.0;
        interpolating = false;
        haltTime = 0.0;
        interTime = 0.0;

        currentAnimation = nullptr;
        nextAnimation = nullptr;
        queueAnimation = nullptr;

        finalBoneMatrices.reserve(100); // �w�d�Ŷ��� 100 �Ӱ��f�x�}

        for (int i = 0; i < 100; i++)
            finalBoneMatrices.push_back(glm::mat4(1.0f)); // �w�]���f�x�}�����x�}
    }

    // ��s�ʵe�A�C�V�I�s�@��
    void updateAnimation(float dt)
    {
        if (currentAnimation) {
            // ��s��e�ɶ��A�ھڰʵe�t�שM�ɶ��W�q�i���s
            currentTime = fmod(currentTime + currentAnimation->getTicksPerSecond() * dt, currentAnimation->getDuration());
            float transitionTime = currentAnimation->getTicksPerSecond() * 0.2f; // �L��ɶ��� 0.2 ��

            // �p�G���b�L��ʵe
            if (interpolating && interTime <= transitionTime) {
                interTime += currentAnimation->getTicksPerSecond() * dt; // �W�[�L��ɶ�
                // �p��ʵe�L�窺���f�ܴ�
                calculateBoneTransition(currentAnimation->getRootNode(), glm::mat4(1.0f), currentAnimation, nextAnimation, haltTime, interTime, transitionTime);
                return; // �L�窬�A������^ ���έp��ۤv���ܤ�, �ӬO�p���U��U�Ӱʵe���U���쪺���׮t  
            }
            else if (interpolating) { // �L�絲�� interpolating == ture ��inner time �w�g�W�L�L��ɶ�
                if (queueAnimation) {
                    currentAnimation = nextAnimation; // �N�U�@�Ӱʵe�]����e�ʵe
                    haltTime = 0.0f;
                    nextAnimation = queueAnimation; // �N���ݦ�C���ʵe�]���U�@�Ӱʵe
                    queueAnimation = nullptr;
                    currentTime = 0.0f;
                    interTime = 0.0;
                    return;
                }

                // �����L�窬�A
                interpolating = false;
                currentAnimation = nextAnimation;
                currentTime = 0.0;
                interTime = 0.0;
            }

            // �p�Ⱙ�f�ܴ�()
            calculateBoneTransform(currentAnimation->getRootNode(), glm::mat4(1.0f), currentAnimation, currentTime);
        }
    }

    // ������w�ʵe
    void playAnimation(Animation* pAnimation, bool repeat = true)
    {
        if (!currentAnimation) {
            currentAnimation = pAnimation; // �p�G�S����e�ʵe�A�����]�m
            return;
        }

        if (interpolating) {
            // �p�G���b�L��ʵe�A�N�s�ʵe�[�J���ݦ�C
            if (pAnimation != nextAnimation)
                queueAnimation = pAnimation;
        }
        else {
            // �����Ұʰʵe�L��
            if (pAnimation != nextAnimation) {
                interpolating = true;
                haltTime = fmod(currentTime, currentAnimation->getDuration()); // �]�m�Ȱ��ɶ��I
                nextAnimation = pAnimation; // �]�m�U�@�Ӱʵe
                currentTime = 0.0f;
                interTime = 0.0;
            }
        }
    }

    // �p��ʵe�L�窺���f�ܴ�
    void calculateBoneTransition(const AssimpNodeData* curNode, glm::mat4 parentTransform, Animation* prevAnimation, Animation* nextAnimation, float haltTime, float currentTime, float transitionTime)
    {
        std::string nodeName = curNode->name;
        glm::mat4 transform = curNode->transformation;

        Bone* prevBone = prevAnimation->findBone(nodeName);
        Bone* nextBone = nextAnimation->findBone(nodeName);

        if (prevBone && nextBone)
        {
            // ����e�@�ʵe�M�U�@�ʵe�����f��m�B����M�Y��
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

            // ���ȭp�Ⱙ�f��m�B����M�Y��
            glm::mat4 p = interpolatePosition(currentTime, prevPos, nextPos);
            glm::mat4 r = interpolateRotation(currentTime, prevRot, nextRot);
            glm::mat4 s = interpolateScaling(currentTime, prevScl, nextScl);

            transform = p * r * s;
        }

        // �p������ܴ��x�}
        glm::mat4 globalTransformation = parentTransform * transform;

        auto boneProps = nextAnimation->getBoneProps();
        for (unsigned int i = 0; i < boneProps.size(); i++) {
            if (boneProps[i].name == nodeName) {
                glm::mat4 offset = boneProps[i].offset;
                finalBoneMatrices[i] = globalTransformation * offset; // �]�m�̲װ��f�x�}
                break;
            }
        }

        for (int i = 0; i < curNode->childrenCount; i++)
            calculateBoneTransition(&curNode->children[i], globalTransformation, prevAnimation, nextAnimation, haltTime, currentTime, transitionTime);
    }

    // �p�Ⱙ�f�ܴ��]���`����^
    void calculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform, Animation* animation, float currentTime)
    {
        std::string nodeName = node->name;
        glm::mat4 boneTransform = node->transformation;

        Bone* bone = animation->findBone(nodeName);

        if (bone)
        {
            bone->update(currentTime); // ��s���f���A
            boneTransform = bone->getTransform(); // ������f�ܴ��x�}
        }

        glm::mat4 globalTransformation = parentTransform * boneTransform;

        auto boneProps = animation->getBoneProps();
        for (unsigned int i = 0; i < boneProps.size(); i++) {
            if (boneProps[i].name == nodeName) {
                glm::mat4 offset = boneProps[i].offset;
                finalBoneMatrices[i] = globalTransformation * offset; // �]�m�̲װ��f�x�}
                break;
            }
        }

        for (int i = 0; i < node->childrenCount; i++)
            calculateBoneTransform(&node->children[i], globalTransformation, animation, currentTime);
    }

    // ����̲װ��f�x�}
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

        // ��l�ư��f�ܴ�
        glm::vec3 interpolatedPosition(0.0f);
        glm::quat interpolatedRotation(1.0, 0.0, 0.0, 0.0);
        glm::vec3 interpolatedScale(1.0f);

        // �d�䰩�f
        Bone* boneA = animA->findBone(nodeName);
        Bone* boneB = animB->findBone(nodeName);

        if (boneA && boneB)
        {
            // ������f�����ȼƾ�
            KeyPosition posA = boneA->getPositions(currentTimeA);
            KeyRotation rotA = boneA->getRotations(currentTimeA);
            KeyScale sclA = boneA->getScalings(currentTimeA);

            KeyPosition posB = boneB->getPositions(currentTimeB);
            KeyRotation rotB = boneB->getRotations(currentTimeB);
            KeyScale sclB = boneB->getScalings(currentTimeB);

            // �V�X��m�B����M�Y��
            interpolatedPosition = glm::mix(posA.position, posB.position, blendFactor);
            // �ϥ� Assimp �i��|���ƴ���
            aiQuaternion assimpRotA(rotA.orientation.w, rotA.orientation.x, rotA.orientation.y, rotA.orientation.z);
            aiQuaternion assimpRotB(rotB.orientation.w, rotB.orientation.x, rotB.orientation.y, rotB.orientation.z);
            aiQuaternion interpolatedRotation;
            aiQuaternion::Interpolate(interpolatedRotation, assimpRotA, assimpRotB, blendFactor);

            interpolatedScale = glm::mix(sclA.scale, sclB.scale, blendFactor);

            // �զX�V�X�᪺�ܴ��x�}
           // �զX�V�X�᪺�ܴ��x�}
            glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), interpolatedPosition);
            glm::mat4 rotationMatrix = glm::toMat4(glm::normalize(glm::quat(interpolatedRotation.w, interpolatedRotation.x, interpolatedRotation.y, interpolatedRotation.z)));
            glm::mat4 scalingMatrix = glm::scale(glm::mat4(1.0f), interpolatedScale);

            transform = translationMatrix * rotationMatrix * scalingMatrix;
        }

        // �p������ܴ��x�}
        glm::mat4 globalTransformation = parentTransform * transform;

        // ��s���f�̲��ܴ��x�}
        auto boneProps = animA->getBoneProps();
        for (unsigned int i = 0; i < boneProps.size(); i++) {
            if (boneProps[i].name == nodeName) {
                glm::mat4 offset = boneProps[i].offset;
                finalBoneMatrices[i] = globalTransformation * offset;
                break;
            }
        }

        // ���j�B�z�l�`�I
        for (int i = 0; i < curNode->childrenCount; i++)
        {
            calculateBlendedBoneTransform(&curNode->children[i], globalTransformation, animA, animB, currentTimeA, currentTimeB, blendFactor);
        }
    }

    void blendAnimations(float dt, Animation* animA, Animation* animB, float blendFactor)
    {
        if (animA && animB) {
            // �p���Ӱʵe�U�۪��ɶ��I
            float currentTimeA = fmod(currentTime, animA->getDuration());
            float currentTimeB = fmod(currentTime, animB->getDuration());

            // �p�Ⱙ�f�V�X�ܴ�
            calculateBlendedBoneTransform(animA->getRootNode(), glm::mat4(1.0f), animA, animB, currentTimeA, currentTimeB, blendFactor);

            // ��s�ɶ��]���ʵe A ���ɪ��^
            currentTime += animA->getTicksPerSecond() * dt;
            currentTime = fmod(currentTime, animA->getDuration());
        }
    }

};

#endif

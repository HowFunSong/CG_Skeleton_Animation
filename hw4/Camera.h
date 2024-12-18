
#ifndef CAMERA_H
#define CAMERA_H

// 引入必要的函式庫，GLM 用於矩陣和向量計算
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <iostream>
#include <vector>

// 定義相機移動的方向枚舉，便於抽象處理輸入
enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

// 相機的預設參數，如俯仰角、速度、靈敏度和縮放等
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 10.0f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 45.0f;
const glm::vec3 POSITION = glm::vec3(0.0f, 0.0f, 0.0f);
const glm::vec3 WORLDUP = glm::vec3(0.0f, 1.0f, 0.0f);
const glm::vec3 FRONT = glm::vec3(0.0f, 0.0f, -1.0f);

// 相機類，處理輸入並計算對應的歐拉角、向量和矩陣
class Camera {
public:
    // 相機屬性，包括位置、方向和向量
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    // 歐拉角（水平角和垂直角）
    float Yaw;
    float Pitch;

    // 相機選項，如移動速度和靈敏度
    float MovementSpeed;
    float MouseSensitivity;
    float zoom;

    // 使用向量的構造函式，初始化相機屬性
    Camera(glm::vec3 position = POSITION, glm::vec3 up = WORLDUP, float yaw = YAW, float pitch = PITCH)
        : Front(FRONT), Yaw(yaw), Pitch(pitch), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), zoom(ZOOM)
    {
        Position = position;
        WorldUp = up;
        updateCameraVectors(); // 更新相機向量
    }

    // 返回相機的視圖矩陣，使用 `lookAt` 函式計算
    glm::mat4 GetViewMatrix() {
        return glm::lookAt(Position, Position + Front, Up);
    }

    // 根據輸入移動相機
    void Move(Camera_Movement direction, float deltaTime) {
        float velocity = MovementSpeed * deltaTime;
        if (direction == FORWARD)
            Position += Front * velocity;
        if (direction == BACKWARD)
            Position -= Front * velocity;
        if (direction == LEFT)
            Position -= Right * velocity;
        if (direction == RIGHT)
            Position += Right * velocity;
        if (direction == UP)
            Position += Up * velocity;
        if (direction == DOWN)
            Position -= Up * velocity;
    }

    // 根據鼠標輸入旋轉相機
    void RotateMouse(float xoffset, float yoffset, GLboolean constrainPitch = true) {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;
        Yaw += xoffset;
        Pitch += yoffset;
        if (constrainPitch) {
            if (Pitch > 89.0f) Pitch = 89.0f;
            if (Pitch < -89.0f) Pitch = -89.0f;
        }
        updateCameraVectors(); // 更新相機向量
    }

    // 使用角度繞特定方向旋轉相機（不以弧度表示）
    void RotateRad(Camera_Movement direction, float angle, GLboolean constrainPitch = true) {
        switch (direction) {
        case UP: Pitch -= angle; break;
        case DOWN: Pitch += angle; break;
        case LEFT: Yaw += angle; break;
        case RIGHT: Yaw -= angle; break;
        default: break;
        }
        if (constrainPitch) {
            if (Pitch > 89.0f) Pitch = 89.0f;
            if (Pitch < -89.0f) Pitch = -89.0f;
        }
        updateCameraVectors(); // 更新相機向量
    }

    // 讓相機繞焦點進行軌道旋轉
    void Orbit(Camera_Movement direction, float radius, float angle) {
        // 1. 平移到觀察中心
        Position += Front * radius;

        // 2. 根據給定方向和角度進行旋轉
        RotateRad(direction, angle);

        // 3. 更新相機向量
        updateCameraVectors();

        // 4. 將相機移動回原位，使其繞著焦點旋轉
        Position -= Front * radius;
    }

    // 處理滾輪縮放
    void Zoom(float yoffset) {
        if (zoom >= 1.0f && zoom <= 45.0f)
            zoom -= yoffset;
        if (zoom <= 1.0f)
            zoom = 1.0f;
        if (zoom >= 45.0f)
            zoom = 45.0f;
    }

private:
    // 更新相機的向量，根據歐拉角計算新的方向向量
    void updateCameraVectors() {
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        Right = glm::normalize(glm::cross(Front, WorldUp)); // 計算右向量
        Up = glm::normalize(glm::cross(Right, Front)); // 計算上向量
    }
};

#endif

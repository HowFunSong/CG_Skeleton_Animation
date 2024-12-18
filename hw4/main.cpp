#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include "Camera.h"
#include "shader.hpp"
#include "vaoutils.hpp"
#include "scene.hpp"
#include "mesh.hpp"
#include "model.hpp"
#include "helper.hpp"
#include "animation.hpp"
#include "animator.hpp"
#include <filesystem>
#include <queue>
#include <GL/glut.h>


namespace fs = std::filesystem;
// 回調函數的宣告
void framebuffer_size_callback(GLFWwindow* window, int width, int height); // 處理視窗大小調整
void processInput(GLFWwindow* window, Animation* animations); // 處理鍵盤與滑鼠輸入
void mouse_callback(GLFWwindow* window, double xpos, double ypos); // 處理滑鼠移動
void renderNode(Node* node); // 渲染場景節點
void updateNodeTransformations(Node* node, glm::mat4 transformationThusFar); // 更新節點變換矩陣
void setUniformBoneTransforms(std::vector<glm::mat4> transforms, unsigned int shaderId); // 設定骨骼變換到著色器
static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
//取得根目錄
string getRootPath();
// 全域變數的定義
bool VSYNC = true; // 是否啟用垂直同步
bool FULLSCREEN = false; // 是否啟用全螢幕模式
int WINDOW_WIDTH = 1920; // 視窗寬度
int WINDOW_HEIGHT = 1080; // 視窗高度
int FPS = 999999; // 最大幀率限制

// 攝影機相關參數
glm::vec3 cameraPos = glm::vec3(2.0f, 2.0f, 5.0f); // 攝影機初始位置
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f); // 攝影機前方向
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f); // 攝影機上方向
bool canAdjustCameraPos = true;
bool moveAxisX = true;
bool moveAxisZ = false;


// 滑鼠相關參數
bool firstMouse = true; // 第一次偵測滑鼠移動
float yaw = -90.0f; // 攝影機的 Yaw 角度
float pitch = 0.0f; // 攝影機的 Pitch 角度
float lastX = 800.0f / 2.0; // 滑鼠的最後 X 座標
float lastY = 600.0 / 2.0; // 滑鼠的最後 Y 座標
float fov = 45.0f; // 攝影機視野範圍 (Field of View)// 


// 時間相關參數
float deltaTime = 0.0f; // 每幀的時間差 (用於移動計算)

// 動畫管理器
Animator animator = Animator(); // 用於控制動畫播放的物件

// 場景節點
Node* checkerFloor = createSceneNode(); // 地板的場景節點
Node* character = createSceneNode(); // 角色的場景節點

// 特殊動畫事件
bool stateA = true;
bool StateB = false;
float blendFactor = 0.0f;
Animation* animationA;
Animation* animationB;

int main()
{

	std::string projectRoot = getRootPath();
	std::cout << "Root Directory: " << projectRoot << endl;
	// 初始化 GLFW
	glfwInit();
	// 定義 OpenGL 版本
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4); // 主版本號為 4
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6); // 次版本號為 6
	// 定義使用 Core Profile
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// 啟用多重採樣抗鋸齒 (AA)
	glfwWindowHint(GLFW_SAMPLES, 4);

	// 使用 GLFW 創建視窗
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Project", FULLSCREEN ? glfwGetPrimaryMonitor() : NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	//設定 鍵盤回調函數
	glfwSetKeyCallback(window, KeyCallback);


	// 設置 OpenGL 上下文
	glfwMakeContextCurrent(window);

	// 設定滑鼠回調函數
	glfwSetCursorPosCallback(window, mouse_callback);

	// 設置 OpenGL 的視窗大小與視窗同步
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// 禁用滑鼠游標
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// 驗證 GLAD 是否成功加載 OpenGL，僅在成功後繼續使用 OpenGL 函數
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// 打印系統相關信息
	printInfo();

	// 啟用深度測試
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); // 深度比較函數

	// 啟用面剔除以提高效能
	glEnable(GL_CULL_FACE);

	// 禁用內建的顏色抖動
	glDisable(GL_DITHER);

	// 啟用透明度
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// 如果禁用垂直同步
	if (!VSYNC)
		glfwSwapInterval(0);

	// 加載模型與動畫資源

	std::string daeFile = projectRoot + "resource/vanguard/vanguard.dae";

	std::string animFile1 = projectRoot + "resource/vanguard/Offensive_Idle.dae";
	std::string animFile2 = projectRoot + "resource/vanguard/Running.dae";
	std::string animFile3 = projectRoot + "resource/vanguard/Left_Strafe.dae";
	std::string animFile4 = projectRoot + "resource/vanguard/Right_Strafe.dae";
	std::string animFile5 = projectRoot + "resource/vanguard/Running Backward.dae";
	std::string animFile6 = projectRoot + "resource/vanguard/Jump.dae";
	std::string animFile7 = projectRoot + "resource/vanguard/HipHopDancing.dae";
	std::string animFile8 = projectRoot + "resource/vanguard/Wave_Hip_Hop_Dance.dae";
	std::string animFile9 = projectRoot + "resource/vanguard/Moonwalk.dae";
	std::string animFile10 = projectRoot + "resource/vanguard/Bboy Hip Hop Move.dae";
	std::string animFile11 = projectRoot + "resource/vanguard/Punching.dae";
	std::string animFile12 = projectRoot + "resource/vanguard/Flair.dae";
	std::string animFile13 = projectRoot + "resource/vanguard/Male Dance Pose.dae";
	std::string animFile14 = projectRoot + "resource/vanguard/Idle.dae";

	std::vector<TextureOverride> overrides = {
		{0, DIFFUSE, "textures/vanguard_diffuse1.png"},
		{0, NORMAL, "textures/vanguard_normal.png"},
		{0, SPECULAR, "textures/vanguard_specular.png"},
	};
	// 加載模型
	Model m = Model(daeFile, overrides);
	vector<Mesh> squareMeshes = m.meshes;
	std::cout << "Loaded meshes: " << m.meshes.size() << std::endl;

	// 創建場景節點
	Node* root = createSceneNode();
	root->type = ROOT;

	//// 創建地板網格
	Mesh floorMesh;
	floorMesh.vertices = {
		glm::vec3(-40.0f, 0.0f, -40.0f),
		glm::vec3(-40.0f, 0.0f, 40.0f),
		glm::vec3(40.0f, 0.0f, 40.0f),
		glm::vec3(20.0f, 0.0f, -40.0f),
	};
	floorMesh.normals = {
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
	};
	floorMesh.indices = { 0, 1, 2, 2, 3, 0 };

	unsigned int floorVAO = generateBuffer(floorMesh);

	//// 設置地板節點
	checkerFloor->type = GEOMETRY;
	checkerFloor->vertexArrayObjectIDs = { (int)floorVAO };
	checkerFloor->VAOIndexCounts = { (unsigned int)floorMesh.indices.size() };
	addChild(root, checkerFloor);

	// 配置角色節點
	character->type = CHARACTER;
	character->scale = glm::vec3(0.01, 0.01, 0.01);

	for (int i = 0; i < m.meshes.size(); i++)
	{
		unsigned int charVAO = generateBuffer(squareMeshes[i]);
		character->vertexArrayObjectIDs.push_back(charVAO);
		character->VAOIndexCounts.push_back(squareMeshes[i].indices.size());

		character->textureIDs.push_back(m.diffuseMaps[i]);
		character->normalMapIDs.push_back(m.normalMaps[i]);
		character->specularMapIDs.push_back(m.specularMaps[i]);
	}

	addChild(root, character);

	// 加載動畫
	//Animation anim0(daeFile, &m);
	Animation anim1(animFile1, &m);
	Animation anim2(animFile2, &m);
	Animation anim3(animFile3, &m);
	Animation anim4(animFile4, &m);
	Animation anim5(animFile5, &m);
	Animation anim6(animFile6, &m);
	Animation anim7(animFile7, &m);
	Animation anim8(animFile8, &m);
	Animation anim9(animFile9, &m);
	Animation anim10(animFile10, &m);
	Animation anim11(animFile11, &m);
	Animation anim12(animFile12, &m);
	Animation anim13(animFile13, &m);
	Animation anim14(animFile14, &m);

	//設定漸層動畫
	//
	animationA = &anim14;
	animationB	= &anim2;
	//
	// 
	Animation animations[] = { anim1 , anim2 , anim3 ,
							   anim4 , anim5 , anim6 ,
							   anim7 , anim8 , anim9 ,
		                       anim10, anim11, anim12,
							   anim13, anim14,};
	
	// 加載著色器
	Shader shader = Shader((projectRoot + "src/shaders/default.vert").c_str(),
		(projectRoot + "src/shaders/default.frag").c_str());

	Shader depthShader = Shader((projectRoot + "src/shaders/depth.vert").c_str(),
		(projectRoot + "src/shaders/depth.frag").c_str());

	// 渲染迴圈
	float frameTime = 1.0f / FPS;
	float lastFrame = 0.0f;

	unsigned int depthMap;
	unsigned int depthFBO;

	const unsigned int s_width = 4096, s_height = 4096;

	// 生成深度圖
	generateDepthMap(depthMap, depthFBO, s_width, s_height);

	std::cout << "Starting.." << std::endl;

	while (!glfwWindowShouldClose(window))
	{
		// 計算每幀的時間差
		float now = glfwGetTime();
		if (!VSYNC && now - lastFrame <= frameTime)
		{
			continue;
		}
		deltaTime = now - lastFrame;
		std::cout << "FPS: " << (1.0f / deltaTime) << "\t\r" << std::flush;
		lastFrame = now;

		// 處理輸入並更新動畫
		processInput(window, animations);

		if (stateA) {
			animator.updateAnimation(deltaTime);
		}
		else {
			//animator.updateAnimation(deltaTime);
			animator.blendAnimations(deltaTime, animationA, animationB, blendFactor);
		}

		updateNodeTransformations(root, glm::mat4(1.0));

		auto transforms = animator.getFinalBoneMatrices();

		// ----------------- 投影陰影 ---------------
		glCullFace(GL_FRONT);
		glm::mat4 lightProjection = glm::perspective(glm::radians(fov), (float)s_width / (float)s_height, 0.1f, 100.0f);
		glm::vec3 lightPos = glm::vec3(-10.0f, 10.0f, 20.0f);
		glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
		glm::mat4 lightSpaceMatrix = lightProjection * lightView;

		depthShader.use();

		setUniformBoneTransforms(transforms, depthShader.ID);

		glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, s_width, s_height);
		glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		renderNode(root);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glCullFace(GL_BACK);

		shader.use();

		// ---------------- 陰影處理結束 ------------

		setUniformBoneTransforms(transforms, shader.ID);

		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

		glm::mat4 projection = glm::perspective(glm::radians(fov), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = glm::lookAt(cameraPos, character->position + glm::vec3(0.0f, 1.0f, 0.0f), cameraUp);

		glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(projection));
		glUniform3fv(3, 1, glm::value_ptr(cameraPos));

		glUniformMatrix4fv(5, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, depthMap);

		renderNode(root);

		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);


		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	std::cout << std::endl << "Terminating.." << std::endl;

	// 渲染結束後終止 GLFW
	glfwTerminate();
	return 0;
}


void setUniformBoneTransforms(std::vector<glm::mat4> transforms, unsigned int shaderId) {
	// 將骨骼轉換矩陣設定到著色器中
	for (int i = 0; i < transforms.size(); ++i) {
		string boneStr = "boneTransforms[" + std::to_string(i) + "]"; // 生成骨骼名稱
		int boneLocation = glGetUniformLocation(shaderId, boneStr.c_str()); // 獲取 uniform 位置
		glUniformMatrix4fv(boneLocation, 1, GL_FALSE, glm::value_ptr(transforms[i])); // 設定矩陣
	}
}

void updateNodeTransformations(Node* node, glm::mat4 transformationThusFar) {
	// 計算當前節點的變換矩陣
	glm::mat4 transformationMatrix =
		glm::translate(node->position) *
		glm::translate(node->referencePoint) *
		glm::rotate(node->rotation.y, glm::vec3(0, 1, 0)) *
		glm::rotate(node->rotation.x, glm::vec3(1, 0, 0)) *
		glm::rotate(node->rotation.z, glm::vec3(0, 0, 1)) *
		glm::scale(node->scale) *
		glm::translate(-node->referencePoint);

	node->currentTransformationMatrix = transformationThusFar * transformationMatrix;

	// 遞迴處理子節點的變換
	for (Node* child : node->children) {
		updateNodeTransformations(child, node->currentTransformationMatrix);
	}
}

void renderNode(Node* node) {
	// 設定節點類型到著色器
	glUniform1ui(4, node->type);

	// 根據節點類型進行繪製
	switch (node->type) {
	case CHARACTER:
		// 繪製角色
		for (unsigned int i = 0; i < node->VAOIndexCounts.size(); i++) {
			if (node->vertexArrayObjectIDs[i] != -1) {
				// 綁定材質貼圖
				if (node->textureIDs[i] >= 0) {
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, node->textureIDs[i]);
				}

				if (node->normalMapIDs[i] >= 0) {
					glActiveTexture(GL_TEXTURE1);
					glBindTexture(GL_TEXTURE_2D, node->normalMapIDs[i]);
				}

				if (node->specularMapIDs[i] >= 0) {
					glActiveTexture(GL_TEXTURE2);
					glBindTexture(GL_TEXTURE_2D, node->specularMapIDs[i]);
				}

				// 設定當前節點的變換矩陣
				glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(node->currentTransformationMatrix));
				glBindVertexArray(node->vertexArrayObjectIDs[i]); // 綁定 VAO
				glDrawElements(GL_TRIANGLES, node->VAOIndexCounts[i], GL_UNSIGNED_INT, nullptr); // 繪製三角形
			}
		}
		break;
	case GEOMETRY:
		// 繪製幾何物體
		for (unsigned int i = 0; i < node->VAOIndexCounts.size(); i++) {
			glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(node->currentTransformationMatrix));
			glBindVertexArray(node->vertexArrayObjectIDs[i]);
			glDrawElements(GL_TRIANGLES, node->VAOIndexCounts[i], GL_UNSIGNED_INT, nullptr);
		}
		break;
	}

	// 遞迴繪製子節點
	for (Node* child : node->children) {
		renderNode(child);
	}
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	// 更新視窗大小時，重新設定視口
	glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window, Animation* animations) {
	// 處理使用者輸入
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true); // 按下 ESC 關閉視窗

	if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) {
	
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // 按下 ALT 解鎖滑鼠游標
		canAdjustCameraPos = true;
	
	}
	
	float speed = 4.0f * deltaTime; // 根據時間計算移動速度
	bool isIdle = stateA; // A 
	bool isIdle2 = StateB; // B


	// 處理移動鍵盤輸入
	if (isIdle) {
		//idle0 才有機會切換 , 否則交由callbackfunc處理
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
			character->position.z += 1.2f * speed;
			cameraPos.z += 1.0f * speed;
			animator.playAnimation(&animations[1]); // 播放前進動畫
			isIdle = false;
		}
		else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
			character->position.x += 0.75f * speed;
			cameraPos.x += 0.75f * speed;
			animator.playAnimation(&animations[2]); // 播放左平移動畫
			isIdle = false;

		}
		else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
			character->position.x -= 0.75f * speed;
			cameraPos.x -= 0.75f * speed;
			animator.playAnimation(&animations[3]); // 播放右平移動畫
			isIdle = false;

		}
		else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
			character->position.z -= 0.5f * speed;
			cameraPos.z -= 0.5f * speed;
			animator.playAnimation(&animations[4]); // 播放後退動畫
			isIdle = false;

		}
		else if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {

			character->position.z -= 0.2f * speed;
			cameraPos.z -= 0.2f * speed;
			animator.playAnimation(&animations[5]); // 播放跳躍動畫
			isIdle = false;

		}
		else if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {

			animator.playAnimation(&animations[6]); // 播放其他動畫
			isIdle = false;

		}
		else if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {

			animator.playAnimation(&animations[7]); // 播放其他動畫
			isIdle = false;

		}
		else if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {

			animator.playAnimation(&animations[8]); // 播放其他動畫
			isIdle = false;

		}
		else if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) {

			animator.playAnimation(&animations[9]); // 播放其他動畫
			isIdle = false;

		}
		else if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) {

			animator.playAnimation(&animations[10]); // 播放其他動畫
			isIdle = false;

		}
		else if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {

			animator.playAnimation(&animations[11]); // 播放其他動畫
			isIdle = false;

		}
		else if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
			animator.playAnimation(&animations[12]); // 播放其他動畫
			isIdle = false;

		}
	}

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // 左鍵按下隱藏游標
		canAdjustCameraPos = false;
	}

	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
		moveAxisZ = true;
		moveAxisX = false;
	}
	else if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
		moveAxisZ = false;
		moveAxisX = true;
	}


	// 如果角色靜止，播放待機動畫
	if (isIdle) {
		animator.playAnimation(&animations[0]);
	}

}
void updateCameraPos(float CameraPosXOffset, float CameraPosZOffset ) {
	//// 更新攝影機方向
	//// 根據滑鼠移動更新攝影機方向
	glm::vec3 direction;
	direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction.y = sin(glm::radians(pitch));
	direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraFront = glm::normalize(direction); // 更新攝影機前方向
	//std::cout << "CameraPosXOffset, CameraPosZOffset" << CameraPosXOffset << ", " << CameraPosZOffset << std::endl;
	// 固定攝影機位置，僅更新方向
	
	// 檢查偏移量是否超過閾值，僅在超過閾值時更新攝影機位置
	if (moveAxisX) cameraPos.x += 0.2f * CameraPosXOffset; // 更新 X 軸位置
	if (moveAxisZ) cameraPos.z -= 0.2f * CameraPosZOffset; // 更新 Z 軸位置

}


void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
	if (!canAdjustCameraPos) return;
	// 處理滑鼠輸入
	if (firstMouse) {
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX; // 計算滑鼠的 X 偏移量
	float yoffset = lastY - ypos; // 計算滑鼠的 Y 偏移量
	lastX = xpos;
	lastY = ypos;
	
	float sensitivity = 0.1f; // 滑鼠靈敏度
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;   // 更新視角的 Yaw
	pitch += yoffset; // 更新視角的 Pitch

	// 限制 Pitch 角度範圍
	if (pitch > 90.0f)
		pitch = 90.0f;
	if (pitch < -90.0f)
		pitch = -90.0f;

	updateCameraPos(xoffset, yoffset);
	
}

std::string getRootPath() {
	fs::path currentPath = fs::current_path();

	// 遍歷父目錄直到找到包含 "src" 和 "resource" 資料夾的專案根目錄
	while (currentPath.has_parent_path()) {
		if (fs::exists(currentPath / "src") && fs::exists(currentPath / "resource")) {
			std::string rootPath = currentPath.string();

			// 將路徑中的 "\" 替換為 "/"
			std::replace(rootPath.begin(), rootPath.end(), '\\', '/');

			// 確保路徑以 "/" 結尾
			if (rootPath.back() != '/') {
				rootPath += '/';
			}

			return rootPath; // 返回根目錄路徑
		}
		currentPath = currentPath.parent_path(); // 向上遍歷一層
	}

	throw std::runtime_error("找不到專案根目錄，請確認路徑設置。");
}

//單次調用
static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_F && action == GLFW_PRESS) {
		stateA = !stateA;
		StateB = !StateB;
		
		animator.playAnimation(animationA);
		return;
	}

	if (StateB && key == GLFW_KEY_Y && action == GLFW_PRESS) {
	
		blendFactor += 0.02f;
		if (blendFactor > 1.0f) {
			blendFactor = 1.0f;
		}
		
		return;

	}else if (StateB && key == GLFW_KEY_T && action == GLFW_PRESS) {
	
		blendFactor -= 0.02f;
		if (blendFactor < 0.0f) {
			blendFactor = 0.0f;
		}

		return;
	}
}

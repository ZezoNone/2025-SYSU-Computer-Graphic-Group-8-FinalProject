#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <shader.h>
#include <camera.h>
#include <model.h>
#include "SceneRender.h"
#include "skybox.h"

#include <iostream>
#include <functional>
#include <map>
#include <vector>

// 资源缓存池：Key是文件路径，Value是模型指针
std::map<string, Model*> modelCache;

// 获取模型的函数 (资源管理器)
Model* getModelResource(string path, bool gltf = false)
{
	// 1. 如果缓存里已经有了，直接返回指针
	if (modelCache.find(path) != modelCache.end())
	{
		return modelCache[path];
	}

	// 2. 如果没有，加载新模型，存入缓存，并返回指针
	std::cout << "Loading new model: " << path << std::endl;
	Model* newModel = new Model(path, gltf);
	modelCache[path] = newModel;
	return newModel;
}

// 场景对象：包含变换信息和指向模型资源的指针
struct Object
{
	Model* modelData; // 指向共享的模型资源
	glm::vec3 position;
	glm::vec3 rotation; // 欧拉角
	glm::vec3 scale;

	Object(Model* model, glm::vec3 pos, glm::vec3 rot, glm::vec3 scl)
		: modelData(model), position(pos), rotation(rot), scale(scl) {
	}

	// 计算模型矩阵
	glm::mat4 getModelMatrix() const
	{
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, position);
		model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, scale);
		return model;
	}

	// 正常渲染
	void Draw(Shader& shader)
	{
		glm::mat4 modelMatrix = getModelMatrix();
		shader.setMat4("model", modelMatrix);
		modelData->Draw(shader);
	}

	// 深度/阴影渲染
	void DrawDepth(Shader& shader)
	{
		glm::mat4 modelMatrix = getModelMatrix();
		shader.setMat4("model", modelMatrix);
		// 使用深度模式绘制
		// 这里的 true 表示开启深度绘制优化（不绑定材质）
		modelData->Draw(shader, true);
	}
};

// 场景中的所有物体列表
std::vector<Object> sceneObjects;


// ==========================================
// 控制参数与回调声明
const float TRANS_SPEED = 5.0f;    // 平移速度
const float ROTATE_SPEED = 40.0f;  // 旋转速度
const float SCALE_SPEED = 0.5f;    // 缩放速度
const float MIN_SCALE = 0.001f;	   // 最小缩放

// 针对 Object 的控制函数，用来安放模型
void controlSingleObject(GLFWwindow* window, Object& obj, float deltaTime, bool isSelected = true)
{
	if (!isSelected) return;

	// 直接修改 Object 的公共成员
	glm::vec3& pos = obj.position;
	glm::vec3& rot = obj.rotation;
	glm::vec3& scale = obj.scale;

	// 平移
	if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) pos.z -= TRANS_SPEED * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) pos.z += TRANS_SPEED * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) pos.x -= TRANS_SPEED * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) pos.x += TRANS_SPEED * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) pos.y += TRANS_SPEED * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) pos.y -= TRANS_SPEED * deltaTime;

	// 旋转 (Shift + 方向键 = Z轴, 否则 X/Y)
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
	{
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)  rot.z += ROTATE_SPEED * deltaTime;
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) rot.z -= ROTATE_SPEED * deltaTime;
	}
	else
	{
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)  rot.y -= ROTATE_SPEED * deltaTime;
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) rot.y += ROTATE_SPEED * deltaTime;
		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)    rot.x += ROTATE_SPEED * deltaTime;
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)  rot.x -= ROTATE_SPEED * deltaTime;
	}
	rot = glm::mod(rot, 360.0f);

	// 缩放(O/P x轴缩放； N/M y轴缩放； -/+ z轴缩放)
	if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) scale += glm::vec3(SCALE_SPEED * deltaTime);
	if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS) scale -= glm::vec3(SCALE_SPEED * deltaTime);
	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) scale.x += (SCALE_SPEED * deltaTime);
	if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) scale.x -= (SCALE_SPEED * deltaTime);
	if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) scale.z += (SCALE_SPEED * deltaTime);
	if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) scale.z -= (SCALE_SPEED * deltaTime);
	scale = glm::max(scale, glm::vec3(MIN_SCALE));

	// 重置 (R键)
	static bool rKeyPressed = false;
	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && !rKeyPressed)
	{
		rKeyPressed = true;
		pos = glm::vec3(0.0f); rot = glm::vec3(0.0f); scale = glm::vec3(1.0f);
		std::cout << "对象变换已重置！" << std::endl;
	}
	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_RELEASE) rKeyPressed = false;

	// 调试输出
	static float printTimer = 0.0f;
	printTimer += deltaTime;
	if (printTimer > 1.0f)
	{
		printTimer = 0.0f;
		std::cout << "Pos: " << pos.x << ", " << pos.y << ", " << pos.z
			<< " | Rot: " << rot.x << ", " << rot.y << ", " << rot.z
			<< " | Scale: " << scale.x << ", " << scale.y << ", " << scale.z << std::endl;
	}
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
unsigned int loadTexture(const char* path);
void initPointLights();
void renderAllObjectsToDepth(Shader& depthShader);

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;
const unsigned int SHADOW_WIDTH = 1024;
const unsigned int SHADOW_HEIGHT = 1024;
const unsigned int MAX_POINT_LIGHTS = 4;

// camera
Camera camera(glm::vec3(0.0f, 2.5f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// light
struct PointLight
{
	glm::vec3 position;
	glm::vec3 color;
	float far_plane;
	unsigned int depthCubemap;
	unsigned int depthFBO;
	glm::mat4 shadowMatrices[6];
};
std::vector<PointLight> pointLights;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int main()
{
	// glfw: initialize and configure
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "CG_Group8_FinalProject_V1.0", NULL, NULL);
	glfwMakeContextCurrent(window);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	// 开启混合
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// 加载着色器
	Shader pbrShader("../code/assets/shader/pbr.vs", "../code/assets/shader/pbr.fs");
	Shader depthShader("../code/assets/shader/depth.vs", "../code/assets/shader/depth.fs", "../code/assets/shader/depth.gs");
	Shader equirectangularToCubemapShader("../code/assets/shader/cubemap.vs", "../code/assets/shader/equirectangular_to_cubemap.fs");
	Shader irradianceShader("../code/assets/shader/cubemap.vs", "../code/assets/shader/irradiance.fs");
	Shader prefilterShader("../code/assets/shader/cubemap.vs", "../code/assets/shader/prefilter.fs");
	Shader brdfShader("../code/assets/shader/brdf.vs", "../code/assets/shader/brdf.fs");
	Shader skyboxShader("../code/assets/shader/skybox.vs", "../code/assets/shader/skybox.fs");

	// 着色器参数设置
	pbrShader.use();
	pbrShader.setInt("irradianceMap", 0);
	pbrShader.setInt("prefilterMap", 1);
	pbrShader.setInt("brdfLUT", 2);
	pbrShader.setInt("albedoMap", 3);
	pbrShader.setInt("normalMap", 4);
	pbrShader.setInt("metallicMap", 5);
	pbrShader.setInt("heightMap", 5);
	pbrShader.setInt("metallic_roughnessMap", 5);
	pbrShader.setInt("roughnessMap", 6);
	pbrShader.setInt("aoMap", 7);

	skyboxShader.use();
	skyboxShader.setInt("environmentMap", 0);

	// IBL 设定与生成
	initSkyboxFrameBuffer();
	unsigned int envCubemap = loadCubemap("../code/assets/skybox/modern_evening_street_4k.hdr", equirectangularToCubemapShader);
	unsigned int irradianceMap = createIrradianceMap(envCubemap, irradianceShader);
	unsigned int prefilterMap = createPrefilterMap(envCubemap, prefilterShader);
	unsigned int brdfLUTTexture = createBRDFLUT(brdfShader);

	// Lights
	pointLights.push_back({ glm::vec3(-10.0f,14.0f, -10.0f), glm::vec3(500.0f), 40.0f });
	pointLights.push_back({ glm::vec3(20.0f, 14.0f, -10.0f), glm::vec3(500.0f), 40.0f });
	pointLights.push_back({ glm::vec3(20.0f, 14.0f, 15.0f), glm::vec3(500.0f), 40.0f });
	pointLights.push_back({ glm::vec3(-10.0f, 14.0f, 15.0f), glm::vec3(500.0f), 40.0f });
	initPointLights();

	// =============================================================
	// 模型加载

	// 1. 预加载所有需要的模型资源 (gltf=true)
	Model* resBookShelf = getModelResource("../code/assets/model/book_shelf/scene.gltf", true);
	Model* resBook1 = getModelResource("../code/assets/model/book1/scene.gltf", true);
	Model* resBook2 = getModelResource("../code/assets/model/book2/scene.gltf", true);
	Model* resBooks1 = getModelResource("../code/assets/model/books1/scene.gltf", true);
	Model* resBooks2 = getModelResource("../code/assets/model/books2/scene.gltf", true);
	Model* resBook3 = getModelResource("../code/assets/model/book3/scene.gltf", true);

	Model* resLight = getModelResource("../code/assets/model/ceiling_light/scene.gltf", true);
	Model* resTable = getModelResource("../code/assets/model/table/scene.gltf", true);
	Model* resChair = getModelResource("../code/assets/model/chair/scene.gltf", true);
	Model* resSofa1 = getModelResource("../code/assets/model/sofa/scene.gltf", true);
	Model* resSofa2 = getModelResource("../code/assets/model/sofa2/scene.gltf", true);
	Model* resDoor = getModelResource("../code/assets/model/door/scene.gltf", true);
	Model* resWindow = getModelResource("../code/assets/model/glass_window/scene.gltf", true);

	Model* resBookShelf2 = getModelResource("../code/assets/model/book_shelf2/scene.gltf", true);
	Model* resBookcase1 = getModelResource("../code/assets/model/bookcase/scene.gltf", true);
	Model* resBookcase2 = getModelResource("../code/assets/model/bookcase2/scene.gltf", true);

	Model* resfurniture = getModelResource("../code/assets/model/furniture/scene.gltf", true);
	Model* respicture1 = getModelResource("../code/assets/model/picture1/scene.gltf", true);
	Model* respicture2 = getModelResource("../code/assets/model/picture2/scene.gltf", true);
	Model* resclock = getModelResource("../code/assets/model/clock/scene.gltf", true);
	Model* resplant2 = getModelResource("../code/assets/model/plant2/scene.gltf", true);
	Model* resbookshelf3 = getModelResource("../code/assets/model/bookshelf3/scene.gltf", true);

	// 2. 创建场景物体 (实例化)
	sceneObjects.emplace_back(resBookShelf, glm::vec3(15.27f, -0.005f, -16.40f), glm::vec3(0.0f), glm::vec3(3.95f));
	sceneObjects.emplace_back(resBook1, glm::vec3(18.05f, 7.60f, -20.99f), glm::vec3(90.0f, 0.0f, 0.0f), glm::vec3(0.005f));
	sceneObjects.emplace_back(resBook2, glm::vec3(21.76f, 7.42f, -20.19f), glm::vec3(-90.0f), glm::vec3(0.065f));
	sceneObjects.emplace_back(resBooks1, glm::vec3(17.66f, 0.16f, -20.73f), glm::vec3(90.0f, 270.0f, 90.0f), glm::vec3(0.789f));

	sceneObjects.emplace_back(resLight, glm::vec3(-10.0f, 14.1f, 15.0f), glm::vec3(0.0f), glm::vec3(0.8f));
	sceneObjects.emplace_back(resLight, glm::vec3(-10.0f, 14.1f, -10.0f), glm::vec3(0.0f), glm::vec3(0.8f));
	sceneObjects.emplace_back(resLight, glm::vec3(20.0f, 14.1f, -10.0f), glm::vec3(0.0f), glm::vec3(0.8f));
	sceneObjects.emplace_back(resLight, glm::vec3(20.0f, 14.1f, 15.0f), glm::vec3(0.0f), glm::vec3(0.8f));

	sceneObjects.emplace_back(resTable, glm::vec3(14.0f, 1.0f, -11.0f), glm::vec3(0.0f), glm::vec3(5.0f));
	sceneObjects.emplace_back(resChair, glm::vec3(10.0f, -0.2f, -13.0f), glm::vec3(0.0f), glm::vec3(30.0f));
	sceneObjects.emplace_back(resChair, glm::vec3(10.0f, -0.2f, -8.0f), glm::vec3(0.0f, 30.0f, 0.0f), glm::vec3(30.0f));
	sceneObjects.emplace_back(resChair, glm::vec3(17.0f, -0.2f, -8.0f), glm::vec3(0.0f, 180.0f, 0.0f), glm::vec3(30.0f));
	sceneObjects.emplace_back(resChair, glm::vec3(17.5f, -0.2f, -14.0f), glm::vec3(0.0f, 200.0f, 0.0f), glm::vec3(30.0f));

	sceneObjects.emplace_back(resBooks2, glm::vec3(14.5f, 2.67f, -12.0f), glm::vec3(0.0f), glm::vec3(3.13f));
	sceneObjects.emplace_back(resBook3, glm::vec3(12.47f, 2.63f, -13.71f), glm::vec3(127.5f, 270.3f, 127.7f), glm::vec3(0.447f));

	sceneObjects.emplace_back(resDoor, glm::vec3(-14.94f, -0.01f, 27.06f), glm::vec3(0.0f), glm::vec3(0.053f, 0.036f, 0.020f));

	sceneObjects.emplace_back(resBookShelf2, glm::vec3(27.00f, -0.004f, -13.48f), glm::vec3(180.0f, 270.0f, 180.0f), glm::vec3(2.00f));
	sceneObjects.emplace_back(resBookShelf2, glm::vec3(27.00f, -0.004f, -4.48f), glm::vec3(180.0f, 270.0f, 180.0f), glm::vec3(2.00f));

	sceneObjects.emplace_back(resBookcase1, glm::vec3(24.00f, -0.004f, 4.48f), glm::vec3(180.0f, 141.9f, 180.0f), glm::vec3(3.43f));
	sceneObjects.emplace_back(resBookcase1, glm::vec3(24.00f, -0.004f, 17.48f), glm::vec3(180.0f, 230.0f, 180.0f), glm::vec3(3.43f));
	sceneObjects.emplace_back(resBookcase2, glm::vec3(25.87f, -0.008f, 10.48f), glm::vec3(180.0f, 0.0f, 180.0f), glm::vec3(3.38f));

	sceneObjects.emplace_back(resSofa2, glm::vec3(16.87f, 1.48f, 10.48f), glm::vec3(180.0f, 90.0f, 180.0f), glm::vec3(5.55f));
	sceneObjects.emplace_back(resSofa2, glm::vec3(12.87f, 1.48f, 10.48f), glm::vec3(180.0f, 270.0f, 180.0f), glm::vec3(5.55f));

	// --- Set 1 ---
	float set1_x = -15.0f;
	float set1_z = 0.0f;
	sceneObjects.emplace_back(resBookShelf, glm::vec3(15.27f + set1_x, -0.005f, -16.40f + set1_z), glm::vec3(0.0f), glm::vec3(3.95f));
	sceneObjects.emplace_back(resBook1, glm::vec3(18.05f + set1_x, 7.60f, -20.99f + set1_z), glm::vec3(90.0f, 0.0f, 0.0f), glm::vec3(0.005f));
	sceneObjects.emplace_back(resBook2, glm::vec3(21.76f + set1_x, 7.42f, -20.19f + set1_z), glm::vec3(-90.0f), glm::vec3(0.065f));
	sceneObjects.emplace_back(resBooks1, glm::vec3(17.66f + set1_x, 0.16f, -20.73f + set1_z), glm::vec3(90.0f, 270.0f, 90.0f), glm::vec3(0.789f));

	// --- Set 2 ---
	float set2_x = -20.0f;
	float set2_z = 20.0f;
	sceneObjects.emplace_back(resTable, glm::vec3(14.0f + set2_x, 1.0f, -11.0f + set2_z), glm::vec3(0.0f, 90.0f, 0.0f), glm::vec3(5.0f));
	sceneObjects.emplace_back(resChair, glm::vec3(12.0f + set2_x, -0.2f, -13.0f + set2_z), glm::vec3(0.0f, 270.0f, 0.0f), glm::vec3(30.0f));
	sceneObjects.emplace_back(resChair, glm::vec3(10.0f + set2_x, -0.2f, -8.0f + set2_z), glm::vec3(0.0f, 30.0f, 0.0f), glm::vec3(30.0f));
	sceneObjects.emplace_back(resChair, glm::vec3(17.0f + set2_x, -0.2f, -8.0f + set2_z), glm::vec3(0.0f, 90.0f, 0.0f), glm::vec3(30.0f));
	sceneObjects.emplace_back(resChair, glm::vec3(16.5f + set2_x, -0.2f, -14.0f + set2_z), glm::vec3(0.0f, 230.0f, 0.0f), glm::vec3(30.0f));
	sceneObjects.emplace_back(resBooks2, glm::vec3(14.5f + set2_x, 2.67f, -12.0f + set2_z), glm::vec3(0.0f), glm::vec3(3.13f));
	sceneObjects.emplace_back(resBook3, glm::vec3(12.47f + set2_x, 2.63f, -11.71f + set2_z), glm::vec3(127.5f, 270.3f, 127.7f), glm::vec3(0.447f));

	// --- Set 3 ---
	float set3_x = -20.0f;
	float set3_z = 0.0f;
	sceneObjects.emplace_back(resBookcase1, glm::vec3(24.00f + set3_x, -0.004f, 4.48f + set3_z), glm::vec3(180.0f, 321.9f, 180.0f), glm::vec3(3.43f));
	sceneObjects.emplace_back(resBookcase1, glm::vec3(24.00f + set3_x, -0.004f, 17.48f + set3_z), glm::vec3(180.0f, 50.0f, 180.0f), glm::vec3(3.43f));
	sceneObjects.emplace_back(resBookcase2, glm::vec3(25.87f + set3_x, -0.008f, 10.48f + set3_z), glm::vec3(180.0f, 180.0f, 180.0f), glm::vec3(3.38f));

	// --- Set 4 ---
	float set4_x = -20.0f;
	float set4_z = 10.0f;
	sceneObjects.emplace_back(resTable, glm::vec3(14.0f + set4_x, 1.0f, -11.0f + set4_z), glm::vec3(0.0f, 90.0f, 0.0f), glm::vec3(5.0f));
	sceneObjects.emplace_back(resChair, glm::vec3(12.0f + set4_x, -0.2f, -13.0f + set4_z), glm::vec3(0.0f, 270.0f, 0.0f), glm::vec3(30.0f));
	sceneObjects.emplace_back(resChair, glm::vec3(10.0f + set4_x, -0.2f, -8.0f + set4_z), glm::vec3(0.0f, 30.0f, 0.0f), glm::vec3(30.0f));
	sceneObjects.emplace_back(resChair, glm::vec3(17.0f + set4_x, -0.2f, -8.0f + set4_z), glm::vec3(0.0f, 90.0f, 0.0f), glm::vec3(30.0f));
	sceneObjects.emplace_back(resChair, glm::vec3(16.5f + set4_x, -0.2f, -14.0f + set4_z), glm::vec3(0.0f, 230.0f, 0.0f), glm::vec3(30.0f));
	sceneObjects.emplace_back(resBooks2, glm::vec3(14.5f + set4_x, 2.67f, -12.0f + set4_z), glm::vec3(0.0f), glm::vec3(3.13f));
	sceneObjects.emplace_back(resBook3, glm::vec3(12.47f + set4_x, 2.63f, -11.71f + set4_z), glm::vec3(127.5f, 270.3f, 127.7f), glm::vec3(0.447f));

	// --- Set 5 ---
	float set5_x = -20.0f;
	float set5_z = 0.0f;
	sceneObjects.emplace_back(resTable, glm::vec3(14.0f + set5_x, 1.0f, -11.0f + set5_z), glm::vec3(0.0f, 90.0f, 0.0f), glm::vec3(5.0f));
	sceneObjects.emplace_back(resChair, glm::vec3(12.0f + set5_x, -0.2f, -13.0f + set5_z), glm::vec3(0.0f, 270.0f, 0.0f), glm::vec3(30.0f));
	sceneObjects.emplace_back(resChair, glm::vec3(10.0f + set5_x, -0.2f, -8.0f + set5_z), glm::vec3(0.0f, 30.0f, 0.0f), glm::vec3(30.0f));
	sceneObjects.emplace_back(resChair, glm::vec3(17.0f + set5_x, -0.2f, -8.0f + set5_z), glm::vec3(0.0f, 90.0f, 0.0f), glm::vec3(30.0f));
	sceneObjects.emplace_back(resChair, glm::vec3(16.5f + set5_x, -0.2f, -14.0f + set5_z), glm::vec3(0.0f, 230.0f, 0.0f), glm::vec3(30.0f));
	sceneObjects.emplace_back(resBooks2, glm::vec3(14.5f + set5_x, 2.67f, -12.0f + set5_z), glm::vec3(0.0f), glm::vec3(3.13f));
	sceneObjects.emplace_back(resBook3, glm::vec3(11.47f + set5_x, 2.63f, -12.71f + set5_z), glm::vec3(127.5f, 270.3f, 127.7f), glm::vec3(0.447f));

	sceneObjects.emplace_back(resfurniture, glm::vec3(15.4316, 0.0038861, 26.7556), glm::vec3(0.0f, 180.0f, 0.0f), glm::vec3(1.16696, 2.06043, 2.0505));
	sceneObjects.emplace_back(resfurniture, glm::vec3(10.4316, 0.0038861, 26.7556), glm::vec3(0.0f, 180.0f, 0.0f), glm::vec3(1.16696, 2.06043, 2.0505));
	sceneObjects.emplace_back(resfurniture, glm::vec3(26.8158, -0.690885, 20.8633), glm::vec3(0.0f, 270.0f, 0.0f), glm::vec3(1.16696, 2.06043, 2.0505));

	sceneObjects.emplace_back(resplant2, glm::vec3(25.6434, -0.0194125, 25.421), glm::vec3(0.0f, 270.0f, 0.0f), glm::vec3(3.0661, 3.95957, 3.94964));
	sceneObjects.emplace_back(resplant2, glm::vec3(5.6434, -0.0445704, 25.0621), glm::vec3(0.0f, 270.0f, 0.0f), glm::vec3(3.0661, 3.95957, 3.94964));
	sceneObjects.emplace_back(resplant2, glm::vec3(12.2396, -0.0445704, -20.3727), glm::vec3(0.0f, 270.0f, 0.0f), glm::vec3(3.0661, 3.95957, 3.94964));

	sceneObjects.emplace_back(resSofa1, glm::vec3(-10.82f, 1.65f, -19.62f), glm::vec3(0.0f), glm::vec3(5.90f));
	sceneObjects.emplace_back(resSofa1, glm::vec3(-19.3706, 1.65, -9.32448), glm::vec3(0.0f, 90.0f, 0.0f), glm::vec3(5.90f));

	sceneObjects.emplace_back(resclock, glm::vec3(-19.0088, 7.99993, -21.2711), glm::vec3(0.0f, 180.0f, 0.0f), glm::vec3(2.38271));

	sceneObjects.emplace_back(resbookshelf3, glm::vec3(-21.739, 0.0430055, 18.7215), glm::vec3(0.0f, 90.0f, 0.0f), glm::vec3(24.8179));
	sceneObjects.emplace_back(resbookshelf3, glm::vec3(-21.739, 0.0430055, 10.7215), glm::vec3(0.0f, 90.0f, 0.0f), glm::vec3(24.8179));

	sceneObjects.emplace_back(respicture2, glm::vec3(-21.9017, 8.53611, 10.7215), glm::vec3(0.0f, 90.0f, 0.0f), glm::vec3(0.086184));
	sceneObjects.emplace_back(respicture1, glm::vec3(-21.9169, 5.02535, 12.8817), glm::vec3(0.0f, 90.0f, 0.0f), glm::vec3(3.38485));
	sceneObjects.emplace_back(respicture1, glm::vec3(-0.206604, 3.14911, -22.0161), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(3.40435));

	sceneObjects.emplace_back(resWindow, glm::vec3(19.99f, 4.38f, 27.48f), glm::vec3(0.0f), glm::vec3(0.86f, 2.63f, 2.73f));
	sceneObjects.emplace_back(resWindow, glm::vec3(0.009f, 4.35f, 27.48f), glm::vec3(0.0f), glm::vec3(0.87f, 2.63f, 3.93f));

	// --------------------------
	// 纹理加载

	unsigned int floorAlbedo = loadTexture("../code/assets/texture/floor/basecolor.png");
	unsigned int floorNormal = loadTexture("../code/assets/texture/floor/normal.png");
	unsigned int floorheight = loadTexture("../code/assets/texture/floor/height.png");
	unsigned int floorRoughness = loadTexture("../code/assets/texture/floor/roughness.png");
	unsigned int floorAO = loadTexture("../code/assets/texture/floor/ao.png");

	unsigned int marblealbedo = loadTexture("../code/assets/texture/marble/basecolor.png");
	unsigned int marblenormal = loadTexture("../code/assets/texture/marble/normal.png");
	unsigned int marbleheight = loadTexture("../code/assets/texture/marble/height.png");
	unsigned int marbleroughness = loadTexture("../code/assets/texture/marble/roughness.png");

	unsigned int wallalbedo = loadTexture("../code/assets/texture/wall/basecolor.png");
	unsigned int wallnormal = loadTexture("../code/assets/texture/wall/normal.png");
	unsigned int wallheight = loadTexture("../code/assets/texture/wall/height.png");
	unsigned int wallroughness = loadTexture("../code/assets/texture/wall/roughness.png");
	unsigned int wallao = loadTexture("../code/assets/texture/wall/ao.png");

	unsigned int ceilingalbedo = loadTexture("../code/assets/texture/ceiling/basecolor.jpg");
	unsigned int ceilingnormal = loadTexture("../code/assets/texture/ceiling/normal.jpg");
	unsigned int ceilingheight = loadTexture("../code/assets/texture/ceiling/height.png");
	unsigned int ceilingroughness = loadTexture("../code/assets/texture/ceiling/roughness.jpg");
	unsigned int ceilingao = loadTexture("../code/assets/texture/ceiling/ao.jpg");

	unsigned int tilesalbedo = loadTexture("../code/assets/texture/tiles/basecolor.jpg");
	unsigned int tilesnormal = loadTexture("../code/assets/texture/tiles/normal.jpg");
	unsigned int tilesheight = loadTexture("../code/assets/texture/tiles/height.png");
	unsigned int tilesroughness = loadTexture("../code/assets/texture/tiles/roughness.jpg");
	unsigned int tilesao = loadTexture("../code/assets/texture/tiles/ao.jpg");

	// Projection
	glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
	pbrShader.use();
	pbrShader.setMat4("projection", projection);
	skyboxShader.use();
	skyboxShader.setMat4("projection", projection);

	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window);

		// 清空
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// 1. 阴影渲染
		renderAllObjectsToDepth(depthShader);

		// 2. PBR 主渲染
		pbrShader.use();
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();
		pbrShader.setMat4("view", view);
		pbrShader.setMat4("projection", projection);
		pbrShader.setVec3("camPos", camera.Position);

		// 绑定 IBL 贴图
		glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
		glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
		glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);

		// 绑定灯光与阴影贴图
		int validLightCount = min((int)pointLights.size(), 4);
		pbrShader.setInt("pointLightCount", validLightCount);
		for (int i = 0; i < validLightCount; ++i)
		{
			std::string iStr = std::to_string(i);
			pbrShader.setInt(("pointLightDepthCubemaps[" + iStr + "]").c_str(), 8 + i);
			pbrShader.setVec3(("pointLightsBase[" + iStr + "].position").c_str(), pointLights[i].position);
			pbrShader.setVec3(("pointLightsBase[" + iStr + "].color").c_str(), pointLights[i].color);
			pbrShader.setFloat(("pointLightsBase[" + iStr + "].far_plane").c_str(), pointLights[i].far_plane);

			glActiveTexture(GL_TEXTURE8 + i);
			glBindTexture(GL_TEXTURE_CUBE_MAP, pointLights[i].depthCubemap);
		}

		// --- 渲染地面 / 墙壁 / 天花板 ---
		pbrShader.setInt("albedoMap", 3);
		pbrShader.setInt("normalMap", 4);
		pbrShader.setInt("metallicMap", 5);
		pbrShader.setInt("roughnessMap", 6);
		pbrShader.setInt("aoMap", 7);
		pbrShader.setBool("gltf", false);
		pbrShader.setBool("useAlbedoMap", true);
		pbrShader.setBool("useNormalMap", true);
		pbrShader.setBool("useAOMap", false);
		pbrShader.setBool("useMetallicRoughnessMap", false);
		pbrShader.setVec4("materialBaseColor", glm::vec4(1.0f));
		pbrShader.setFloat("materialRoughness", 1.0f);
		pbrShader.setFloat("materialMetallic", 0.1f);

		int length = 10, width = 10, height = 3;
		float l_spacing = 5.0f, w_spacing = 5.0f, h_spacing = 5.0f;
		float h = 0.0f, l = -20.0, w = -20.0f;

		// 1. 地面 (Marble)
		glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, marblealbedo);
		glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, marblenormal);
		glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, marbleheight);
		glActiveTexture(GL_TEXTURE6); glBindTexture(GL_TEXTURE_2D, marbleroughness);
		pbrShader.setBool("usePOM", true);
		pbrShader.setFloat("texScale", 1.0f); // 调整平铺
		pbrShader.setFloat("heightScale", 0.002f);
		w = -200.0f;
		l = -200.0f;
		for (int i = 0; i < 4; i++)
		{
			l = -100.0f;
			for (int j = 0; j < 4; j++)
			{
				setModelMatrix(glm::vec3(w, -0.1, l), glm::vec3(100.0f, 3.0f, 100.0f), glm::vec3(0.0f), pbrShader);
				renderGround();
				l += 100.0f;
			}
			w += 100.0f;
		}

		pbrShader.setBool("useAOMap", true);
		// 2. 地板 (floor)
		glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, floorAlbedo);
		glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, floorNormal);
		glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, floorheight);
		glActiveTexture(GL_TEXTURE6); glBindTexture(GL_TEXTURE_2D, floorRoughness);
		glActiveTexture(GL_TEXTURE7); glBindTexture(GL_TEXTURE_2D, floorAO);

		pbrShader.setFloat("texScale", 1.0f); // 调整平铺
		pbrShader.setFloat("heightScale", 0.002f);

		w = -20.0f;
		for (int i = 0; i < width; i++)
		{
			l = -20.0f;
			for (int j = 0; j < length; j++)
			{
				setModelMatrix(glm::vec3(w, h, l), glm::vec3(5.0f, 3.0f, 5.0f), glm::vec3(0.0f), pbrShader);
				renderGround();
				l += l_spacing;
			}
			w += w_spacing;
		}

		// 3. 墙壁 (Tiles)
		glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, tilesalbedo);
		glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, tilesnormal);
		glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, tilesheight);
		glActiveTexture(GL_TEXTURE6); glBindTexture(GL_TEXTURE_2D, tilesroughness);
		glActiveTexture(GL_TEXTURE7); glBindTexture(GL_TEXTURE_2D, tilesao);
		pbrShader.setFloat("texScale", 1.0f);

		// 墙壁循环逻辑

		h = 2.0f;
		for (int i = 0; i < height; i++)
		{
			float l = -20.0f;
			for (int j = 0; j < length; j++)
			{
				// 左墙
				setModelMatrix(glm::vec3(27.0f, h, l), glm::vec3(5.0f, 3.0f, 5.0f), glm::vec3(90.0f, 0.0f, 90.0f), pbrShader);
				renderWall();
				// 右墙
				setModelMatrix(glm::vec3(-22.5f, h, l), glm::vec3(5.0f, 3.0f, 5.0f), glm::vec3(90.0f, 0.0f, 90.0f), pbrShader);
				renderWall();
				l += l_spacing;
			}
			h += h_spacing;
		}
		// 后墙
		h = 2.0f;
		for (int i = 0; i < height; i++)
		{
			l = -20.0f;
			for (int j = 0; j < length; j++)
			{
				setModelMatrix(glm::vec3(l, h, -22.0), glm::vec3(5.0f, 3.0f, 5.0f), glm::vec3(90.0f, 0.0f, 0.0f), pbrShader);
				renderWall();
				l += l_spacing;
			}
			h += h_spacing;
		}
		// 前墙 (带门窗孔)
		l = -20.0f;
		for (int i = 0; i < length; i++)
		{
			if (i == 1)
			{ // 门
				setModelMatrix(glm::vec3(l, 10.9, 27.5), glm::vec3(5.0f, 3.0f, 7.0f), glm::vec3(90.0f, 0.0f, 0.0f), pbrShader);
				renderWall();
			}
			else if (i == 4 || i == 8)
			{ // 窗
				float local_h = 2.0f;
				for (int j = 0; j < 2; j++)
				{
					setModelMatrix(glm::vec3(l, local_h, 27.5), glm::vec3(5.0f, 3.0f, 5.0f), glm::vec3(90.0f, 0.0f, 0.0f), pbrShader);
					renderWall();
					local_h += 2 * h_spacing;
				}
			}
			else
			{ // 实心
				float local_h = 2.0f;
				for (int j = 0; j < height; j++)
				{
					setModelMatrix(glm::vec3(l, local_h, 27.5), glm::vec3(5.0f, 3.0f, 5.0f), glm::vec3(90.0f, 0.0f, 0.0f), pbrShader);
					renderWall();
					local_h += h_spacing;
				}
			}
			l += l_spacing;
		}

		// 4. 天花板
		glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, ceilingalbedo);
		glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, ceilingnormal);
		glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, ceilingheight);
		glActiveTexture(GL_TEXTURE6); glBindTexture(GL_TEXTURE_2D, ceilingroughness);
		glActiveTexture(GL_TEXTURE7); glBindTexture(GL_TEXTURE_2D, ceilingao);
		pbrShader.setBool("usePOM", false); // 天花板太远，无需POM

		h = 15.0f;
		w = -20.0f;
		for (int i = 0; i < width; i++)
		{
			float l = -20.0f;
			for (int j = 0; j < length; j++)
			{
				setModelMatrix(glm::vec3(w, h, l), glm::vec3(5.0f, 3.0f, 5.0f), glm::vec3(0.0f), pbrShader);
				renderGround();
				l += l_spacing;
			}
			w += w_spacing;
		}


		//if (!sceneObjects.empty()) {
		//    controlSingleObject(window, sceneObjects.back(), deltaTime); // 控制最后一个添加的物体
		//}
		// --- 渲染场景物体 ---
		for (auto& obj : sceneObjects) {
			obj.Draw(pbrShader);
		}

		// --- 天空盒 ---
		skyboxShader.use();
		skyboxShader.setMat4("view", view);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
		renderCube();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// 资源清理
	for (auto& light : pointLights) {
		glDeleteTextures(1, &light.depthCubemap);
		glDeleteFramebuffers(1, &light.depthFBO);
	}
	glDeleteTextures(1, &envCubemap);
	glDeleteTextures(1, &irradianceMap);
	glDeleteTextures(1, &prefilterMap);
	glDeleteTextures(1, &brdfLUTTexture);

	// 清理模型资源
	modelCache.clear();

	glfwTerminate();
	return 0;
}

// =======================================================
// 工具函数实现
void renderAllObjectsToDepth(Shader& depthShader)
{
	depthShader.use();
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);

	// 开启正面剔除以修复阴影痤疮
	glCullFace(GL_FRONT);

	for (auto& light : pointLights)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, light.depthFBO);
		glClear(GL_DEPTH_BUFFER_BIT);

		for (unsigned int i = 0; i < 6; ++i) {
			depthShader.setMat4(("shadowMatrices[" + std::to_string(i) + "]").c_str(), light.shadowMatrices[i]);
		}
		depthShader.setVec3("lightPos", light.position);
		depthShader.setFloat("far_plane", light.far_plane);


		int length = 10, width = 10, height = 3;;
		float l_spacing = 5.0f, w_spacing = 5.0f, h_spacing = 5.0f;
		float h = 2.0f, l = -20.0f;

		for (int i = 0; i < height; i++)
		{
			l = -20.0f;
			for (int j = 0; j < length; j++)
			{
				// 左墙
				setModelMatrix(glm::vec3(27.0f, h, l), glm::vec3(5.0f, 3.0f, 5.0f), glm::vec3(90.0f, 0.0f, 90.0f), depthShader);
				renderWall();
				// 右墙
				setModelMatrix(glm::vec3(-22.5f, h, l), glm::vec3(5.0f, 3.0f, 5.0f), glm::vec3(90.0f, 0.0f, 90.0f), depthShader);
				renderWall();
				l += l_spacing;
			}
			h += h_spacing;
		}
		// 后墙
		h = 2.0f;
		for (int i = 0; i < height; i++)
		{
			l = -20.0f;
			for (int j = 0; j < length; j++)
			{
				setModelMatrix(glm::vec3(l, h, -22.0), glm::vec3(5.0f, 3.0f, 5.0f), glm::vec3(90.0f, 0.0f, 0.0f), depthShader);
				renderWall();
				l += l_spacing;
			}
			h += h_spacing;
		}
		// 前墙 (带门窗孔)
		l = -20.0f;
		for (int i = 0; i < length; i++)
		{
			if (i == 1)
			{ // 门
				setModelMatrix(glm::vec3(l, 10.9, 27.5), glm::vec3(5.0f, 3.0f, 7.0f), glm::vec3(90.0f, 0.0f, 0.0f), depthShader);
				renderWall();
			}
			else if (i == 4 || i == 8)
			{ // 窗
				float local_h = 2.0f;
				for (int j = 0; j < 2; j++)
				{
					setModelMatrix(glm::vec3(l, local_h, 27.5), glm::vec3(5.0f, 3.0f, 5.0f), glm::vec3(90.0f, 0.0f, 0.0f), depthShader);
					renderWall();
					local_h += 2 * h_spacing;
				}
			}
			else { // 实心
				float local_h = 2.0f;
				for (int j = 0; j < height; j++)
				{
					setModelMatrix(glm::vec3(l, local_h, 27.5), glm::vec3(5.0f, 3.0f, 5.0f), glm::vec3(90.0f, 0.0f, 0.0f), depthShader);
					renderWall();
					local_h += h_spacing;
				}
			}
			l += l_spacing;
		}

		// 地板
		float w = -20.0f;
		for (int i = 0; i < width; i++)
		{
			float l = -20.0f;
			for (int j = 0; j < length; j++)
			{
				setModelMatrix(glm::vec3(w, 0.0f, l), glm::vec3(5.0f, 3.0f, 5.0f), glm::vec3(0.0f), depthShader);
				renderGround();
				l += l_spacing;
			}
			w += w_spacing;
		}
		// 地面
		w = -200.0f;
		l = -200.0f;
		for (int i = 0; i < 4; i++)
		{
			l = -100.0f;
			for (int j = 0; j < 4; j++)
			{
				setModelMatrix(glm::vec3(w, -0.1, l), glm::vec3(100.0f, 3.0f, 100.0f), glm::vec3(0.0f), depthShader);
				renderGround();
				l += 100.0f;
			}
			w += 100.0f;
		}

		// 天花板
		w = -20.0f;
		for (int i = 0; i < width; i++)
		{
			l = -20.0f;
			for (int j = 0; j < length; j++)
			{
				setModelMatrix(glm::vec3(w, 15.0f, l), glm::vec3(5.0f, 3.0f, 5.0f), glm::vec3(0.0f), depthShader);
				renderGround();
				l += l_spacing;
			}
			w += w_spacing;
		}

		// 2. 渲染场景对象
		for (auto& obj : sceneObjects)
		{
			obj.DrawDepth(depthShader);
		}
	}

	glCullFace(GL_BACK); // 恢复背面剔除
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
}

void initPointLights()
{
	for (auto& light : pointLights)
	{
		// 创建深度 FBO
		glGenFramebuffers(1, &light.depthFBO);
		// 创建立方体深度纹理
		glGenTextures(1, &light.depthCubemap);
		glBindTexture(GL_TEXTURE_CUBE_MAP, light.depthCubemap);
		for (unsigned int i = 0; i < 6; ++i)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT32F,
				SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		glBindFramebuffer(GL_FRAMEBUFFER, light.depthFBO);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, light.depthCubemap, 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			std::cerr << "ERROR::FRAMEBUFFER:: Point light FBO incomplete! Error code: "
				<< glCheckFramebufferStatus(GL_FRAMEBUFFER) << std::endl;
			exit(-1);
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0); // 解绑 FBO，确保默认 FBO 激活

		// 计算阴影矩阵
		glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)SHADOW_WIDTH / SHADOW_HEIGHT, 0.1f, light.far_plane);
		light.shadowMatrices[0] = shadowProj * glm::lookAt(light.position, light.position + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
		light.shadowMatrices[1] = shadowProj * glm::lookAt(light.position, light.position + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
		light.shadowMatrices[2] = shadowProj * glm::lookAt(light.position, light.position + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		light.shadowMatrices[3] = shadowProj * glm::lookAt(light.position, light.position + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
		light.shadowMatrices[4] = shadowProj * glm::lookAt(light.position, light.position + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
		light.shadowMatrices[5] = shadowProj * glm::lookAt(light.position, light.position + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

unsigned int loadTexture(char const* path)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	int width, height, nrComponents;
	unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		camera.ProcessKeyboard(UP, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		camera.ProcessKeyboard(DOWN, deltaTime);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}
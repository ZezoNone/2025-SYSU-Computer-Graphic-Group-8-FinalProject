#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

void BuildMatrix();
void setModelMatrix(glm::vec3 position, glm::vec3 scale, glm::vec3 rotation, Shader& shader);
pair<glm::mat4, glm::mat3> getModelMatrix(glm::vec3 position, glm::vec3 scale, glm::vec3 rotation);
void renderGround(const std::vector<glm::mat4>& modelMatrices, const std::vector<glm::mat3>& NormalMatrices, bool depth = false);
void renderWall(const std::vector<glm::mat4>& modelMatrices, const std::vector<glm::mat3>& NormalMatrices, bool depth = false);
void renderSphere();
void renderCube();
void renderQuad();

std::vector<glm::mat4> GmodelMatrices;
std::vector<glm::mat3> GNormalMatrices;
std::vector<glm::mat4> FmodelMatrices;
std::vector<glm::mat3> FNormalMatrices;
std::vector<glm::mat4> WmodelMatrices;
std::vector<glm::mat3> WNormalMatrices;
std::vector<glm::mat4> CmodelMatrices;
std::vector<glm::mat3> CNormalMatrices;

void BuildMatrix()
{
	int length = 10, width = 10, height = 3;
	float l_spacing = 5.0f, w_spacing = 5.0f, h_spacing = 5.0f;
	float h = 0.0f, l = -20.0, w = -20.0f;

	pair<glm::mat4, glm::mat3> temp;
	w = -200.0f;
	l = -200.0f;
	for (int i = 0; i < 4; i++)
	{
		l = -100.0f;
		for (int j = 0; j < 4; j++)
		{
			temp = getModelMatrix(glm::vec3(w, -0.1, l), glm::vec3(100.0f, 3.0f, 100.0f), glm::vec3(0.0f));
			GmodelMatrices.push_back(temp.first);
			GNormalMatrices.push_back(temp.second);
			l += 100.0f;
		}
		w += 100.0f;
	}

	w = -20.0f;
	for (int i = 0; i < width; i++)
	{
		l = -20.0f;
		for (int j = 0; j < length; j++)
		{
			temp = getModelMatrix(glm::vec3(w, h, l), glm::vec3(5.0f, 3.0f, 5.0f), glm::vec3(0.0f));
			FmodelMatrices.push_back(temp.first);
			FNormalMatrices.push_back(temp.second);
			l += l_spacing;
		}
		w += w_spacing;
	}

	// 墙壁循环逻辑

	h = 2.0f;
	for (int i = 0; i < height; i++)
	{
		float l = -20.0f;
		for (int j = 0; j < length; j++)
		{
			// 左墙

			temp = getModelMatrix(glm::vec3(27.0f, h, l), glm::vec3(5.0f, 3.0f, 5.0f), glm::vec3(90.0f, 0.0f, 90.0f));
			WmodelMatrices.push_back(temp.first);
			WNormalMatrices.push_back(temp.second);
			// 右墙
			temp = getModelMatrix(glm::vec3(-22.5f, h, l), glm::vec3(5.0f, 3.0f, 5.0f), glm::vec3(90.0f, 0.0f, 90.0f));
			WmodelMatrices.push_back(temp.first);
			WNormalMatrices.push_back(temp.second);
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
			temp = getModelMatrix(glm::vec3(l, h, -22.0), glm::vec3(5.0f, 3.0f, 5.0f), glm::vec3(90.0f, 0.0f, 0.0f));
			WmodelMatrices.push_back(temp.first);
			WNormalMatrices.push_back(temp.second);
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
			temp = getModelMatrix(glm::vec3(l, 10.9, 27.5), glm::vec3(5.0f, 3.0f, 7.0f), glm::vec3(90.0f, 0.0f, 0.0f));
			WmodelMatrices.push_back(temp.first);
			WNormalMatrices.push_back(temp.second);
		}
		else if (i == 4 || i == 8)
		{ // 窗
			float local_h = 2.0f;
			for (int j = 0; j < 2; j++)
			{
				temp = getModelMatrix(glm::vec3(l, local_h, 27.5), glm::vec3(5.0f, 3.0f, 5.0f), glm::vec3(90.0f, 0.0f, 0.0f));
				WmodelMatrices.push_back(temp.first);
				WNormalMatrices.push_back(temp.second);
				local_h += 2 * h_spacing;
			}
		}
		else
		{ // 实心
			float local_h = 2.0f;
			for (int j = 0; j < height; j++)
			{
				temp = getModelMatrix(glm::vec3(l, local_h, 27.5), glm::vec3(5.0f, 3.0f, 5.0f), glm::vec3(90.0f, 0.0f, 0.0f));
				WmodelMatrices.push_back(temp.first);
				WNormalMatrices.push_back(temp.second);
				local_h += h_spacing;
			}
		}
		l += l_spacing;
	}

	h = 15.0f;
	w = -20.0f;
	for (int i = 0; i < width; i++)
	{
		float l = -20.0f;
		for (int j = 0; j < length; j++)
		{
			temp = getModelMatrix(glm::vec3(w, h, l), glm::vec3(5.0f, 3.0f, 5.0f), glm::vec3(0.0f));
			CmodelMatrices.push_back(temp.first);
			CNormalMatrices.push_back(temp.second);
			l += l_spacing;
		}
		w += w_spacing;
	}
}


void setModelMatrix(glm::vec3 position, glm::vec3 scale, glm::vec3 rotation, Shader& shader)
{
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, position);
	model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, scale);
	shader.setMat4("model", model);
	shader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
}

pair<glm::mat4, glm::mat3> getModelMatrix(glm::vec3 position, glm::vec3 scale, glm::vec3 rotation)
{
	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 normalMatrix = glm::mat4(1.0f);
	model = glm::translate(model, position);
	model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, scale);
	normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
	return pair<glm::mat4, glm::mat3>(model, normalMatrix);
}

// 球体生成函数
unsigned int sphereVAO = 0;
unsigned int indexCount;
void renderSphere()
{
	if (sphereVAO == 0)
	{
		glGenVertexArrays(1, &sphereVAO);

		unsigned int vbo, ebo;
		glGenBuffers(1, &vbo);
		glGenBuffers(1, &ebo);

		std::vector<glm::vec3> positions;
		std::vector<glm::vec2> uv;
		std::vector<glm::vec3> normals;
		std::vector<unsigned int> indices;

		const unsigned int X_SEGMENTS = 64;
		const unsigned int Y_SEGMENTS = 64;
		const float PI = 3.14159265359f;
		for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
		{
			for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
			{
				float xSegment = (float)x / (float)X_SEGMENTS;
				float ySegment = (float)y / (float)Y_SEGMENTS;
				float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
				float yPos = std::cos(ySegment * PI);
				float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

				positions.push_back(glm::vec3(xPos, yPos, zPos));
				uv.push_back(glm::vec2(xSegment, ySegment));
				normals.push_back(glm::vec3(xPos, yPos, zPos));
			}
		}

		bool oddRow = false;
		for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
		{
			if (!oddRow) // even rows: y == 0, y == 2; and so on
			{
				for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
				{
					indices.push_back(y * (X_SEGMENTS + 1) + x);
					indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
				}
			}
			else
			{
				for (int x = X_SEGMENTS; x >= 0; --x)
				{
					indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
					indices.push_back(y * (X_SEGMENTS + 1) + x);
				}
			}
			oddRow = !oddRow;
		}
		indexCount = static_cast<unsigned int>(indices.size());

		std::vector<float> data;
		for (unsigned int i = 0; i < positions.size(); ++i)
		{
			data.push_back(positions[i].x);
			data.push_back(positions[i].y);
			data.push_back(positions[i].z);
			if (normals.size() > 0)
			{
				data.push_back(normals[i].x);
				data.push_back(normals[i].y);
				data.push_back(normals[i].z);
			}
			if (uv.size() > 0)
			{
				data.push_back(uv[i].x);
				data.push_back(uv[i].y);
			}
		}
		glBindVertexArray(sphereVAO);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
		unsigned int stride = (3 + 2 + 3) * sizeof(float);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
	}

	glBindVertexArray(sphereVAO);
	glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
}

// 地面生成函数
static unsigned int groundVAO = 0;
static unsigned int groundVBO = 0;
static unsigned int groundEBO = 0;
static unsigned int modelVBO = 0;  // 存放 Model 矩阵的 VBO
static unsigned int normalVBO = 0; // 存放 Normal 矩阵的 VBO
static unsigned int groundIndexCount = 0;

void renderGround(const std::vector<glm::mat4>& modelMatrices, const std::vector<glm::mat3>& normalMatrices, bool depth)
{
	unsigned int instanceCount = modelMatrices.size();
	if (instanceCount == 0) return;

	if (groundVAO == 0)
	{
		//生成几何数据
		const unsigned int SEGMENTS = 32;
		const float THICK = 0.2f;
		const float SCALE = 1.0f;
		const float halfS = SCALE * 0.5f;

		std::vector<float> vertices;
		std::vector<unsigned int> indices;

		auto addFace = [&](glm::vec3 normal, glm::vec3 tangent, glm::vec3 center, glm::vec3 axisX, glm::vec3 axisY, float width, float height)
			{
				unsigned int startIdx = vertices.size() / 11;
				for (unsigned int y = 0; y <= SEGMENTS; ++y)
				{
					for (unsigned int x = 0; x <= SEGMENTS; ++x)
					{
						float u = (float)x / SEGMENTS;
						float v = (float)y / SEGMENTS;
						glm::vec3 pos = center + (u - 0.5f) * width * axisX + (v - 0.5f) * height * axisY;
						vertices.push_back(pos.x); vertices.push_back(pos.y); vertices.push_back(pos.z);
						vertices.push_back(normal.x); vertices.push_back(normal.y); vertices.push_back(normal.z);
						vertices.push_back(u); vertices.push_back(v);
						vertices.push_back(tangent.x); vertices.push_back(tangent.y); vertices.push_back(tangent.z);
					}
				}
				for (unsigned int y = 0; y < SEGMENTS; ++y)
				{
					for (unsigned int x = 0; x < SEGMENTS; ++x)
					{
						unsigned int i0 = startIdx + y * (SEGMENTS + 1) + x;
						unsigned int i1 = i0 + 1;
						unsigned int i2 = startIdx + (y + 1) * (SEGMENTS + 1) + x;
						unsigned int i3 = i2 + 1;
						indices.push_back(i0); indices.push_back(i1); indices.push_back(i2);
						indices.push_back(i1); indices.push_back(i3); indices.push_back(i2);
					}
				}
			};

		addFace(glm::vec3(0, 1, 0), glm::vec3(1, 0, 0), glm::vec3(0, 0, 0), glm::vec3(1, 0, 0), glm::vec3(0, 0, 1), SCALE, SCALE);
		addFace(glm::vec3(0, -1, 0), glm::vec3(1, 0, 0), glm::vec3(0, -THICK, 0), glm::vec3(1, 0, 0), glm::vec3(0, 0, -1), SCALE, SCALE);
		addFace(glm::vec3(0, 0, 1), glm::vec3(1, 0, 0), glm::vec3(0, -THICK / 2, halfS), glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), SCALE, THICK);
		addFace(glm::vec3(0, 0, -1), glm::vec3(-1, 0, 0), glm::vec3(0, -THICK / 2, -halfS), glm::vec3(-1, 0, 0), glm::vec3(0, 1, 0), SCALE, THICK);
		addFace(glm::vec3(-1, 0, 0), glm::vec3(0, 0, 1), glm::vec3(-halfS, -THICK / 2, 0), glm::vec3(0, 0, 1), glm::vec3(0, 1, 0), SCALE, THICK);
		addFace(glm::vec3(1, 0, 0), glm::vec3(0, 0, -1), glm::vec3(halfS, -THICK / 2, 0), glm::vec3(0, 0, -1), glm::vec3(0, 1, 0), SCALE, THICK);

		groundIndexCount = indices.size();

		glGenVertexArrays(1, &groundVAO);
		glGenBuffers(1, &groundVBO);
		glGenBuffers(1, &groundEBO);
		glGenBuffers(1, &modelVBO);
		glGenBuffers(1, &normalVBO);

		glBindVertexArray(groundVAO);

		// 绑定几何数据
		glBindBuffer(GL_ARRAY_BUFFER, groundVBO);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, groundEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

		// 几何属性设置
		GLsizei stride = 11 * sizeof(float);
		glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
		glEnableVertexAttribArray(1); glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2); glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
		glEnableVertexAttribArray(3); glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(float)));

		//实例化 Model 矩阵属性
		glBindBuffer(GL_ARRAY_BUFFER, modelVBO);
		// 初始化时不传数据，只分配空间
		glBufferData(GL_ARRAY_BUFFER, 500 * sizeof(glm::mat4), NULL, GL_STATIC_DRAW);

		for (int i = 0; i < 4; i++)
		{
			glEnableVertexAttribArray(4 + i);
			glVertexAttribPointer(4 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(i * sizeof(glm::vec4)));
			glVertexAttribDivisor(4 + i, 1); //每渲染一个实例更新一次属性
		}

		//实例化 Normal 矩阵属性
		glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
		glBufferData(GL_ARRAY_BUFFER, 500 * sizeof(glm::mat3), NULL, GL_STATIC_DRAW);

		for (int i = 0; i < 3; i++)
		{
			glEnableVertexAttribArray(8 + i);
			glVertexAttribPointer(8 + i, 3, GL_FLOAT, GL_FALSE, sizeof(glm::mat3), (void*)(i * sizeof(glm::vec3)));
			glVertexAttribDivisor(8 + i, 1);
		}

		glBindVertexArray(0);
	}

	//每一帧更新实例化缓冲区并绘制
	glBindBuffer(GL_ARRAY_BUFFER, modelVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, instanceCount * sizeof(glm::mat4), modelMatrices.data());

	if (!depth)
	{
		glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, instanceCount * sizeof(glm::mat3), normalMatrices.data());
	}

	glBindVertexArray(groundVAO);
	glDrawElementsInstanced(GL_TRIANGLES, groundIndexCount, GL_UNSIGNED_INT, 0, instanceCount);
	glBindVertexArray(0);
}

//墙体生成函数
unsigned int wallVAO = 0;
unsigned int wallVBO = 0;
unsigned int wallEBO = 0;
unsigned int wallInstanceVBO = 0;
unsigned int wallNormalVBO = 0;
unsigned int wallIndexCount = 0;

void renderWall(const std::vector<glm::mat4>& modelMatrices, const std::vector<glm::mat3>& NormalMatrices, bool depth) {
	unsigned int instanceCount = modelMatrices.size();
	if (instanceCount == 0) return;

	if (wallVAO == 0) {
		//生成几何数据 (只执行一次)
		const unsigned int SEGMENTS = 32;
		const float THICK = 0.2f;
		const float SCALE = 1.0f;
		const float halfS = SCALE * 0.5f;

		std::vector<float> vertices;
		std::vector<unsigned int> indices;

		auto addFace = [&](glm::vec3 normal, glm::vec3 tangent, glm::vec3 origin, glm::vec3 right, glm::vec3 up, float width, float height) {
			unsigned int startIdx = vertices.size() / 11;
			for (int j = 0; j <= (int)SEGMENTS; ++j) {
				for (int i = 0; i <= (int)SEGMENTS; ++i) {
					float u = (float)i / SEGMENTS;
					float v = (float)j / SEGMENTS;
					glm::vec3 pos = origin + (u - 0.5f) * width * right + (v - 0.5f) * height * up;
					vertices.push_back(pos.x); vertices.push_back(pos.y); vertices.push_back(pos.z);
					vertices.push_back(normal.x); vertices.push_back(normal.y); vertices.push_back(normal.z);
					vertices.push_back(u); vertices.push_back(v);
					vertices.push_back(tangent.x); vertices.push_back(tangent.y); vertices.push_back(tangent.z);
				}
			}
			for (int j = 0; j < (int)SEGMENTS; ++j) {
				for (int i = 0; i < (int)SEGMENTS; ++i) {
					unsigned int row1 = startIdx + j * (SEGMENTS + 1);
					unsigned int row2 = startIdx + (j + 1) * (SEGMENTS + 1);
					indices.push_back(row1 + i); indices.push_back(row1 + i + 1); indices.push_back(row2 + i);
					indices.push_back(row1 + i + 1); indices.push_back(row2 + i + 1); indices.push_back(row2 + i);
				}
			}
			};

		// 生成六个面
		addFace(glm::vec3(0, 1, 0), glm::vec3(1, 0, 0), glm::vec3(0, 0, 0), glm::vec3(1, 0, 0), glm::vec3(0, 0, 1), SCALE, SCALE);
		addFace(glm::vec3(0, -1, 0), glm::vec3(1, 0, 0), glm::vec3(0, -THICK, 0), glm::vec3(1, 0, 0), glm::vec3(0, 0, -1), SCALE, SCALE);
		addFace(glm::vec3(0, 0, 1), glm::vec3(1, 0, 0), glm::vec3(0, -THICK / 2.0f, halfS), glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), SCALE, THICK);
		addFace(glm::vec3(0, 0, -1), glm::vec3(-1, 0, 0), glm::vec3(0, -THICK / 2.0f, -halfS), glm::vec3(-1, 0, 0), glm::vec3(0, 1, 0), SCALE, THICK);
		addFace(glm::vec3(-1, 0, 0), glm::vec3(0, 0, 1), glm::vec3(-halfS, -THICK / 2.0f, 0), glm::vec3(0, 0, 1), glm::vec3(0, 1, 0), SCALE, THICK);
		addFace(glm::vec3(1, 0, 0), glm::vec3(0, 0, -1), glm::vec3(halfS, -THICK / 2.0f, 0), glm::vec3(0, 0, -1), glm::vec3(0, 1, 0), SCALE, THICK);

		wallIndexCount = (unsigned int)indices.size();

		glGenVertexArrays(1, &wallVAO);
		glGenBuffers(1, &wallVBO);
		glGenBuffers(1, &wallEBO);
		glGenBuffers(1, &wallInstanceVBO);
		glGenBuffers(1, &wallNormalVBO);

		glBindVertexArray(wallVAO);

		// 静态几何 VBO
		glBindBuffer(GL_ARRAY_BUFFER, wallVBO);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wallEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

		GLsizei stride = 11 * sizeof(float);
		glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
		glEnableVertexAttribArray(1); glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2); glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
		glEnableVertexAttribArray(3); glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(float)));

		// 实例变换矩阵
		glBindBuffer(GL_ARRAY_BUFFER, wallInstanceVBO);
		// 分配空间，动态刷新
		glBufferData(GL_ARRAY_BUFFER, 1000 * sizeof(glm::mat4), NULL, GL_DYNAMIC_DRAW);
		for (unsigned int i = 0; i < 4; i++)
		{
			glEnableVertexAttribArray(4 + i);
			glVertexAttribPointer(4 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(i * sizeof(glm::vec4)));
			glVertexAttribDivisor(4 + i, 1); 
		}

		//实例法线矩阵
		glBindBuffer(GL_ARRAY_BUFFER, wallNormalVBO);
		glBufferData(GL_ARRAY_BUFFER, 1000 * sizeof(glm::mat3), NULL, GL_DYNAMIC_DRAW);
		for (unsigned int i = 0; i < 3; i++)
		{
			glEnableVertexAttribArray(8 + i);
			glVertexAttribPointer(8 + i, 3, GL_FLOAT, GL_FALSE, sizeof(glm::mat3), (void*)(i * sizeof(glm::vec3)));
			glVertexAttribDivisor(8 + i, 1);
		}

		glBindVertexArray(0);
	}

	//每一帧更新实例化缓冲区并绘制
	glBindBuffer(GL_ARRAY_BUFFER, modelVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, instanceCount * sizeof(glm::mat4), modelMatrices.data());

	if (!depth)
	{
		glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, instanceCount * sizeof(glm::mat3), NormalMatrices.data());
	}

	glBindVertexArray(groundVAO);
	glDrawElementsInstanced(GL_TRIANGLES, groundIndexCount, GL_UNSIGNED_INT, 0, instanceCount);
	glBindVertexArray(0);
}


unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderCube()
{
	// initialize (if necessary)
	if (cubeVAO == 0)
	{
		float vertices[] = {
			// back face
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
			// front face
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			// left face
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			// right face
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
			 // bottom face
			 -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			  1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
			  1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			  1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			 -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			 -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			 // top face
			 -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			  1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			  1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
			  1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			 -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			 -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
		};
		glGenVertexArrays(1, &cubeVAO);
		glGenBuffers(1, &cubeVBO);
		// fill buffer
		glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		// link vertex attributes
		glBindVertexArray(cubeVAO);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	// render Cube
	glBindVertexArray(cubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}

// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
	if (quadVAO == 0)
	{
		float quadVertices[] = {
			// positions        // texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		// setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}
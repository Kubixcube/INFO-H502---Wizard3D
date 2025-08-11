#include "object.h"

Object::Object(const char* path) {
	std::ifstream infile(path);
	std::string line;
	while (std::getline(infile, line))
	{
		std::istringstream iss(line);
		std::string indice;
		iss >> indice;
		if (indice == "v") {
			float x, y, z;
			iss >> x >> y >> z;
			positions.push_back(glm::vec3(x, y, z));
		}
		else if (indice == "vn") {
			float x, y, z;
			iss >> x >> y >> z;
			normals.push_back(glm::vec3(x, y, z));
		}
		else if (indice == "vt") {
			float u, v;
			iss >> u >> v;
			textures.push_back(glm::vec2(u, v));
		}
		else if (indice == "f") {
			std::string f1, f2, f3;
			iss >> f1 >> f2 >> f3;

			std::string p, t, n;

			Vertex v1;
			p = f1.substr(0, f1.find("/"));
			f1.erase(0, f1.find("/") + 1);
			t = f1.substr(0, f1.find("/"));
			f1.erase(0, f1.find("/") + 1);
			n = f1.substr(0, f1.find("/"));
			v1.Position = positions.at(std::stof(p) - 1);
			v1.Normal = normals.at(std::stof(n) - 1);
			v1.Texture = textures.at(std::stof(t) - 1);
			vertices.push_back(v1);

			Vertex v2;
			p = f2.substr(0, f2.find("/"));
			f2.erase(0, f2.find("/") + 1);
			t = f2.substr(0, f2.find("/"));
			f2.erase(0, f2.find("/") + 1);
			n = f2.substr(0, f2.find("/"));
			v2.Position = positions.at(std::stof(p) - 1);
			v2.Normal = normals.at(std::stof(n) - 1);
			v2.Texture = textures.at(std::stof(t) - 1);
			vertices.push_back(v2);

			Vertex v3;
			p = f3.substr(0, f3.find("/"));
			f3.erase(0, f3.find("/") + 1);
			t = f3.substr(0, f3.find("/"));
			f3.erase(0, f3.find("/") + 1);
			n = f3.substr(0, f3.find("/"));
			v3.Position = positions.at(std::stof(p) - 1);
			v3.Normal = normals.at(std::stof(n) - 1);
			v3.Texture = textures.at(std::stof(t) - 1);
			vertices.push_back(v3);
		}
	}
	std::cout << "Load model with " << vertices.size() << " vertices" << std::endl;
	infile.close();
	numVertices = vertices.size();
	computeCollisionBox();
}

Object::Object(const char* path, Properties p) : Object(path) {
	properties = p;
}

void Object::computeCollisionBox() {
	if (vertices.empty()) {
		collisionBox = { glm::vec3(0), glm::vec3(0) };
		localCenter = glm::vec3(0);
		localExtents = glm::vec3(0);
		return;
	}
	glm::vec3 minV(std::numeric_limits<float>::max());
	glm::vec3 maxV(-std::numeric_limits<float>::max());
	for (const auto& v : vertices) {
		minV = glm::min(minV, v.Position);
		maxV = glm::max(maxV, v.Position);
	}
	collisionBox.min = minV;
	collisionBox.max = maxV;
	localCenter = 0.5f * (minV + maxV);
	localExtents = 0.5f * (maxV - minV);
}

AABB Object::worldAABB() const {
	glm::vec3 c = glm::vec3(model * glm::vec4(localCenter, 1.0f));
	glm::mat3 R = glm::mat3(model);
	glm::vec3 e;
	e.x = glm::abs(R[0].x) * localExtents.x + glm::abs(R[1].x) * localExtents.y + glm::abs(R[2].x) * localExtents.z;
	e.y = glm::abs(R[0].y) * localExtents.x + glm::abs(R[1].y) * localExtents.y + glm::abs(R[2].y) * localExtents.z;
	e.z = glm::abs(R[0].z) * localExtents.x + glm::abs(R[1].z) * localExtents.y + glm::abs(R[2].z) * localExtents.z;
	return { c - e, c + e };
}

void Object::bindTexture(std::string path) {
	texture = Texture(path);
	properties.hasTexture = true;
}

void Object::makeObject(Shader shader, bool texture) {
	float* data = new float[8 * numVertices];
	for (int i = 0; i < numVertices; i++) {
		Vertex v = vertices.at(i);
		data[i * 8] = v.Position.x;
		data[i * 8 + 1] = v.Position.y;
		data[i * 8 + 2] = v.Position.z;
		data[i * 8 + 3] = v.Texture.x;
		data[i * 8 + 4] = v.Texture.y;
		data[i * 8 + 5] = v.Normal.x;
		data[i * 8 + 6] = v.Normal.y;
		data[i * 8 + 7] = v.Normal.z;
	}

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * numVertices, data, GL_STATIC_DRAW);

	auto att_pos = glGetAttribLocation(shader.ID, "aPos");
	glEnableVertexAttribArray(att_pos);
	glVertexAttribPointer(att_pos, 3, GL_FLOAT, false, 8 * sizeof(float), (void*)0);

	if (texture) {
		auto att_tex = glGetAttribLocation(shader.ID, "aTexCoord");
		glEnableVertexAttribArray(att_tex);
		glVertexAttribPointer(att_tex, 2, GL_FLOAT, false, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	}

	auto att_col = glGetAttribLocation(shader.ID, "aNormal");
	glEnableVertexAttribArray(att_col);
	glVertexAttribPointer(att_col, 3, GL_FLOAT, false, 8 * sizeof(float), (void*)(5 * sizeof(float)));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	delete[] data;
}

void Object::draw() {
	if (properties.hasTexture) texture.map();
	glBindVertexArray(this->VAO);
	glDrawArrays(GL_TRIANGLES, 0, numVertices);
}

#include "core.h"

#include "import.h"

#include <fstream>

#include "../util/stringUtil.h"
#include "../physics/physical.h"
#include "../graphics/visualShape.h"
/*
	Import
*/

int Import::parseInt(const std::string& num) {
	return std::stoi(num);
}

long long Import::parseLong(const std::string& num) {
	return std::stoll(num);
}

double Import::parseDouble(const std::string& num) {
	return std::stod(num);
}

float Import::parseFloat(const std::string& num) {
	return std::stof(num);
}

Vec3 Import::parseVec3(const std::string& vec) {
	std::vector<std::string> tokens = split(vec, ' ');
	Vec3 vector = Vec3();
	for (int i = 0; i < 3; i++)
		vector[i] = Import::parseDouble(tokens[i]);
	
	return vector;
}

Position Import::parsePosition(const std::string& vec) {
	std::vector<std::string> tokens = split(vec, ' ');
	Position vector;
	for (int i = 0; i < 3; i++)
		vector[i] = Fix<32>(Import::parseLong(tokens[i]));
	
	return vector;
}

Vec4 Import::parseVec4(const std::string& vec) {
	std::vector<std::string> tokens = split(vec, ' ');
	Vec4 vector;
	for (int i = 0; i < 4; i++)
		vector[i] = Import::parseDouble(tokens[i]);
	
	return vector;
}

Vec4f Import::parseVec4f(const std::string& vec) {
	std::vector<std::string> tokens = split(vec, ' ');
	Vec4f vector;
	for (int i = 0; i < 4; i++)
		vector[i] = Import::parseFloat(tokens[i]);
	
	return vector;
}

Vec3f Import::parseVec3f(const std::string& vec) {
	std::vector<std::string> tokens = split(vec, ' ');
	Vec3f vector = Vec3f();
	for (int i = 0; i < 3; i++)
		vector[i] = Import::parseFloat(tokens[i]);
	
	return vector;
}

DiagonalMat3 Import::parseDiagonalMat3(const std::string& mat) {
	std::vector<std::string> tokens = split(mat, ' ');
	DiagonalMat3 matrix = DiagonalMat3();
	for (int i = 0; i < 3; i++)
		matrix[i] = Import::parseDouble(tokens[i]);
	
	return matrix;
}

Mat3 Import::parseMat3(const std::string& mat) {
	std::vector<std::string> tokens = split(mat, ' ');
	double data[9];

	for (int i = 0; i < 9; i++)
		data[i] = Import::parseDouble(tokens[i]);

	return Matrix<double, 3, 3>::fromColMajorData(data);
}

template<typename T>
T Import::read(std::istream& input) {
	char * buffer = (char*) alloca(sizeof(T));
	input.read(buffer, sizeof(T));
	return *reinterpret_cast<T*>(buffer);
}

/*
	End of Import
*/



/*
	OBJImport
*/

struct Vertex {
	int position = -1;
	int normal = -1;
	int uv = -1;

	inline Vertex(const std::string& line) {
		std::vector<std::string> tokens = split(line, '/');
		size_t length = tokens.size();

		// Positions
		position = std::stoi(tokens[0]) - 1;

		// Uvs
		if (length > 1) 
			uv = tokens[1].size() > 0 ? std::stoi(tokens[1]) - 1 : -1;

		// Normals
		if (length > 2)
			normal = tokens[2].size() > 0 ? std::stoi(tokens[2]) - 1 : -1;
	}
};

struct Flags {
	bool normals;
	bool uvs;
};

struct Face {
	Vertex v1;
	Vertex v2;
	Vertex v3;

	inline Face(const Vertex& v1, const Vertex& v2, const Vertex& v3) : v1(v1), v2(v2), v3(v3) {};

	inline Vertex& operator[] (int index) {
		switch (index) {
			case 0: return v1;
			case 1: return v2;
			case 2: return v3;
		}

		throw "Subscript out of range";
	}

	inline const Vertex& operator[] (int index) const {
		switch (index) {
			case 0: return v1;
			case 1: return v2;
			case 2: return v3;
		}

		throw "Subscript out of range";
	}
};

Graphics::VisualShape reorder(const std::vector<Vec3f>& positions, const std::vector<Vec3f>& normals, const std::vector<Vec2f>& uvs, const std::vector<Face>& faces, const Flags& flags) {
	
	// Positions
	Vec3f* positionArray = new Vec3f[positions.size()];
	for (int i = 0; i < positions.size(); i++) 
		positionArray[i] = positions[i];
	
	// Normals
	Vec3f* normalArray = nullptr;
	if (flags.normals) 
		normalArray = new Vec3f[positions.size()];

	// UVs
	Vec2f* uvArray = nullptr;
	if (flags.uvs) 
		uvArray = new Vec2f[positions.size()];

	// Triangles
	Triangle* triangleArray = new Triangle[faces.size()];
	for (int i = 0; i < faces.size(); i++) {
		const Face& face = faces[i];

		// Save triangle
		triangleArray[i] = { face.v1.position, face.v2.position, face.v3.position };

		for (int i = 0; i < 3; i++) {
			const Vertex& vertex = face[i];

			// Save normal
			if (flags.normals && vertex.normal != -1) 
				normalArray[vertex.position] = normals[vertex.normal];

			// Save uv
			if (flags.uvs && vertex.uv != -1)
				uvArray[vertex.position] = Vec2(uvs[vertex.uv].x, 1.0 - uvs[vertex.uv].y);
		}
	}

	return Graphics::VisualShape(positionArray, SharedArrayPtr<const Vec3f>(normalArray), SharedArrayPtr<const Vec2f>(uvArray), triangleArray, (int) positions.size(), (int) faces.size());
}

Graphics::VisualShape loadBinaryObj(std::istream& input) {
	char flag = Import::read<char>(input);
	int vertexCount = Import::read<int>(input);
	int triangleCount = Import::read<int>(input);

	char V = 0;
	char VN = 1;
	char VT = 2;
	char VNT = 3;

	Vec3f* vertices = new Vec3f[vertexCount];
	for (int i = 0; i < vertexCount; i++) {
		Vec3 t = Import::read<Vec3f>(input);

		vertices[i] = t;
	}

	Vec3f* normals = nullptr;
	if (flag == VN || flag == VNT) {
		normals = new Vec3f[vertexCount];
		for (int i = 0; i < vertexCount; i++) {
			normals[i] = Import::read<Vec3f>(input);
		}
	}

	Vec2f* uvs = nullptr;
	if (flag == VT || flag == VNT) {
		uvs = new Vec2f[vertexCount];
		for (int i = 0; i < vertexCount; i++) {
			uvs[i] = Import::read<Vec2f>(input);
		}
	}

	Triangle* triangles = new Triangle[triangleCount];
	for (int i = 0; i < triangleCount; i++) {
		triangles[i] = Import::read<Triangle>(input);
	}

	return Graphics::VisualShape(vertices, SharedArrayPtr<const Vec3f>(normals), SharedArrayPtr<const Vec2f>(uvs), triangles, vertexCount, triangleCount);
}

Graphics::VisualShape loadNonBinaryObj(std::istream& input) {
	std::vector<Vec3f> vertices;
	std::vector<Vec3f> normals;
	std::vector<Vec2f> uvs;
	std::vector<Face> faces;
	Flags flags = { false, false };

	std::string line;
	while (getline(input, line)) {
		std::vector<std::string> tokens = split(line, ' ');

		if (tokens.size() == 0)
			continue;

		if (tokens[0] == "v") {
			Vec3f vertex = Vec3f(stof(tokens[1]), stof(tokens[2]), stof(tokens[3]));
			vertices.push_back(vertex);
		} else if (tokens[0] == "f") {
			Vertex v1(tokens[1]);
			Vertex v2(tokens[2]);
			Vertex v3(tokens[3]);

			Face face = Face(v1, v2, v3);
			faces.push_back(face);

			if (tokens.size() > 4) {
				Vertex v4 = tokens[4];
				Face face = Face(v1, v3, v4);
				faces.push_back(face);
			}
		} else if (tokens[0] == "vt") {
			flags.uvs = true;
			Vec2f uv = Vec2f(stof(tokens[1]), stof(tokens[2]));
			uvs.push_back(uv);
		} else if (tokens[0] == "vn") {
			flags.normals = true;
			Vec3f normal = Vec3f(stof(tokens[1]), stof(tokens[2]), stof(tokens[3]));
			normals.push_back(normal);
		}
	}

	return reorder(vertices, normals, uvs, faces, flags);
}

Graphics::VisualShape OBJImport::load(std::istream& file, bool binary) {
	if (binary)
		return loadBinaryObj(file);
	else
		return loadNonBinaryObj(file);
}

Graphics::VisualShape OBJImport::load(const std::string& file) {
	bool binary;
	if (endsWith(file, ".bobj"))
		binary = true;
	else if (endsWith(file, ".obj"))
		binary = false;
	else
		return Graphics::VisualShape();

	return OBJImport::load(file, binary);
}

Graphics::VisualShape OBJImport::load(const std::string& file, bool binary) {
	struct stat buffer;
	if (stat(file.c_str(), &buffer) == -1) {
		Log::subject(file.c_str());
		Log::error("File not found: %s", file.c_str());

		return Graphics::VisualShape();
	}

	std::ifstream input;

	if (binary)
		input.open(file, std::ios::binary);
	else
		input.open(file);

	Graphics::VisualShape shape = load(input, binary);

	input.close();

	return shape;
}

/*
	End of OBJImport
*/
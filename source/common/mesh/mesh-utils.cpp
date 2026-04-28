#include "mesh-utils.hpp"

// We will use "Tiny OBJ Loader" to read and process '.obj" files
#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobj/tiny_obj_loader.h>

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <stb/stb_image.h>

#include <glm/gtc/matrix_inverse.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <vector>
#include <unordered_map>

#include "../animation/animated-mesh.hpp"
#include <map>

namespace {

    glm::mat4 aiToGlm(const aiMatrix4x4& m) {
        glm::mat4 result(1.0f);
        result[0] = glm::vec4(m.a1, m.b1, m.c1, m.d1);
        result[1] = glm::vec4(m.a2, m.b2, m.c2, m.d2);
        result[2] = glm::vec4(m.a3, m.b3, m.c3, m.d3);
        result[3] = glm::vec4(m.a4, m.b4, m.c4, m.d4);
        return result;
    }

    std::string toLower(std::string value) {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return value;
    }

    our::Color colorFromAssimp(const aiColor4D& c) {
        auto toByte = [](float v) -> glm::uint8 {
            float clamped = std::clamp(v, 0.0f, 1.0f);
            return static_cast<glm::uint8>(clamped * 255.0f + 0.5f);
        };
        return our::Color(toByte(c.r), toByte(c.g), toByte(c.b), toByte(c.a));
    }

    our::Color multiplyColor(const our::Color& a, const our::Color& b) {
        auto mul = [](glm::uint8 lhs, glm::uint8 rhs) -> glm::uint8 {
            return static_cast<glm::uint8>((static_cast<unsigned int>(lhs) * static_cast<unsigned int>(rhs) + 127u) / 255u);
        };
        return our::Color(mul(a.r, b.r), mul(a.g, b.g), mul(a.b, b.b), mul(a.a, b.a));
    }

    GLuint createSolidTexture(const our::Color& color) {
        GLuint texture = 0;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        const unsigned char pixel[] = { color.r, color.g, color.b, color.a };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        return texture;
    }

    struct AssimpBuildContext {
        std::string directory;
        std::unordered_map<std::string, GLuint> textureCache;
        std::vector<GLuint> ownedTextures;
        GLuint whiteTexture = 0;
    };

    std::string resolveTexturePath(const aiString& texturePath, const std::string& directory) {
        std::string path = texturePath.C_Str();
        std::replace(path.begin(), path.end(), '\\', '/');

        if (path.empty() || path[0] == '*') {
            return "";
        }

        const std::filesystem::path given(path);
        if (given.is_absolute() && std::filesystem::exists(given)) {
            return given.string();
        }

        const std::filesystem::path joined = std::filesystem::path(directory) / given;
        if (std::filesystem::exists(joined)) {
            return joined.string();
        }

        const std::filesystem::path basename = std::filesystem::path(directory) / given.filename();
        if (std::filesystem::exists(basename)) {
            return basename.string();
        }

        return "";
    }

    GLuint uploadRGBA8Texture(const unsigned char* pixels, int width, int height) {
        if (!pixels || width <= 0 || height <= 0) {
            return 0;
        }

        GLuint texture = 0;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        return texture;
    }

    GLuint loadTexture2D(const std::string& path, AssimpBuildContext& context) {
        auto cached = context.textureCache.find(path);
        if (cached != context.textureCache.end()) {
            return cached->second;
        }

        stbi_set_flip_vertically_on_load(false);

        int width = 0, height = 0, channels = 0;
        unsigned char* pixels = stbi_load(path.c_str(), &width, &height, &channels, 4);
        if (!pixels) {
            std::cerr << "FAILED: Texture failed to load at path: " << path << std::endl;
            context.textureCache[path] = context.whiteTexture;
            return context.whiteTexture;
        }

        GLuint texture = uploadRGBA8Texture(pixels, width, height);

        stbi_image_free(pixels);

        if (texture == 0) {
            std::cerr << "FAILED: Texture upload failed for path: " << path << std::endl;
            context.textureCache[path] = context.whiteTexture;
            return context.whiteTexture;
        }

        context.textureCache[path] = texture;
        context.ownedTextures.push_back(texture);
        std::cout << "SUCCESS: Loaded texture at path: " << path << std::endl;

        return texture;
    }

    GLuint loadEmbeddedTexture(const aiString& texturePath, const aiScene* scene, AssimpBuildContext& context) {
        std::string textureRef = texturePath.C_Str();
        if (!scene || textureRef.empty()) {
            return 0;
        }

        const aiTexture* embedded = scene->GetEmbeddedTexture(textureRef.c_str());
        if (!embedded && textureRef.size() > 1 && textureRef[0] == '*') {
            char* end = nullptr;
            long index = std::strtol(textureRef.c_str() + 1, &end, 10);
            if (
                end != (textureRef.c_str() + 1) &&
                *end == '\0' &&
                index >= 0 &&
                static_cast<unsigned long>(index) < static_cast<unsigned long>(scene->mNumTextures)
                ) {
                embedded = scene->mTextures[index];
            }
        }

        if (!embedded) {
            return 0;
        }

        const std::string cacheKey = "embedded:" + textureRef;
        auto cached = context.textureCache.find(cacheKey);
        if (cached != context.textureCache.end()) {
            return cached->second == context.whiteTexture ? 0 : cached->second;
        }

        GLuint texture = 0;

        if (embedded->mHeight == 0) {
            stbi_set_flip_vertically_on_load(false);

            int width = 0;
            int height = 0;
            int channels = 0;

            const stbi_uc* data = reinterpret_cast<const stbi_uc*>(embedded->pcData);
            const int dataSize = static_cast<int>(std::min<unsigned int>(embedded->mWidth, static_cast<unsigned int>(std::numeric_limits<int>::max())));
            unsigned char* pixels = stbi_load_from_memory(data, dataSize, &width, &height, &channels, 4);

            if (pixels) {
                texture = uploadRGBA8Texture(pixels, width, height);
                stbi_image_free(pixels);
            }
        }
        else {
            if (embedded->mWidth > 0 && embedded->mHeight > 0) {
                std::vector<unsigned char> rgba;
                const size_t pixelCount = static_cast<size_t>(embedded->mWidth) * static_cast<size_t>(embedded->mHeight);
                rgba.resize(pixelCount * 4);

                for (size_t i = 0; i < pixelCount; ++i) {
                    const aiTexel& texel = embedded->pcData[i];
                    rgba[i * 4 + 0] = texel.r;
                    rgba[i * 4 + 1] = texel.g;
                    rgba[i * 4 + 2] = texel.b;
                    rgba[i * 4 + 3] = texel.a;
                }

                int width = static_cast<int>(std::min<unsigned int>(embedded->mWidth, static_cast<unsigned int>(std::numeric_limits<int>::max())));
                int height = static_cast<int>(std::min<unsigned int>(embedded->mHeight, static_cast<unsigned int>(std::numeric_limits<int>::max())));
                texture = uploadRGBA8Texture(rgba.data(), width, height);
            }
        }

        if (texture != 0) {
            context.textureCache[cacheKey] = texture;
            context.ownedTextures.push_back(texture);
            std::cout << "SUCCESS: Loaded embedded texture: " << textureRef << std::endl;
            return texture;
        }

        std::cerr << "FAILED: Embedded texture failed to load: " << textureRef << std::endl;
        return 0;
    }

    constexpr aiTextureType kPreferredTextureTypes[] = {
        aiTextureType_BASE_COLOR,
        aiTextureType_DIFFUSE,
        aiTextureType_NORMALS,
        aiTextureType_METALNESS,
        aiTextureType_DIFFUSE_ROUGHNESS,
        aiTextureType_EMISSIVE,
        aiTextureType_UNKNOWN
    };

    our::Color getMaterialColor(const aiMaterial* material) {
        aiColor4D diffuse(1.0f, 1.0f, 1.0f, 1.0f);
        aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &diffuse);
        return colorFromAssimp(diffuse);
    }

    GLuint getMaterialTexture(const aiMaterial* material, const aiScene* scene, const std::string& directory, AssimpBuildContext& context) {
        aiString texturePath;

        for (aiTextureType textureType : kPreferredTextureTypes) {
            const unsigned int textureCount = material->GetTextureCount(textureType);
            for (unsigned int index = 0; index < textureCount; index++) {
                if (material->GetTexture(textureType, index, &texturePath) != AI_SUCCESS) {
                    continue;
                }

                const GLuint embeddedTexture = loadEmbeddedTexture(texturePath, scene, context);
                if (embeddedTexture != 0) {
                    return embeddedTexture;
                }

                const std::string fullPath = resolveTexturePath(texturePath, directory);
                if (!fullPath.empty()) {
                    return loadTexture2D(fullPath, context);
                }
            }
        }

        return context.whiteTexture;
    }

    unsigned int countMaterialTextures(const aiMaterial* material) {
        unsigned int count = 0;
        for (aiTextureType textureType : kPreferredTextureTypes) {
            count += material->GetTextureCount(textureType);
        }
        return count;
    }

    void logMaterialTextureTypes(const aiMaterial* material) {
        for (int type = static_cast<int>(aiTextureType_NONE); type <= static_cast<int>(aiTextureType_UNKNOWN); type++) {
            unsigned int count = material->GetTextureCount(static_cast<aiTextureType>(type));
            if (count > 0) {
                aiString path;
                material->GetTexture(static_cast<aiTextureType>(type), 0, &path);
                std::cout << "  - Has " << count << " texture(s) of type " << type << ". Path: " << path.C_Str() << std::endl;
            }
        }
    }

    void logMaterialLightInfo(const aiMaterial* material) {
        std::cout << "Material Lighting Properties:" << std::endl;
        aiColor3D color;
        if (material->Get(AI_MATKEY_COLOR_AMBIENT, color) == AI_SUCCESS) {
            std::cout << "  - Ambient: " << color.r << ", " << color.g << ", " << color.b << std::endl;
        }
        if (material->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
            std::cout << "  - Diffuse: " << color.r << ", " << color.g << ", " << color.b << std::endl;
        }
        if (material->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS) {
            std::cout << "  - Specular: " << color.r << ", " << color.g << ", " << color.b << std::endl;
        }
        float shininess;
        if (material->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS) {
            std::cout << "  - Shininess: " << shininess << std::endl;
        }
    }

    void logMeshInfo(const aiMesh* mesh, const aiScene* scene) {
        std::cout << "\n--- Processing Mesh: " << mesh->mName.C_Str() << " ---" << std::endl;

        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        aiString materialName;
        material->Get(AI_MATKEY_NAME, materialName);
        std::cout << "Material Name: " << materialName.C_Str() << std::endl;
        logMaterialTextureTypes(material);
        logMaterialLightInfo(material);

        unsigned int textureCount = countMaterialTextures(material);
        if (textureCount == 0) {
            std::cout << "Mesh has NO textures assigned in preferred channels." << std::endl;
        }
        else {
            std::cout << "Mesh loaded with " << textureCount << " textures in preferred channels." << std::endl;
        }

        if (mesh->HasBones()) {
            std::cout << "Mesh has " << mesh->mNumBones << " bones." << std::endl;
        }
    }

    void appendAssimpMesh(
        const aiMesh* mesh,
        const aiScene* scene,
        const glm::mat4& nodeTransform,
        std::vector<our::Vertex>& vertices,
        std::vector<GLuint>& elements,
        std::vector<our::Mesh::DrawBatch>& drawBatches,
        AssimpBuildContext& context
    ) {
        logMeshInfo(mesh, scene);

        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

    const GLuint texture = getMaterialTexture(material, scene, context.directory, context);
    
    const our::Color materialColor = (texture != context.whiteTexture)
        ? our::Color(255, 255, 255, 255)
        : getMaterialColor(material);
        

        our::Mesh::DrawBatch batch;

        // Load material lighting properties into our 'batch' object.
        aiColor3D color;
        // if (material->Get(AI_MATKEY_COLOR_AMBIENT, color) == AI_SUCCESS)
        //     batch.ambient = { color.r, color.g, color.b };

        if (material->Get(AI_MATKEY_COLOR_AMBIENT, color) == AI_SUCCESS) {
            batch.ambient = { color.r, color.g, color.b };
            // Set a default if ambient is zero
            if (batch.ambient == glm::vec3(0.0f)) {
                batch.ambient = glm::vec3(0.3f); 
            }
        }
        if (material->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
            batch.diffuse = { color.r, color.g, color.b };
        if (material->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS)
            batch.specular = { color.r, color.g, color.b };

        material->Get(AI_MATKEY_SHININESS, batch.shininess);

        logMaterialLightInfo(material);

        aiString texturePath;
        if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS) {
            batch.diffuseTexture = loadTexture2D(resolveTexturePath(texturePath, context.directory), context);
        }
        if (material->GetTexture(aiTextureType_SPECULAR, 0, &texturePath) == AI_SUCCESS) {
            batch.specularTexture = loadTexture2D(resolveTexturePath(texturePath, context.directory), context);
        }

        if (batch.diffuseTexture == 0) {
            batch.diffuseTexture = context.whiteTexture;
        }



        const GLuint vertexOffset = static_cast<GLuint>(vertices.size());
        const GLuint firstIndex = static_cast<GLuint>(elements.size());
        const glm::mat3 normalTransform = glm::transpose(glm::inverse(glm::mat3(nodeTransform)));

        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            our::Vertex vertex{};

            glm::vec3 position = {
                mesh->mVertices[i].x,
                mesh->mVertices[i].y,
                mesh->mVertices[i].z
            };
            glm::vec4 worldPosition = nodeTransform * glm::vec4(position, 1.0f);
            vertex.position = glm::vec3(worldPosition);

            vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
            if (mesh->HasNormals()) {
                glm::vec3 normal = {
                    mesh->mNormals[i].x,
                    mesh->mNormals[i].y,
                    mesh->mNormals[i].z
                };
                vertex.normal = glm::normalize(normalTransform * normal);
            }

            vertex.tex_coord = glm::vec2(0.0f, 0.0f);
            if (mesh->mTextureCoords[0]) {
                vertex.tex_coord = glm::vec2(
                    mesh->mTextureCoords[0][i].x,
                    mesh->mTextureCoords[0][i].y
                );
            }

            vertex.color = materialColor;
            if (mesh->HasVertexColors(0) && mesh->mColors[0]) {
                vertex.color = multiplyColor(materialColor, colorFromAssimp(mesh->mColors[0][i]));
            }

            vertices.push_back(vertex);
        }

        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            const aiFace& face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                elements.push_back(vertexOffset + face.mIndices[j]);
            }
        }

        const GLsizei indexCount = static_cast<GLsizei>(elements.size() - firstIndex);
        if (indexCount > 0) {
            batch.indexCount = indexCount;
            batch.firstIndex = firstIndex;
            batch.texture = texture;
            batch.hasTexture = (texture != context.whiteTexture);
            drawBatches.push_back(batch);
        }
    }

    void processAssimpNode(
        const aiNode* node,
        const aiScene* scene,
        const glm::mat4& parentTransform,
        std::vector<our::Vertex>& vertices,
        std::vector<GLuint>& elements,
        std::vector<our::Mesh::DrawBatch>& drawBatches,
        AssimpBuildContext& context
    ) {
        const glm::mat4 nodeTransform = parentTransform * aiToGlm(node->mTransformation);

        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            appendAssimpMesh(mesh, scene, nodeTransform, vertices, elements, drawBatches, context);
        }

        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processAssimpNode(node->mChildren[i], scene, nodeTransform, vertices, elements, drawBatches, context);
        }
    }

    our::Mesh* loadWithAssimp(const std::string& filename) {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(
            filename,
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_FlipUVs |
            aiProcess_JoinIdenticalVertices |
            aiProcess_ImproveCacheLocality
        );

        if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
            std::cerr << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
            return nullptr;
        }

        AssimpBuildContext context;
        context.directory = std::filesystem::path(filename).parent_path().string();
        if (context.directory.empty()) {
            context.directory = ".";
        }
        context.whiteTexture = createSolidTexture(our::Color(255, 255, 255, 255));
        context.ownedTextures.push_back(context.whiteTexture);

        std::vector<our::Vertex> vertices;
        std::vector<GLuint> elements;
        std::vector<our::Mesh::DrawBatch> drawBatches;
        processAssimpNode(scene->mRootNode, scene, glm::mat4(1.0f), vertices, elements, drawBatches, context);

        if (vertices.empty() || elements.empty()) {
            glDeleteTextures(static_cast<GLsizei>(context.ownedTextures.size()), context.ownedTextures.data());
            std::cerr << "Failed to build mesh buffers for file: " << filename << std::endl;
            return nullptr;
        }

        std::cout << "Loaded model file \"" << filename << "\" with "
            << vertices.size() << " vertices and "
            << (elements.size() / 3) << " triangles." << std::endl;

        return new our::Mesh(vertices, elements, drawBatches, context.ownedTextures);
    }

} // namespace

our::Mesh* our::mesh_utils::loadMesh(const std::string& filename) {
    std::string extension = toLower(std::filesystem::path(filename).extension().string());
    if (extension == ".obj") {
        return loadOBJ(filename);
    }
    return loadWithAssimp(filename);
}

our::Mesh* our::mesh_utils::loadOBJ(const std::string& filename) {

    // The data that we will use to initialize our mesh
    std::vector<our::Vertex> vertices;
    std::vector<GLuint> elements;

    // Since the OBJ can have duplicated vertices, we make them unique using this map
    // The key is the vertex, the value is its index in the vector "vertices".
    // That index will be used to populate the "elements" vector.
    std::unordered_map<our::Vertex, GLuint> vertex_map;

    // The data loaded by Tiny OBJ Loader
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str())) {
        std::cerr << "Failed to load obj file \"" << filename << "\" due to error: " << err << std::endl;
        return nullptr;
    }
    if (!warn.empty()) {
        std::cout << "WARN while loading obj file \"" << filename << "\": " << warn << std::endl;
    }

    // An obj file can have multiple shapes where each shape can have its own material
    // Ideally, we would load each shape into a separate mesh or store the start and end of it in the element buffer to be able to draw each shape separately
    // But we ignored this fact since we don't plan to use multiple materials in the examples
    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            if (index.vertex_index < 0) continue;

            Vertex vertex = {};

            vertex.position = { 0.0f, 0.0f, 0.0f };
            vertex.normal = { 0.0f, 1.0f, 0.0f };
            vertex.tex_coord = { 0.0f, 0.0f };
            vertex.color = { 255, 255, 255, 255 };

            // Read the data for a vertex from the "attrib" object
            if (index.vertex_index >= 0) {
                vertex.position = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                if (!attrib.colors.empty() && (3 * index.vertex_index + 2) < static_cast<int>(attrib.colors.size())) {
                    vertex.color = {
                        static_cast<glm::uint8>(std::clamp(attrib.colors[3 * index.vertex_index + 0], 0.0f, 1.0f) * 255.0f),
                        static_cast<glm::uint8>(std::clamp(attrib.colors[3 * index.vertex_index + 1], 0.0f, 1.0f) * 255.0f),
                        static_cast<glm::uint8>(std::clamp(attrib.colors[3 * index.vertex_index + 2], 0.0f, 1.0f) * 255.0f),
                        255
                    };
                }
            }

            if (index.normal_index >= 0 && !attrib.normals.empty() && (3 * index.normal_index + 2) < static_cast<int>(attrib.normals.size())) {
                vertex.normal = {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]
                };
            }

            if (index.texcoord_index >= 0 && !attrib.texcoords.empty() && (2 * index.texcoord_index + 1) < static_cast<int>(attrib.texcoords.size())) {
                vertex.tex_coord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    attrib.texcoords[2 * index.texcoord_index + 1]
                };
            }

            // See if we already stored a similar vertex
            auto it = vertex_map.find(vertex);
            if (it == vertex_map.end()) {
                // if no, add it to the vertices and record its index
                auto new_vertex_index = static_cast<GLuint>(vertices.size());
                vertex_map[vertex] = new_vertex_index;
                elements.push_back(new_vertex_index);
                vertices.push_back(vertex);
            }
            else {
                // if yes, just add its index in the elements vector
                elements.push_back(it->second);
            }
        }
    }

    return new our::Mesh(vertices, elements);
}

// Create a sphere (the vertex order in the triangles are CCW from the outside)
// Segments define the number of divisions on the both the latitude and the longitude
our::Mesh* our::mesh_utils::sphere(const glm::ivec2& segments) {
    std::vector<our::Vertex> vertices;
    std::vector<GLuint> elements;

    // We populate the sphere vertices by looping over its longitude and latitude
    for (int lat = 0; lat <= segments.y; lat++) {
        float v = (float)lat / segments.y;
        float pitch = v * glm::pi<float>() - glm::half_pi<float>();
        float cos = glm::cos(pitch), sin = glm::sin(pitch);
        for (int lng = 0; lng <= segments.x; lng++) {
            float u = (float)lng / segments.x;
            float yaw = u * glm::two_pi<float>();
            glm::vec3 normal = { cos * glm::cos(yaw), sin, cos * glm::sin(yaw) };
            glm::vec3 position = normal;
            glm::vec2 tex_coords = glm::vec2(u, v);
            our::Color color = our::Color(255, 255, 255, 255);
            vertices.push_back({ position, color, tex_coords, normal });
        }
    }

    for (int lat = 1; lat <= segments.y; lat++) {
        int start = lat * (segments.x + 1);
        for (int lng = 1; lng <= segments.x; lng++) {
            int prev_lng = lng - 1;
            elements.push_back(lng + start);
            elements.push_back(lng + start - segments.x - 1);
            elements.push_back(prev_lng + start - segments.x - 1);
            elements.push_back(prev_lng + start - segments.x - 1);
            elements.push_back(prev_lng + start);
            elements.push_back(lng + start);
        }
    }

    return new our::Mesh(vertices, elements);
}

namespace {

    void setVertexBoneData(our::Vertex& vertex, int boneID, float weight) {
        for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
            if (vertex.boneIDs[i] < 0) {
                vertex.boneIDs[i] = boneID;
                vertex.boneWeights[i] = weight;
                return;
            }
        }
        // All slots full, replace the smallest weight if this is bigger
        int minIdx = 0;
        for (int i = 1; i < MAX_BONE_INFLUENCE; ++i) {
            if (vertex.boneWeights[i] < vertex.boneWeights[minIdx]) {
                minIdx = i;
            }
        }
        if (weight > vertex.boneWeights[minIdx]) {
            vertex.boneIDs[minIdx] = boneID;
            vertex.boneWeights[minIdx] = weight;
        }
    }

    bool matricesNearlyEqual(const glm::mat4& a, const glm::mat4& b, float epsilon = 1e-4f) {
        for (int column = 0; column < 4; ++column) {
            for (int row = 0; row < 4; ++row) {
                if (std::abs(a[column][row] - b[column][row]) > epsilon) {
                    return false;
                }
            }
        }
        return true;
    }

    void normalizeVertexBoneWeights(
        std::vector<our::Vertex>& vertices,
        unsigned int vertexOffset,
        unsigned int vertexCount
    ) {
        const unsigned int end = std::min(vertexOffset + vertexCount, static_cast<unsigned int>(vertices.size()));
        for (unsigned int index = vertexOffset; index < end; ++index) {
            our::Vertex& vertex = vertices[index];

            float weightSum = 0.0f;
            for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
                if (vertex.boneIDs[i] >= 0 && vertex.boneWeights[i] > 0.0f) {
                    weightSum += vertex.boneWeights[i];
                }
            }

            if (weightSum > 0.0f) {
                const float invWeightSum = 1.0f / weightSum;
                for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
                    if (vertex.boneIDs[i] >= 0 && vertex.boneWeights[i] > 0.0f) {
                        vertex.boneWeights[i] *= invWeightSum;
                    }
                }
            }
        }
    }

    void extractBoneWeightForVertices(
        std::vector<our::Vertex>& vertices,
        unsigned int vertexOffset,
        const aiMesh* mesh,
        std::map<std::string, our::BoneInfo>& boneInfoMap,
        int& boneCounter
    ) {
        for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
            const aiBone* bone = mesh->mBones[boneIndex];
            if (!bone) continue;

            std::string boneName = bone->mName.C_Str();
            int boneID = -1;
            const glm::mat4 boneOffset = aiToGlm(bone->mOffsetMatrix);

            auto it = boneInfoMap.find(boneName);
            if (it == boneInfoMap.end()) {
                our::BoneInfo newBoneInfo;
                newBoneInfo.id = boneCounter;
                newBoneInfo.offset = boneOffset;
                boneInfoMap[boneName] = newBoneInfo;
                boneID = boneCounter;
                boneCounter++;
            }
            else {
                boneID = it->second.id;

                if (!matricesNearlyEqual(it->second.offset, boneOffset)) {
                    std::cerr << "[ANIM] WARNING: Bone offset mismatch for bone \"" << boneName
                        << "\" while processing mesh \"" << mesh->mName.C_Str()
                        << "\". Keeping first discovered offset matrix." << std::endl;
                }
            }

            for (unsigned int weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex) {
                unsigned int vertexId = bone->mWeights[weightIndex].mVertexId;
                float weight = bone->mWeights[weightIndex].mWeight;
                if (weight <= 0.0f) continue;
                unsigned int globalVertexId = vertexOffset + vertexId;
                if (globalVertexId < vertices.size()) {
                    setVertexBoneData(vertices[globalVertexId], boneID, weight);
                }
            }
        }

        std::cout << "[ANIM] Extracted bones from mesh \"" << mesh->mName.C_Str()
            << "\": " << mesh->mNumBones << " bones, total bone count now: " << boneCounter << std::endl;
    }

    void appendAssimpMeshAnimated(
        const aiMesh* mesh,
        const aiScene* scene,
        std::vector<our::Vertex>& vertices,
        std::vector<GLuint>& elements,
        std::vector<our::Mesh::DrawBatch>& drawBatches,
        AssimpBuildContext& context,
        std::map<std::string, our::BoneInfo>& boneInfoMap,
        int& boneCounter,
        const aiNode* node,
        const glm::mat4& nodeTransform
    ) {
        logMeshInfo(mesh, scene);

    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
    const GLuint texture = getMaterialTexture(material, scene, context.directory, context);
    const our::Color materialColor = (texture != context.whiteTexture)
        ? our::Color(255, 255, 255, 255)
        : getMaterialColor(material);

        const unsigned int vertexOffset = static_cast<unsigned int>(vertices.size());
        const GLuint firstIndex = static_cast<GLuint>(elements.size());

    bool hasBones = mesh->HasBones();
    int nodeBoneID = -1;

        if (!hasBones && node != nullptr) {
            std::string nodeName = node->mName.C_Str();
            auto it = boneInfoMap.find(nodeName);
            if (it == boneInfoMap.end()) {
                boneInfoMap[nodeName] = { boneCounter, glm::inverse(nodeTransform) };
                nodeBoneID = boneCounter++;
            }
            else {
                nodeBoneID = it->second.id;
            }
        }

        const glm::mat3 normalTransform = glm::transpose(glm::inverse(glm::mat3(nodeTransform)));

        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            our::Vertex vertex{};

            glm::vec3 position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
            vertex.position = hasBones ? position : glm::vec3(nodeTransform * glm::vec4(position, 1.0f));

            vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
            if (mesh->HasNormals()) {
                glm::vec3 normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
                vertex.normal = hasBones ? normal : glm::normalize(normalTransform * normal);
            }

            vertex.tex_coord = glm::vec2(0.0f, 0.0f);
            if (mesh->mTextureCoords[0]) {
                vertex.tex_coord = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
            }

            vertex.color = materialColor;
            if (mesh->HasVertexColors(0) && mesh->mColors[0]) {
                vertex.color = multiplyColor(materialColor, colorFromAssimp(mesh->mColors[0][i]));
            }

        for (int j = 0; j < MAX_BONE_INFLUENCE; j++) {
            vertex.boneIDs[j] = -1;
            vertex.boneWeights[j] = 0.0f;
        }

            if (!hasBones && nodeBoneID != -1) {
                vertex.boneIDs[0] = nodeBoneID;
                vertex.boneWeights[0] = 1.0f;
            }

            vertices.push_back(vertex);
        }

        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            const aiFace& face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                elements.push_back(vertexOffset + face.mIndices[j]);
            }
        }

    if (mesh->HasBones()) {
        extractBoneWeightForVertices(vertices, vertexOffset, mesh, boneInfoMap, boneCounter);
        normalizeVertexBoneWeights(vertices, vertexOffset, mesh->mNumVertices);
    }

        const GLsizei indexCount = static_cast<GLsizei>(elements.size() - firstIndex);
        if (indexCount > 0) {
            drawBatches.push_back({ static_cast<GLuint>(firstIndex), indexCount, texture, true });
        }
    }

    void processAssimpNodeAnimated(
        const aiNode* node,
        const aiScene* scene,
        const glm::mat4& parentTransform,
        std::vector<our::Vertex>& vertices,
        std::vector<GLuint>& elements,
        std::vector<our::Mesh::DrawBatch>& drawBatches,
        AssimpBuildContext& context,
        std::map<std::string, our::BoneInfo>& boneInfoMap,
        int& boneCounter
    ) {
        glm::mat4 nodeTransform = parentTransform * aiToGlm(node->mTransformation);

        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            appendAssimpMeshAnimated(mesh, scene, vertices, elements, drawBatches, context, boneInfoMap, boneCounter, node, nodeTransform);
        }

        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processAssimpNodeAnimated(node->mChildren[i], scene, nodeTransform, vertices, elements, drawBatches, context, boneInfoMap, boneCounter);
        }
    }

}

our::AnimatedMesh* our::mesh_utils::loadAnimatedMesh(const std::string& filename) {
    auto animMesh = new our::AnimatedMesh();
    animMesh->importer = std::make_unique<Assimp::Importer>();

    const aiScene* scene = animMesh->importer->ReadFile(
        filename,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_FlipUVs |
        aiProcess_LimitBoneWeights |
        aiProcess_CalcTangentSpace
    );

    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
        std::cerr << "[ANIM] ERROR::ASSIMP:: " << animMesh->importer->GetErrorString() << std::endl;
        delete animMesh;
        return nullptr;
    }

    animMesh->scene = scene;

    aiMatrix4x4 globalTransform = scene->mRootNode->mTransformation;
    animMesh->globalInverseTransform = glm::inverse(aiToGlm(globalTransform));

    std::cout << "[ANIM] Loading animated mesh from \"" << filename << "\"" << std::endl;
    std::cout << "[ANIM] Scene has " << scene->mNumAnimations << " animation(s), "
        << scene->mNumMeshes << " mesh(es)" << std::endl;

    AssimpBuildContext context;
    context.directory = std::filesystem::path(filename).parent_path().string();
    if (context.directory.empty()) {
        context.directory = ".";
    }
    context.whiteTexture = createSolidTexture(our::Color(255, 255, 255, 255));
    context.ownedTextures.push_back(context.whiteTexture);

    std::vector<our::Vertex> vertices;
    std::vector<GLuint> elements;
    std::vector<our::Mesh::DrawBatch> drawBatches;

    processAssimpNodeAnimated(scene->mRootNode, scene, glm::mat4(1.0f), vertices, elements, drawBatches, context,
        animMesh->boneInfoMap, animMesh->boneCounter);

    if (vertices.empty() || elements.empty()) {
        glDeleteTextures(static_cast<GLsizei>(context.ownedTextures.size()), context.ownedTextures.data());
        std::cerr << "[ANIM] Failed to build animated mesh buffers for: " << filename << std::endl;
        delete animMesh;
        return nullptr;
    }

    std::cout << "[ANIM] Animated mesh loaded: " << vertices.size() << " vertices, "
        << (elements.size() / 3) << " triangles, "
        << animMesh->boneCounter << " bones" << std::endl;

    animMesh->mesh = new our::Mesh(vertices, elements, drawBatches, context.ownedTextures);
    return animMesh;
}
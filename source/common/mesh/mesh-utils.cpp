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
#include <filesystem>
#include <iostream>
#include <limits>
#include <vector>
#include <unordered_map>

namespace {

glm::mat4 aiToGlm(const aiMatrix4x4& m) {
    glm::mat4 result;
    result[0][0] = m.a1; result[1][0] = m.a2; result[2][0] = m.a3; result[3][0] = m.a4;
    result[0][1] = m.b1; result[1][1] = m.b2; result[2][1] = m.b3; result[3][1] = m.b4;
    result[0][2] = m.c1; result[1][2] = m.c2; result[2][2] = m.c3; result[3][2] = m.c4;
    result[0][3] = m.d1; result[1][3] = m.d2; result[2][3] = m.d3; result[3][3] = m.d4;
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

    const unsigned char pixel[] = {color.r, color.g, color.b, color.a};
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
    if (!scene) {
        return 0;
    }

    std::string textureRef = texturePath.C_Str();
    if (textureRef.empty()) {
        return 0;
    }

    const aiTexture* embedded = scene->GetEmbeddedTexture(textureRef.c_str());
    if (!embedded) {
        return 0;
    }

    const std::string cacheKey = "embedded:" + textureRef;
    auto cached = context.textureCache.find(cacheKey);
    if (cached != context.textureCache.end()) {
        return cached->second;
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
    } else {
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
    context.textureCache[cacheKey] = context.whiteTexture;
    return 0;
}

our::Color getMaterialColor(const aiMaterial* material) {
    aiColor4D diffuse(1.0f, 1.0f, 1.0f, 1.0f);
    aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &diffuse);
    return colorFromAssimp(diffuse);
}

GLuint getMaterialTexture(const aiMaterial* material, const aiScene* scene, const std::string& directory, AssimpBuildContext& context) {
    aiString texturePath;
    bool hasTexture = false;

    if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS) {
        hasTexture = true;
    } else if (material->GetTexture(aiTextureType_BASE_COLOR, 0, &texturePath) == AI_SUCCESS) {
        hasTexture = true;
    }

    if (hasTexture) {
        const GLuint embeddedTexture = loadEmbeddedTexture(texturePath, scene, context);
        if (embeddedTexture != 0) {
            return embeddedTexture;
        }

        const std::string fullPath = resolveTexturePath(texturePath, directory);
        if (!fullPath.empty()) {
            return loadTexture2D(fullPath, context);
        }
    }

    return context.whiteTexture;
}

unsigned int countMaterialTextures(const aiMaterial* material) {
    unsigned int count = 0;
    count += material->GetTextureCount(aiTextureType_DIFFUSE);
    count += material->GetTextureCount(aiTextureType_BASE_COLOR);
    count += material->GetTextureCount(aiTextureType_SPECULAR);
    count += material->GetTextureCount(aiTextureType_NORMALS);
    count += material->GetTextureCount(aiTextureType_HEIGHT);
    count += material->GetTextureCount(aiTextureType_AMBIENT);
    return count;
}

void logMeshInfo(const aiMesh* mesh, const aiScene* scene) {
    std::cout << "\n--- Processing Mesh: " << mesh->mName.C_Str() << " ---" << std::endl;

    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
    aiString materialName;
    material->Get(AI_MATKEY_NAME, materialName);
    std::cout << "Material Name: " << materialName.C_Str() << std::endl;

    unsigned int textureCount = countMaterialTextures(material);
    if (textureCount == 0) {
        std::cout << "Mesh has NO textures assigned in material." << std::endl;
    } else {
        std::cout << "Mesh loaded with " << textureCount << " textures." << std::endl;
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
    const our::Color materialColor = getMaterialColor(material);
    const GLuint texture = getMaterialTexture(material, scene, context.directory, context);

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
        drawBatches.push_back({firstIndex, indexCount, texture, true});
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
    for (const auto &shape : shapes) {
        for (const auto &index : shape.mesh.indices) {
            if (index.vertex_index < 0) continue;

            Vertex vertex = {};

            vertex.position = {0.0f, 0.0f, 0.0f};
            vertex.normal = {0.0f, 1.0f, 0.0f};
            vertex.tex_coord = {0.0f, 0.0f};
            vertex.color = {255, 255, 255, 255};

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
            } else {
                // if yes, just add its index in the elements vector
                elements.push_back(it->second);
            }
        }
    }

    return new our::Mesh(vertices, elements);
}

// Create a sphere (the vertex order in the triangles are CCW from the outside)
// Segments define the number of divisions on the both the latitude and the longitude
our::Mesh* our::mesh_utils::sphere(const glm::ivec2& segments){
    std::vector<our::Vertex> vertices;
    std::vector<GLuint> elements;

    // We populate the sphere vertices by looping over its longitude and latitude
    for(int lat = 0; lat <= segments.y; lat++){
        float v = (float)lat / segments.y;
        float pitch = v * glm::pi<float>() - glm::half_pi<float>();
        float cos = glm::cos(pitch), sin = glm::sin(pitch);
        for(int lng = 0; lng <= segments.x; lng++){
            float u = (float)lng/segments.x;
            float yaw = u * glm::two_pi<float>();
            glm::vec3 normal = {cos * glm::cos(yaw), sin, cos * glm::sin(yaw)};
            glm::vec3 position = normal;
            glm::vec2 tex_coords = glm::vec2(u, v);
            our::Color color = our::Color(255, 255, 255, 255);
            vertices.push_back({position, color, tex_coords, normal});
        }
    }

    for(int lat = 1; lat <= segments.y; lat++){
        int start = lat*(segments.x+1);
        for(int lng = 1; lng <= segments.x; lng++){
            int prev_lng = lng-1;
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
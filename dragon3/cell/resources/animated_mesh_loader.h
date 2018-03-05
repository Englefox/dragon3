#ifndef CELL_RESOURCES_ANIMATED_MESH_LOADER_H
#define CELL_RESOURCES_ANIMATED_MESH_LOADER_H

#include <string>
#include <vector>
#include <map>
#include "../../lib/includes/assimp/Importer.hpp"
#include "../../lib/includes/assimp/scene.h"
#include "../../lib/includes/assimp/postprocess.h"
#include "../../utility/logging/log.h"
#include "../../math/math.h"
#include "../shading/texture.h"
#include "../shading/material.h"
#include "../scene/scene_node.h"
#include "../mesh/mesh.h"
#include "../renderer/renderer.h"
#include "../resources/resources.h"
#include "../shading/texture.h"

namespace Cell
{
	/*
	Animated Mesh load functionality.
	*/
	class AnimatedMeshLoader
	{
	private:
		static std::vector<Mesh*> ameshStore;
	public:
		void Clean();
		SceneNode* LoadAnimatedMesh(Renderer* renderer, std::string path, bool setDefaultMaterial = true);
		bool hasLoaded = false;
		unsigned int NumBones() const
		{
			return m_NumBones;
		}

		void BoneTransform(float TimeInSeconds, std::vector<math::mat4>& Transforms);

	public:
		struct BoneInfo
		{
			math::mat4 BoneOffset;
			math::mat4 FinalTransformation;

			BoneInfo()
			{
				memset(&BoneOffset, 0, sizeof(BoneOffset));
				memset(&FinalTransformation, 0, sizeof(FinalTransformation));
			}
		};

		SceneNode* processNode(Renderer* renderer, aiNode* aNode, const aiScene* aScene, std::string directory, bool setDefaultMaterial);

		Mesh* parseMesh(aiMesh* aMesh, const aiScene* aScene, math::vec3& out_Min, math::vec3& out_Max, unsigned int index);

		Material*  parseMaterial(Renderer* renderer, aiMaterial* aMaterial, const aiScene* aScene, std::string directory);

		std::string processPath(aiString* aPath, std::string directory);

		void CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
		void CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
		void CalcInterpolatedPosition(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
		unsigned int FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim);
		unsigned int FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim);
		unsigned int FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim);
		const aiNodeAnim* FindNodeAnim(const aiAnimation* pAnimation, const std::string NodeName);
		void ReadNodeHeirarchy(float AnimationTime, const aiNode* pNode, math::mat4& ParentTransform);

		enum VB_TYPES {
			INDEX_BUFFER,
			POS_VB,
			NORMAL_VB,
			TEXCOORD_VB,
			BONE_VB,
			NUM_VBs
		};
		GLuint m_VAO;
		GLuint m_Buffers[NUM_VBs];

		//for calculation of bones
		std::vector<VertexBoneData> m_bones;

        #define INVALID_MATERIAL 0xFFFFFFFF
		struct MeshEntry {
			MeshEntry()
			{
				NumIndices = 0;
				BaseVertex = 0;
				BaseIndex = 0;
				MaterialIndex = INVALID_MATERIAL;
			}

			unsigned int NumIndices;
			unsigned int BaseVertex;
			unsigned int BaseIndex;
			unsigned int MaterialIndex;
		};
		std::vector<MeshEntry> m_Entries;
		std::vector<Texture*> m_Textures;

		std::vector<math::vec3> m_Positions;
		std::vector<math::vec3> m_Normals;
		std::vector<math::vec2> m_TexCoords;
		std::vector<unsigned int> m_Indices;

		std::map<std::string, unsigned int> m_BoneMapping; //maps a bone name to its index
		unsigned int m_NumBones;
		std::vector<BoneInfo> m_BoneInfo;
		math::mat4 m_GlobalInverseTransform;

		const aiScene* m_pScene = nullptr;
		Assimp::Importer m_Importer;
	};
}
#endif

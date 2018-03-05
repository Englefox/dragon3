#include "animated_mesh_loader.h"

#define POSITION_LOCATION    0
#define TEX_COORD_LOCATION   1
#define NORMAL_LOCATION      2
#define BONE_ID_LOCATION     3
#define BONE_WEIGHT_LOCATION 4

namespace Cell
{
	std::vector<Mesh*> AnimatedMeshLoader::ameshStore = std::vector<Mesh*>();
	void AnimatedMeshLoader::Clean()
	{
		for (unsigned int i = 0; i < AnimatedMeshLoader::ameshStore.size(); ++i)
		{
			delete AnimatedMeshLoader::ameshStore[i];
		}
	}

	SceneNode* AnimatedMeshLoader::LoadAnimatedMesh(Renderer* renderer, std::string path, bool setDefaultMaterial)
	{
		Log::Message("Loading mesh file at: " + path + ".", LOG_INIT);

		m_pScene = m_Importer.ReadFile(path, aiProcess_Triangulate | aiProcess_CalcTangentSpace);

		if (!m_pScene || m_pScene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !m_pScene->mRootNode)
		{
			Log::Message("Assimp failed to load model at path: " + path, LOG_ERROR);
			return nullptr;
		}
		hasLoaded = true;
		aiMatrix4x4 T = m_pScene->mRootNode->mTransformation;
		m_GlobalInverseTransform[0][0] = T.a1;
		m_GlobalInverseTransform[0][1] = T.b1;
		m_GlobalInverseTransform[0][2] = T.c1;
		m_GlobalInverseTransform[0][3] = T.d1;
		m_GlobalInverseTransform[1][0] = T.a2;
		m_GlobalInverseTransform[1][1] = T.b2;
		m_GlobalInverseTransform[1][2] = T.c2;
		m_GlobalInverseTransform[1][3] = T.d2;
		m_GlobalInverseTransform[2][0] = T.a3;
		m_GlobalInverseTransform[2][1] = T.b3;
		m_GlobalInverseTransform[2][2] = T.c3;
		m_GlobalInverseTransform[2][3] = T.d3;
		m_GlobalInverseTransform[3][0] = T.a4;
		m_GlobalInverseTransform[3][1] = T.b4;
		m_GlobalInverseTransform[3][2] = T.c4;
		m_GlobalInverseTransform[3][3] = T.d4;
		math::inverse(m_GlobalInverseTransform);
		
		std::string directory = path.substr(0, path.find_last_of("/"));

		Log::Message("Succesfully loaded: " + path + ".", LOG_INIT);

		//for bone calculation
		m_Entries.resize(m_pScene->mNumMeshes);
		m_Textures.resize(m_pScene->mNumMeshes);
		unsigned int NumVertices = 0;
		unsigned int NumIndices = 0;
		for (unsigned int i = 0; i < m_Entries.size(); i++)
		{
			m_Entries[i].MaterialIndex = m_pScene->mMeshes[i]->mMaterialIndex;
			m_Entries[i].NumIndices = m_pScene->mMeshes[i]->mNumFaces * 3;
			m_Entries[i].BaseVertex = NumVertices;
			m_Entries[i].BaseIndex = NumIndices;

			NumVertices += m_pScene->mMeshes[i]->mNumVertices;
			NumIndices += m_Entries[i].NumIndices;
		}
		m_bones.resize(NumVertices);
		m_Positions.reserve(NumVertices);
		m_Normals.reserve(NumVertices);
		m_TexCoords.reserve(NumVertices);
		m_Indices.reserve(NumVertices);

		return AnimatedMeshLoader::processNode(renderer, m_pScene->mRootNode, m_pScene, directory, setDefaultMaterial);
	}

	SceneNode* AnimatedMeshLoader::processNode(Renderer* renderer, aiNode* aNode, const aiScene* aScene, std::string directory, bool setDefaultMaterial)
	{

		//usual ones start here
		SceneNode* node = new SceneNode(0);

		for (unsigned int i = 0; i < aNode->mNumMeshes; ++i)
		{
			math::vec3 boxMin, boxMax;
			aiMesh*     assimpMesh = aScene->mMeshes[aNode->mMeshes[i]];
			aiMaterial* assimpMat = aScene->mMaterials[assimpMesh->mMaterialIndex];
			Mesh*       mesh = AnimatedMeshLoader::parseMesh(assimpMesh, aScene, boxMin, boxMax, i);
			Material*   material = nullptr;
			if (setDefaultMaterial)
			{
				material = AnimatedMeshLoader::parseMaterial(renderer, assimpMat, aScene, directory);
			}

			// if we only have one mesh, this node itself contains the mesh/material.
			if (aNode->mNumMeshes == 1)
			{
				node->Mesh = mesh;
				if (setDefaultMaterial)
				{
					node->Material = material;
				}
				node->BoxMin = boxMin;
				node->BoxMax = boxMax;
			}
			// otherwise, the meshes are considered on equal depth of its children
			else
			{
				SceneNode* child = new SceneNode(0);
				child->Mesh = mesh;
				child->Material = material;
				child->BoxMin = boxMin;
				child->BoxMax = boxMax;
				node->AddChild(child);
			}
		}

		for (unsigned int i = 0; i < aNode->mNumChildren; ++i)
		{
			node->AddChild(AnimatedMeshLoader::processNode(renderer, aNode->mChildren[i], aScene, directory, setDefaultMaterial));
		}
		node->aMesh = true;
		
		// Generate and populate the buffers with vertex attributes and the indices
		glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[POS_VB]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(m_Positions[0]) * m_Positions.size(), &m_Positions[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(POSITION_LOCATION);
		glVertexAttribPointer(POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[TEXCOORD_VB]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(m_TexCoords[0]) * m_TexCoords.size(), &m_TexCoords[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(TEX_COORD_LOCATION);
		glVertexAttribPointer(TEX_COORD_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[NORMAL_VB]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(m_Normals[0]) * m_Normals.size(), &m_Normals[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(NORMAL_LOCATION);
		glVertexAttribPointer(NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[BONE_VB]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(m_bones[0]) * m_bones.size(), &m_bones[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(BONE_ID_LOCATION);
		glVertexAttribIPointer(BONE_ID_LOCATION, 4, GL_INT, sizeof(VertexBoneData), (const GLvoid*)0);
		glEnableVertexAttribArray(BONE_WEIGHT_LOCATION);
		glVertexAttribPointer(BONE_WEIGHT_LOCATION, 4, GL_FLOAT, GL_FALSE, sizeof(VertexBoneData), (const GLvoid*)16);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Buffers[INDEX_BUFFER]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(m_Indices[0]) * m_Indices.size(), &m_Indices[0], GL_STATIC_DRAW);
		return node;
	}

	Mesh* AnimatedMeshLoader::parseMesh(aiMesh* aMesh, const aiScene* aScene, math::vec3& out_Min, math::vec3& out_Max, unsigned int index)
	{
		std::vector<math::vec3> tangents;
		std::vector<math::vec3> bitangents;

		// store min/max point in local coordinates for calculating approximate bounding box.
		math::vec3 pMin(99999.0);
		math::vec3 pMax(-99999.0);

		for (unsigned int i = 0; i < aMesh->mNumVertices; ++i)
		{
			m_Positions.push_back(math::vec3(aMesh->mVertices[i].x, aMesh->mVertices[i].y, aMesh->mVertices[i].z));
			if ((aMesh->mNormals) == nullptr)
				continue;
			else
			    m_Normals.push_back(math::vec3(aMesh->mNormals[i].x, aMesh->mNormals[i].y, aMesh->mNormals[i].z));
			if (aMesh->mTextureCoords[0])
			{
				m_TexCoords.push_back(math::vec2(aMesh->mTextureCoords[0][i].x, aMesh->mTextureCoords[0][i].y));

			}
			if (aMesh->mTangents)
			{
				tangents[i] = math::vec3(aMesh->mTangents[i].x, aMesh->mTangents[i].y, aMesh->mTangents[i].z);
				bitangents[i] = math::vec3(aMesh->mBitangents[i].x, aMesh->mBitangents[i].y, aMesh->mBitangents[i].z);
			}
			//if (m_Positions[i].x < pMin.x) pMin.x = m_Positions[i].x;
			//if (m_Positions[i].y < pMin.y) pMin.y = m_Positions[i].y;
			//if (m_Positions[i].z < pMin.z) pMin.z = m_Positions[i].z;
			//if (m_Positions[i].x > pMax.x) pMax.x = m_Positions[i].x;
			//if (m_Positions[i].y > pMax.y) pMax.y = m_Positions[i].y;
			//if (m_Positions[i].z > pMax.z) pMax.z = m_Positions[i].z;
		}
		for (unsigned int f = 0; f < aMesh->mNumFaces; ++f)
		{
			// we know we're always working with triangles due to TRIANGULATE option.
			for (unsigned int i = 0; i < 3; ++i)
			{
				m_Indices.push_back(aMesh->mFaces[f].mIndices[i]);
			}
		}
		//bones
		for (unsigned int i = 0; i < aMesh->mNumBones; i++)
		{
			unsigned int BoneIndex = 0;
			std::string BoneName(aMesh->mBones[i]->mName.data);

			if (m_BoneMapping.find(BoneName) == m_BoneMapping.end())
			{
				//Allocate an index for a new bone
				BoneIndex = m_NumBones;
				m_NumBones++;
				BoneInfo bi;
				m_BoneInfo.push_back(bi);
				aiMatrix4x4 T = aMesh->mBones[i]->mOffsetMatrix;
				m_BoneInfo[BoneIndex].BoneOffset[0][0] = T.a1;
				m_BoneInfo[BoneIndex].BoneOffset[0][1] = T.b1;
				m_BoneInfo[BoneIndex].BoneOffset[0][2] = T.c1;
				m_BoneInfo[BoneIndex].BoneOffset[0][3] = T.d1;
				m_BoneInfo[BoneIndex].BoneOffset[1][0] = T.a2;
				m_BoneInfo[BoneIndex].BoneOffset[1][1] = T.b2;
				m_BoneInfo[BoneIndex].BoneOffset[1][2] = T.c2;
				m_BoneInfo[BoneIndex].BoneOffset[1][3] = T.d2;
				m_BoneInfo[BoneIndex].BoneOffset[2][0] = T.a3;
				m_BoneInfo[BoneIndex].BoneOffset[2][1] = T.b3;
				m_BoneInfo[BoneIndex].BoneOffset[2][2] = T.c3;
				m_BoneInfo[BoneIndex].BoneOffset[2][3] = T.d3;
				m_BoneInfo[BoneIndex].BoneOffset[3][0] = T.a4;
				m_BoneInfo[BoneIndex].BoneOffset[3][1] = T.b4;
				m_BoneInfo[BoneIndex].BoneOffset[3][2] = T.c4;
				m_BoneInfo[BoneIndex].BoneOffset[3][3] = T.d4;
				m_BoneMapping[BoneName] = BoneIndex;
			}
			else
				BoneIndex = m_BoneMapping[BoneName];

			for (unsigned int j = 0; j < aMesh->mBones[i]->mNumWeights; j++)
			{
				unsigned int VertexID = m_Entries[index].BaseVertex + aMesh->mBones[i]->mWeights[j].mVertexId;
				float Weight = aMesh->mBones[i]->mWeights[j].mWeight;
				m_bones[VertexID].AddBoneData(BoneIndex, Weight);
			}
		}

		Mesh *mesh = new Mesh;
		mesh->Bones = m_bones;
		//mesh->Positions = positions;
		//mesh->UV = uv;
		//mesh->Normals = normals;
		mesh->Tangents = tangents;
		mesh->Bitangents = bitangents;
		//mesh->Indices = indices;
		mesh->Topology = TRIANGLES;
		mesh->Finalize(true);

		out_Min.x = pMin.x;
		out_Min.y = pMin.y;
		out_Min.z = pMin.z;
		out_Max.x = pMax.x;
		out_Max.y = pMax.y;
		out_Max.z = pMax.z;

		// store newly generated mesh in globally stored mesh store for memory de-allocation when 
		// a clean is required.
		AnimatedMeshLoader::ameshStore.push_back(mesh);
		return mesh;
	}

	Material* AnimatedMeshLoader::parseMaterial(Renderer* renderer, aiMaterial* aMaterial, const aiScene* aScene, std::string directory)
	{
		// create a unique default material for each loaded mesh.     
		Material* material;
		// check if diffuse texture has alpha, if so: make alpha blend material; 
		aiString file;
		aMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &file);
		std::string diffPath = std::string(file.C_Str());
		bool alpha = false;
		if (diffPath.find("_alpha") != std::string::npos)
		{
			material = renderer->CreateMaterial("alpha discard");
			alpha = true;
		}
		else  // else, make default deferred material
		{
			material = renderer->CreateMaterial();
		}

		/* NOTE(Joey):

		About texture types:

		We use a PBR metallic/roughness workflow so the loaded models are expected to have
		textures conform the workflow: albedo, (normal), metallic, roughness, (ao). Since Assimp
		made certain assumptions regarding possible types of loaded textures it doesn't directly
		translate to our model thus we make some assumptions as well which the 3D author has to
		comply with if he wants the mesh(es) to directly render with its specified textures:

		- aiTextureType_DIFFUSE:   Albedo
		- aiTextureType_NORMALS:   Normal
		- aiTextureType_SPECULAR:  metallic
		- aiTextureType_SHININESS: roughness
		- aiTextureType_AMBIENT:   AO (ambient occlusion)
		- aiTextureType_EMISSIVE:  Emissive

		*/
		if (aMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0)
		{
			// we only load the first of the list of diffuse textures, we don't really care about 
			// meshes with multiple diffuse layers; same holds for other texture types.
			aiString file;
			aMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &file);
			std::string fileName = AnimatedMeshLoader::processPath(&file, directory);
			// we name the texture the same as the filename as to reduce naming conflicts while 
			// still only loading unique textures.
			Texture* texture = Resources::LoadTexture(fileName, fileName, GL_TEXTURE_2D, alpha ? GL_RGBA : GL_RGB, true);
			if (texture)
			{
				material->SetTexture("TexAlbedo", texture, 3);
			}
			m_Textures.push_back(texture);
		}
		if (aMaterial->GetTextureCount(aiTextureType_DISPLACEMENT) > 0)
		{
			aiString file;
			aMaterial->GetTexture(aiTextureType_DISPLACEMENT, 0, &file);
			std::string fileName = AnimatedMeshLoader::processPath(&file, directory);

			Texture* texture = Resources::LoadTexture(fileName, fileName);
			if (texture)
			{
				material->SetTexture("TexNormal", texture, 4);
			}
		}
		if (aMaterial->GetTextureCount(aiTextureType_SPECULAR) > 0)
		{
			aiString file;
			aMaterial->GetTexture(aiTextureType_SPECULAR, 0, &file);
			std::string fileName = AnimatedMeshLoader::processPath(&file, directory);

			Texture* texture = Resources::LoadTexture(fileName, fileName);
			if (texture)
			{
				material->SetTexture("TexMetallic", texture, 5);
			}
		}
		if (aMaterial->GetTextureCount(aiTextureType_SHININESS) > 0)
		{
			aiString file;
			aMaterial->GetTexture(aiTextureType_SHININESS, 0, &file);
			std::string fileName = AnimatedMeshLoader::processPath(&file, directory);

			Texture* texture = Resources::LoadTexture(fileName, fileName);
			if (texture)
			{
				material->SetTexture("TexRoughness", texture, 6);
			}
		}
		if (aMaterial->GetTextureCount(aiTextureType_AMBIENT) > 0)
		{
			aiString file;
			aMaterial->GetTexture(aiTextureType_AMBIENT, 0, &file);
			std::string fileName = AnimatedMeshLoader::processPath(&file, directory);

			Texture* texture = Resources::LoadTexture(fileName, fileName);
			if (texture)
			{
				material->SetTexture("TexAO", texture, 7);
			}
		}

		return material;
	}

	std::string AnimatedMeshLoader::processPath(aiString* aPath, std::string directory)
	{
		std::string path = std::string(aPath->C_Str());
		// parse path directly if path contains "/" indicating it is an absolute path;  otherwise 
		// parse as relative.
		if (path.find(":/") == std::string::npos || path.find(":\\") == std::string::npos)
			path = directory + "/" + path;
		return path;
	}

	unsigned int AnimatedMeshLoader::FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim)
	{
		for (unsigned int i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++) {
			if (AnimationTime < (float)pNodeAnim->mPositionKeys[i + 1].mTime) {
				return i;
			}
		}

		assert(0);

		return 0;
	}


	unsigned int AnimatedMeshLoader::FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim)
	{
		assert(pNodeAnim->mNumRotationKeys > 0);

		for (unsigned int i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++) {
			if (AnimationTime < (float)pNodeAnim->mRotationKeys[i + 1].mTime) {
				return i;
			}
		}

		assert(0);

		return 0;
	}


	unsigned int AnimatedMeshLoader::FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim)
	{
		assert(pNodeAnim->mNumScalingKeys > 0);

		for (unsigned int i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++) {
			if (AnimationTime < (float)pNodeAnim->mScalingKeys[i + 1].mTime) {
				return i;
			}
		}

		assert(0);

		return 0;
	}


	void AnimatedMeshLoader::CalcInterpolatedPosition(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
	{
		if (pNodeAnim->mNumPositionKeys == 1) {
			Out = pNodeAnim->mPositionKeys[0].mValue;
			return;
		}

		unsigned int PositionIndex = FindPosition(AnimationTime, pNodeAnim);
		unsigned int NextPositionIndex = (PositionIndex + 1);
		assert(NextPositionIndex < pNodeAnim->mNumPositionKeys);
		float DeltaTime = (float)(pNodeAnim->mPositionKeys[NextPositionIndex].mTime - pNodeAnim->mPositionKeys[PositionIndex].mTime);
		float Factor = (AnimationTime - (float)pNodeAnim->mPositionKeys[PositionIndex].mTime) / DeltaTime;
		assert(Factor >= 0.0f && Factor <= 1.0f);
		const aiVector3D& Start = pNodeAnim->mPositionKeys[PositionIndex].mValue;
		const aiVector3D& End = pNodeAnim->mPositionKeys[NextPositionIndex].mValue;
		aiVector3D Delta = End - Start;
		Out = Start + Factor * Delta;
	}


	void AnimatedMeshLoader::CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
	{
		// we need at least two values to interpolate...
		if (pNodeAnim->mNumRotationKeys == 1) {
			Out = pNodeAnim->mRotationKeys[0].mValue;
			return;
		}

		unsigned int RotationIndex = FindRotation(AnimationTime, pNodeAnim);
		unsigned int NextRotationIndex = (RotationIndex + 1);
		assert(NextRotationIndex < pNodeAnim->mNumRotationKeys);
		float DeltaTime = (float)(pNodeAnim->mRotationKeys[NextRotationIndex].mTime - pNodeAnim->mRotationKeys[RotationIndex].mTime);
		float Factor = (AnimationTime - (float)pNodeAnim->mRotationKeys[RotationIndex].mTime) / DeltaTime;
		assert(Factor >= 0.0f && Factor <= 1.0f);
		const aiQuaternion& StartRotationQ = pNodeAnim->mRotationKeys[RotationIndex].mValue;
		const aiQuaternion& EndRotationQ = pNodeAnim->mRotationKeys[NextRotationIndex].mValue;
		aiQuaternion::Interpolate(Out, StartRotationQ, EndRotationQ, Factor);
		Out = Out.Normalize();
	}


	void AnimatedMeshLoader::CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
	{
		if (pNodeAnim->mNumScalingKeys == 1) {
			Out = pNodeAnim->mScalingKeys[0].mValue;
			return;
		}

		unsigned int ScalingIndex = FindScaling(AnimationTime, pNodeAnim);
		unsigned int NextScalingIndex = (ScalingIndex + 1);
		assert(NextScalingIndex < pNodeAnim->mNumScalingKeys);
		float DeltaTime = (float)(pNodeAnim->mScalingKeys[NextScalingIndex].mTime - pNodeAnim->mScalingKeys[ScalingIndex].mTime);
		float Factor = (AnimationTime - (float)pNodeAnim->mScalingKeys[ScalingIndex].mTime) / DeltaTime;
		assert(Factor >= 0.0f && Factor <= 1.0f);
		const aiVector3D& Start = pNodeAnim->mScalingKeys[ScalingIndex].mValue;
		const aiVector3D& End = pNodeAnim->mScalingKeys[NextScalingIndex].mValue;
		aiVector3D Delta = End - Start;
		Out = Start + Factor * Delta;
	}


	void AnimatedMeshLoader::ReadNodeHeirarchy(float AnimationTime, const aiNode* pNode, math::mat4& ParentTransform)
	{
		std::string NodeName(pNode->mName.data);

		const aiAnimation* pAnimation = m_pScene->mAnimations[0];

		math::mat4 NodeTransformation;
		aiMatrix4x4 T = pNode->mTransformation;


		const aiNodeAnim* pNodeAnim = FindNodeAnim(pAnimation, NodeName);

		if (pNodeAnim) {
			// Interpolate scaling and generate scaling transformation matrix
			aiVector3D Scaling;
			CalcInterpolatedScaling(Scaling, AnimationTime, pNodeAnim);
			math::mat4 ScalingM = scale(math::vec3(Scaling.x, Scaling.y, Scaling.z));

			// Interpolate rotation and generate rotation transformation matrix
			aiQuaternion RotationQ;
			CalcInterpolatedRotation(RotationQ, AnimationTime, pNodeAnim);
			math::mat4 RotationM;
			aiMatrix3x3 T = RotationQ.GetMatrix();
		    RotationM[0][0] = T.a1;
			RotationM[0][1] = T.b1;
			RotationM[0][2] = T.c1;
			RotationM[0][3] = 0;
			RotationM[1][0] = T.a2;
			RotationM[1][1] = T.b2;
			RotationM[1][2] = T.c2;
			RotationM[1][3] = 0;
			RotationM[2][0] = T.a3;
			RotationM[2][1] = T.b3;
			RotationM[2][2] = T.c3;
			RotationM[2][3] = 0;
			RotationM[3][0] = 0;
			RotationM[3][1] = 0;
			RotationM[3][2] = 0;
			RotationM[3][3] = 1;

			// Interpolate translation and generate translation transformation matrix
			aiVector3D Translation;
			CalcInterpolatedPosition(Translation, AnimationTime, pNodeAnim);
			math::mat4 TranslationM = translate(math::vec3(Translation.x, Translation.y, Translation.z));

			// Combine the above transformations
			NodeTransformation = TranslationM * RotationM * ScalingM;
		}

		math::mat4 GlobalTransformation = ParentTransform * NodeTransformation;

		if (m_BoneMapping.find(NodeName) != m_BoneMapping.end()) {
			unsigned int BoneIndex = m_BoneMapping[NodeName];
			m_BoneInfo[BoneIndex].FinalTransformation = m_GlobalInverseTransform * GlobalTransformation * m_BoneInfo[BoneIndex].BoneOffset;
		}

		for (unsigned int i = 0; i < pNode->mNumChildren; i++) {
			ReadNodeHeirarchy(AnimationTime, pNode->mChildren[i], GlobalTransformation);
		}
	}


	void AnimatedMeshLoader::BoneTransform(float TimeInSeconds, std::vector<math::mat4>& Transforms)
	{
		math::mat4 Identity;

		float TicksPerSecond = (float)(m_pScene->mAnimations[0]->mTicksPerSecond != 0 ? m_pScene->mAnimations[0]->mTicksPerSecond : 25.0f);
		float TimeInTicks = TimeInSeconds * TicksPerSecond;
		float AnimationTime = fmod(TimeInTicks, (float)m_pScene->mAnimations[0]->mDuration);

		ReadNodeHeirarchy(AnimationTime, m_pScene->mRootNode, Identity);

		Transforms.resize(m_NumBones);

		for (unsigned int i = 0; i < m_NumBones; i++) {
			Transforms[i] = m_BoneInfo[i].FinalTransformation;
		}
	}


	const aiNodeAnim* AnimatedMeshLoader::FindNodeAnim(const aiAnimation* pAnimation, const std::string NodeName)
	{
		for (unsigned int i = 0; i < pAnimation->mNumChannels; i++) {
			const aiNodeAnim* pNodeAnim = pAnimation->mChannels[i];

			if (std::string(pNodeAnim->mNodeName.data) == NodeName) {
				return pNodeAnim;
			}
		}

		return NULL;
	}
}
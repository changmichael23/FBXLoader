#include "fbxsdk.h"
#include <GL/glew.h>
#include <iostream>

class FBXLoader
{
public:
	FbxAMatrix finalGlobalTransform;
	int nb_vertices;
	int nb_indices;
	int nb_normals;
	int nb_uvs;

	GLfloat* vertex_pos;
	GLuint* vertex_ind;
	GLfloat* vertex_nor;

	GLfloat* vertex_uv;

	GLuint v_vbo;
	GLuint i_vbo;

	GLuint VAO;

	FbxScene* scene;
	FbxMesh* pMesh;

	FbxLongLong startFrame;
	FbxLongLong stopFrame;

	//FbxMesh* g_Mesh;

	struct DeformerInfluence
	{
		std::vector<char> jointIndex;
		std::vector<float> weights;
	};

	struct Skeleton
	{
		struct Bone {
			std::string name;
			FbxNode* node;
		};
		std::vector<Bone> bones;
	} g_Skeleton;

	struct Mesh
	{
		std::vector<glm::mat4> bindPose;
	} g_Mesh;

	struct Joint
	{
		glm::mat4 transform;
	};
	struct Animation
	{
		std::vector<Joint> keyframes;
	};

	FBXLoader(GLuint VVBO, GLuint IVBO, GLuint VAO)
	{
		v_vbo = VVBO;
		i_vbo = IVBO;
		this->VAO = VAO;
	}


	~FBXLoader(void)
	{
	}

	void LoadFBX()
	{
		FbxManager *g_fbxManager = FbxManager::Create();
		FbxIOSettings *ioSettings = FbxIOSettings::Create(g_fbxManager, IOSROOT);
		g_fbxManager->SetIOSettings(ioSettings);

		scene = FbxScene::Create(g_fbxManager, "Ma Scene");
		FbxImporter *importer = FbxImporter::Create(g_fbxManager, "");
		bool status = importer->Initialize("../samples/ThirdPersonWalk.FBX", -1, g_fbxManager->GetIOSettings());
		status = importer->Import(scene);
		importer->Destroy();
		FbxNode* rootNode = scene->GetRootNode();

		// On compare le repère de la scene avec le repere souhaite
		FbxAxisSystem SceneAxisSystem = scene->GetGlobalSettings().GetAxisSystem();
		FbxAxisSystem OurAxisSystem(FbxAxisSystem::eYAxis, FbxAxisSystem::eParityOdd, FbxAxisSystem::eRightHanded);
		OurAxisSystem.ConvertScene(scene);
		if (SceneAxisSystem != OurAxisSystem) {
			OurAxisSystem.ConvertScene(scene);
		}

		FbxSystemUnit SceneSystemUnit = scene->GetGlobalSettings().GetSystemUnit();
		// L'unite standard du Fbx est le centimetre, que l'on peut tester ainsi
		if (SceneSystemUnit != FbxSystemUnit::cm) {
			printf("[warning] SystemUnity vaut %f cm!\n", SceneSystemUnit.GetScaleFactor());
		}


		if (rootNode)
		{
			//// Matrices geometriques, différentes pour chaque mesh ---

			//FbxVector4 translation = rootNode->GetChild(0)->GetGeometricTranslation(FbxNode::eSourcePivot);
			//FbxVector4 rotation = rootNode->GetChild(0)->GetGeometricRotation(FbxNode::eSourcePivot);
			//FbxVector4 scaling = rootNode->GetChild(0)->GetGeometricScaling(FbxNode::eSourcePivot);
			//FbxAMatrix geometryTransform;
			//geometryTransform.SetTRS(translation, rotation, scaling);

			//// etape 2. on recupere la matrice global (world) du mesh
			//FbxAMatrix globalTransform = rootNode->GetChild(0)->EvaluateGlobalTransform();

			//// etape 3. on concatene les deux matrices, ce qui donne la matrice world finale
			//finalGlobalTransform = globalTransform * geometryTransform;
			ProcessNode(rootNode, rootNode);
			FbxTimeSpan timeSpan = FbxTimeSpan();
			//ProcessSkeleton(rootNode, rootNode, timeSpan);
			FbxAnimStack* anim = GetAnimAtIndex(rootNode, 0);
			int u = 0;
		}
		//g_fbxManager->Destroy();



	}

	void ProcessSkeleton(FbxNode* node, FbxNode* parent, FbxTimeSpan animInterval)
	{
		FbxNodeAttribute* att = node->GetNodeAttribute();

		if (att != NULL)
		{
			FbxNodeAttribute::EType type = att->GetAttributeType();
			if (type == FbxNodeAttribute::eSkeleton)
			{
				//FbxSkeleton* skeleton = node->GetSkeleton();
				Skeleton::Bone bone;
				bone.node = node;
				bone.name = node->GetName();
				g_Skeleton.bones.push_back(bone);
			}
		}
		// demander imbrication node, skeleton
		int childCount = node->GetChildCount();
		for (int i = 0; i < childCount; i++) {
			FbxNode* child = node->GetChild(i);
			ProcessSkeleton(child, node, animInterval);
		}
	}

	void GetBindPose()
	{
		// Recupere le nombre de "deformer"s de type "skin" (pour le skinning / rigging).
		// Techniquement il n'est cense n'y'en avoir qu'un seul qui va nous servir a determiner la "bind pose" ou "pose de reference".
		// Par la suite on va ensuite se servir de chaque "joint" de la bind pose pour transformer les vertices du repere global du joint vers son repere local.
		// On va donc s'interesser surtout a la matrice INVERSE du joint.
		int skinCount = pMesh->GetDeformerCount(FbxDeformer::eSkin);
		FbxSkin* skin;
		auto skinIndex = 0;
		int jointsCount = 0;
		int boneCount = g_Skeleton.bones.size();

		if (skinCount)
		{
			skin = (FbxSkin *)pMesh->GetDeformer(skinIndex, FbxDeformer::eSkin);
			jointsCount = skin->GetClusterCount();
			g_Mesh.bindPose.resize(boneCount);
		}

		std::vector<DeformerInfluence> influences(pMesh->GetControlPointsCount());

		for (auto clusterIndex = 0; clusterIndex < jointsCount; clusterIndex++)
		{
			FbxCluster* cluster = skin->GetCluster(clusterIndex);

			// 1. Traitement de la bind pose

			FbxNode* link = cluster->GetLink();
			int jointIndex = 0;
			// on verifie que ce node fait bien parti de notre squelette
			for (; jointIndex < g_Skeleton.bones.size(); jointIndex++) {
				if (g_Skeleton.bones[jointIndex].node == link)
					break;
			}

			FbxAMatrix transformLinkMatrix;
			cluster->GetTransformLinkMatrix(transformLinkMatrix);
			transformLinkMatrix = transformLinkMatrix.Inverse();

			glm::mat4 bindPoseJointMatrix;
			for (auto i = 0; i < 4; ++i) {
				FbxVector4 col = transformLinkMatrix.GetRow(i);
				for (auto j = 0; j < 4; ++j)
					bindPoseJointMatrix[i][j] = (float)col.mData[j];
			}
			g_Mesh.bindPose[jointIndex] = bindPoseJointMatrix;

			// 2. Traitement des influences

			int influenceCount = cluster->GetControlPointIndicesCount();
			int* influenceIndices = cluster->GetControlPointIndices();
			double* influenceWeights = cluster->GetControlPointWeights();

			for (int influenceIndex = 0; influenceIndex < influenceCount; influenceIndex++)
			{
				int controlPointIndex = influenceIndices[influenceIndex];				
				influences[controlPointIndex].jointIndex.emplace_back(jointIndex);		
				influences[controlPointIndex].weights.emplace_back((float)influenceWeights[influenceIndex]);
			}

			// 3. Traitement des animations (peut se faire en à part, mais plus pratique ici)
			// toutes les keyframes des bones du squelette à un instant t
			
			FbxTime evalTime = 0;
			Animation animation;
			for (auto frame = startFrame; frame <= stopFrame; frame++)
			{
				evalTime.SetFrame(frame);
				auto& currentKey = animation.keyframes[frame];

				FbxAMatrix jointTransform = cluster->GetLink()->EvaluateGlobalTransform(evalTime);

				// une simple structure qui stocke un glm::mat4				
				Joint jointData;
				for (auto i = 0; i < 4; ++i) {
					FbxVector4 col = jointTransform.GetRow(i);
					for (auto j = 0; j < 4; ++j)
						jointData.transform[i][j] = (float)col.mData[j];
				}

				currentKey.joints[jointIndex] = jointData;
			}
		}
	}

	int GetFrameCount(FbxTimeSpan animInterval)
	{
		startFrame = animInterval.GetStart().GetFrameCount();
		stopFrame = animInterval.GetStop().GetFrameCount();
		//int keyframeCount = (stopFrame - startFrame + 1);

		return (stopFrame - startFrame + 1);
	}

	FbxAnimStack* GetAnimAtIndex(FbxNode* rootNode, int index)
	{
		FbxTimeSpan animInterval;
		rootNode->GetAnimationInterval(animInterval);

		int animStackCount = scene->GetSrcObjectCount<FbxAnimStack>();
		FbxAnimStack* animStack = scene->GetSrcObject<FbxAnimStack>(0);
		if (animStack)
		{
			FbxString animStackName = animStack->GetName();
			FbxTakeInfo* takeInfo = scene->GetTakeInfo(animStackName);
			FbxTime start = takeInfo->mLocalTimeSpan.GetStart();
			FbxTime end = takeInfo->mLocalTimeSpan.GetStop();
			FbxLongLong animduration = end.GetFrameCount(FbxTime::eFrames30) - start.GetFrameCount(FbxTime::eFrames30);

			ProcessSkeleton(rootNode, nullptr, animInterval);
		}

		GetBindPose();

		return animStack;
	}

	void ProcessNode(FbxNode* node, FbxNode* parentNode)
	{
		int nbChildren = node->GetChildCount();
		FbxNode* childNode = 0;
		for (int n = 0; n < nbChildren; n++)
		{
			childNode = node->GetChild(n);
			FbxMesh* mesh = childNode->GetMesh();
			if (mesh != NULL)
			{
				pMesh = mesh;
				// Matrices geometriques, différentes pour chaque mesh ---

				// etape 1. calcul de la matrice geometrique
				FbxVector4 translation = childNode->GetGeometricTranslation(FbxNode::eSourcePivot);
				FbxVector4 rotation = childNode->GetGeometricRotation(FbxNode::eSourcePivot);
				FbxVector4 scaling = childNode->GetGeometricScaling(FbxNode::eSourcePivot);
				FbxAMatrix geometryTransform;
				geometryTransform.SetTRS(translation, rotation, scaling);

				// etape 2. on recupere la matrice global (world) du mesh
				FbxAMatrix globalTransform = childNode->EvaluateGlobalTransform();

				// etape 3. on concatene les deux matrices, ce qui donne la matrice world finale
				finalGlobalTransform = globalTransform * geometryTransform;

				int nbVertex = mesh->GetPolygonCount() * 3;
				GLfloat* tmpVertex = new GLfloat[nbVertex * 4];

				vertex_pos = tmpVertex;
				nb_vertices = nbVertex;

				int nbNormals = mesh->GetPolygonCount() * 3;
				GLfloat* tmpNormals = new GLfloat[nbNormals * 3];
				int vertexID = 0;

				FbxStringList nameListUV;
				mesh->GetUVSetNames(nameListUV);
				int totalUVChannels = nameListUV.GetCount();

				int nbUVs = mesh->GetPolygonCount() * 3;
				GLfloat* tmpUVs = new GLfloat[nbUVs * 2 * totalUVChannels];
				
				for (int polyIndex = 0; polyIndex < mesh->GetPolygonCount(); polyIndex++)
				{
					for (int i = 0; i < 3; i++)
					{
						int j = mesh->GetPolygonVertex(polyIndex, i);
						FbxVector4 coord = finalGlobalTransform.MultT(mesh->GetControlPointAt(j));
						tmpVertex[vertexID * 4 + 0] = (GLfloat)coord.mData[0];
						tmpVertex[vertexID * 4 + 1] = (GLfloat)coord.mData[1];
						tmpVertex[vertexID * 4 + 2] = (GLfloat)coord.mData[2];
						tmpVertex[vertexID * 4 + 3] = 1.0f;

						FbxVector4 normal;
						mesh->GetPolygonVertexNormal(polyIndex, i, normal);

						normal = finalGlobalTransform.MultT(normal);
						tmpNormals[vertexID * 3 + 0] = (GLfloat)normal[0];
						tmpNormals[vertexID * 3 + 1] = (GLfloat)normal[1];
						tmpNormals[vertexID * 3 + 2] = (GLfloat)normal[2];
						/*std::cerr << (GLfloat)normal[0] << std::endl;
						std::cerr << (GLfloat)normal[1] << std::endl;
						std::cerr << (GLfloat)normal[2] << std::endl;
						*/
						for (int q = 0; q < totalUVChannels; ++q)
						{
							const char *nameUV = nameListUV.GetStringAt(q);
							FbxVector2 uv;
							bool isUnMapped;
							bool hasUV = mesh->GetPolygonVertexUV(polyIndex, i, nameUV, uv, isUnMapped);

							tmpUVs[vertexID * 2 + 0] = (GLfloat)uv[0];
							tmpUVs[vertexID * 2 + 1] = 1 - (GLfloat)uv[1];
						}

						vertexID++;
					}
				}
				vertex_nor = tmpNormals;
				nb_normals = nbNormals;

				vertex_uv = tmpUVs;
				nb_uvs = nbUVs;

			}

			ProcessNode(childNode, node);
		}
	}
};

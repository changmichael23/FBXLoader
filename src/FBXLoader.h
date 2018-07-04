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

		FbxScene * scene = FbxScene::Create(g_fbxManager, "Ma Scene");
		FbxImporter *importer = FbxImporter::Create(g_fbxManager, "");
		bool status = importer->Initialize("../samples/ironman/ironman.fbx", -1, g_fbxManager->GetIOSettings());
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
			ProcessNode(rootNode,rootNode);
		}
		//g_fbxManager->Destroy();

		

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
							tmpUVs[vertexID * 2 + 1] = 1-(GLfloat)uv[1];
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

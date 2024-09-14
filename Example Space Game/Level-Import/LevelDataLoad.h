// added 1/11/2024 by DD for file input
#pragma once
#include "h2bParser.h"
#include "MatParser.h"
#include <map>
class LEVEL_DATA {
	/// <summary>
	/// FileIntoString output
	/// </summary>
	std::set<std::string> stringLevelData;
	/// <summary>
	/// private player position accessible with getter
	/// </summary>
	GW::MATH::GMATRIXF playerPosition;
public:
	/// <summary>
	/// data for reading models
	/// </summary>
	struct MODEL_DATA
	{
		/// <summary>
		/// h2b file the model is from
		/// </summary>
		const char* filename;
		unsigned vertCount, indexCount, materialCount, meshCount;
		/// <summary>
		/// offsets
		/// </summary>
		unsigned vertStart, indexStart, materialStart, meshStart, batchStart;
		unsigned colliderIndex;
	};
	/// <summary>
	/// an instance of a model
	/// </summary>
	struct MODEL_INSTANCES
	{
		unsigned modelIndex, transformStart, transformCount; // flags optional
	};
	/// <summary>
	/// data for a texture
	/// </summary>
	struct TEXTURE_DATA
	{
		unsigned int albedoIndex, roughnessIndex, metalIndex, normalIndex;
	};
	/// <summary>
	/// data from blender objects
	/// </summary>
	struct BLENDER_DATA
	{
		/// <summary>
		/// name of model in blender
		/// </summary>
		const char* blendername;
		unsigned int modelIndex, transformIndex;
	};
	/// <summary>
	/// every vert in the level
	/// </summary>
	std::vector<H2B::VERTEX> levelVerts;
	/// <summary>
	/// indexed level data
	/// </summary>
	std::vector<unsigned int> levelIndices;
	/// <summary>
	/// all materials used in the data [levelTextures.Size() == levelMaterials.Size()]
	/// </summary>
	std::vector<H2B::ATTRIBUTES> levelMaterials;
	/// <summary>
	/// TEXTURE_DATA information [levelTextures.Size() == levelMaterials.Size()]
	/// </summary>
	std::vector<TEXTURE_DATA> levelTextures;
	/// <summary>
	/// transform data for each model
	/// </summary>
	std::vector<GW::MATH::GMATRIXF> levelTransforms;
	GW::MATH::GMATRIXF cameraMatrix;
	std::vector<GW::MATH::GMATRIXF> pointLightTransforms;
	std::vector<GW::MATH::GMATRIXF> spotLightTransforms;
	// maybe add this eventually
	std::vector<GW::MATH::GOBBF> levelColliders;
	/// <summary>
	/// map between model index 
	/// </summary>
	std::map<unsigned, GW::MATH::GMATRIXF> collisionMap;
	// write a summary here when I figure out what these do
	std::vector<H2B::BATCH> levelBatches;
	std::vector<H2B::MESH> levelMeshes;
	/// <summary>
	/// data for each model in the level
	/// </summary>
	std::vector<MODEL_DATA> levelModels;
	/// <summary>
	/// stored instances of each model in a level
	/// </summary>
	std::vector<MODEL_INSTANCES> levelInstancesData;
	/// <summary>
	/// stored each item from blender
	/// </summary>
	std::vector<BLENDER_DATA> blenderObjects;
	/// <summary>
	/// used to hold potentially multiple tracks
	/// </summary>
	std::vector<std::vector<GW::MATH::GMATRIXF>> trackNodesLists;
	std::vector<GW::MATH::GMATRIXF> trackStartPosList;
	std::vector<GW::MATH::GMATRIXF> trackEndPosList;
	bool LoadLevel(std::string gameLevelPath, std::string h2bFolderPath) {

		std::set<CLASS_MODEL_DATA> uniqueModels; // unique models and their locations

		UnloadLevel();// clear previous level data if there is any
		if (ReadGameLevel(gameLevelPath, uniqueModels) == false) {
			return false;
		}
		ReadH2BLevelData(h2bFolderPath, uniqueModels);
		return true;
	}
	// used to wipe CPU level data between levels
	void UnloadLevel() {
		stringLevelData.clear();
		levelVerts.clear();
		levelIndices.clear();
		levelMaterials.clear();
		levelTextures.clear();
		levelBatches.clear();
		levelMeshes.clear();
		levelModels.clear();
		levelTransforms.clear();
		pointLightTransforms.clear();
		spotLightTransforms.clear();
		levelInstancesData.clear();
		blenderObjects.clear();
		for (size_t i = 0; i < trackNodesLists.size(); i++) {
			trackNodesLists[i].clear();
		}
		trackNodesLists.clear();
		trackStartPosList.clear();
		trackEndPosList.clear();
		collisionMap.clear();
	}
	GW::MATH::GMATRIXF GetPlayerPos() {
		return playerPosition;
	}
private:

	struct CLASS_MODEL_DATA
	{
		/// <summary>
		/// .h2b filepath
		/// </summary>
		std::string modelFilePath;
		std::string materialFilePath;
		// potentially add later
		GW::MATH2D::GVECTOR3F boundry[8];
		mutable std::vector<std::string> blenderNames;
		mutable std::vector<GW::MATH::GMATRIXF> drawInstances;
		bool operator<(const CLASS_MODEL_DATA& cmp) const {
			return modelFilePath < cmp.modelFilePath; // you need this for std::set to work
		}
		// potentially use information later
		GW::MATH::GOBBF ComputeOBB() const {
			GW::MATH::GOBBF out = {
				GW::MATH::GIdentityVectorF,
				GW::MATH::GIdentityVectorF,
				GW::MATH::GIdentityQuaternionF // initally unrotated (local space)
			};
			out.center.x = (boundry[0].x + boundry[4].x) * 0.5f;
			out.center.y = (boundry[0].y + boundry[1].y) * 0.5f;
			out.center.z = (boundry[0].z + boundry[2].z) * 0.5f;
			out.extent.x = std::fabsf(boundry[0].x - boundry[4].x) * 0.5f;
			out.extent.y = std::fabsf(boundry[0].y - boundry[1].y) * 0.5f;
			out.extent.z = std::fabsf(boundry[0].z - boundry[2].z) * 0.5f;
			return out;
		}
	};

	bool ReadGameLevel(std::string _filePath, std::set<CLASS_MODEL_DATA>& outModelData) {
		GW::SYSTEM::GFile file;
		file.Create();
		const char* filePath = _filePath.c_str();
		if (-file.OpenTextRead(filePath)) {// exits function if not opened
			return false;
		}
		char lineReadBuffer[1024];
		while (+file.ReadLine(lineReadBuffer, 1024, '\n'))
		{
			if (lineReadBuffer[0] == '\0')
				break;
			if (std::strcmp(lineReadBuffer, "CAMERA") == 0) {// search for the camera matrix and copy
				file.ReadLine(lineReadBuffer, 1024, '\n');
				//DO NOTHING WITH DATA

				// now read the transform data as we will need that regardless
				GW::MATH::GMATRIXF transform;
				for (int i = 0; i < 4; ++i) {
					file.ReadLine(lineReadBuffer, 1024, '\n');
					// read floats
					std::sscanf(lineReadBuffer + 13, "%f, %f, %f, %f",
						&transform.data[0 + i * 4], &transform.data[1 + i * 4],
						&transform.data[2 + i * 4], &transform.data[3 + i * 4]);
				}
				std::string loc = "Location: X ";
				loc += std::to_string(transform.row4.x) + " Y " +
					std::to_string(transform.row4.y) + " Z " + std::to_string(transform.row4.z);
				cameraMatrix = transform;
			}
			if (std::strcmp(lineReadBuffer, "LIGHT") == 0) {// search for the light matrix and copy
				file.ReadLine(lineReadBuffer, 1024, '\n');
				//Determine Light Type
				bool isPointLight = false;
				bool isSpotLight = false;
				CLASS_MODEL_DATA add = { lineReadBuffer };
				add.modelFilePath = add.modelFilePath.substr(0, add.modelFilePath.find_last_of("."));
				if (add.modelFilePath == "Point") {
					isPointLight = true;
				}
				else if (add.modelFilePath == "Spot") {
					isSpotLight = true;
				}// potentially read in dirLight afterwards
				// now read the transform data as we will need that regardless
				GW::MATH::GMATRIXF transform;
				for (int i = 0; i < 4; ++i) {
					file.ReadLine(lineReadBuffer, 1024, '\n');
					// read floats
					std::sscanf(lineReadBuffer + 13, "%f, %f, %f, %f",
						&transform.data[0 + i * 4], &transform.data[1 + i * 4],
						&transform.data[2 + i * 4], &transform.data[3 + i * 4]);
				};
				if (isPointLight)
					pointLightTransforms.push_back(transform);
				else if (isSpotLight)
					spotLightTransforms.push_back(transform);

			}
			if (std::strcmp(lineReadBuffer, "EMPTY") == 0) {// search for point data
				file.ReadLine(lineReadBuffer, 1024, '\n');
				CLASS_MODEL_DATA add = { lineReadBuffer };
				add.modelFilePath = add.modelFilePath.substr(0, add.modelFilePath.find_last_of("."));
				if (add.modelFilePath == "Player Start") {
					GW::MATH::GMATRIXF transform;
					for (int i = 0; i < 4; ++i) {
						file.ReadLine(lineReadBuffer, 1024, '\n');
						// read floats
						std::sscanf(lineReadBuffer + 13, "%f, %f, %f, %f",
							&transform.data[0 + i * 4], &transform.data[1 + i * 4],
							&transform.data[2 + i * 4], &transform.data[3 + i * 4]);
					};
					playerPosition = transform;
				}
				else if (add.modelFilePath[0] == 'N' && add.modelFilePath[1] == 'o' &&
					add.modelFilePath[2] == 'd' && add.modelFilePath[3] == 'e') {// check if first few letters say Node

					if (add.modelFilePath.size() > 4) {// if modelFilePath says more than Node
						int trackNumber = add.modelFilePath[9] - 48;
						for (; trackNumber > trackNodesLists.size();) {
							std::vector<GW::MATH::GMATRIXF> emptyVec{  };
							trackNodesLists.push_back(emptyVec);
						}
						GW::MATH::GMATRIXF transform;
						for (int i = 0; i < 4; ++i) {
							file.ReadLine(lineReadBuffer, 1024, '\n');
							// read floats
							std::sscanf(lineReadBuffer + 13, "%f, %f, %f, %f",
								&transform.data[0 + i * 4], &transform.data[1 + i * 4],
								&transform.data[2 + i * 4], &transform.data[3 + i * 4]);
						};
						trackNodesLists[trackNumber - 1].push_back(transform);
					}
					else {// 
						if (trackNodesLists.size() == 0) {// resize to size 1
							std::vector<GW::MATH::GMATRIXF> emptyVec{  };
							trackNodesLists.push_back(emptyVec);
						}
						GW::MATH::GMATRIXF transform;
						for (int i = 0; i < 4; ++i) {
							file.ReadLine(lineReadBuffer, 1024, '\n');
							// read floats
							std::sscanf(lineReadBuffer + 13, "%f, %f, %f, %f",
								&transform.data[0 + i * 4], &transform.data[1 + i * 4],
								&transform.data[2 + i * 4], &transform.data[3 + i * 4]);
						};
						trackNodesLists[0].push_back(transform);
					}
				}
			}
			if (std::strcmp(lineReadBuffer, "MESH") == 0)// search for mesh matrices and copy
			{
				bool isStartPos = false;
				bool isEndPos = false;
				file.ReadLine(lineReadBuffer, 1024, '\n');
				std::string blenderName = lineReadBuffer;
				// create the model file name from this (strip the .001)
				CLASS_MODEL_DATA add = { lineReadBuffer };
				add.modelFilePath = add.modelFilePath.substr(0, add.modelFilePath.find_last_of("."));
				if (add.modelFilePath == "Start")
					isStartPos = true;
				if (add.modelFilePath == "End")
					isEndPos = true;
				add.materialFilePath = add.modelFilePath + ".mtl";
				add.modelFilePath += ".h2b";

				// now read the transform data as we will need that regardless
				GW::MATH::GMATRIXF transform;
				for (int i = 0; i < 4; ++i) {
					file.ReadLine(lineReadBuffer, 1024, '\n');
					// read floats
					std::sscanf(lineReadBuffer + 13, "%f, %f, %f, %f",
						&transform.data[0 + i * 4], &transform.data[1 + i * 4],
						&transform.data[2 + i * 4], &transform.data[3 + i * 4]);
				}
				std::string loc = "Location: X ";
				loc += std::to_string(transform.row4.x) + " Y " +
					std::to_string(transform.row4.y) + " Z " + std::to_string(transform.row4.z);

				if (isStartPos)
					trackStartPosList.push_back(transform);
				if (isEndPos)
					trackEndPosList.push_back(transform);

				// does this model already exist?
				auto found = outModelData.find(add);
				if (found == outModelData.end()) // no
				{
					add.blenderNames.push_back(blenderName); // *NEW*
					add.drawInstances.push_back(transform);
					outModelData.insert(add);
				}
				else // yes
				{
					found->blenderNames.push_back(blenderName); // *NEW*
					found->drawInstances.push_back(transform);
				}
			}
			if (std::strcmp(lineReadBuffer, "COLLISION") == 0) {// read in collision data
				file.ReadLine(lineReadBuffer, 1024, '\n');
				std::string modelName = lineReadBuffer;

				// now read the transform data as we will need that regardless
				GW::MATH::GMATRIXF transform;
				for (int i = 0; i < 4; ++i) {
					file.ReadLine(lineReadBuffer, 1024, '\n');
					// read floats
					std::sscanf(lineReadBuffer + 13, "%f, %f, %f, %f",
						&transform.data[0 + i * 4], &transform.data[1 + i * 4],
						&transform.data[2 + i * 4], &transform.data[3 + i * 4]);
				}
				std::string loc = "Location: X ";
				loc += std::to_string(transform.row4.x) + " Y " +
					std::to_string(transform.row4.y) + " Z " + std::to_string(transform.row4.z);
				size_t loopNdx = 0;
				for (auto iter = outModelData.begin(); iter != outModelData.end(); iter++, loopNdx++) {
					if (iter->blenderNames[0] == modelName) {
						collisionMap[loopNdx] = transform;
					}
				}

				
			}
		}
		return true;
	}
	void ReadH2BLevelData(std::string filePath, const std::set<CLASS_MODEL_DATA>& modelSet) {// add summary when functional
		H2B::Parser parser;
		H2B::MatParser matParser;
		const std::string modelPath = filePath;
		bool loopFirstTime = true;
		for (auto iter = modelSet.begin(); iter != modelSet.end(); ++iter)
		{
			if (parser.Parse((modelPath + "/" + iter->modelFilePath).c_str()))
			{
				// transfer all string data
				for (int j = 0; j < parser.materialCount; ++j) {
					for (int k = 0; k < 10; ++k) {
						if (*((&parser.materials[j].name) + k) != nullptr)
							*((&parser.materials[j].name) + k) =
							stringLevelData.insert(*((&parser.materials[j].name) + k)).first->c_str();
					}
				}
				for (int j = 0; j < parser.meshCount; ++j) {
					if (parser.meshes[j].name != nullptr)
						parser.meshes[j].name =
						stringLevelData.insert(parser.meshes[j].name).first->c_str();
				}
				// record source file name & sizes
				MODEL_DATA model;
				model.filename = stringLevelData.insert(iter->modelFilePath).first->c_str();
				model.vertCount = parser.vertexCount;
				model.indexCount = parser.indexCount;
				model.materialCount = parser.materialCount;
				model.meshCount = parser.meshCount;
				// record offsets
				model.vertStart = levelVerts.size();
				model.indexStart = levelIndices.size();
				model.materialStart = levelMaterials.size();
				model.batchStart = levelBatches.size();
				model.meshStart = levelMeshes.size();
				// append/move all data
				levelVerts.insert(levelVerts.end(), parser.vertices.begin(), parser.vertices.end());
				levelIndices.insert(levelIndices.end(), parser.indices.begin(), parser.indices.end());
				auto matEndIterStore = levelMaterials.end();
				if (matParser.Parse((modelPath + "/" + iter->materialFilePath).c_str())) {
					levelMaterials.insert(levelMaterials.end(), matParser.materials.begin(), matParser.materials.end());
				}
				else {
					for (size_t i = 0; i < model.materialCount; i++) {
						H2B::ATTRIBUTES temp = {};
						temp.Kd = { 0.5, 0.5, 0.5 };
						levelMaterials.push_back(temp);
					}
				}
				levelBatches.insert(levelBatches.end(), parser.batches.begin(), parser.batches.end());
				levelMeshes.insert(levelMeshes.end(), parser.meshes.begin(), parser.meshes.end());
				// potentially add this
				model.colliderIndex = levelColliders.size();
				levelColliders.push_back(iter->ComputeOBB());
				// add level model
				levelModels.push_back(model);
				// add level model instances
				MODEL_INSTANCES instances;
				instances.modelIndex = levelModels.size() - 1;
				instances.transformStart = levelTransforms.size();
				instances.transformCount = iter->drawInstances.size();
				levelTransforms.insert(levelTransforms.end(), iter->drawInstances.begin(), iter->drawInstances.end());
				// add instance set
				levelInstancesData.push_back(instances);
				// *NEW* Add an entry for each unique blender object
				int offset = 0;
				for (auto& n : iter->blenderNames) {
					BLENDER_DATA obj{
						stringLevelData.insert(n).first->c_str(),
						instances.modelIndex, instances.transformStart + offset++
					};
					blenderObjects.push_back(obj);
				}
			}
		}
	}
private:

};
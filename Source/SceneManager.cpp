///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


/***********************************************************
 *  LoadSceneTextures()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/


void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;

	// texture loaded for wooden table
	bReturn = CreateGLTexture(
		"textures/wood.jpg",
		"wood"
	);

	// textures to be used for hotsauce
	bReturn = CreateGLTexture(
		"textures/sauce1.jpg",
		"sauce1"
	);

	bReturn = CreateGLTexture(
		"textures/sauce2.jpg",
		"sauce2"
	);

	bReturn = CreateGLTexture(
		"textures/sauce3.jpg",
		"sauce3"
	);

	bReturn = CreateGLTexture(
		"textures/sauce4.jpg",
		"sauce4"
	);

	// texture to be used for bottle lid
	bReturn = CreateGLTexture(
		"textures/lid.jpg",
		"lid"
	);

	// texture to be used for kitchen wall
	bReturn = CreateGLTexture(
		"textures/wall.jpg",
		"wall"
	);

	// texture to be used for shelf
	bReturn = CreateGLTexture(
		"textures/shelfwood.jpg",
		"shelf"
	);

	// texture for red plastic
	bReturn = CreateGLTexture(
		"textures/redplastic.jpg",
		"redplastic"
	);

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}




/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	//glass material to be used for bottle
	OBJECT_MATERIAL glassMaterial;
	glassMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
	glassMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	glassMaterial.shininess = 95.0;
	glassMaterial.tag = "glass";
	m_objectMaterials.push_back(glassMaterial);

	//plastic material to be used for lids
	OBJECT_MATERIAL plasticMaterial;
	plasticMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f);
	plasticMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	plasticMaterial.shininess = 0.01;
	plasticMaterial.tag = "plastic";
	m_objectMaterials.push_back(plasticMaterial);

	//shiny wood material for table
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	woodMaterial.specularColor = glm::vec3(0.7f, 0.7f, 0.7f);
	woodMaterial.shininess = 80.0;
	woodMaterial.tag = "wood";
	m_objectMaterials.push_back(woodMaterial);

	//material for wall
	OBJECT_MATERIAL wallMaterial;
	wallMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3);
	wallMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f);
	wallMaterial.shininess = 75.0;
	wallMaterial.tag = "wall";
	m_objectMaterials.push_back(wallMaterial);

	//material for shelf
	OBJECT_MATERIAL shelfMaterial;
	shelfMaterial.diffuseColor = glm::vec3(0.6f, 0.6f, 0.6f);
	shelfMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	shelfMaterial.shininess = 0.2;
	shelfMaterial.tag = "shelf";
	m_objectMaterials.push_back(shelfMaterial);
}

/***********************************************************
* SetupSceneLightsOP                                
* 
* This method is used for configuring the lighting settings
* for the shader
* *********************************************************/
void SceneManager::SetupSceneLights()
{
	//enable lighting
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// general distant light like sunlight applied for general lighting, the room is largely
	// lit by open windows
	// reddish light used to simulate light shining through red curtains
	m_pShaderManager->setVec3Value("directionalLight.direction", -0.05f, -0.3f, -0.1f);
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.07f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 0.8f, 0.6f, 0.6f);
	m_pShaderManager->setVec3Value("directionalLight.specular", 1.0f, 0.8f, 0.8f);
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	// point lights set up to mimic the 3 lighting sources identified for the scene
	// point light 1
	m_pShaderManager->setVec3Value("pointLights[0].position", 3.0f, 2.0f, 2.0f);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.15f, 0.15f, 0.15f);
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.35f, 0.35f, 0.35f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

	// point light 2
	m_pShaderManager->setVec3Value("pointLights[1].position", -3.0f, 2.0f, 2.0f);
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.15f, 0.15f, 0.15f);
	m_pShaderManager->setVec3Value("pointLights[1].specular", 0.35f, 0.35f, 0.35f);
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);

	// point light 3
	m_pShaderManager->setVec3Value("pointLights[2].position", 0.0f, 2.0f, 2.0f);
	m_pShaderManager->setVec3Value("pointLights[2].ambient", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("pointLights[2].diffuse", 0.5f, 0.5f, 0.5f);
	m_pShaderManager->setVec3Value("pointLights[2].specular", 0.9f, 0.9f, 0.9f);
	m_pShaderManager->setBoolValue("pointLights[2].bActive", true);
}
/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// load scene textures
	LoadSceneTextures();

	// define the materials that will be used for the objects
	// in the 3D Scene
	DefineObjectMaterials();

	// setup lights
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	// all needed shapes identified in Module 1 Milestone loaded
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
}

// Method to create shelf for holding hotsauce
void SceneManager::CreateShelf()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// create shorter shelf
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(10.0f, 1.0f, 2.0f);

	// set the XYZ rotation for the mesh
	// no rotation needed for bottles
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.5f, 3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set materials and textures
	SetShaderTexture("shelf");
	SetShaderMaterial("shelf");

	// draw shelf piece
	m_basicMeshes->DrawBoxMesh();

	// create medium shelf
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(10.0f, 2.0f, 2.0f);

	// set the XYZ rotation for the mesh
	// no rotation needed for bottles
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 1.0f, 1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set materials and textures
	SetShaderTexture("shelf");
	SetShaderMaterial("shelf");

	// draw shelf piece
	m_basicMeshes->DrawBoxMesh();

	// draw large shelf
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(10.0f, 3.0f, 2.0f);

	// set the XYZ rotation for the mesh
	// no rotation needed for bottles
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 1.5f, -1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set materials and textures
	SetShaderTexture("shelf");
	SetShaderMaterial("shelf");

	// draw shelf piece
	m_basicMeshes->DrawBoxMesh();

	// create side of lower shelf
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 1.3f, 2.2f);

	// set the XYZ rotation for the mesh
	// no rotation needed for bottles
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.0f, 0.65f, 3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set materials and textures
	SetShaderTexture("shelf");
	SetShaderMaterial("shelf");

	// draw shelf piece
	m_basicMeshes->DrawBoxMesh();

	// create side of lower shelf
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 1.3f, 2.2f);

	// set the XYZ rotation for the mesh
	// no rotation needed for bottles
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.0f, 0.65f, 3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set materials and textures
	SetShaderTexture("shelf");
	SetShaderMaterial("shelf");

	// draw shelf piece
	m_basicMeshes->DrawBoxMesh();

	// create side of middle shelf
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 2.3f, 2.2f);

	// set the XYZ rotation for the mesh
	// no rotation needed for bottles
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.0f, 1.15f, 1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set materials and textures
	SetShaderTexture("shelf");
	SetShaderMaterial("shelf");

	// draw shelf piece
	m_basicMeshes->DrawBoxMesh();

	// create side of middle shelf
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 2.3f, 2.2f);

	// set the XYZ rotation for the mesh
	// no rotation needed for bottles
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.0f, 1.15f, 1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set materials and textures
	SetShaderTexture("shelf");
	SetShaderMaterial("shelf");

	// draw shelf piece
	m_basicMeshes->DrawBoxMesh();

	// create side of top shelf
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 3.3f, 2.2f);

	// set the XYZ rotation for the mesh
	// no rotation needed for bottles
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.0f, 1.65f, -1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set materials and textures
	SetShaderTexture("shelf");
	SetShaderMaterial("shelf");

	// draw shelf piece
	m_basicMeshes->DrawBoxMesh();

	// create side of top shelf
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 3.3f, 2.2f);

	// set the XYZ rotation for the mesh
	// no rotation needed for bottles
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.0f, 1.65f, -1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set materials and textures
	SetShaderTexture("shelf");
	SetShaderMaterial("shelf");

	// draw shelf piece
	m_basicMeshes->DrawBoxMesh();
}
/*********************************************************************************
* Method to create a bottle of hotsauce to be reused when creating the scene     *
* A few varients of hotsauce bottles will be created                             *
* This represents the first varient and most frequently appearing in the image   *
* hs1x, hs1y, hs1z move the bottle along the x, y, and z axes respectively       *
* hs1 scales the entire bottle object                                            *
*********************************************************************************/
void SceneManager::CreateBottle1(float hs1x, float hs1y, float hs1z, float hs1, std::string tex)
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// draw cylinder inside to represent sauces
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(hs1 * 0.7, hs1 * 2.8f, hs1 * 0.7f); // scaled by hs1

	// set the XYZ rotation for the mesh
	// no rotation needed for bottles
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f + hs1x, hs1 * (0.2f + hs1y), 0.0f + hs1z); // height scaled by hs1 to ensure basic shapes link

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set sauce texture based on passed in texture string
	// the texture was tiled several times. This allows us to effectively zoom out of the texture
	// since sauces are a blend of ingredients, removes detail of texture for homogeneous look
	// improves the scene and makes use of complex texturing tequnique
	SetTextureUVScale(50.0, 50.0);
	SetShaderTexture(tex);

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh(); // draw sauce

	// create cylinder base
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(hs1 * 0.8f, hs1 * 3.0f, hs1 * 0.8f); // cylinder size scaled by hs1

	// set the XYZ rotation for the mesh
	// bottles are generally straight up and rotation is not needed
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f + hs1x, hs1 * (0.0f + hs1y), 0.0f + hs1z); // height scaled by hs1 to ensure basic shapes continue to link

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set shape texture - glass material used - inspiration taken from sample project
	SetShaderColor(.7, .7, .8, 0.3);
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh(); // draw base

	//create tapered cylinder for neck of bottle
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(hs1 * 0.8f, hs1 * 1.0f, hs1 * 0.8f); // size scaled by hs1

	// set the XYZ rotation for the mesh
	// no rotation needed for bottles
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f + hs1x, hs1 * (3.0f + hs1y), 0.0f + hs1z); // height scaled by hs1 to ensure basic shapes link

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set shape texture - glass material used - inspiration taken from sample project
	SetShaderColor(.7, .7, .8, 0.3);
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh(); // draw neck

	// cylinder for lid
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(hs1 * 0.5f, hs1 * 0.7f, hs1 * 0.5f); // scaled by hs1

	// set the XYZ rotation for the mesh
	// no rotation needed for bottles
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f + hs1x, hs1 * (4.8f + hs1y), 0.0f + hs1z); // height scaled by hs1 to ensure basic shapes link

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set texture for lid
	SetTextureUVScale(1.0, 1.0);
	SetShaderTexture("lid");
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh(); // draw lid

	// additional cylinder for non-tapered part of neck
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(hs1 * 0.4f, hs1 * 1.0f, hs1 * 0.4f); // shape scaled by hs1

	// set the XYZ rotation for the mesh
	// no rotation needed for bottles
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f + hs1x, hs1 * (4.0f + hs1y), 0.0f + hs1z); // height scaled by hs1 to ensure basic shapes link

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set shape texture - glass material used - inspiration taken from sample project
	SetShaderColor(.7, .7, .8, 0.3);
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh(); // draw neck	
}

// creates a bottle with a ring around the neck
void SceneManager::CreateBottle2(float hs2x, float hs2y, float hs2z, float hs2, std::string tex)
{
	// create standard bottle using CreateBottle1
	CreateBottle1(hs2x, hs2y, hs2z, hs2, tex);

	// create torus for ring
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// scale the torus
	scaleXYZ = glm::vec3(hs2 * 0.6f, hs2 * 0.4f, hs2 * 0.6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = -67.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 13.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(hs2x + 0.0f, hs2 * (hs2y + 4.2f), hs2z + 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// plastic texture used for ring
	SetShaderTexture("lid");
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();
}

// create a bottle with pointed nozzle
void SceneManager::CreateBottle3(float hs3x, float hs3y, float hs3z, float hs3, std::string tex)
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// draw cylinder inside to represent sauces
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(hs3 * 1.0, hs3 * 2.8f, hs3 * 1.0f); // scaled by hs3

	// set the XYZ rotation for the mesh
	// no rotation needed for bottles
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f + hs3x, hs3 * (0.2f + hs3y), 0.0f + hs3z); // height scaled by hs3 to ensure basic shapes link

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set sauce texture based on passed in texture string
	// the texture was tiled several times. This allows us to effectively zoom out of the texture
	// since sauces are a blend of ingredients, removes detail of texture for homogeneous look
	// improves the scene and makes use of complex texturing tequnique
	SetTextureUVScale(50.0, 50.0);
	SetShaderTexture(tex);

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh(); // draw sauce

	// create cylinder base
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(hs3 * 1.1f, hs3 * 3.0f, hs3 * 1.1f); // cylinder size scaled by hs3

	// set the XYZ rotation for the mesh
	// bottles are generally straight up and rotation is not needed
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f + hs3x, hs3 * (0.0f + hs3y), 0.0f + hs3z); // height scaled by hs1 to ensure basic shapes continue to link

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set shape texture - glass material used - inspiration taken from sample project
	SetShaderColor(.7, .7, .8, 0.3);
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh(); // draw base

	//create tapered cylinder for neck of bottle
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(hs3 * 1.0f, hs3 * 1.0f, hs3 * 1.0f); // size scaled by hs1

	// set the XYZ rotation for the mesh
	// no rotation needed for bottles
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f + hs3x, hs3 * (3.0f + hs3y), 0.0f + hs3z); // height scaled by hs1 to ensure basic shapes link

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set shape texture and plastic material
	SetShaderTexture("redplastic");
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh(); // draw neck

	//create tapered cylinder for neck of bottle
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(hs3 * 0.4f, hs3 * 1.3f, hs3 * 0.4f); // size scaled by hs3

	// set the XYZ rotation for the mesh
	// no rotation needed for bottles
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f + hs3x, hs3 * (4.0f + hs3y), 0.0f + hs3z); // height scaled by hs3 to ensure basic shapes link

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set shape texture - and plastic material
	SetShaderTexture("redplastic");
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh(); // draw neck

	//create tapered cylinder for tip of bottle
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(hs3 * 0.1f, hs3 * 0.1f, hs3 * 0.1f); // size scaled by hs3

	// set the XYZ rotation for the mesh
	// no rotation needed for bottles
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f + hs3x, hs3 * (5.33f + hs3y), 0.0f + hs3z); // height scaled by hs3 to ensure basic shapes link

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set shape texture - and plastic material
	SetShaderTexture("redplastic");
	SetShaderMaterial("plastic");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh(); // draw neck
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// wood texture used for table
	SetShaderTexture("wood");
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// Draw kitchen wall behind the sauces
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 10.0f, -5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// wall texture used for painted wall
	SetShaderTexture("wall");
	SetShaderMaterial("wall");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// create shelf for hotsauce
	CreateShelf();

	// top shelf left to right
	CreateBottle1(-4., 10, -1, 0.3, "sauce4");
	CreateBottle1(-2.8, 5, -1, 0.6, "sauce1");
	CreateBottle1(-1.5, 5, -1, 0.6, "sauce4");
	CreateBottle1(3, 5, -1, 0.6, "sauce4");

	// middle shelf left to right
	CreateBottle1(-4, 6.7, 1, 0.3, "sauce4");
	CreateBottle1(-3, 6.7, 1, 0.3, "sauce2");
	CreateBottle2(2.0, 4.9, 1, 0.4, "sauce4");
	CreateBottle3(3.2, 4.9, 1, 0.4, "sauce2");
	CreateBottle1(4.1, 3.35, 1, 0.6, "sauce1");

	// bottom shelf left to right
	CreateBottle1(-4, 3.4, 3, 0.3, "sauce1");
	CreateBottle1(-3, 3.4, 3, 0.3, "sauce2");
	CreateBottle1(-2, 3.4, 3, 0.3, "sauce3");
	CreateBottle1(-1, 3.4, 3, 0.3, "sauce1");
	CreateBottle1(2.5, 3.4, 3, 0.3, "sauce4");
	CreateBottle1(3.5, 3.4, 3, 0.3, "sauce2");
	CreateBottle1(4.5, 3.4, 3, 0.3, "sauce1");
	
	// large bottles to the right
	CreateBottle1(6.3, 0.0, 3.9, 0.8, "sauce1");
}

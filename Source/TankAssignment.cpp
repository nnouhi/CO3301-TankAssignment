/*******************************************
	TankAssignment.cpp

	Shell scene and game functions
********************************************/

#include <sstream>
#include <string>
using namespace std;

#include <d3d10.h>
#include <d3dx10.h>

#include "Defines.h"
#include "CVector3.h"
#include "Camera.h"
#include "Light.h"
#include "EntityManager.h"
#include "Messenger.h"
#include "TankAssignment.h"
#include "CVector4.h"
#include "RayCast.h"
#include "ParseLevel.h"

namespace gen
{

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

// Control speed
const float CameraRotSpeed = 2.0f;
float CameraMoveSpeed = 80.0f;

// Amount of time to pass before calculating new average update time
const float UpdateTimePeriod = 1.0f;

//-----------------------------------------------------------------------------
// Global system variables
//-----------------------------------------------------------------------------

// Get reference to global DirectX variables from another source file
extern ID3D10Device*           g_pd3dDevice;
extern IDXGISwapChain*         SwapChain;
extern ID3D10DepthStencilView* DepthStencilView;
extern ID3D10RenderTargetView* BackBufferRenderTarget;
extern ID3DX10Font*            OSDFont;

// Actual viewport dimensions (fullscreen or windowed)
extern TUInt32 ViewportWidth;
extern TUInt32 ViewportHeight;

// Current mouse position
extern TUInt32 MouseX;
extern TUInt32 MouseY;

// Messenger class for sending messages to and between entities
extern CMessenger Messenger;


//-----------------------------------------------------------------------------
// Global game/scene variables
//-----------------------------------------------------------------------------

// Entity manager
CEntityManager EntityManager;
CParseLevel LevelParser(&EntityManager);
CRayCast ray = &EntityManager;

// Tank UIDs
//TEntityUID TankA;
//TEntityUID TankB;
const INT32 NumOfTanks = 6;
const INT32 NumOfPatrolPoints = 3;

TEntityUID tanks[NumOfTanks];
vector<CTankEntity*> tankEntities;
CEntity* NearestEntity = 0;
CEntity* SelectedEntity = 0;

// Other scene elements
const INT32 NumLights = 2;
CLight*  Lights[NumLights];
SColourRGBA AmbientLight;
CCamera* MainCamera;
CCamera* FreeMovingCamera;

const INT32 NumOfAmmoCrates = 2;
const TFloat32 AmmoCrateRotSpeed = 5.0f;
const TFloat32 AmmoCrateRespawnTime = 5.0f;
const TFloat32 AmmoCratePickUpDistance = 5.0f;
TFloat32 SpawnNewAmmoCrateInterval = 20.0f;
TInt32 NumOfSpawnedAmmoCrates = 0;

// Sum of recent update times and number of times in the sum - used to calculate
// average over a given time period
float SumUpdateTimes = 0.0f;
int NumUpdateTimes = 0;
float AverageUpdateTime = -1.0f; // Invalid value at first

bool ShowExtendedInformation = false;
TInt32 CurrentChaseCameraIndex = 0;

//-----------------------------------------------------------------------------
// Scene management
//-----------------------------------------------------------------------------

// Creates the scene geometry
bool SceneSetup()
{
	//////////////////////////////////////////////
	// Prepare render methods

	InitialiseMethods();
	LevelParser.ParseFile("Entities.xml");

	//////////////////////////////////////////
	// Create scenery templates and entities

	// Create scenery templates - loads the meshes
	// Template type, template name, mesh name
	/*EntityManager.CreateTemplate("Scenery", "Skybox", "Skybox.x");
	EntityManager.CreateTemplate("Scenery", "Floor", "Floor.x");
	EntityManager.CreateTemplate("Scenery", "Building", "Building.x");
	EntityManager.CreateTemplate("Scenery", "Tree", "Tree1.x");*/

	// Creates scenery entities
	// Type (template name), entity name, position, rotation, scale
	/*EntityManager.CreateEntity("Skybox", "Skybox", CVector3(0.0f, -10000.0f, 0.0f), CVector3::kZero, CVector3(10, 10, 10));
	EntityManager.CreateEntity("Floor", "Floor");
	EntityManager.CreateEntity("Building", "Building", CVector3(0.0f, 0.0f, 10000.0f), CVector3(0.0f, 0.0f, 0.0f));*/
	//for (int tree = 0; tree < 100; ++tree)
	//{
	//	// Some random trees
	//	EntityManager.CreateEntity( "Tree", "Tree",
	//		                        CVector3(Random(-200.0f, 30.0f), 0.0f, Random(40.0f, 150.0f)),
	//		                        CVector3(0.0f, Random(0.0f, 2.0f * kfPi), 0.0f) );
	//}


	///////////////////////////////////
	//// Create tank templates

	// Template type, template name, mesh name, top speed, acceleration, tank turn speed, turret
	// turn speed, max HP and shell damage. These latter settings are for advanced requirements only
	//EntityManager.CreateTankTemplate("Tank", "Rogue Scout", "HoverTank02.x",
	//	40.0f, 4.2f, 2.0f, kfPi / 3, 200, 20);
	//EntityManager.CreateTankTemplate("Tank", "Oberon MkII", "HoverTank07.x",
	//	30.0f, 2.6f, 1.3f, kfPi / 4, 200, 35);

	//// Template for tank shell
	//EntityManager.CreateTemplate("Projectile", "Shell Type 1", "Bullet.x");

	//// Template for ammo crate
	//EntityManager.CreateTemplate("Ammo", "AmmoCrate", "Cube.x");

	// Dictionary that stores the patrol path for each tank
	//std::map<TInt32, vector<CVector3>> tankPatrolPaths
	//{
	//	// Team A
	//	{0, { CVector3(-30.0f, 0.5f, 20.0f), CVector3(-15.0f, 0.5f, 25.0f), CVector3(-30.0f, 0.5f, -20.0f) }},
	//	{1, { CVector3(-20.0f, 0.5f, 0.0f), CVector3(-10.0f, 0.5f, 0.0f), CVector3(-30.0f, 0.5f, 20.0f) }},
	//	{2, { CVector3(-30.0f, 0.5f, -20.0f), CVector3(-15.0f, 0.5f, -25.0f), CVector3(-20.0f, 0.5f, 0.0f) }},

	//	// Team B
	//	{3, { CVector3(30.0f, 0.5f, 20.0f),  CVector3(15.0f, 0.5f, 25.0f), CVector3(30.0f, 0.5f, -20.0f) }},
	//	{4, { CVector3(20.0f, 0.5f, 0.0f), CVector3(10.0f, 0.5f, 0.0f), CVector3(30.0f, 0.5f, 20.0f) }},
	//	{5, { CVector3(30.0f, 0.5f, -20.0f), CVector3(15.0f, 0.5f, -25.0f), CVector3(30.0f, 0.5f, 20.0f) }}
	//};

	////////////////////////////////
	// Create tank entities

	//// Type (template name), team number, tank name, position, rotation
	//// Team A
	//tanks[0] = EntityManager.CreateTank("Rogue Scout", 0, "A-1", CVector3(-75.0f, 0.5f, 20.0f),
	//	CVector3(0.0f, ToRadians(90.0f), 0.0f));

	//tanks[1] = EntityManager.CreateTank("Rogue Scout", 0, "A-2", CVector3(-65.0f, 0.5f, 0.0f),
	//	CVector3(0.0f, ToRadians(90.0f), 0.0f));

	//tanks[2] = EntityManager.CreateTank("Rogue Scout", 0, "A-3", CVector3(-75.0f, 0.5f, -20.0f),
	//	CVector3(0.0f, ToRadians(90.0f), 0.0f));
	//
	//// Team B
	//tanks[3] = EntityManager.CreateTank("Oberon MkII", 1, "B-1", CVector3(75.0f, 0.5f, 20.0f),
	//	CVector3(0.0f, ToRadians(-90.0f), 0.0f));

	//tanks[4] = EntityManager.CreateTank("Oberon MkII", 1, "B-2", CVector3(65.0f, 0.5f, 0.0f),
	//	CVector3(0.0f, ToRadians(-90.0f), 0.0f));

	//tanks[5] = EntityManager.CreateTank("Oberon MkII", 1, "B-3", CVector3(75.0f, 0.5f, -20.0f),
	//	CVector3(0.0f, ToRadians(-90.0f), 0.0f));

	tankEntities = EntityManager.GetTankEntities();
	
	//// Assign patrol paths to each tank
	//for (TInt32 i = 0; i < tankEntities.size(); i++)
	//{
	//	 tankEntities[i]->SetPatrolPoints(tankPatrolPaths[i]);
	//}

	/////////////////////////////
	// Camera / light setup

	// Set camera position and clip planes
	FreeMovingCamera = new CCamera(CVector3(0.0f, 75.0f, -160.0f), CVector3(ToRadians(30.0f), 0, 0));
	FreeMovingCamera->SetNearFarClip(1.0f, 20000.0f);

	MainCamera = FreeMovingCamera;

	// Sunlight and light in building
	Lights[0] = new CLight(CVector3(-5000.0f, 4000.0f, -10000.0f), SColourRGBA(1.0f, 0.9f, 0.6f), 15000.0f);
	Lights[1] = new CLight(CVector3(6.0f, 7.5f, 40.0f), SColourRGBA(1.0f, 0.0f, 0.0f), 1.0f);

	// Ambient light level
	AmbientLight = SColourRGBA(0.6f, 0.6f, 0.6f, 1.0f);

	return true;
}


// Release everything in the scene
void SceneShutdown()
{
	// Release render methods
	ReleaseMethods();

	// Release lights
	for (int light = NumLights - 1; light >= 0; --light)
	{
		delete Lights[light];
	}

	// Release camera
	delete MainCamera;

	// Destroy all entities
	EntityManager.DestroyAllEntities();
	EntityManager.DestroyAllTemplates();
}


//-----------------------------------------------------------------------------
// Game Helper functions
//-----------------------------------------------------------------------------

// Get UID of tank A (team 0) or B (team 1)
TEntityUID GetTankUID(int team)
{
	return (team == 0) ? tanks[0] : tanks[1];
}

//-----------------------------------------------------------------------------
// Game loop functions
//-----------------------------------------------------------------------------

// Draw one frame of the scene
void RenderScene( float updateTime )
{
	// Setup the viewport - defines which part of the back-buffer we will render to (usually all of it)
	D3D10_VIEWPORT vp;
	vp.Width  = ViewportWidth;
	vp.Height = ViewportHeight;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pd3dDevice->RSSetViewports( 1, &vp );

	// Select the back buffer and depth buffer to use for rendering
	g_pd3dDevice->OMSetRenderTargets( 1, &BackBufferRenderTarget, DepthStencilView );
	
	// Clear previous frame from back buffer and depth buffer
	g_pd3dDevice->ClearRenderTargetView( BackBufferRenderTarget, &AmbientLight.r );
	g_pd3dDevice->ClearDepthStencilView( DepthStencilView, D3D10_CLEAR_DEPTH, 1.0f, 0 );

	// Update camera aspect ratio based on viewport size - for better results when changing window size
	MainCamera->SetAspect( static_cast<TFloat32>(ViewportWidth) / ViewportHeight );

	// Set camera and light data in shaders
	MainCamera->CalculateMatrices();
	SetCamera(MainCamera);
	SetAmbientLight(AmbientLight);
	SetLights(&Lights[0]);

	// Render entities and draw on-screen text
	EntityManager.RenderAllEntities();
	RenderSceneText( updateTime );

    // Present the backbuffer contents to the display
	SwapChain->Present( 0, 0 );
}


// Render a single text string at the given position in the given colour, may optionally centre it
void RenderText( const string& text, int X, int Y, float r, float g, float b, bool centre = false )
{
	RECT rect;
	if (!centre)
	{
		SetRect( &rect, X, Y, 0, 0 );
		OSDFont->DrawText( NULL, text.c_str(), -1, &rect, DT_NOCLIP, D3DXCOLOR( r, g, b, 1.0f ) );
	}
	else
	{
		SetRect( &rect, X - 100, Y, X + 100, 0 );
		OSDFont->DrawText( NULL, text.c_str(), -1, &rect, DT_CENTER | DT_NOCLIP, D3DXCOLOR( r, g, b, 1.0f ) );
	}
}

// Render on-screen text each frame
void RenderSceneText( float updateTime )
{
	// Accumulate update times to calculate the average over a given period
	SumUpdateTimes += updateTime;
	++NumUpdateTimes;
	if (SumUpdateTimes >= UpdateTimePeriod)
	{
		AverageUpdateTime = SumUpdateTimes / NumUpdateTimes;
		SumUpdateTimes = 0.0f;
		NumUpdateTimes = 0;
	}

	// Write FPS text string
	stringstream outText;
	if (AverageUpdateTime >= 0.0f)
	{
		outText << "Frame Time: " << updateTime * 1000.0f << "ms" << endl
				<< "FPS:" << 1.0f / updateTime << endl;
		RenderText(outText.str(), 0, 0, 1.0f, 1.0f, 0.0f);
		outText.str("");
	}

	// Calculate nearest entity  to mouse cursor
	NearestEntity = 0;
	TFloat32 nearestDistance = 50;
	EntityManager.BeginEnumEntities("", "", "Tank");
	CEntity* entity = EntityManager.EnumEntity();
	TInt32 X, Y = 0;
	while (entity != 0)
	{
		if (MainCamera->PixelFromWorldPt(entity->Position(), ViewportWidth, ViewportHeight, &X, &Y))
		{
			CVector2 entityPixel = CVector2((float)X, (float)Y);
			CVector2 mousePixel = CVector2((float)MouseX, (float)MouseY);
			TFloat32 pixelDistance = Distance(mousePixel, entityPixel);

			if (pixelDistance < nearestDistance)
			{
				NearestEntity = entity;
				nearestDistance = pixelDistance;
			}
		}
		entity = EntityManager.EnumEntity();
	}
	EntityManager.EndEnumEntities();

	ShowTankInfo(outText);
}

void ShowTankInfo(stringstream& outText)
{
	EntityManager.BeginEnumEntities("", "", "Tank");
	CEntity* entity = EntityManager.EnumEntity();
	while (entity != 0)
	{
		CTankEntity* tankEntity = static_cast<CTankEntity*>(entity);
		if (tankEntity != 0)
		{
			CVector3 entityPosition = tankEntity->Position();
			TInt32 X = 0.0f, Y = 0.0f;

			if (MainCamera->PixelFromWorldPt(entityPosition, ViewportWidth, ViewportHeight, &X, &Y))
			{
				string tankEntityName = tankEntity->GetName().c_str();
				string tankEntityState = tankEntity->GetState().c_str();
				TInt32 tankHP = tankEntity->GetHP();
				TInt32 shellsFired = tankEntity->GetShellsFired();
				TInt32 shellsAvailable = tankEntity->GetShellsAvailable();
				string tankIntersects = (ray.RayBoxIntersect(entityPosition, Normalise(tankEntity->GetTurretWorldMatrix().ZAxis()), "Building")) ? "Intersects" : "Not";
#
				// Display extented info
				if (ShowExtendedInformation)
				{
					outText << "Name: " << tankEntityName << endl
							<< "State: " << tankEntityState << endl
							<< "HP: " << tankHP << endl
							<< "Shells Avilable: " << shellsAvailable << endl
							<< "Shells Fired: " << shellsFired << endl
							<< "Hit: " << tankIntersects << endl
							<< "TargetPoint: " << tankEntity->GetTargetPosition().x << " " << tankEntity->GetTargetPosition().y << " " << tankEntity->GetTargetPosition().z << endl
							<< "CurrentPosition: " << tankEntity->Position().x << " " << tankEntity->Position().y << " " << tankEntity->Position().z << endl;

					if (SelectedEntity == entity)
					{
						outText << "-SELECTED ENTITY-" << endl
								<< "Press 'Mouse Left' on the tank again to evade" << endl
								<< "OR" << endl
								<< "Press 'Mouse Right' to a random location to move the tank there" << endl;
					}
				}
				else
				{
					outText << "Name: " << tankEntityName << endl;
				}

				CVector3 rgb;
				if (entity == NearestEntity || entity == SelectedEntity)
				{
					// Highlight nearest entity
					rgb.x = 1.0f;
					rgb.y = 1.0f;
					rgb.z = 0.0f;
				}
				else
				{
					// Display each team with different color
					rgb.x = 0.0f;
					rgb.y = tankEntity->GetTeam() == 0 ? 1.0f : 0.0f;
					rgb.z = tankEntity->GetTeam() == 1 ? 1.0f : 0.0f;
				}

				RenderText(outText.str(), X, Y, rgb.x, rgb.y, rgb.z);
				outText.str("");
			}
		}
		entity = EntityManager.EnumEntity();
	}

	EntityManager.EndEnumEntities();
}


// Update the scene between rendering
void UpdateScene( float updateTime )
{
	// Periodically spawn ammo crates
	/*if (SpawnNewAmmoCrateInterval > 0.0f)
	{
		SpawnNewAmmoCrateInterval -= updateTime;
	}
	else
	{
		if (NumOfSpawnedAmmoCrates < NumOfAmmoCrates)
		{
			EntityManager.CreateAmmoCrate("AmmoCrate", AmmoCrateRotSpeed,
				AmmoCrateRespawnTime, AmmoCratePickUpDistance, "Ammo" + NumOfSpawnedAmmoCrates, CVector3(0.0f, -10.0f, 0.0f), CVector3(0.0f, 0.0f, 0.0f), CVector3(1.0f, 1.0f, 1.0f));
			NumOfSpawnedAmmoCrates++;
			SpawnNewAmmoCrateInterval = 20.0f;
		}
	}*/

	// Call all entity update functions
	EntityManager.UpdateAllEntities( updateTime );

	// Set camera speeds
	// Key F1 used for full screen toggle
	if (KeyHit(Key_F2)) CameraMoveSpeed = 5.0f;
	if (KeyHit(Key_F3)) CameraMoveSpeed = 40.0f;

	if (MainCamera == FreeMovingCamera)
	{
		// Move the camera
		MainCamera->Control( Key_Up, Key_Down, Key_Left, Key_Right, Key_W, Key_S, Key_A, Key_D, CameraMoveSpeed * updateTime, CameraRotSpeed * updateTime );
	}

	// Toggle show extended information
	if (KeyHit(Key_0))
	{
		ShowExtendedInformation = !ShowExtendedInformation;
	}

	// Start game
	if (KeyHit(Key_1))
	{
		EntityManager.BeginEnumEntities("", "", "Tank");
		CEntity* entity = EntityManager.EnumEntity();
		while (entity != 0)
		{
			SMessage msg;
			msg.from = SystemUID;
			msg.type = Msg_Start;
			Messenger.SendMessage(entity->GetUID(), msg);

			entity = EntityManager.EnumEntity();
		}
		EntityManager.EndEnumEntities();
	}

	// Stop game
	if (KeyHit(Key_2))
	{
		EntityManager.BeginEnumEntities("", "", "Tank");
		CEntity* entity = EntityManager.EnumEntity();
		while (entity != 0)
		{
			SMessage msg;
			msg.from = SystemUID;
			msg.type = Msg_Stop;
			Messenger.SendMessage(entity->GetUID(), msg);

			entity = EntityManager.EnumEntity();
		}
		EntityManager.EndEnumEntities();
	}

	// Chase camera functionality (NOTE: I am using a 60% keyboard so I don't have a num pad so I replaced it with other keys)
	// Go through all the tanks chase cameras in an ascending order (tank0 -> tank1 -> tankn)
	if (KeyHit(Key_X))
	{
		CurrentChaseCameraIndex = CurrentChaseCameraIndex == tankEntities.size() - 1 ? CurrentChaseCameraIndex = 0 : CurrentChaseCameraIndex += 1;
		MainCamera = tankEntities[CurrentChaseCameraIndex]->GetChaseCamera();
	}

	// Go through all the tanks chase cameras in an descending order (tankn -> tank1 -> tank0)
	if (KeyHit(Key_Z))
	{
		CurrentChaseCameraIndex = CurrentChaseCameraIndex == 0 ? CurrentChaseCameraIndex = tankEntities.size() - 1 : CurrentChaseCameraIndex -= 1;
		MainCamera = tankEntities[CurrentChaseCameraIndex]->GetChaseCamera();
	}

	// Switch back to the free moving camera
	if (KeyHit(Key_C))
	{
		MainCamera = FreeMovingCamera;
	}

	// Camera picking (Tank enters evade state)
	if (KeyHit(Mouse_LButton) && NearestEntity != 0)
	{
		CTankEntity* selectedTankEntity = static_cast<CTankEntity*>(NearestEntity);
		if (selectedTankEntity)
		{
			if (selectedTankEntity->CanEnterEvadeState())
			{
				// If the tank is not selected 
				if (SelectedEntity == 0)
				{
					SelectedEntity = NearestEntity;
				}
				else if (SelectedEntity == NearestEntity)
				{
					// Not setting the target point here actually Im just marking the IsControlledByPlayer flag as false to calculate a new point
					selectedTankEntity->SetTargetPoint(CVector3::kOrigin);

					SMessage msg;
					msg.from = SystemUID;
					msg.type = Msg_Evade;
					Messenger.SendMessage(NearestEntity->GetUID(), msg);
					SelectedEntity = 0;
				}
			}
		}
	}

	// Camera picking (Move tank towards mouse cursor)
	if (KeyHit(Mouse_RButton) && SelectedEntity != 0)
	{	
		CTankEntity* selectedTankEntity = static_cast<CTankEntity*>(SelectedEntity);
		if (selectedTankEntity)
		{
			if (selectedTankEntity->CanEnterEvadeState())
			{
				CVector3 mouseWorld = MainCamera->WorldPtFromPixel(MouseX, MouseY, ViewportWidth, ViewportHeight);
				selectedTankEntity->SetTargetPoint(CVector3(mouseWorld.x, selectedTankEntity->Position().y, mouseWorld.z), true);

				SMessage msg;
				msg.from = SystemUID;
				msg.type = Msg_Evade;
				Messenger.SendMessage(SelectedEntity->GetUID(), msg);
				SelectedEntity = 0;
			}
			
		}
	}
}


} // namespace gen
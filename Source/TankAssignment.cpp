/*******************************************
	TankAssignment.cpp

	Shell scene and game functions
********************************************/

#include <sstream>
#include <string>
using namespace std;

#include <d3d10.h>
#include <d3dx10.h>

#include <string> 
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
#include "CParticleSystem.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx10.h"

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

// Constructors
CEntityManager EntityManager;
CParseLevel LevelParser(&EntityManager);
CRayCast ray = &EntityManager;
CParticalSystem particleSystem;

// Tank UIDs
//TEntityUID TankA;
//TEntityUID TankB;
const INT32 NumOfTanks = 6;
const INT32 NumOfPatrolPoints = 3;

TEntityUID tanks[NumOfTanks];
vector<CTankEntity*> tankEntities;
map<string, CTankEntity*> tankEntitiesMap;
CEntity* NearestEntity = 0;
CEntity* SelectedEntity = 0;

// Other scene elements
const INT32 NumLights = 2;
CLight*  Lights[NumLights];
SColourRGBA AmbientLight;
CCamera* m_MainCamera;
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
bool ShouldExitGame = false;
TFloat32 TimeToExitGame = 5.0f;
//-----------------------------------------------------------------------------
// Game Helper functions
//-----------------------------------------------------------------------------

// Get UID of tank A (team 0) or B (team 1)
TEntityUID GetTankUID(int team)
{
	return (team == 0) ? tanks[0] : tanks[1];
}

bool ExitGame() { return ShouldExitGame; }

void UpdateMainCamera(CCamera* camera) { m_MainCamera = camera; SetCamera(m_MainCamera); }

//-----------------------------------------------------------------------------
// Scene management
//-----------------------------------------------------------------------------

// Creates the scene geometry
bool SceneSetup()
{
	//////////////////////////////////////////////
	// Prepare render methods

	InitialiseMethods();
	if (LevelParser.ParseFile("Entities.xml"))
	{
		tankEntities = EntityManager.GetTankEntities();
		// Create a map of the tanks key: Tank's name 
		for each (CTankEntity* tankEntity in tankEntities)
		{
			tankEntitiesMap.insert({tankEntity->GetName(), tankEntity});
		}
	}

	/////////////////////////////
	// Camera / light setup

	// Set camera position and clip planes
	FreeMovingCamera = new CCamera(CVector3(0.0f, 75.0f, -160.0f), CVector3(ToRadians(30.0f), 0, 0));
	FreeMovingCamera->SetNearFarClip(1.0f, 20000.0f);

	UpdateMainCamera(FreeMovingCamera);

	// Sunlight and light in building
	Lights[0] = new CLight(CVector3(-5000.0f, 4000.0f, -10000.0f), SColourRGBA(1.0f, 0.9f, 0.6f), 15000.0f);
	Lights[1] = new CLight(CVector3(6.0f, 7.5f, 40.0f), SColourRGBA(1.0f, 0.0f, 0.0f), 1.0f);

	// Ambient light level
	AmbientLight = SColourRGBA(0.6f, 0.6f, 0.6f, 1.0f);

	particleSystem.Setup();
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
	delete m_MainCamera;

	// Destroy all entities
	EntityManager.DestroyAllEntities();
	EntityManager.DestroyAllTemplates();
}


//-----------------------------------------------------------------------------
// Game loop functions
//-----------------------------------------------------------------------------

// Draw one frame of the scene
void RenderScene( float updateTime )
{
	tankEntities = EntityManager.GetTankEntities();

	//IMGUI
	//*******************************
	// Prepare ImGUI for this frame
	//*******************************
	ImGui_ImplDX10_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	//// Setup the viewport - defines which part of the back-buffer we will render to (usually all of it)
	//D3D10_VIEWPORT vp;
	//vp.Width  = ViewportWidth;
	//vp.Height = ViewportHeight;
	//vp.MinDepth = 0.0f;
	//vp.MaxDepth = 1.0f;
	//vp.TopLeftX = 0;
	//vp.TopLeftY = 0;
	//g_pd3dDevice->RSSetViewports( 1, &vp );

	//// Select the back buffer and depth buffer to use for rendering
	//g_pd3dDevice->OMSetRenderTargets( 1, &BackBufferRenderTarget, DepthStencilView );
	
	// Clear previous frame from back buffer and depth buffer
	g_pd3dDevice->ClearRenderTargetView( BackBufferRenderTarget, &AmbientLight.r );
	g_pd3dDevice->ClearDepthStencilView( DepthStencilView, D3D10_CLEAR_DEPTH, 1.0f, 0 );

	// Update camera aspect ratio based on viewport size - for better results when changing window size
	m_MainCamera->SetAspect( static_cast<TFloat32>(ViewportWidth) / ViewportHeight );

	// Set camera and light data in shaders
	m_MainCamera->CalculateMatrices();
	SetCamera(m_MainCamera);
	SetAmbientLight(AmbientLight);
	SetLights(&Lights[0]);

	// Render entities and draw on-screen text
	EntityManager.RenderAllEntities();
	RenderSceneText( updateTime );
	particleSystem.Render(updateTime);

	TankManagerGUI();
	
	//IMGUI
	//*******************************
	// Finalise ImGUI for this frame
	//*******************************
	ImGui::Render();
	//g_pd3dDevice->OMSetRenderTargets(1, &BackBufferRenderTarget, nullptr);
	ImGui_ImplDX10_RenderDrawData(ImGui::GetDrawData());

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
	string winningTeam = "";
	if (AverageUpdateTime >= 0.0f)
	{
		if (!EntityManager.GetWinningTeam(winningTeam))
		{
			outText << "Frame Time: " << updateTime * 1000.0f << "ms" << endl
				<< "FPS:" << 1.0f / updateTime << endl;
			RenderText(outText.str(), 0, 0, 1.0f, 1.0f, 0.0f);
		}
		else
		{
			TimeToExitGame -= updateTime;
			outText << winningTeam << " WAS VICTORIOUS!" << endl;
			RenderText(outText.str(), ViewportWidth / 2, ViewportHeight / 2, 1.0f, 1.0f, 0.0f);
			SMessage msg;
			msg.from = SystemUID;
			msg.type = Msg_Stop;
			for each (CTankEntity* tankEntity in tankEntities)
			{
				// In case a tank is destructing do not send a message because its gonna crash
				if (tankEntity->GetState() != "Destruct")
				{
					Messenger.SendMessage(tankEntity->GetUID(), msg);
				}
			}

			if (TimeToExitGame < 0.0f)
			{
				ShouldExitGame = true;
			}
		}
		outText.str("");
	}

	// Calculate nearest entity  to mouse cursor
	NearestEntity = 0;
	TFloat32 nearestDistance = 50;	
	TInt32 X, Y = 0;
	for each (CTankEntity * tankEntity in tankEntities)
	{
		if (m_MainCamera->PixelFromWorldPt(tankEntity->Position(), ViewportWidth, ViewportHeight, &X, &Y))
		{
			CVector2 entityPixel = CVector2((float)X, (float)Y);
			CVector2 mousePixel = CVector2((float)MouseX, (float)MouseY);
			TFloat32 pixelDistance = Distance(mousePixel, entityPixel);

			if (pixelDistance < nearestDistance)
			{
				NearestEntity = tankEntity;
				nearestDistance = pixelDistance;
			}
		}
	}
	ShowTankInfo(outText);
}

void ShowTankInfo(stringstream& outText)
{
	for each (CTankEntity* tankEntity in tankEntities)
	{
		CVector3 entityPosition = tankEntity->Position();
		TInt32 X = 0, Y = 0;

		if (m_MainCamera->PixelFromWorldPt(entityPosition, ViewportWidth, ViewportHeight, &X, &Y))
		{
			string tankEntityName = tankEntity->GetName().c_str();
			string tankEntityState = tankEntity->GetState().c_str();
			TInt32 tankHP = tankEntity->GetHP();
			TInt32 shellsFired = tankEntity->GetShellsFired();
			TInt32 shellsAvailable = tankEntity->GetShellsAvailable();
			string tankIntersects = (ray.RayBoxIntersect(entityPosition, Normalise(tankEntity->GetTurretWorldMatrix().ZAxis()), "Building")) ? "Intersects" : "Not";

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

				if (SelectedEntity == tankEntity)
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
			if (tankEntity == NearestEntity || tankEntity == SelectedEntity)
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
}

// Update the scene between rendering
void UpdateScene( float updateTime )
{
	// Call all entity update functions
	EntityManager.UpdateAllEntities( updateTime );
	particleSystem.Update(updateTime);

	// Set camera speeds
	// Key F1 used for full screen toggle
	if (KeyHit(Key_F2)) CameraMoveSpeed = 5.0f;
	if (KeyHit(Key_F3)) CameraMoveSpeed = 40.0f;
	if (KeyHit(Key_F4)) particleSystem.ResetParticles();

	if (m_MainCamera == FreeMovingCamera)
	{
		// Move the camera
		m_MainCamera->Control( Key_Up, Key_Down, Key_Left, Key_Right, Key_W, Key_S, Key_A, Key_D, CameraMoveSpeed * updateTime, CameraRotSpeed * updateTime );
	}

	// Toggle show extended information
	if (KeyHit(Key_0))
	{
		ShowExtendedInformation = !ShowExtendedInformation;
	}

	// Start game
	if (KeyHit(Key_1))
	{
		SMessage msg;
		msg.from = SystemUID;
		msg.type = Msg_Start;
		for each (CTankEntity* tankEntity in tankEntities)
		{
			Messenger.SendMessage(tankEntity->GetUID(), msg);
		}
	}

	// Stop game
	if (KeyHit(Key_2))
	{
		SMessage msg;
		msg.from = SystemUID;
		msg.type = Msg_Stop;
		for each (CTankEntity* tankEntity in tankEntities)
		{
			Messenger.SendMessage(tankEntity->GetUID(), msg);
		}
	}

	// Chase camera functionality (NOTE: I am using a 60% keyboard so I don't have a num pad so I replaced it with other keys)
	// Go through all the tanks chase cameras in an ascending order (tank0 -> tank1 -> tankn)
	if (KeyHit(Key_X))
	{
		CurrentChaseCameraIndex = CurrentChaseCameraIndex == tankEntities.size() - 1 ? CurrentChaseCameraIndex = 0 : CurrentChaseCameraIndex += 1;
		UpdateMainCamera(tankEntities[CurrentChaseCameraIndex]->GetChaseCamera());
	}

	// Go through all the tanks chase cameras in an descending order (tankn -> tank1 -> tank0)
	if (KeyHit(Key_Z))
	{
		CurrentChaseCameraIndex = CurrentChaseCameraIndex == 0 ? CurrentChaseCameraIndex = tankEntities.size() - 1 : CurrentChaseCameraIndex -= 1;
		UpdateMainCamera(tankEntities[CurrentChaseCameraIndex]->GetChaseCamera());
	}

	// Switch back to the free moving camera
	if (KeyHit(Key_C))
	{
		UpdateMainCamera(FreeMovingCamera);
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
				CVector3 mouseWorld = m_MainCamera->WorldPtFromPixel(MouseX, MouseY, ViewportWidth, ViewportHeight);
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

void TankManagerGUI(bool* p_open)
{
	IM_ASSERT(ImGui::GetCurrentContext() != NULL && "Missing dear imgui context. Refer to examples app!"); // Exceptionally add an extra assert here for people confused with initial dear imgui setup

	// Examples Apps (accessible from the "Examples" menu)
	static bool show_app_documents = false;
	static bool show_app_main_menu_bar = false;
	static bool show_app_console = false;
	static bool show_app_log = false;
	static bool show_app_layout = false;
	static bool show_app_property_editor = false;
	static bool show_app_long_text = false;
	static bool show_app_auto_resize = false;
	static bool show_app_constrained_resize = false;
	static bool show_app_simple_overlay = false;
	static bool show_app_window_titles = false;
	static bool show_app_custom_rendering = false;

	// Dear ImGui Apps (accessible from the "Tools" menu)
	static bool show_app_metrics = false;
	static bool show_app_style_editor = false;
	static bool show_app_about = false;

	if (show_app_metrics) { ImGui::ShowMetricsWindow(&show_app_metrics); }
	if (show_app_style_editor) { ImGui::Begin("Style Editor", &show_app_style_editor); ImGui::ShowStyleEditor(); ImGui::End(); }
	if (show_app_about) { ImGui::ShowAboutWindow(&show_app_about); }

	// Demonstrate the various window flags. Typically you would just use the default!
	static bool no_titlebar = false;
	static bool no_scrollbar = false;
	static bool no_menu = false;
	static bool no_move = false;
	static bool no_resize = false;
	static bool no_collapse = false;
	static bool no_close = false;
	static bool no_nav = false;
	static bool no_background = false;
	static bool no_bring_to_front = false;

	ImGuiWindowFlags window_flags = 0;
	if (no_titlebar)        window_flags |= ImGuiWindowFlags_NoTitleBar;
	if (no_scrollbar)       window_flags |= ImGuiWindowFlags_NoScrollbar;
	if (!no_menu)           window_flags |= ImGuiWindowFlags_MenuBar;
	if (no_move)            window_flags |= ImGuiWindowFlags_NoMove;
	if (no_resize)          window_flags |= ImGuiWindowFlags_NoResize;
	if (no_collapse)        window_flags |= ImGuiWindowFlags_NoCollapse;
	if (no_nav)             window_flags |= ImGuiWindowFlags_NoNav;
	if (no_background)      window_flags |= ImGuiWindowFlags_NoBackground;
	if (no_bring_to_front)  window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
	if (no_close)           p_open = NULL; // Don't pass our bool* to Begin

	// We specify a default position/size in case there's no data in the .ini file. Typically this isn't required! We only do it to make the Demo applications a little more welcoming.
	ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);

	// Main body of the Demo window starts here.
	if (!ImGui::Begin("Nicolas Nouhi - Tank Assignment GUI Menu", p_open, window_flags))
	{
		// Early out if the window is collapsed, as an optimization.
		ImGui::End();
		return;
	}

	// Most "big" widgets share a common width settings by default.
	//ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.65f);    // Use 2/3 of the space for widgets and 1/3 for labels (default)
	ImGui::PushItemWidth(ImGui::GetFontSize() * -12);           // Use fixed width for labels (by passing a negative value), the rest goes to widgets. We choose a width proportional to our font size.

	ImGui::Text("GUI that acts like a tank manager. (%s)", IMGUI_VERSION);
	ImGui::Text("You can spectate and adjust different attributes of the tanks. (%s)", IMGUI_VERSION);
	ImGui::Spacing();

	if (ImGui::CollapsingHeader("Configuration"))
	{
		ImGuiIO& io = ImGui::GetIO();

		if (ImGui::TreeNode("Configuration##2"))
		{
			ImGui::CheckboxFlags("io.ConfigFlags: NavEnableKeyboard", (unsigned int*)&io.ConfigFlags, ImGuiConfigFlags_NavEnableKeyboard);
			ImGui::CheckboxFlags("io.ConfigFlags: NavEnableGamepad", (unsigned int*)&io.ConfigFlags, ImGuiConfigFlags_NavEnableGamepad);
			ImGui::CheckboxFlags("io.ConfigFlags: NavEnableSetMousePos", (unsigned int*)&io.ConfigFlags, ImGuiConfigFlags_NavEnableSetMousePos);
			ImGui::CheckboxFlags("io.ConfigFlags: NoMouse", (unsigned int*)&io.ConfigFlags, ImGuiConfigFlags_NoMouse);
			if (io.ConfigFlags & ImGuiConfigFlags_NoMouse) // Create a way to restore this flag otherwise we could be stuck completely!
			{
				if (fmodf((float)ImGui::GetTime(), 0.40f) < 0.20f)
				{
					ImGui::SameLine();
					ImGui::Text("<<PRESS SPACE TO DISABLE>>");
				}
				if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Space)))
					io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
			}
			ImGui::CheckboxFlags("io.ConfigFlags: NoMouseCursorChange", (unsigned int*)&io.ConfigFlags, ImGuiConfigFlags_NoMouseCursorChange);
			ImGui::Checkbox("io.ConfigInputTextCursorBlink", &io.ConfigInputTextCursorBlink);
			ImGui::Checkbox("io.ConfigWindowsResizeFromEdges", &io.ConfigWindowsResizeFromEdges);
			ImGui::Checkbox("io.ConfigWindowsMoveFromTitleBarOnly", &io.ConfigWindowsMoveFromTitleBarOnly);
			ImGui::Checkbox("io.MouseDrawCursor", &io.MouseDrawCursor);
			ImGui::TreePop();
			ImGui::Separator();
		}

		if (ImGui::TreeNode("Backend Flags"))
		{
			ImGuiBackendFlags backend_flags = io.BackendFlags; // Make a local copy to avoid modifying actual back-end flags.
			ImGui::CheckboxFlags("io.BackendFlags: HasGamepad", (unsigned int*)&backend_flags, ImGuiBackendFlags_HasGamepad);
			ImGui::CheckboxFlags("io.BackendFlags: HasMouseCursors", (unsigned int*)&backend_flags, ImGuiBackendFlags_HasMouseCursors);
			ImGui::CheckboxFlags("io.BackendFlags: HasSetMousePos", (unsigned int*)&backend_flags, ImGuiBackendFlags_HasSetMousePos);
			ImGui::CheckboxFlags("io.BackendFlags: RendererHasVtxOffset", (unsigned int*)&backend_flags, ImGuiBackendFlags_RendererHasVtxOffset);
			ImGui::TreePop();
			ImGui::Separator();
		}

		if (ImGui::TreeNode("Style"))
		{
			ImGui::ShowStyleEditor();
			ImGui::TreePop();
			ImGui::Separator();
		}

		if (ImGui::TreeNode("Capture/Logging"))
		{
			ImGui::TextWrapped("The logging API redirects all text output so you can easily capture the content of a window or a block. Tree nodes can be automatically expanded.");
			ImGui::LogButtons();
			ImGui::TextWrapped("You can also call ImGui::LogText() to output directly to the log without a visual output.");
			if (ImGui::Button("Copy \"Hello, world!\" to clipboard"))
			{
				ImGui::LogToClipboard();
				ImGui::LogText("Hello, world!");
				ImGui::LogFinish();
			}
			ImGui::TreePop();
		}
	}

	if (ImGui::CollapsingHeader("Window options"))
	{
		ImGui::Checkbox("No titlebar", &no_titlebar); ImGui::SameLine(150);
		ImGui::Checkbox("No scrollbar", &no_scrollbar); ImGui::SameLine(300);
		ImGui::Checkbox("No menu", &no_menu);
		ImGui::Checkbox("No move", &no_move); ImGui::SameLine(150);
		ImGui::Checkbox("No resize", &no_resize); ImGui::SameLine(300);
		ImGui::Checkbox("No collapse", &no_collapse);
		ImGui::Checkbox("No close", &no_close); ImGui::SameLine(150);
		ImGui::Checkbox("No nav", &no_nav); ImGui::SameLine(300);
		ImGui::Checkbox("No background", &no_background);
		ImGui::Checkbox("No bring to front", &no_bring_to_front);
	}

	if (ImGui::CollapsingHeader("Choose Tank - Modify Tank's Properties"))
	{
		ImGui::Text("Select Tank");
		ImGui::Separator();
		ImGui::Spacing();

		// Create buttons for each alive tank
		ImGui::Columns(2, "mycolumns"); 
		ImGui::Text("Team A"); ImGui::NextColumn();
		ImGui::Text("Team B"); ImGui::NextColumn();
		static int selectedTank = -1;
		for (int i = 0; i < tankEntities.size(); i++)
		{
			if (tankEntities[i]->GetTeam() == 1)
			{
				if (ImGui::GetColumnIndex() == 0)
				{
					ImGui::NextColumn();
				}
			}
			
			char tankName[100];
			strcpy(tankName, "Tank: ");
			strcat(tankName, tankEntities[i]->GetName().c_str());
			if (ImGui::Button(tankName))
			{
				selectedTank = i;
			}
		}
		ImGui::Columns(1);
		ImGui::Separator();

		// Tank Properties
		// If user selected any tank
		if (selectedTank != -1)
		{
			ImGui::Spacing();
			ImGui::Text("Spectate");
			ImGui::Separator();

			// Spectate selected tank
			string tankName = tankEntities[selectedTank]->GetName();
			string key = tankName;
			char result[100];
			strcpy(result, "Spectate ");
			strcat(result, tankName.c_str());
			if (ImGui::Button(result))
			{
				CurrentChaseCameraIndex = selectedTank;
				UpdateMainCamera(tankEntitiesMap[key]->GetChaseCamera());
			}
			// end of Spectate selected tank

			ImGui::Spacing();
			ImGui::Text("Modify Tank Properties");
			ImGui::Separator();
		
			// Modify tank's HP and ammo
			ImGui::Text("Modify HP and Ammo");

			// Display current HP 
			char currentHP[100];
			strcpy(currentHP, "Current HP: ");
			strcat(currentHP, std::to_string(tankEntitiesMap[key]->GetHP()).c_str());
			ImGui::Text(currentHP);

			// Display current Ammo 
			char currentAmmo[100];
			strcpy(currentAmmo, "Current Ammo: ");
			strcat(currentAmmo, std::to_string(tankEntitiesMap[key]->GetShellsAvailable()).c_str());
			ImGui::Text(currentAmmo);

			static int amountToRestore = 0;
			TInt32 amountToRestoreLimit = 0;
			ImGui::InputInt("", &amountToRestore);
			if (amountToRestore > 0)
			{
				if (ImGui::Button("Modify Hp"))
				{
					amountToRestoreLimit = tankEntitiesMap[key]->GetMaxHP() - tankEntitiesMap[key]->GetHP();
					tankEntitiesMap[key]->RestoreHealth(Min(amountToRestoreLimit, amountToRestore));
				}

				ImGui::SameLine();
			
				if (ImGui::Button("Modify Ammo"))
				{
					amountToRestoreLimit = tankEntitiesMap[key]->GetShellCapacity() - tankEntitiesMap[key]->GetShellsAvailable();
					tankEntitiesMap[key]->RestoreShells(Min(amountToRestoreLimit, amountToRestore));
				}
			}
			
			ImGui::Spacing();
			ImGui::Text("Update tank's state");

			// Display current State
			char currentState[100];
			strcpy(currentState, "Current State: ");
			strcat(currentState, tankEntitiesMap[key]->GetState().c_str());
			ImGui::Text(currentState);

			// Modify the tank's state
			SMessage msg;
			msg.from = SystemUID;

			// Destroy state
			if (ImGui::Button("Destroy"))
			{
				msg.type = Msg_Hit;
				msg.damageToApply = tankEntitiesMap[key]->GetMaxHP();
				Messenger.SendMessage(tankEntitiesMap[key]->GetUID(), msg);
				tankEntitiesMap.erase(key);
				tankEntities.erase(tankEntities.begin() + selectedTank);
				selectedTank = -1;
			}

			ImGui::SameLine();

			// Patrol state
			if (ImGui::Button("Patrol"))
			{
				msg.type = Msg_Patrol;
				Messenger.SendMessage(tankEntitiesMap[key]->GetUID(), msg);
			}

			ImGui::SameLine();

			// Evade state
			if (ImGui::Button("Evade"))
			{
				msg.type = Msg_Evade;
				Messenger.SendMessage(tankEntitiesMap[key]->GetUID(), msg);
			}

			ImGui::SameLine();

			// Inactive state
			if (ImGui::Button("Inactive"))
			{
				msg.type = Msg_Stop;
				Messenger.SendMessage(tankEntitiesMap[key]->GetUID(), msg);
			}
			
		}
		// end of Tank Properties

		/*for (int i = 0; i < numberOfTanks; ++i)
		{
			delete[] listbox_items[i];
		}
		delete[] listbox_items;*/
	}

	// End of ShowDemoWindow()
	ImGui::End();
}

} // namespace gen

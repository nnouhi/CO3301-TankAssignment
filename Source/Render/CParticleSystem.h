/*******************************************
	CParticleSystem.h
	Created a class to support a particle system (code obtained from DX10Particles lab)
********************************************/

#pragma once
#include "Shader.h"   // Vertex / pixel shader support
#include "../Scene/Camera.h"

namespace gen
{
	class CParticalSystem
	{
		private:
			// Textures for models
			// In DX10, textures used in shaders are replaced by "shader resource views"
			ID3D10ShaderResourceView* ParticleTexture;

			// Shader variables
			ID3D10VertexShader* VS_PassThruGS = NULL;
			ID3D10GeometryShader* GS_ParticlesUpdate = NULL;
			ID3D10GeometryShader* GS_ParticlesDraw = NULL;
			ID3D10PixelShader* PS_TexOnly = NULL;

			// Retained vertex shader code for DX10
			ID3D10Blob* VSCode_PassThruGS = NULL;

			// DX10 shader constant buffers
			ID3D10Buffer* GS_ConstBuffer = NULL;

			// C++ forms of shader constant buffers. Careful alignment required
			#define AL16 __declspec(align(16))	// Align any variable prefixed with AL16 to a 16 byte (float4) boundary,
										// which is (roughly) what the HLSL compiler does

			// The vertex shaders use the same two matrices
			struct VS_CONSTS
			{
				AL16 D3DXMATRIX WorldMatrix;
				AL16 D3DXMATRIX ViewProjMatrix;
			};

			// Geometry shader constants
			struct GS_CONSTS
			{
				AL16 D3DXMATRIX  ViewProjMatrix;
				AL16 D3DXMATRIX  InvViewMatrix;
				float       UpdateTime; // GPU is performing some scene update, so it needs timing info
			};

			// Pixel shader constants
			struct PS_CONSTS
			{
				// Current lighting information - ambient + two point lights
				AL16 D3DXCOLOR   BaseColour;       // Shared colour value between shaders (ambient colour or plain colour)
				AL16 D3DXVECTOR4 Light1Position;   // Point light 1 - position
				AL16 D3DXCOLOR   Light1Colour;     // Point light 1 - colour
				AL16 float       Light1Brightness; // Point light 1 - brightness
				AL16 D3DXVECTOR4 Light2Position;   // Point light 2...
				AL16 D3DXCOLOR   Light2Colour;
				AL16 float       Light2Brightness;

				// Shininess of material and camera position needed for specular calculation
				AL16 D3DXVECTOR4 CameraPosition;
				AL16 float       SpecularPower;
			};

			// Also create a single pointer to each of the above structures for use in rendering code
			GS_CONSTS* GS_Consts;
			unsigned int offset;

			//*****************************************************************************
			// Particle Data
			//*****************************************************************************

			const int MaxNumParticles = 200000;
			int NumParticles = 200000;

			// The particles are going to be rendered in one draw call as a point list. The points will be expanded to quads in
			// the geometry shader. This point list needs to be initialised in a vertex buffer, and will be updated using stream out.
			// So the layout of the vertex (particle) data needs to be specified in three ways: for our use in C++, for the creation
			// of the vertex buffer, and to indicate what data that will be updated using the stream output stage

			// C++ data structure for a particle. Contains both rendering and update information. Contrast this with the instancing
			// lab, where there were two seperate structures. The GPU will do all the work in this example
			struct SParticle
			{
				D3DXVECTOR3 Position;
				D3DXVECTOR3 Velocity;
				float       Life;
			};

			// An array of element descriptions to create the vertex buffer, *** This must match the SParticle structure ***
			// Contents explained with an example: the third entry in the list below indicates that 24 bytes into each
			// vertex (into the particle structure) is a TEXCOORD1 which is a single float - this is the Life value (recall
			// that non-standard shader input (such as Life) uses TEXCOORD semantics
			D3D10_INPUT_ELEMENT_DESC ParticleElts[3] =
			{
				// Semantic  &  Index,   Type (eg. 1st one is float3),  Slot,   Byte Offset,  Instancing or not (not here), --"--
				{ "POSITION",   0,       DXGI_FORMAT_R32G32B32_FLOAT,   0,      0,            D3D10_INPUT_PER_VERTEX_DATA,    0 },
				{ "TEXCOORD",   0,       DXGI_FORMAT_R32G32B32_FLOAT,   0,      12,           D3D10_INPUT_PER_VERTEX_DATA,    0 },
				{ "TEXCOORD",   1,       DXGI_FORMAT_R32_FLOAT,         0,      24,           D3D10_INPUT_PER_VERTEX_DATA,    0 },
			};
			const unsigned int NumParticleElts = sizeof(ParticleElts) / sizeof(D3D10_INPUT_ELEMENT_DESC);

			// Vertex layout and buffer described by the above - buffer held on the GPU
			ID3D10InputLayout* ParticleLayout;
			ID3D10Buffer* ParticleBufferFrom;
			ID3D10Buffer* ParticleBufferTo;

			// Third specification is for the data that will be updated using the stream out stage. This array indicates which
			// outputs of the vertex or geometry shader will be streamed back into GPU memory. Again, in this case the structure
			// below must match the SParticle structure above (although more complex stream out arrangements are possible)
			D3D10_SO_DECLARATION_ENTRY ParticleStreamOutDecl[3] =
			{
				// Semantic name & index, start component, component count, output slot (always 0 here)
				{ "POSITION", 0,   0, 3,   0 }, // Output first 3 components of "POSITION",     - i.e. Position, 3 floats (xyz)
				{ "TEXCOORD", 0,   0, 3,   0 }, // Output the first 3 components of "TEXCOORD0" - i.e. Velocity, 3 floats (xyz)
				{ "TEXCOORD", 1,   0, 1,   0 }, // Output the first component of "TEXCOORD1"    - i.e. Life, 1 float
			};

			//CCamera* m_MainCamera;
		public:
			CParticalSystem(/*CCamera* mainCamera*/);

			~CParticalSystem();

			void Update(TFloat32 updateTime);
			
			void Render(TFloat32 updateTime);
			
			bool Setup();
			
			void ResetParticles();
			
			void SetTexture(int texNum, ID3D10ShaderResourceView* texture);
			
			ID3D10ShaderResourceView* LoadTexture(const string& fileName);
			
			void DepthStencilEnable(bool depth = true, bool depthWrite = true, bool stencil = false);
			
			void BlendEnable(bool blend, D3D10_BLEND source = D3D10_BLEND_ONE, D3D10_BLEND dest = D3D10_BLEND_ZERO,
				D3D10_BLEND_OP operation = D3D10_BLEND_OP_ADD);
	};
}
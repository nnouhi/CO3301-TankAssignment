/*******************************************
	CParticleSystem.h
	Created a class to support a particle system (code obtained from DX10Particles lab)
********************************************/

#include "CParticleSystem.h"

namespace gen
{
	extern CCamera* m_MainCamera;

	CParticalSystem::CParticalSystem(/*CCamera* mainCamera*/)
	{
		offset = 0;
	}

	CParticalSystem::~CParticalSystem()
	{
		// Release DirectX allocated objects
		SAFE_RELEASE(ParticleBufferTo);
		SAFE_RELEASE(ParticleBufferFrom);
		SAFE_RELEASE(ParticleLayout);
		SAFE_RELEASE(GS_ConstBuffer);
		SAFE_RELEASE(PS_TexOnly);
		SAFE_RELEASE(GS_ParticlesDraw);
		SAFE_RELEASE(GS_ParticlesUpdate);
		SAFE_RELEASE(VSCode_PassThruGS);
		SAFE_RELEASE(VS_PassThruGS);
		SAFE_RELEASE(ParticleTexture);
	}

	bool CParticalSystem::Setup()
	{
		if (!LoadVertexShader("Source\\Render\\PassThruGS.vsh", &VS_PassThruGS, &VSCode_PassThruGS) ||
			!LoadStreamOutGeometryShader("Source\\Render\\DX10ParticlesUpdate.gsh", ParticleStreamOutDecl,
				sizeof(ParticleStreamOutDecl) / sizeof(D3D10_SO_DECLARATION_ENTRY),
				sizeof(SParticle), &GS_ParticlesUpdate) ||
			!LoadGeometryShader("Source\\Render\\DX10ParticlesDraw.gsh", &GS_ParticlesDraw) ||
			!LoadPixelShader("Source\\Render\\TexOnly.psh", &PS_TexOnly))
		{
			return false;
		}

		// Create constant buffers to access global variables in shaders. The buffers sized to hold the structures
		// declared above (see shader variables)
		GS_ConstBuffer = CreateConstantBuffer(sizeof(GS_CONSTS));

		// As these buffers are shared across all shaders, we can set them up for use now at set-up time
		SetGeometryConstantBuffer(GS_ConstBuffer);

		// Load textures to apply to models / particles
		ParticleTexture = LoadTexture("Media\\Flare.jpg");
		if (!ParticleTexture)
			return false;

		//*************************************************************************
		// Initialise particles

		// Create the vertex layout using the vertex elements declared near the top of the file. We must also send information
		// about the vertex shader that will accept vertices of this form
		g_pd3dDevice->CreateInputLayout(ParticleElts, NumParticleElts, VSCode_PassThruGS->GetBufferPointer(),
			VSCode_PassThruGS->GetBufferSize(), &ParticleLayout);


		// Set up some initial particle data. This will be transferred to the vertex buffer and the CPU will not use it again
		SParticle* particles = new SParticle[MaxNumParticles];
		for (int p = 0; p < MaxNumParticles; ++p)
		{
			particles[p].Position = D3DXVECTOR3(gen::Random(-10.0f, 10.0f), gen::Random(0.0f, 50.0f), gen::Random(-10.0f, 10.0f));
			particles[p].Velocity = D3DXVECTOR3(gen::Random(-40.0f, 40.0f), gen::Random(0.0f, 60.0f), gen::Random(-40.0f, 40.0f));
			particles[p].Life = (5.0f * p) / MaxNumParticles;
		}


		// Create / initialise the vertex buffers to hold the particles
		// Need to create two vertex buffers, because we cannot stream output to the same buffer than we are reading
		// the input from. Instead we input from one buffer, and stream out to another, and swap the buffers each frame
		D3D10_BUFFER_DESC bufferDesc;
		bufferDesc.BindFlags = D3D10_BIND_VERTEX_BUFFER | D3D10_BIND_STREAM_OUTPUT; // Vertex buffer using stream output
		bufferDesc.Usage = D3D10_USAGE_DEFAULT;                  // Default buffer use is read / write from GPU only
		bufferDesc.ByteWidth = MaxNumParticles * sizeof(SParticle); // Buffer size in bytes
		bufferDesc.CPUAccessFlags = 0;   // Indicates that CPU won't access this buffer at all after creation
		bufferDesc.MiscFlags = 0;
		D3D10_SUBRESOURCE_DATA initData; // Initial data (only needed in one of the buffers to start)
		initData.pSysMem = particles;
		if (FAILED(g_pd3dDevice->CreateBuffer(&bufferDesc, &initData, &ParticleBufferFrom)) ||
			FAILED(g_pd3dDevice->CreateBuffer(&bufferDesc, 0, &ParticleBufferTo)))
		{
			delete[] particles;
			return false;
		}
		delete[] particles;

		return true;
	}

	void CParticalSystem::Render(TFloat32 updateTime)
	{
		//////////////////////////
		// Particle Rendering

		// Set shaders for particle rendering - the vertex shader just passes the data to the 
		// geometry shader, which generates a camera-facing 2D quad from the particle world position 
		// The pixel shader is a very simple texture-only shader
		SetVertexShader(VS_PassThruGS);
		SetGeometryShader(GS_ParticlesDraw);
		SetPixelShader(PS_TexOnly);

		// Set constants for geometry shader, it needs the view/projection matrix to transform the
		// particles to 2D. It also needs the inverse view matrix (the camera's world matrix effectively)
		// to make all the particles face the camera
		GS_ConstBuffer->Map(D3D10_MAP_WRITE_DISCARD, NULL, (void**)&GS_Consts);
		GS_Consts->ViewProjMatrix = m_MainCamera->GetViewProjectionD3DXMatrix();
		D3DXMATRIX viewMatrix = m_MainCamera->GetViewD3DXMatrix();
		D3DXMATRIX invViewMatrix;
		D3DXMatrixInverse(&invViewMatrix, 0, &viewMatrix);
		GS_Consts->InvViewMatrix = invViewMatrix;
		GS_Consts->UpdateTime = updateTime;
		GS_ConstBuffer->Unmap();

		// Select a texture and set up additive blending (using helper function above)
		SetTexture(0, ParticleTexture);
		BlendEnable(true, D3D10_BLEND_ONE, D3D10_BLEND_ONE);
		DepthStencilEnable(true, false, false); // Fix sorting for blending

		// Set up particle vertex buffer / layout
		unsigned int particleVertexSize = sizeof(SParticle);
		offset = 0;
		g_pd3dDevice->IASetVertexBuffers(0, 1, &ParticleBufferFrom, &particleVertexSize, &offset);
		g_pd3dDevice->IASetInputLayout(ParticleLayout);

		// Indicate that this is a point list and render it
		g_pd3dDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_POINTLIST);
		g_pd3dDevice->Draw(NumParticles, 0);

		// Disable additive blending
		DepthStencilEnable();
		BlendEnable(false);
	}

	void CParticalSystem::Update(TFloat32 updateTime)
	{
		//////////////////////////
		// Particle Update

		// Shaders for particle update. Again the vertex shader does nothing, and this time we explicitly
		// switch off rendering by setting no pixel shader (also need to switch off depth buffer)
		// All the particle update work is done by the geometry shader
		SetVertexShader(VS_PassThruGS);
		SetGeometryShader(GS_ParticlesUpdate);
		SetPixelShader(0);
		DepthStencilEnable(false, false, false);

		// The updated particle data is streamed back to GPU memory, but we can't write to the same buffer
		// we are reading from, so it goes to a second (identical) particle buffer
		g_pd3dDevice->SOSetTargets(1, &ParticleBufferTo, &offset);

		// Calling draw doesn't actually draw anything, just performs the particle update
		g_pd3dDevice->Draw(NumParticles, 0);

		// Switch off stream output
		ID3D10Buffer* nullBuffer = 0;
		g_pd3dDevice->SOSetTargets(1, &nullBuffer, &offset);

		// Switch depth buffer back to default for subseqent rendering
		DepthStencilEnable();

		// Swap the two particle buffers
		ID3D10Buffer* temp = ParticleBufferFrom;
		ParticleBufferFrom = ParticleBufferTo;
		ParticleBufferTo = temp;
	}

	// Reset all the particles to their original positions
	void CParticalSystem::ResetParticles()
	{
		// Release existing particles
		SAFE_RELEASE(ParticleBufferFrom);

		// Just create a new particle buffer as before
		SParticle* particles = new SParticle[MaxNumParticles];
		for (int p = 0; p < MaxNumParticles; ++p)
		{
			particles[p].Position = D3DXVECTOR3(gen::Random(-10.0f, 10.0f), gen::Random(-50.0f, 50.0f), gen::Random(-10.0f, 10.0f));
			particles[p].Velocity = D3DXVECTOR3(gen::Random(-40.0f, 40.0f), gen::Random(0.0f, 60.0f), gen::Random(-40.0f, 40.0f));
			particles[p].Life = (5.0f * p) / MaxNumParticles;
		}

		// Create / initialise the vertex buffer to hold the particles
		D3D10_BUFFER_DESC bufferDesc;
		bufferDesc.BindFlags = D3D10_BIND_VERTEX_BUFFER | D3D10_BIND_STREAM_OUTPUT; // Vertex buffer using stream output
		bufferDesc.Usage = D3D10_USAGE_DEFAULT;                  // Default buffer use is read / write from GPU only
		bufferDesc.ByteWidth = MaxNumParticles * sizeof(SParticle); // Buffer size in bytes
		bufferDesc.CPUAccessFlags = 0;   // Indicates that CPU won't access this buffer at all after creation
		bufferDesc.MiscFlags = 0;
		D3D10_SUBRESOURCE_DATA initData;
		initData.pSysMem = particles;
		g_pd3dDevice->CreateBuffer(&bufferDesc, &initData, &ParticleBufferFrom);

		delete[] particles;
	}

	// Select the given texture into the given pixel shader slot
	void CParticalSystem::SetTexture(int texNum, ID3D10ShaderResourceView* texture)
	{
		g_pd3dDevice->PSSetShaderResources(texNum, 1, &texture);
	}

	//-----------------------------------------------------------------------------
	// Texture functions
	//-----------------------------------------------------------------------------

	// Load a texture
	ID3D10ShaderResourceView* CParticalSystem::LoadTexture(const string& fileName)
	{
		ID3D10ShaderResourceView* texture = NULL;

		// Create the texture using D3DX helper function
		// The D3DX10 helper function has extra parameters (all null here - look them up)
		HRESULT hr = D3DX10CreateShaderResourceViewFromFile(g_pd3dDevice, fileName.c_str(), NULL, NULL, &texture, NULL);
		if (FAILED(hr))
		{
			MessageBox(NULL, "Could not find texture map", "CO3303 Assignment: Nicolas Nouhi Assignment", MB_OK);
		}

		return texture;
	}

	//-----------------------------------------------------------------------------
	// State functions
	//-----------------------------------------------------------------------------

	// Helper function to enable / disable depth and stencil buffering
	// Can disable entire depth buffer use, or writes only (e.g. for sorting blended polygons)
	void CParticalSystem::DepthStencilEnable(bool depth, bool depthWrite, bool stencil)
	{
		D3D10_DEPTH_STENCIL_DESC dsDesc;
		ZeroMemory(&dsDesc, sizeof(dsDesc));
		dsDesc.DepthEnable = depth;
		dsDesc.DepthFunc = D3D10_COMPARISON_LESS;
		dsDesc.DepthWriteMask = depthWrite ? D3D10_DEPTH_WRITE_MASK_ALL : D3D10_DEPTH_WRITE_MASK_ZERO;
		dsDesc.StencilEnable = false;
		ID3D10DepthStencilState* dsState;
		g_pd3dDevice->CreateDepthStencilState(&dsDesc, &dsState);
		g_pd3dDevice->OMSetDepthStencilState(dsState, 1);
	}

	// Helper function to enable / disable blending modes
	void CParticalSystem::BlendEnable(bool blend, D3D10_BLEND source, D3D10_BLEND dest,
		D3D10_BLEND_OP operation)
	{
		D3D10_BLEND_DESC blendDesc;
		ZeroMemory(&blendDesc, sizeof(blendDesc));
		blendDesc.BlendEnable[0] = blend;
		blendDesc.SrcBlend = source;
		blendDesc.DestBlend = dest;
		blendDesc.BlendOp = operation;
		blendDesc.SrcBlendAlpha = D3D10_BLEND_ONE;
		blendDesc.DestBlendAlpha = D3D10_BLEND_ZERO;
		blendDesc.BlendOpAlpha = D3D10_BLEND_OP_ADD;
		blendDesc.RenderTargetWriteMask[0] = D3D10_COLOR_WRITE_ENABLE_ALL;
		ID3D10BlendState* blendState = NULL;
		g_pd3dDevice->CreateBlendState(&blendDesc, &blendState);
		g_pd3dDevice->OMSetBlendState(blendState, 0, 0xffffffff);
	}

}

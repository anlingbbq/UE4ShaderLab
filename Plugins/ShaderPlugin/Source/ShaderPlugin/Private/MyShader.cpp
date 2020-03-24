#include "MyShader.h"

#include "Engine/TextureRenderTarget2D.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "RenderGraphBuilder.h"
#include "Engine/World.h"
#include "SceneInterface.h"
#include "ShaderParameterUtils.h"
#include "Runtime/RenderCore/Public/PixelShaderUtils.h"
#include "Containers/DynamicRHIResourceArray.h"

class FSimpleScreenVertexBuffer : public FVertexBuffer
{
public:
	/** Initialize the RHI for this rendering resource */
	void InitRHI() override
	{
		TResourceArray<FFilterVertex, VERTEXBUFFER_ALIGNMENT> Vertices;
		Vertices.SetNumUninitialized(6);

		Vertices[0].Position = FVector4(-1, 1, 0, 1);
		Vertices[0].UV = FVector2D(0, 0);

		Vertices[1].Position = FVector4(1, 1, 0, 1);
		Vertices[1].UV = FVector2D(1, 0);

		Vertices[2].Position = FVector4(-1, -1, 0, 1);
		Vertices[2].UV = FVector2D(0, 1);

		Vertices[3].Position = FVector4(1, -1, 0, 1);
		Vertices[3].UV = FVector2D(1, 1);

		// Create vertex buffer. Fill buffer with initial data upon creation
		FRHIResourceCreateInfo CreateInfo(&Vertices);
		VertexBufferRHI = RHICreateVertexBuffer(Vertices.GetResourceDataSize(), BUF_Static, CreateInfo);
	}
};
TGlobalResource<FSimpleScreenVertexBuffer> GSimpleScreenVertexBuffer;

class FMyShaderVS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FMyShaderVS);

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}
	
	FMyShaderVS() {}

	FMyShaderVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer) { }
};

class FMyShaderPS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FMyShaderPS);

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}
	
	FMyShaderPS() {}
	FMyShaderPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer)
	{
		SimpleColor.Bind(Initializer.ParameterMap, TEXT("SimpleColor"));
	}

	void SetParameters(FRHICommandListImmediate& RHICmdList, const FLinearColor& MyColor)
	{
		SetShaderValue(RHICmdList, GetPixelShader(), SimpleColor, MyColor);
	}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << SimpleColor;
		return bShaderHasOutdatedParameters;
	}
	
private:
	FShaderParameter SimpleColor;
};

IMPLEMENT_GLOBAL_SHADER(FMyShaderVS, "/ShaderPlugin/Private/MyShader.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FMyShaderPS, "/ShaderPlugin/Private/MyShader.usf", "MainPS", SF_Pixel);

static void DrawMyShaderRenderTarget_RnderThread(FRHICommandListImmediate& RHICmdList,
	FTextureRenderTargetResource* OutRTRes,
	FLinearColor MyColor)
{
	check(IsInRenderingThread());

	SCOPED_DRAW_EVENT(RHICmdList, ShaderPlugin_MyShaderPixel);

	FRHIRenderPassInfo RenderPassInfo(OutRTRes->GetRenderTargetTexture(), ERenderTargetActions::Clear_Store);
	RHICmdList.BeginRenderPass(RenderPassInfo, TEXT("ShaderPlugin_OutputToRenderTarget"));

	auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	TShaderMapRef<FMyShaderVS> VertexShader(ShaderMap);
	TShaderMapRef<FMyShaderPS> PixelShader(ShaderMap);

	// Set the graphic pipeline state.
	FGraphicsPipelineStateInitializer GraphicsPSOInit;
	RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
	GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
	GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
	GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
	GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
	GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
	GraphicsPSOInit.PrimitiveType = PT_TriangleStrip;
	SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

	// Setup the pixel shader
	PixelShader->SetParameters(RHICmdList, MyColor);

	// Draw
	RHICmdList.SetStreamSource(0, GSimpleScreenVertexBuffer.VertexBufferRHI, 0);
	RHICmdList.DrawPrimitive(0, 2, 1);

	// Resolve render target
	RHICmdList.CopyToResolveTarget(OutRTRes->GetRenderTargetTexture(), OutRTRes->TextureRHI, FResolveParams());
	
	RHICmdList.EndRenderPass();
}

void UMyShaderBlueprintLibrary::DrawMyShaderRenderTarget(UTextureRenderTarget2D* OutRenderTarget, FLinearColor MyColor)
{
	if (!OutRenderTarget)
		return;

	FTextureRenderTargetResource* TextureRTRes = OutRenderTarget->GameThread_GetRenderTargetResource();
	ENQUEUE_RENDER_COMMAND(CaptureCommand)([TextureRTRes, MyColor](FRHICommandListImmediate& RHICmdList)
	{
		DrawMyShaderRenderTarget_RnderThread(RHICmdList, TextureRTRes, MyColor);
	});
}

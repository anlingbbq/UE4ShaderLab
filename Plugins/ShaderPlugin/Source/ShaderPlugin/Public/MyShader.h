#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "MyShader.generated.h"

UCLASS()
class UMyShaderBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
		static void DrawMyShaderRenderTarget(class UTextureRenderTarget2D* OutRenderTarget, FLinearColor MyColor);
};
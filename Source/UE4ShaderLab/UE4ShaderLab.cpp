// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "UE4ShaderLab.h"
#include "Misc/Paths.h"
#include "GlobalShader.h"

IMPLEMENT_PRIMARY_GAME_MODULE(FUE4ShaderLab, UE4ShaderLab, "UE4ShaderLab");

void FUE4ShaderLab::StartupModule()
{
	FString ShaderDirectory = FPaths::Combine(FPaths::ProjectDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping("/Project", ShaderDirectory);
}

void FUE4ShaderLab::ShutdownModule()
{
}
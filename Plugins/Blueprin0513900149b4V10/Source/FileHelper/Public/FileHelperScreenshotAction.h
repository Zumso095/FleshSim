// Copyright 2025 RLoris

#pragma once

#include "Async/Future.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Misc/Paths.h"
#include "FileHelperScreenshotAction.generated.h"

namespace ECameraProjectionMode
{
	enum Type : int;
}

USTRUCT(BlueprintType)
struct FFileHelperScreenshotActionOptions
{
	GENERATED_BODY()

	/** Directory path where to store screenshot */
	UPROPERTY(BlueprintReadWrite, Category = "FileHelper|Screenshot")
	FString DirectoryPath = FPaths::ScreenShotDir();

	/** File name without extension or path information, extension will be added internally (.png or .exr) */
	UPROPERTY(BlueprintReadWrite, Category = "FileHelper|Screenshot")
	FString Filename;

	/** Prefix filename with a custom timestamp */
	UPROPERTY(BlueprintReadWrite, Category = "FileHelper|Screenshot")
	bool bPrefixTimestamp = true;

	/** Include the viewport UI in the screenshot, only used when CustomCameraActor is not provided */
	UPROPERTY(BlueprintReadWrite, Category = "FileHelper|Screenshot")
	bool bShowUI = false;

	/**
	 * Uses this option only if the scene has HDR enabled,
	 * extension of screenshot file will be exr instead of png,
	 * if the scene is not using HDR, fallback to png
	 */
	UPROPERTY(BlueprintReadWrite, Category = "FileHelper|Screenshot")
	bool bWithHDR = false;

	/**
	 * Leave this empty for default viewport screenshot,
	 * if set, a different type of screenshot will be taken from a different perspective,
	 * options and settings quality may differ from regular screenshot, no UI is shown
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Screenshot")
	TObjectPtr<ACameraActor> CustomCameraActor = nullptr;
};

UCLASS()
class FILEHELPER_API UFileHelperScreenshotAction : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOutputPin, UTexture2D*, Screenshot, FString, Path);

	UPROPERTY(BlueprintAssignable)
	FOutputPin Completed;

	UPROPERTY(BlueprintAssignable)
	FOutputPin Failed;

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", Keywords = "File plugin screenshot save load texture", ToolTip = "Take a screenshot, save and load it"), Category = "FileHelper|Screenshot")
	static UFileHelperScreenshotAction* TakeScreenshot(const FFileHelperScreenshotActionOptions& InOptions);

	UFUNCTION(BlueprintCallable, meta = (Keywords = "screenshot load texture FileHelper", ToolTip = "Load a screenshot into a texture"), Category = "FileHelper|Screenshot")
	static UTexture2D* LoadScreenshot(const FString& InFilePath);

private:
	//~ Begin UBlueprintAsyncActionBase
	virtual void Activate() override;
	//~ End UBlueprintAsyncActionBase

	void OnTaskCompleted();
	void OnTaskFailed();

	void CreateCustomCameraScreenshot();
	void CreateViewportScreenshot();
	void CreatePlayerPOVScreenshot();
	void CreateRenderTargetScreenshot(UWorld* InWorld, float InFOV, ECameraProjectionMode::Type InProjectionMode, float InOrthoWidth, const FVector& InLocation, const FRotator& InRotation, int32 InWidth, int32 InHeight);

	void ConvertLinearColorToColorBuffer(const TArray<FLinearColor>& InSourceBuffer, TArray<FColor>& OutDestBuffer);
	bool WriteColorBufferToDisk(const TArray<FColor>& InBuffer, int32 InWidth, int32 InHeight, UTexture2D*& OutTexture) const;
	TFuture<bool> WriteLinearColorBufferToDiskAsync(TArray<FLinearColor>&& InBuffer, int32 InWidth, int32 InHeight);
	TFuture<bool> WriteColorBufferToDiskAsync(TArray<FColor>&& InBuffer, int32 InWidth, int32 InHeight);

	void Reset();

	/** Final screenshot texture */
	UPROPERTY()
	TObjectPtr<UTexture2D> ScreenshotTexture;

	/** Screenshot options */
	UPROPERTY()
	FFileHelperScreenshotActionOptions Options;

	/** The file path of the new screenshot taken */
	UPROPERTY()
	FString FilePath;

	/** Is this node active */
	UPROPERTY()
	bool bActive = false;
};

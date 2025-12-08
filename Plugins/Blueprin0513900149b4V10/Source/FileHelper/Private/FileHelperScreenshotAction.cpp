// Copyright 2025 RLoris

#include "FileHelperScreenshotAction.h"

#include "Async/Async.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Engine.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformFilemanager.h"
#include "IImageWrapper.h"
#include "ImageUtils.h"
#include "ImageWriteQueue.h"
#include "ImageWriteTask.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Modules/ModuleManager.h"
#include "TextureResource.h"
#include "UnrealClient.h"
#include "Widgets/SViewport.h"

UFileHelperScreenshotAction* UFileHelperScreenshotAction::TakeScreenshot(const FFileHelperScreenshotActionOptions& InOptions)
{
	UFileHelperScreenshotAction* Node = NewObject<UFileHelperScreenshotAction>();
	Node->Options = InOptions;
	Node->Options.Filename = FPaths::GetBaseFilename(Node->Options.Filename, /** Remove path */true);
	Node->bActive = false;
	return Node;
}

UTexture2D* UFileHelperScreenshotAction::LoadScreenshot(const FString& InFilePath)
{
	return FImageUtils::ImportFileAsTexture2D(InFilePath);
}

void UFileHelperScreenshotAction::Activate()
{
	if (bActive)
	{
		FFrame::KismetExecutionMessage(TEXT("ScreenshotUtility is already running"), ELogVerbosity::Warning);
		OnTaskFailed();
		return;
	}
	
	Reset();

	FText ErrorFilename;
	if (!FFileHelper::IsFilenameValidForSaving(Options.Filename, ErrorFilename))
	{
		FFrame::KismetExecutionMessage(TEXT("Filename is not valid"), ELogVerbosity::Warning);
		OnTaskFailed();
		return;
	}

	bActive = true;
	ScreenshotTexture = nullptr;
	const FString FinalFilename = (Options.bPrefixTimestamp ? (FDateTime::Now().ToString(TEXT("%Y_%m_%d__%H_%M_%S__"))) : "") + Options.Filename;
	FilePath = FPaths::Combine(Options.DirectoryPath, FinalFilename);

	if (!Options.CustomCameraActor)
	{
		if (Options.bShowUI)
		{
			CreateViewportScreenshot();
		}
		else
		{
			CreatePlayerPOVScreenshot();
		}
	}
	else
	{
		CreateCustomCameraScreenshot();
	}
}

void UFileHelperScreenshotAction::OnTaskCompleted()
{
	IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();

	if (!ScreenshotTexture && FileManager.FileExists(*FilePath))
	{
		ScreenshotTexture = FImageUtils::ImportFileAsTexture2D(FilePath);
	}

	if (ScreenshotTexture)
	{
		Completed.Broadcast(ScreenshotTexture, FilePath);
	}
	else
	{
		OnTaskFailed();
	}

	Reset();
}

void UFileHelperScreenshotAction::OnTaskFailed()
{
	Reset();
	Failed.Broadcast(ScreenshotTexture, FilePath);
}

void UFileHelperScreenshotAction::CreateCustomCameraScreenshot()
{
	const ACameraActor* Camera = Options.CustomCameraActor;
	if (!Camera)
	{
		OnTaskFailed();
		return;
	}

	const UWorld* World = Camera->GetWorld();
	if (!World)
	{
		OnTaskFailed();
		return;
	}

	const UCameraComponent* CameraComponent = Camera->GetCameraComponent();
	if (!CameraComponent)
	{
		OnTaskFailed();
		return;
	}

	const FViewport* GameViewport = World->GetGameViewport()->Viewport;
	const FIntRect ViewRect(0, 0, GameViewport->GetSizeXY().X, GameViewport->GetSizeXY().Y);
	const FVector CameraLocation = Camera->GetActorLocation();
	const FRotator CameraRotation = Camera->GetActorRotation();
	const float CameraFOV = CameraComponent->FieldOfView;
	const ECameraProjectionMode::Type CameraProjectionMode = CameraComponent->ProjectionMode;
	const float CameraOrthoWidth = CameraComponent->OrthoWidth;

	CreateRenderTargetScreenshot(GWorld, CameraFOV, CameraProjectionMode, CameraOrthoWidth, CameraLocation, CameraRotation, ViewRect.Width(), ViewRect.Height());
}

void UFileHelperScreenshotAction::CreateViewportScreenshot()
{
	if (!GEngine || !GEngine->GameViewport || !GEngine->GameViewport->Viewport)
	{
		OnTaskFailed();
		return;
	}

	FViewport* Viewport = GEngine->GameViewport->Viewport;

	// Get viewport size
	const FIntPoint ViewportSize = Viewport->GetSizeXY();
	int32 Width = ViewportSize.X;
	int32 Height = ViewportSize.Y;

	TWeakObjectPtr<UFileHelperScreenshotAction> ThisWeak(this);
	auto ThenTask = [ThisWeak](const TFuture<bool>& InFuture)
	{
		bool bResult = InFuture.Get();
		AsyncTask(ENamedThreads::Type::GameThread, [ThisWeak, bResult]()
		{
			UFileHelperScreenshotAction* This = ThisWeak.Get();

			if (!This)
			{
				return;
			}

			if (!bResult)
			{
				This->OnTaskFailed();
				return;
			}

			UTexture2D* Texture = FImageUtils::ImportFileAsTexture2D(This->FilePath);
			if (!Texture)
			{
				This->OnTaskFailed();
				return;
			}

			This->ScreenshotTexture = Texture;
			This->OnTaskCompleted();
		});
	};

	if (Options.bWithHDR && Viewport->IsHDRViewport())
	{
		TArray<FLinearColor> OutPixels;
		OutPixels.Reserve(Width * Height);

		// If UI is enabled, capture it separately and blend it
		const TSharedPtr<SViewport> ViewportWidget = GEngine->GameViewport->GetGameViewportWidget();
		if (Options.bShowUI && ViewportWidget.IsValid())
		{
			FIntVector OutSize;
			if (!FSlateApplication::Get().TakeHDRScreenshot(ViewportWidget.ToSharedRef(), OutPixels, OutSize))
			{
				OnTaskFailed();
				return;
			}

			Width = OutSize.X;
			Height = OutSize.Y;
		}
		else if (!Viewport->ReadLinearColorPixels(OutPixels) || OutPixels.IsEmpty())
		{
			OnTaskFailed();
			return;
		}

		OutPixels.Shrink();

		FilePath += TEXT(".exr");
		WriteLinearColorBufferToDiskAsync(MoveTemp(OutPixels), Width, Height)
			.Then(ThenTask);
	}
	else
	{
		TArray<FColor> OutPixels;
		OutPixels.Reserve(Width * Height);

		// If UI is enabled, capture it separately and blend it
		const TSharedPtr<SViewport> ViewportWidget = GEngine->GameViewport->GetGameViewportWidget();
		if (Options.bShowUI && ViewportWidget.IsValid())
		{
			FIntVector OutSize;
			if (!FSlateApplication::Get().TakeScreenshot(ViewportWidget.ToSharedRef(), OutPixels, OutSize))
			{
				OnTaskFailed();
				return;
			}

			Width = OutSize.X;
			Height = OutSize.Y;
		}
		else if (!Viewport->ReadPixels(OutPixels) || OutPixels.IsEmpty())
		{
			OnTaskFailed();
			return;
		}

		OutPixels.Shrink();

		for (FColor& Color : OutPixels)
		{
			Color.A = 255;
		}

		FilePath += TEXT(".png");
		WriteColorBufferToDiskAsync(MoveTemp(OutPixels), Width, Height)
			.Then(ThenTask);
	}
}

void UFileHelperScreenshotAction::CreatePlayerPOVScreenshot()
{
	if (!GEngine || !GEngine->GameViewport || !GEngine->GameViewport->Viewport)
	{
		OnTaskFailed();
		return;
	}

	const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GWorld, 0);
	if (!PlayerController || !PlayerController->PlayerCameraManager)
	{
		OnTaskFailed();
		return;
	}

	FVector2D ViewportSize;
	GEngine->GameViewport->GetViewportSize(ViewportSize);

	FVector CameraLocation;
	FRotator CameraRotation;
	PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);

	const float FOV = PlayerController->PlayerCameraManager->GetFOVAngle();
	constexpr ECameraProjectionMode::Type ProjectionMode = ECameraProjectionMode::Perspective;

	CreateRenderTargetScreenshot(GWorld, FOV, ProjectionMode, 0.f, CameraLocation, CameraRotation, ViewportSize.X, ViewportSize.Y);
}

void UFileHelperScreenshotAction::CreateRenderTargetScreenshot(UWorld* InWorld, float InFOV, ECameraProjectionMode::Type InProjectionMode, float InOrthoWidth, const FVector& InLocation, const FRotator& InRotation, int32 InWidth, int32 InHeight)
{
	USceneCaptureComponent2D* SceneComponent = NewObject<USceneCaptureComponent2D>(this, TEXT("SceneComponent"));
	SceneComponent->RegisterComponentWithWorld(InWorld);
	SceneComponent->bCaptureEveryFrame = false;
	SceneComponent->bCaptureOnMovement = false;
	SceneComponent->bAlwaysPersistRenderingState = true;
	SceneComponent->CaptureSource = ESceneCaptureSource::SCS_FinalColorHDR;
	SceneComponent->FOVAngle = InFOV;
	SceneComponent->ProjectionType = InProjectionMode;
	SceneComponent->OrthoWidth = InOrthoWidth;
	SceneComponent->SetWorldLocationAndRotation(InLocation, InRotation);

	UTextureRenderTarget2D* TextureRenderTarget = NewObject<UTextureRenderTarget2D>();
	TextureRenderTarget->InitCustomFormat(InWidth,InHeight,PF_B8G8R8A8,true);
	TextureRenderTarget->UpdateResourceImmediate();
	SceneComponent->TextureTarget = TextureRenderTarget;
	SceneComponent->CaptureScene();
	SceneComponent->UnregisterComponent();
	SceneComponent->MarkAsGarbage();

	FTextureRenderTargetResource* RenderTargetResource = TextureRenderTarget->GameThread_GetRenderTargetResource();
	if (!RenderTargetResource)
	{
		OnTaskFailed();
		return;
	}

	TWeakObjectPtr<UFileHelperScreenshotAction> ThisWeak(this);
	auto ThenTask = [ThisWeak](const TFuture<bool>& InFuture)
	{
		bool bResult = InFuture.Get();
		AsyncTask(ENamedThreads::Type::GameThread, [ThisWeak, bResult]()
		{
			UFileHelperScreenshotAction* This = ThisWeak.Get();

			if (!This)
			{
				return;
			}

			if (!bResult)
			{
				This->OnTaskFailed();
				return;
			}

			UTexture2D* Texture = FImageUtils::ImportFileAsTexture2D(This->FilePath);
			if (!Texture)
			{
				This->OnTaskFailed();
				return;
			}

			This->ScreenshotTexture = Texture;
			This->OnTaskCompleted();
		});
	};

	if (Options.bWithHDR && RenderTargetResource->GetSceneHDREnabled())
	{
		TArray<FLinearColor> OutPixels;
		OutPixels.Reserve(InWidth * InHeight);
		if (!RenderTargetResource->ReadLinearColorPixels(OutPixels) || OutPixels.IsEmpty())
		{
			OnTaskFailed();
			return;
		}

		OutPixels.Shrink();

		FilePath += TEXT(".exr");
		WriteLinearColorBufferToDiskAsync(MoveTemp(OutPixels), InWidth, InHeight)
			.Then(ThenTask);
	}
	else
	{
		TArray<FColor> OutPixels;
		OutPixels.Reserve(InWidth * InHeight);

		if (!RenderTargetResource->ReadPixels(OutPixels) || OutPixels.IsEmpty())
		{
			OnTaskFailed();
			return;
		}

		OutPixels.Shrink();

		for (FColor& Color : OutPixels)
		{
			Color.A = 255;
		}

		FilePath += TEXT(".png");
		WriteColorBufferToDiskAsync(MoveTemp(OutPixels), InWidth, InHeight)
			.Then(ThenTask);
	}

	TextureRenderTarget->MarkAsGarbage();
}

bool UFileHelperScreenshotAction::WriteColorBufferToDisk(const TArray<FColor>& InBuffer, int32 InWidth, int32 InHeight, UTexture2D*& OutTexture) const
{
	if (InBuffer.IsEmpty())
	{
		return false;
	}

	TArray<uint8> OutImage;
	FImageUtils::ThumbnailCompressImageArray(InWidth, InHeight, InBuffer, OutImage);

	if (OutImage.IsEmpty())
	{
		return false;
	}

	if (!FFileHelper::SaveArrayToFile(OutImage, *FilePath))
	{
		return false;
	}

	OutTexture = FImageUtils::ImportBufferAsTexture2D(OutImage);
	return IsValid(OutTexture);
}

TFuture<bool> UFileHelperScreenshotAction::WriteLinearColorBufferToDiskAsync(TArray<FLinearColor>&& InBuffer, int32 InWidth, int32 InHeight)
{
	IImageWriteQueueModule& ImageWriteQueueModule = FModuleManager::LoadModuleChecked<IImageWriteQueueModule>("ImageWriteQueue");
	TUniquePtr<FImageWriteTask> ImageTask = MakeUnique<FImageWriteTask>();
	ImageTask->Format = EImageFormat::EXR;
	ImageTask->PixelData = MakeUnique<TImagePixelData<FLinearColor>>(FIntPoint(InWidth, InHeight), TArray64<FLinearColor>(InBuffer));
	ImageTask->Filename = FilePath;
	ImageTask->bOverwriteFile = true;
	ImageTask->CompressionQuality = 100;
	return ImageWriteQueueModule.GetWriteQueue().Enqueue(MoveTemp(ImageTask));
}

TFuture<bool> UFileHelperScreenshotAction::WriteColorBufferToDiskAsync(TArray<FColor>&& InBuffer, int32 InWidth, int32 InHeight)
{
	IImageWriteQueueModule& ImageWriteQueueModule = FModuleManager::LoadModuleChecked<IImageWriteQueueModule>("ImageWriteQueue");
	TUniquePtr<FImageWriteTask> ImageTask = MakeUnique<FImageWriteTask>();
	ImageTask->Format = EImageFormat::PNG;
	ImageTask->PixelData = MakeUnique<TImagePixelData<FColor>>(FIntPoint(InWidth, InHeight), TArray64<FColor>(InBuffer));
	ImageTask->Filename = FilePath;
	ImageTask->bOverwriteFile = true;
	ImageTask->CompressionQuality = 100;
	return ImageWriteQueueModule.GetWriteQueue().Enqueue(MoveTemp(ImageTask));
}

void UFileHelperScreenshotAction::ConvertLinearColorToColorBuffer(const TArray<FLinearColor>& InSourceBuffer, TArray<FColor>& OutDestBuffer)
{
	OutDestBuffer.Empty(InSourceBuffer.Num());

	for (const FLinearColor& LinearColor : InSourceBuffer)
	{
		OutDestBuffer.Emplace(LinearColor.ToFColor(false)); // Apply gamma correction
	}
}

void UFileHelperScreenshotAction::Reset()
{
	ScreenshotTexture = nullptr;
	bActive = false;
	FilePath.Empty();
}

// Copyright 2025 RLoris

#include "FileHelperBPLibrary.h"

#include "HAL/PlatformFileManager.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Base64.h"
#include "Math/Color.h"
#include "Misc/ConfigCacheIni.h"
#include "Engine/DataTable.h"
#include "Internationalization/Regex.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Serialization/Csv/CsvParser.h"
#include "UObject/TextProperty.h"

class FCustomFileVisitor : public IPlatformFile::FDirectoryVisitor
{
public:
	FCustomFileVisitor(const FString& Path, TArray<FString>& Paths, const FString& Pattern, bool File, bool Directory) : BasePath(Path), Nodes(Paths), Filter(Pattern), CustomPattern(Pattern), bFile(File), bDirectory(Directory) {};

	//~ Begin FDirectoryVisitor
	virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override;
	//~ End FDirectoryVisitor

private:
	FString BasePath;
	TArray<FString>& Nodes;
	FString Filter;
	FRegexPattern CustomPattern;
	bool bFile = true;
	bool bDirectory = true;
};

FEnginePath UFileHelperBPLibrary::GetEngineDirectories()
{
	FEnginePath P;
	P.Directory = FPaths::ConvertRelativePathToFull(FPaths::EngineDir());
	P.Config = FPaths::ConvertRelativePathToFull(FPaths::EngineConfigDir());
	P.Content = FPaths::ConvertRelativePathToFull(FPaths::EngineContentDir());
	P.Intermediate = FPaths::ConvertRelativePathToFull(FPaths::EngineIntermediateDir());
	P.Plugins = FPaths::ConvertRelativePathToFull(FPaths::EnginePluginsDir());
	P.Saved =FPaths::ConvertRelativePathToFull( FPaths::EngineSavedDir());
	P.User = FPaths::ConvertRelativePathToFull(FPaths::EngineUserDir());
	P.DefaultLayout = FPaths::ConvertRelativePathToFull(FPaths::EngineDefaultLayoutDir());
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 4
	P.PlatformExtensions = FPaths::ConvertRelativePathToFull(FPaths::EnginePlatformExtensionDir(TEXT("")).TrimChar('/'));
#else
	P.PlatformExtensions = FPaths::ConvertRelativePathToFull(FPaths::EnginePlatformExtensionsDir());
#endif
	P.UserLayout = FPaths::ConvertRelativePathToFull(FPaths::EngineUserLayoutDir());
	return P;
}

FProjectPath UFileHelperBPLibrary::GetProjectDirectories()
{
	FProjectPath P;
	P.Directory = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	P.Config = FPaths::ConvertRelativePathToFull(FPaths::ProjectConfigDir());
	P.Content = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
	P.Intermediate = FPaths::ConvertRelativePathToFull(FPaths::ProjectIntermediateDir());
	P.Log = FPaths::ConvertRelativePathToFull(FPaths::ProjectLogDir());
	P.Mods = FPaths::ConvertRelativePathToFull(FPaths::ProjectModsDir());
	P.Plugins = FPaths::ConvertRelativePathToFull(FPaths::ProjectPluginsDir());
	P.Saved = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir());
	P.User = FPaths::ConvertRelativePathToFull(FPaths::ProjectUserDir());
	P.PersistentDownload = FPaths::ConvertRelativePathToFull(FPaths::ProjectPersistentDownloadDir());
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 4
	P.PlatformExtensions = FPaths::ConvertRelativePathToFull(FPaths::ProjectPlatformExtensionDir(TEXT("")).TrimChar('/'));
#else
	P.PlatformExtensions = FPaths::ConvertRelativePathToFull(FPaths::ProjectPlatformExtensionsDir());
#endif
	return P;
}

bool UFileHelperBPLibrary::ReadText(FString Path, FString& Output)
{
	IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();
	if (FileManager.FileExists(*Path))
	{
		return FFileHelper::LoadFileToString(Output, *Path);
	}
	return false;
}

bool UFileHelperBPLibrary::SaveText(FString Path, FString Text, FString& Error, bool Append, bool Force)
{
	IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();
	FText ErrorFilename;
	if (!FFileHelper::IsFilenameValidForSaving(Path, ErrorFilename))
	{
		Error = FString("Filename is not valid");
		return false;
	}
	if (!FileManager.FileExists(*Path) || Append || Force)
	{
		return FFileHelper::SaveStringToFile(Text, *Path, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), Append ? FILEWRITE_Append : FILEWRITE_None);
	}
	else
	{
		Error = FString("File already exists");
	}
	return false;
}

bool UFileHelperBPLibrary::SaveCSV(FString Path, TArray<FString> Headers, TArray<FString> Data, int32& Total, bool Force)
{
	FString Output;
	if (!UFileHelperBPLibrary::CSVToString(Output, Headers, Data, Total))
	{
		return false;
	}
	FString Error;
	return UFileHelperBPLibrary::SaveText(Path, Output, Error, false, Force);
}

bool UFileHelperBPLibrary::ReadCSV(FString Path, TArray<FString>& Headers, TArray<FString>& Data, int32& Total, bool HeaderFirst)
{
	Total = 0;
	IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();
	if (!FileManager.FileExists(*Path))
	{
		return false;
	}
	FString Result;
	if (!FFileHelper::LoadFileToString(Result, *Path))
	{
		return false;
	}
	return UFileHelperBPLibrary::StringToCSV(Result, Headers, Data, Total, HeaderFirst);
	// return UFileHelperBPLibrary::StringArrayToCSV(Result, Headers, Data, Total, ",", HeaderFirst);
}

bool UFileHelperBPLibrary::ReadLine(FString Path, FString Pattern, TArray<FString>& Lines)
{
	IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();
	if (!FileManager.FileExists(*Path))
	{
		return false;
	}
	if (!Pattern.IsEmpty())
	{
		FRegexPattern CustomPattern(Pattern);
		return FFileHelper::LoadFileToStringArrayWithPredicate(Lines, *Path, [CustomPattern](const FString& Line) {
			FRegexMatcher CustomMatcher(CustomPattern, Line);
			return CustomMatcher.FindNext();
		});
	}
	return FFileHelper::LoadFileToStringArray(Lines, *Path);
}

bool UFileHelperBPLibrary::ReadLineRange(FString Path, TArray<FString>& Lines, int32 StartIdx, int32 EndIdx)
{
	if (EndIdx != INDEX_NONE && EndIdx < StartIdx)
	{
		return false;
	}
	IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();
	if (!FileManager.FileExists(*Path))
	{
		return false;
	}
	int32 LineCount = 0;
	return FFileHelper::LoadFileToStringArrayWithPredicate(Lines, *Path, [&LineCount, StartIdx, EndIdx](const FString& Line)->bool{
		const bool bRangeStart = LineCount >= StartIdx;
		const bool bRangeEnd = EndIdx == INDEX_NONE || LineCount < EndIdx;
		LineCount++;
		return bRangeStart && bRangeEnd;
	});
}

bool UFileHelperBPLibrary::SaveLine(FString Path, const TArray<FString>& Text, FString& Error, bool Append, bool Force)
{
	IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();
	FText ErrorFilename;
	if (!FFileHelper::IsFilenameValidForSaving(Path, ErrorFilename))
	{
		Error = FString("Filename is not valid");
		return false;
	}
	if (!FileManager.FileExists(*Path) || Append || Force)
	{
		return FFileHelper::SaveStringArrayToFile(Text, *Path, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), Append ? FILEWRITE_Append : FILEWRITE_None);
	}
	else
	{
		Error = FString("File already exists");
	}
	return false;
}

bool UFileHelperBPLibrary::ReadByte(FString Path, TArray<uint8>& Bytes)
{
	IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();
	if (!FileManager.FileExists(*Path))
	{
		return false;
	}
	return FFileHelper::LoadFileToArray(Bytes, *Path);
}

FString UFileHelperBPLibrary::BytesToBase64(const TArray<uint8> Bytes)
{
	return FBase64::Encode(Bytes);
}

bool UFileHelperBPLibrary::BytesFromBase64(const FString Source, TArray<uint8>& Out)
{
	return FBase64::Decode(Source, Out);
}

bool UFileHelperBPLibrary::SaveByte(FString Path, const TArray<uint8>& Bytes, FString& Error, bool Append, bool Force)
{
	IPlatformFile& file = FPlatformFileManager::Get().GetPlatformFile();
	FText ErrorFilename;
	if (!FFileHelper::IsFilenameValidForSaving(Path, ErrorFilename))
	{
		Error = FString("Filename is not valid");
		return false;
	}
	if (!file.FileExists(*Path) || Append || Force)
	{
		return FFileHelper::SaveArrayToFile(Bytes, *Path, &IFileManager::Get(), Append ? FILEWRITE_Append : FILEWRITE_None);
	}
	else
	{
		Error = FString("File already exists");
	}
	return false;
}

bool UFileHelperBPLibrary::StringToCSV(FString Content, TArray<FString>& Headers, TArray<FString>& Data, int32& Total, bool HeaderFirst)
{
	const FCsvParser Parser(Content);
	TArray<TArray<const TCHAR*>> Rows = Parser.GetRows();
	for (TArray<const TCHAR*> Row : Rows)
	{
		Total++;
		for (FString Col : Row)
		{
			if (Total == 1 && HeaderFirst)
			{
				Headers.Add(Col);
			}
			else
			{
				Data.Add(Col);
			}
		}
	}
	return true;
	// auto Result = SplitString(Content, LINE_TERMINATOR, ESearchCase::Type::IgnoreCase);
	// return UFileHelperBPLibrary::StringArrayToCSV(Result, Headers, Data, Total, HeaderFirst);
}

bool UFileHelperBPLibrary::CSVToString(FString& Output, TArray<FString> Headers, TArray<FString> Data, int32& Total)
{
	Total = 0;
	FString Delimiter = ",";
	if (Headers.Num() == 0)
	{
		return false;
	}
	if (Data.Num() % Headers.Num() != 0)
	{
		return false;
	}
	Output = TEXT("");
	// Header row
	for (FString Col : Headers)
	{
		if (Output.Len() > 0)
		{
			Output += Delimiter;
		}
		Output += (TEXT("\"") + Col.Replace(TEXT("\""), TEXT("\"\"")) + TEXT("\""));
	}
	Output += LINE_TERMINATOR;
	FString Row = TEXT("");
	int32 Count = 0;
	// Data row
	for (FString Col : Data)
	{
		Count++;
		if (Row.Len() > 0)
		{
			Row += Delimiter;
		}
		Row += (TEXT("\"") + Col.Replace(TEXT("\""), TEXT("\"\"")) + TEXT("\""));
		if (Count % Headers.Num() == 0)
		{
			Row += LINE_TERMINATOR;
			Output += Row;
			Row = "";
		}
	}
	Total = (Data.Num() / Headers.Num()) + 1;
	return true;
}

bool UFileHelperBPLibrary::StringArrayToCSV(TArray<FString> Lines, TArray<FString>& Headers, TArray<FString>& Data, int32& Total, FString Delimiter, bool HeaderFirst)
{
	for (auto Line : Lines)
	{
		Total++;
		if (!Line.Contains(TEXT("\"") + Delimiter + TEXT("\"")))
		{
			continue;
		}
		if (Total == 1 && HeaderFirst)
		{
			for (FString Col : UFileHelperBPLibrary::SplitString(Line, TEXT("\"") + Delimiter + TEXT("\""), ESearchCase::CaseSensitive))
			{
				Col.TrimQuotesInline();
				Col.ReplaceInline(TEXT("\"\""), TEXT("\""));
				Headers.Add(Col);
			}
		}
		else
		{
			for (FString Col : UFileHelperBPLibrary::SplitString(Line, TEXT("\"") + Delimiter + TEXT("\""), ESearchCase::CaseSensitive))
			{
				Col.TrimQuotesInline();
				Col.ReplaceInline(TEXT("\"\""), TEXT("\""));
				Data.Add(Col);
			}
		}
	}
	return true;
}

bool UFileHelperBPLibrary::IsFile(FString Path)
{
	IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();
	return FileManager.FileExists(*Path);
}

bool UFileHelperBPLibrary::IsDirectory(FString Path)
{
	IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();
	return FileManager.DirectoryExists(*Path);
}

bool UFileHelperBPLibrary::IsValidFilename(FString Filename)
{
	FText Error;
	return FFileHelper::IsFilenameValidForSaving(*Filename, Error);
}

bool UFileHelperBPLibrary::IsValidPath(FString Path)
{
	return FPaths::ValidatePath(Path);
}

bool UFileHelperBPLibrary::ValidateFilename(FString Filename, FString& ValidName)
{
	ValidName = FPaths::MakeValidFileName(Filename);
	return IsValidFilename(ValidName);
}

bool UFileHelperBPLibrary::SetReadOnlyFlag(FString FilePath, bool Flag)
{
	IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();
	return FileManager.SetReadOnly(*FilePath, Flag);
}

bool UFileHelperBPLibrary::GetReadOnlyFlag(FString FilePath)
{
	IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();
	return FileManager.IsReadOnly(*FilePath);
}

int64 UFileHelperBPLibrary::GetFileSize(FString FilePath)
{
	IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();
	return FileManager.FileSize(*FilePath);
}

bool FCustomFileVisitor::Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory)
{
	if ((bFile && !bIsDirectory) || (bDirectory && bIsDirectory))
	{
		FString RelativePath = FString(FilenameOrDirectory);
		FPaths::MakePathRelativeTo(RelativePath, *BasePath);
		if (!Filter.IsEmpty())
		{
			FRegexMatcher CustomMatcher(CustomPattern, RelativePath);
			if (CustomMatcher.FindNext())
			{
				Nodes.Add(RelativePath);
			}
		}
		else
		{
			Nodes.Add(RelativePath);
		}
	}
	return true;
}

bool UFileHelperBPLibrary::ListDirectory(FString Path, FString Pattern, TArray<FString>& Nodes, bool ShowFile, bool ShowDirectory, bool Recursive)
{
	IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();
	if (!FileManager.DirectoryExists(*Path))
	{
		return false;
	}
	if (!ShowDirectory && !ShowFile)
	{
		return true;
	}
	FString BasePath = FPaths::Combine(Path, TEXT("/"));
	FCustomFileVisitor CustomFileVisitor(BasePath, Nodes, Pattern, ShowFile, ShowDirectory);
	if (Recursive)
	{
		return FileManager.IterateDirectoryRecursively(*Path, CustomFileVisitor);
	}
	else
	{
		return FileManager.IterateDirectory(*Path, CustomFileVisitor);
	}
}

bool UFileHelperBPLibrary::MakeDirectory(FString Path, bool Recursive)
{
	IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();
	if (FileManager.DirectoryExists(*Path))
	{
		return true;
	}
	if (Recursive)
	{
		return FileManager.CreateDirectoryTree(*Path);
	}
	else
	{
		return FileManager.CreateDirectory(*Path);
	}
}

bool UFileHelperBPLibrary::RemoveDirectory(FString Path, bool Recursive)
{
	IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();
	if (!FileManager.DirectoryExists(*Path))
	{
		return true;
	}
	if (Recursive)
	{
		return FileManager.DeleteDirectoryRecursively(*Path);
	}
	else
	{
		return FileManager.DeleteDirectory(*Path);
	}
}

bool UFileHelperBPLibrary::CopyDirectory(FString Source, FString Dest) // bool Force = false
{
	IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();
	FPaths::NormalizeDirectoryName(Dest);
	if (!FileManager.DirectoryExists(*Source) && !FileManager.FileExists(*Source))
	{
		return false;
	}
	if (!FileManager.DirectoryExists(*Dest))
	{
		return false;
	}
	return FileManager.CopyDirectoryTree(*Dest, *Source, true);
}

bool UFileHelperBPLibrary::MoveDirectory(FString Source, FString Dest) // bool Force = false
{
	FPaths::NormalizeDirectoryName(Source);
	FPaths::NormalizeDirectoryName(Dest);
	if (Dest.Equals(Source))
	{
		return true;
	}
	if (!UFileHelperBPLibrary::CopyDirectory(Source, Dest))
	{
		return false;
	}
	return UFileHelperBPLibrary::RemoveDirectory(Source, true);
}

bool UFileHelperBPLibrary::NodeStats(FString Path, FCustomNodeStat& Stats)
{
	IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();
	if (!FileManager.DirectoryExists(*Path) && !FileManager.FileExists(*Path))
	{
		return false;
	}
	const FFileStatData Data = FileManager.GetStatData(*Path);
	if (!Data.bIsValid)
	{
		return false;
	}
	Stats.CreationTime = Data.CreationTime;
	Stats.FileSize = Data.FileSize;
	Stats.IsDirectory = Data.bIsDirectory;
	Stats.IsReadOnly = Data.bIsReadOnly;
	Stats.LastAccessTime = Data.AccessTime;
	Stats.ModificationTime = Data.ModificationTime;
	return true;
}

bool UFileHelperBPLibrary::RemoveFile(FString Path)
{
	IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();
	if (FileManager.DirectoryExists(*Path))
	{
		return false;
	}
	if (!FileManager.FileExists(*Path))
	{
		return true;
	}
	return FileManager.DeleteFile(*Path);
}

bool UFileHelperBPLibrary::CopyFile(FString Source, FString Dest, bool Force)
{
	FPaths::NormalizeFilename(Source);
	FPaths::NormalizeFilename(Dest);
	if (Dest.Equals(Source))
	{
		return true;
	}
	FText Error;
	if (!FFileHelper::IsFilenameValidForSaving(*Dest, Error))
	{
		return false;
	}
	IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();
	if (!FileManager.FileExists(*Source))
	{
		return false;
	}
	if (!Force && FileManager.FileExists(*Dest))
	{
		return false;
	}
	UFileHelperBPLibrary::RemoveFile(Dest);
	return FileManager.CopyFile(*Dest, *Source);
}

bool UFileHelperBPLibrary::MoveFile(FString Source, FString Dest, bool Force)
{
	FPaths::NormalizeFilename(Source);
	FPaths::NormalizeFilename(Dest);
	if (Dest.Equals(Source))
	{
		return true;
	}
	FText Error;
	if (!FFileHelper::IsFilenameValidForSaving(*Dest, Error))
	{
		return false;
	}
	IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();
	if (!FileManager.FileExists(*Source))
	{
		return false;
	}
	if (!Force && FileManager.FileExists(*Dest))
	{
		return false;
	}
	UFileHelperBPLibrary::RemoveFile(Dest);
	return FileManager.MoveFile(*Dest, *Source);
}

bool UFileHelperBPLibrary::RenameFile(FString Path, FString NewName)
{
	FText Error;
	if (!FFileHelper::IsFilenameValidForSaving(*NewName, Error))
	{
		return false;
	}
	IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();
	if (!FileManager.FileExists(*Path))
	{
		return false;
	}
	FString Clean = FPaths::GetCleanFilename(NewName);
	FString Base = FPaths::GetPath(Path);
	FString Output = FPaths::Combine(*Base, *Clean);
	if (FileManager.FileExists(*Output) || FileManager.DirectoryExists(*Output))
	{
		return false;
	}
	return FileManager.MoveFile(*Output, *Path);
}

void UFileHelperBPLibrary::GetPathParts(FString Path, FString& PathPart, FString& BasePart, FString& ExtensionPart, FString& FileName)
{
	PathPart = FPaths::GetPath(Path);
	BasePart = FPaths::GetBaseFilename(Path);
	ExtensionPart = FPaths::GetExtension(Path);
	FileName = FPaths::GetCleanFilename(Path);
}

bool UFileHelperBPLibrary::DatatableToCSV(UDataTable* Table, FString& Output)
{
	if (Table == nullptr || !Table->RowStruct)
	{
		return false;
	}

	// See Table->GetTableAsCSV
	return UFileHelperBPLibrary::WriteTableToCSV(*Table, Output);
}

bool UFileHelperBPLibrary::DataTableToJSON(UDataTable* Table, FString& Output)
{
	if (Table == nullptr || !Table->RowStruct)
	{
		return false;
	}

	// See Table->GetTableAsJSON
	return UFileHelperBPLibrary::WriteTableToJSON(*Table, Output);
}

UDataTable* UFileHelperBPLibrary::CSVToDataTable(FString CSV, UScriptStruct* Struct, bool& Success)
{
	Success = false;
	if (Struct == nullptr)
	{
		return nullptr;
	}
	UDataTable* DataTable = NewObject<UDataTable>();
	DataTable->RowStruct = Struct;
	auto Result = DataTable->CreateTableFromCSVString(CSV);
	if (Result.Num() == 0)
	{
		Success = true;
	}
	return DataTable;
}

UDataTable* UFileHelperBPLibrary::JSONToDataTable(FString JSON, UScriptStruct* Struct, bool& Success)
{
	Success = false;
	if (Struct == nullptr)
	{
		return nullptr;
	}
	UDataTable* DataTable = NewObject<UDataTable>();
	DataTable->RowStruct = Struct;
	auto Result = DataTable->CreateTableFromJSONString(JSON);
	if (Result.Num() == 0)
	{
		Success = true;
	}
	return DataTable;
}

void UFileHelperBPLibrary::ReadConfig(FString FilePath, FString Section, FString Key, bool& Success, bool SingleLineArrayRead, UStruct*& OutValue)
{
	checkNoEntry();
}

TArray<FString> UFileHelperBPLibrary::SplitString(FString String, FString Separator, ESearchCase::Type SearchCase)
{
	FString LeftString;
	FString RightString;
	TArray<FString> Array;
	bool Split = false;
	do
	{
		Split = String.Split(Separator, &LeftString, &RightString, SearchCase);
		if (Split)
		{
			Array.Add(LeftString);
		}
		else
		{
			Array.Add(String);
		}
		String = RightString;
	} while (Split);

	return Array;
}

bool UFileHelperBPLibrary::WriteConfigFile(FString Filename, FString Section, FString Key, FProperty* Type, void* Value, bool SingleLineArray)
{
	if (!GConfig)
	{
		return false;
	}

	FConfigFile ConfigFile;
	FindOrCreateConfigFile(Filename, ConfigFile);

	if (FBoolProperty* BoolProperty = CastField<FBoolProperty>(Type))
	{
		GConfig->SetBool(*Section, *Key, *(static_cast<bool*>(Value)), Filename);
	}
	else if (FIntProperty* IntProperty = CastField<FIntProperty>(Type))
	{
		GConfig->SetInt(*Section, *Key, *(static_cast<int32*>(Value)), Filename);
	}
	else if (FStrProperty* StrProperty = CastField<FStrProperty>(Type))
	{
		GConfig->SetString(*Section, *Key, **(static_cast<FString*>(Value)), Filename);
	}
	else if (FFloatProperty* FloatProperty = CastField<FFloatProperty>(Type))
	{
		GConfig->SetFloat(*Section, *Key, *(static_cast<float*>(Value)), Filename);
	}
	else if (FDoubleProperty* DoubleProperty = CastField<FDoubleProperty>(Type))
	{
		GConfig->SetDouble(*Section, *Key, *(static_cast<double*>(Value)), Filename);
	}
	else if (FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Type))
	{
		if (FStrProperty* ArrayInnerProperty = CastField<FStrProperty>(ArrayProperty->Inner))
		{
			TArray<FString>* Arr = (static_cast<TArray<FString>*>(Value));
			if (SingleLineArray)
			{
				GConfig->SetSingleLineArray(*Section, *Key, *Arr, Filename);
			}
			else
			{
				GConfig->SetArray(*Section, *Key, *Arr, Filename);
			}
		}
		else
		{
			return false;
		}
	}
	else if (FTextProperty* TextProperty = CastField<FTextProperty>(Type))
	{
		GConfig->SetText(*Section, *Key, *(static_cast<FText*>(Value)), Filename);
	}
	else if (const FStructProperty* StructProperty = CastField<FStructProperty>(Type))
	{
		if (StructProperty->Struct->IsNative())
		{
			static const FName RotatorType = TEXT("Rotator");
			static const FName VectorType = TEXT("Vector");
			static const FName LinearColorType = TEXT("LinearColor");
			static const FName Vector4Type = TEXT("Vector4");
			static const FName Vector2DType = TEXT("Vector2D");

			const FName TypeName = StructProperty->Struct->GetFName();
			if (TypeName == RotatorType)
			{
				GConfig->SetRotator(*Section, *Key, *(static_cast<FRotator*>(Value)), *Filename);
			}
			else if (TypeName == VectorType)
			{
				GConfig->SetVector(*Section, *Key, *(static_cast<FVector*>(Value)), *Filename);
			}
			else if (TypeName == LinearColorType)
			{
				GConfig->SetColor(*Section, *Key, *(static_cast<FColor*>(Value)), *Filename);
			}
			else if (TypeName == Vector4Type)
			{
				GConfig->SetVector4(*Section, *Key, *(static_cast<FVector4*>(Value)), *Filename);
			}
			else if (TypeName == Vector2DType)
			{
				GConfig->SetVector2D(*Section, *Key, *(static_cast<FVector2D*>(Value)), *Filename);
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}

	return SaveConfigFile(Filename);
}

bool UFileHelperBPLibrary::ReadConfigFile(FString Filename, FString Section, FString Key, FProperty* Type, void* Value, bool SingleLineArray)
{
	if (!GConfig)
	{
		return false;
	}

	FConfigFile ConfigFile;
	FindOrCreateConfigFile(Filename, ConfigFile);

	bool Success = false;
	if (FBoolProperty* BoolProperty = CastField<FBoolProperty>(Type))
	{
		Success = GConfig->GetBool(*Section, *Key, *(static_cast<bool*>(Value)), Filename);
	}
	else if (FIntProperty* IntProperty = CastField<FIntProperty>(Type))
	{
		Success = GConfig->GetInt(*Section, *Key, *(static_cast<int32*>(Value)), Filename);
	}
	else if (FStrProperty* StrProperty = CastField<FStrProperty>(Type))
	{
		Success = GConfig->GetString(*Section, *Key, *(static_cast<FString*>(Value)), Filename);
	}
	else if (FFloatProperty* FloatProperty = CastField<FFloatProperty>(Type))
	{
		Success = GConfig->GetFloat(*Section, *Key, *(static_cast<float*>(Value)), Filename);
	}
	else if (FDoubleProperty* DoubleProperty = CastField<FDoubleProperty>(Type))
	{
		Success = GConfig->GetDouble(*Section, *Key, *(static_cast<double*>(Value)), Filename);
	}
	else if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Type))
	{
		if (FStrProperty* ArrayInnerProperty = CastField<FStrProperty>(ArrayProperty->Inner))
		{
			TArray<FString>* Arr = (static_cast<TArray<FString>*>(Value));
			if (SingleLineArray)
			{
				Success = (GConfig->GetSingleLineArray(*Section, *Key, *Arr, Filename) != 0);
			}
			else
			{
				Success = (GConfig->GetArray(*Section, *Key, *Arr, Filename) != 0);
			}
		}
	}
	else if (FTextProperty* TextProperty = CastField<FTextProperty>(Type))
	{
		Success = GConfig->GetText(*Section, *Key, *(static_cast<FText*>(Value)), Filename);
	}
	else if (const FStructProperty* StructProperty = CastField<FStructProperty>(Type))
	{
		if (StructProperty->Struct->IsNative())
		{
			static const FName RotatorType = TEXT("Rotator");
			static const FName VectorType = TEXT("Vector");
			static const FName LinearColorType = TEXT("LinearColor");
			static const FName Vector4Type = TEXT("Vector4");
			static const FName Vector2DType = TEXT("Vector2D");

			const FName TypeName = StructProperty->Struct->GetFName();
			if (TypeName == RotatorType)
			{
				Success = GConfig->GetRotator(*Section, *Key, *(static_cast<FRotator*>(Value)), *Filename);
			}
			else if (TypeName == VectorType)
			{
				Success = GConfig->GetVector(*Section, *Key, *(static_cast<FVector*>(Value)), *Filename);
			}
			else if (TypeName == LinearColorType)
			{
				Success = GConfig->GetColor(*Section, *Key, *(static_cast<FColor*>(Value)), *Filename);
			}
			else if (TypeName == Vector4Type)
			{
				Success = GConfig->GetVector4(*Section, *Key, *(static_cast<FVector4*>(Value)), *Filename);
			}
			else if (TypeName == Vector2DType)
			{
				Success = GConfig->GetVector2D(*Section, *Key, *(static_cast<FVector2D*>(Value)), *Filename);
			}
		}
	}
	
	return Success;
}

void UFileHelperBPLibrary::FindOrCreateConfigFile(const FString& InFilePath, FConfigFile& OutConfigFile)
{
	const bool bDisabled = GConfig->AreFileOperationsDisabled();

	if (bDisabled)
	{
		GConfig->EnableFileOperations();
	}

	FConfigFile* ConfigFile = GConfig->Find(InFilePath);

	if (!ConfigFile)
	{
		FConfigFile NewConfigFile;
		NewConfigFile.Read(InFilePath);
		OutConfigFile = GConfig->Add(InFilePath, NewConfigFile);
	}
	else
	{
		OutConfigFile = *ConfigFile;
	}

	if (bDisabled)
	{
		GConfig->DisableFileOperations();
	}
}

bool UFileHelperBPLibrary::SaveConfigFile(const FString& InFilePath)
{
	if (FConfigFile* ConfigFile = GConfig->Find(InFilePath))
	{
		const bool bDisabled = GConfig->AreFileOperationsDisabled();

		if (bDisabled)
		{
			GConfig->EnableFileOperations();
		}

		GConfig->Flush(false, InFilePath);

		if (bDisabled)
		{
			GConfig->DisableFileOperations();
		}

		return true;
	}

	return false;
}

void UFileHelperBPLibrary::WriteConfig(FString FilePath, FString Section, FString Key, bool& Success, bool SingleLineArrayWrite, const UStruct* Value)
{
	checkNoEntry();
}

bool UFileHelperBPLibrary::RemoveConfig(FString FilePath, FString Section, FString Key)
{
	if (!GConfig)
	{
		return false;
	}

	FConfigFile ConfigFile;
	FindOrCreateConfigFile(FilePath, ConfigFile);

	if (!GConfig->RemoveKey(*Section, *Key, *FilePath))
	{
		return false;
	}

	return SaveConfigFile(FilePath);
}

// equivalent GetTableAsCSV()

bool UFileHelperBPLibrary::WriteTableToCSV(const UDataTable& InDataTable, FString& ExportedText)
{
	if (!InDataTable.RowStruct)
	{
		return false;
	}

	// Write the header (column titles)
	FString ImportKeyField;
	if (!InDataTable.ImportKeyField.IsEmpty())
	{
		// Write actual name if we have it
		ImportKeyField = InDataTable.ImportKeyField;
		ExportedText += ImportKeyField;
	}
	else
	{
		ExportedText += TEXT("---");
	}

	FProperty* SkipProperty = nullptr;
	for (TFieldIterator<FProperty> It(InDataTable.RowStruct); It; ++It)
	{
		FProperty* BaseProp = *It;
		check(BaseProp);

		FString ColumnHeader = DataTableUtils::GetPropertyExportName(BaseProp, EDataTableExportFlags::None);

		if (ColumnHeader == ImportKeyField)
		{
			// Don't write header again if this is the name field, and save for skipping later
			SkipProperty = BaseProp;
			continue;
		}

		ExportedText += TEXT(",");
		ExportedText += ColumnHeader;
	}
	ExportedText += TEXT("\n");

	// Write each row
	for (auto RowIt = InDataTable.GetRowMap().CreateConstIterator(); RowIt; ++RowIt)
	{
		FName RowName = RowIt.Key();
		ExportedText += RowName.ToString();

		uint8* RowData = RowIt.Value();
		UFileHelperBPLibrary::WriteRowToCSV(InDataTable.RowStruct, RowData, ExportedText);

		ExportedText += TEXT("\n");
	}

	return true;
}

bool UFileHelperBPLibrary::WriteRowToCSV(const UScriptStruct* InRowStruct, const void* InRowData, FString& ExportedText)
{
	if (!InRowStruct)
	{
		return false;
	}

	for (TFieldIterator<FProperty> It(InRowStruct); It; ++It)
	{
		FProperty* BaseProp = *It;
		check(BaseProp);

		const void* Data = BaseProp->ContainerPtrToValuePtr<void>(InRowData, 0);
		UFileHelperBPLibrary::WriteStructEntryToCSV(InRowData, BaseProp, Data, ExportedText);
	}

	return true;
}

bool UFileHelperBPLibrary::WriteStructEntryToCSV(const void* InRowData, FProperty* InProperty, const void* InPropertyData, FString& ExportedText)
{
	ExportedText += TEXT(",");

	const FString PropertyValue = DataTableUtils::GetPropertyValueAsString(InProperty, (uint8*)InRowData, EDataTableExportFlags::None);
	ExportedText += TEXT("\"");
	ExportedText += PropertyValue.Replace(TEXT("\""), TEXT("\"\""));
	ExportedText += TEXT("\"");

	return true;
}

// equivalent GetTableAsJSON()

FString UFileHelperBPLibrary::GetKeyFieldName(const UDataTable& InDataTable)
{
	FString ExplicitString = InDataTable.ImportKeyField;
	if (ExplicitString.IsEmpty())
	{
		return TEXT("Name");
	}
	else
	{
		return ExplicitString;
	}
}

bool UFileHelperBPLibrary::WriteTableToJSON(const UDataTable& InDataTable, FString& OutExportText)
{
	if (!InDataTable.RowStruct)
	{
		return false;
	}

	TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> JsonWriter = TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&OutExportText);

	FString KeyField = UFileHelperBPLibrary::GetKeyFieldName(InDataTable);

	JsonWriter->WriteArrayStart();

	// Iterate over rows
	for (auto RowIt = InDataTable.GetRowMap().CreateConstIterator(); RowIt; ++RowIt)
	{
		JsonWriter->WriteObjectStart();
		{
			// RowName
			const FName RowName = RowIt.Key();
			JsonWriter->WriteValue(KeyField, RowName.ToString());

			// Now the values
			uint8* RowData = RowIt.Value();
			UFileHelperBPLibrary::WriteRowToJSON(InDataTable.RowStruct, RowData, JsonWriter);
		}
		JsonWriter->WriteObjectEnd();
	}

	JsonWriter->WriteArrayEnd();

	JsonWriter->Close();

	return true;
}

bool UFileHelperBPLibrary::WriteTableAsObjectToJSON(const UDataTable& InDataTable, TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> JsonWriter)
{
	if (!InDataTable.RowStruct)
	{
		return false;
	}

	JsonWriter->WriteObjectStart(InDataTable.GetName());

	// Iterate over rows
	for (auto RowIt = InDataTable.GetRowMap().CreateConstIterator(); RowIt; ++RowIt)
	{
		// RowName
		const FName RowName = RowIt.Key();
		JsonWriter->WriteObjectStart(RowName.ToString());
		{
			// Now the values
			uint8* RowData = RowIt.Value();
			UFileHelperBPLibrary::WriteRowToJSON(InDataTable.RowStruct, RowData, JsonWriter);
		}
		JsonWriter->WriteObjectEnd();
	}
	JsonWriter->WriteObjectEnd();

	return true;
}

bool UFileHelperBPLibrary::WriteRowToJSON(const UScriptStruct* InRowStruct, const void* InRowData, TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> JsonWriter)
{
	if (!InRowStruct)
	{
		return false;
	}

	return UFileHelperBPLibrary::WriteStructToJSON(InRowStruct, InRowData, JsonWriter);
}

bool UFileHelperBPLibrary::WriteStructToJSON(const UScriptStruct* InStruct, const void* InStructData, TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> JsonWriter)
{
	for (TFieldIterator<const FProperty> It(InStruct); It; ++It)
	{
		const FProperty* BaseProp = *It;
		check(BaseProp);

		const FString Identifier = DataTableUtils::GetPropertyExportName(BaseProp, EDataTableExportFlags::UseJsonObjectsForStructs);

		if (BaseProp->ArrayDim == 1)
		{
			const void* Data = BaseProp->ContainerPtrToValuePtr<void>(InStructData, 0);
			UFileHelperBPLibrary::WriteStructEntryToJSON(InStructData, BaseProp, Data, JsonWriter);
		}
		else
		{
			JsonWriter->WriteArrayStart(Identifier);

			for (int32 ArrayEntryIndex = 0; ArrayEntryIndex < BaseProp->ArrayDim; ++ArrayEntryIndex)
			{
				const void* Data = BaseProp->ContainerPtrToValuePtr<void>(InStructData, ArrayEntryIndex);
				UFileHelperBPLibrary::WriteContainerEntryToJSON(BaseProp, Data, &Identifier, JsonWriter);
			}

			JsonWriter->WriteArrayEnd();
		}
	}

	return true;
}

bool UFileHelperBPLibrary::WriteStructEntryToJSON(const void* InRowData, const FProperty* InProperty, const void* InPropertyData, TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> JsonWriter)
{
	const FString Identifier = DataTableUtils::GetPropertyExportName(InProperty, EDataTableExportFlags::UseJsonObjectsForStructs);

	if (const FEnumProperty* EnumProp = CastField<const FEnumProperty>(InProperty))
	{
		const FString PropertyValue = DataTableUtils::GetPropertyValueAsString(EnumProp, (uint8*)InRowData, EDataTableExportFlags::UseJsonObjectsForStructs);
		JsonWriter->WriteValue(Identifier, PropertyValue);
	}
	else if (const FNumericProperty* NumProp = CastField<const FNumericProperty>(InProperty))
	{
		if (NumProp->IsEnum())
		{
			const FString PropertyValue = DataTableUtils::GetPropertyValueAsString(InProperty, (uint8*)InRowData, EDataTableExportFlags::UseJsonObjectsForStructs);
			JsonWriter->WriteValue(Identifier, PropertyValue);
		}
		else if (NumProp->IsInteger())
		{
			const int64 PropertyValue = NumProp->GetSignedIntPropertyValue(InPropertyData);
			JsonWriter->WriteValue(Identifier, PropertyValue);
		}
		else
		{
			const double PropertyValue = NumProp->GetFloatingPointPropertyValue(InPropertyData);
			JsonWriter->WriteValue(Identifier, PropertyValue);
		}
	}
	else if (const FBoolProperty* BoolProp = CastField<const FBoolProperty>(InProperty))
	{
		const bool PropertyValue = BoolProp->GetPropertyValue(InPropertyData);
		JsonWriter->WriteValue(Identifier, PropertyValue);
	}
	else if (const FArrayProperty* ArrayProp = CastField<const FArrayProperty>(InProperty))
	{
		JsonWriter->WriteArrayStart(Identifier);

		FScriptArrayHelper ArrayHelper(ArrayProp, InPropertyData);
		for (int32 ArrayEntryIndex = 0; ArrayEntryIndex < ArrayHelper.Num(); ++ArrayEntryIndex)
		{
			const uint8* ArrayEntryData = ArrayHelper.GetRawPtr(ArrayEntryIndex);
			UFileHelperBPLibrary::WriteContainerEntryToJSON(ArrayProp->Inner, ArrayEntryData, &Identifier, JsonWriter);
		}

		JsonWriter->WriteArrayEnd();
	}
	else if (const FSetProperty* SetProp = CastField<const FSetProperty>(InProperty))
	{
		JsonWriter->WriteArrayStart(Identifier);

		FScriptSetHelper SetHelper(SetProp, InPropertyData);
		for (int32 SetSparseIndex = 0; SetSparseIndex < SetHelper.GetMaxIndex(); ++SetSparseIndex)
		{
			if (SetHelper.IsValidIndex(SetSparseIndex))
			{
				const uint8* SetEntryData = SetHelper.GetElementPtr(SetSparseIndex);
				UFileHelperBPLibrary::WriteContainerEntryToJSON(SetHelper.GetElementProperty(), SetEntryData, &Identifier, JsonWriter);
			}
		}

		JsonWriter->WriteArrayEnd();
	}
	else if (const FMapProperty* MapProp = CastField<const FMapProperty>(InProperty))
	{
		JsonWriter->WriteObjectStart(Identifier);

		FScriptMapHelper MapHelper(MapProp, InPropertyData);
		for (int32 MapSparseIndex = 0; MapSparseIndex < MapHelper.GetMaxIndex(); ++MapSparseIndex)
		{
			if (MapHelper.IsValidIndex(MapSparseIndex))
			{
				const uint8* MapKeyData = MapHelper.GetKeyPtr(MapSparseIndex);
				const uint8* MapValueData = MapHelper.GetValuePtr(MapSparseIndex);

				// JSON object keys must always be strings
				const FString KeyValue = DataTableUtils::GetPropertyValueAsStringDirect(MapHelper.GetKeyProperty(), (uint8*)MapKeyData, EDataTableExportFlags::UseJsonObjectsForStructs);
				UFileHelperBPLibrary::WriteContainerEntryToJSON(MapHelper.GetValueProperty(), MapValueData, &KeyValue, JsonWriter);
			}
		}

		JsonWriter->WriteObjectEnd();
	}
	else if (const FStructProperty* StructProp = CastField<const FStructProperty>(InProperty))
	{
		JsonWriter->WriteObjectStart(Identifier);
		UFileHelperBPLibrary::WriteStructToJSON(StructProp->Struct, InPropertyData, JsonWriter);
		JsonWriter->WriteObjectEnd();
	}
	else
	{
		const FString PropertyValue = DataTableUtils::GetPropertyValueAsString(InProperty, (uint8*)InRowData, EDataTableExportFlags::UseJsonObjectsForStructs);
		JsonWriter->WriteValue(Identifier, PropertyValue);
	}

	return true;
}

bool UFileHelperBPLibrary::WriteContainerEntryToJSON(const FProperty* InProperty, const void* InPropertyData, const FString* InIdentifier, TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> JsonWriter)
{
	if (const FEnumProperty* EnumProp = CastField<const FEnumProperty>(InProperty))
	{
		const FString PropertyValue = DataTableUtils::GetPropertyValueAsStringDirect(InProperty, (uint8*)InPropertyData, EDataTableExportFlags::UseJsonObjectsForStructs);
		UFileHelperBPLibrary::WriteJSONValueWithOptionalIdentifier(JsonWriter, InIdentifier, *PropertyValue);
	}
	else if (const FNumericProperty* NumProp = CastField<const FNumericProperty>(InProperty))
	{
		if (NumProp->IsEnum())
		{
			const FString PropertyValue = DataTableUtils::GetPropertyValueAsStringDirect(InProperty, (uint8*)InPropertyData, EDataTableExportFlags::UseJsonObjectsForStructs);
			UFileHelperBPLibrary::WriteJSONValueWithOptionalIdentifier(JsonWriter, InIdentifier, *PropertyValue);
		}
		else if (NumProp->IsInteger())
		{
			const int64 PropertyValue = NumProp->GetSignedIntPropertyValue(InPropertyData);
			UFileHelperBPLibrary::WriteJSONValueWithOptionalIdentifier(JsonWriter, InIdentifier, *FString::FromInt(PropertyValue));
		}
		else
		{
			const double PropertyValue = NumProp->GetFloatingPointPropertyValue(InPropertyData);
			UFileHelperBPLibrary::WriteJSONValueWithOptionalIdentifier(JsonWriter, InIdentifier, *FString::SanitizeFloat(PropertyValue));
		}
	}
	else if (const FBoolProperty* BoolProp = CastField<const FBoolProperty>(InProperty))
	{
		const bool PropertyValue = BoolProp->GetPropertyValue(InPropertyData);
		UFileHelperBPLibrary::WriteJSONValueWithOptionalIdentifier(JsonWriter, InIdentifier, *(PropertyValue ? FString("true") : FString("false")));
	}
	else if (const FStructProperty* StructProp = CastField<const FStructProperty>(InProperty))
	{
		UFileHelperBPLibrary::WriteJSONObjectStartWithOptionalIdentifier(JsonWriter, InIdentifier);
		UFileHelperBPLibrary::WriteStructToJSON(StructProp->Struct, InPropertyData, JsonWriter);
		JsonWriter->WriteObjectEnd();
	}
	else if (const FArrayProperty* ArrayProp = CastField<const FArrayProperty>(InProperty))
	{
		// Cannot nest arrays
		return false;
	}
	else if (const FSetProperty* SetProp = CastField<const FSetProperty>(InProperty))
	{
		// Cannot nest sets
		return false;
	}
	else if (const FMapProperty* MapProp = CastField<const FMapProperty>(InProperty))
	{
		// Cannot nest maps
		return false;
	}
	else
	{
		const FString PropertyValue = DataTableUtils::GetPropertyValueAsStringDirect(InProperty, (uint8*)InPropertyData, EDataTableExportFlags::UseJsonObjectsForStructs);
		UFileHelperBPLibrary::WriteJSONValueWithOptionalIdentifier(JsonWriter, InIdentifier, *PropertyValue);
	}

	return true;
}

void UFileHelperBPLibrary::WriteJSONObjectStartWithOptionalIdentifier(TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> JsonWriter, const FString* InIdentifier)
{
	if (InIdentifier)
	{
		JsonWriter->WriteObjectStart(*InIdentifier);
	}
	else
	{
		JsonWriter->WriteObjectStart();
	}
}

void UFileHelperBPLibrary::WriteJSONValueWithOptionalIdentifier(TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> JsonWriter, const FString* InIdentifier, const TCHAR* InValue)
{
	if (InIdentifier)
	{
		JsonWriter->WriteValue(*InIdentifier, InValue);
	}
	else
	{
		JsonWriter->WriteValue(InValue);
	}
}

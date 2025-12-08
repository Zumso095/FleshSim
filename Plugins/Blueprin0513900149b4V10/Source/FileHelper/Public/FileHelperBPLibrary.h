// Copyright 2025 RLoris

#pragma once

#include "JsonObjectConverter.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FileHelperBPLibrary.generated.h"

class FConfigFile;
class UDataTable;

USTRUCT(BlueprintType)
struct FCustomNodeStat
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "FileHelper|FileSystem")
	bool IsDirectory;

	UPROPERTY(BlueprintReadOnly, Category = "FileHelper|FileSystem")
	bool IsReadOnly;

	UPROPERTY(BlueprintReadOnly, Category = "FileHelper|FileSystem")
	FDateTime LastAccessTime;

	UPROPERTY(BlueprintReadOnly, Category = "FileHelper|FileSystem")
	FDateTime CreationTime;

	UPROPERTY(BlueprintReadOnly, Category = "FileHelper|FileSystem")
	FDateTime ModificationTime;

	UPROPERTY(BlueprintReadOnly, Category = "FileHelper|FileSystem")
	int64 FileSize;

	FCustomNodeStat()
		: IsDirectory(false)
		, IsReadOnly(false)
		, LastAccessTime(FDateTime::MinValue())
		, CreationTime(FDateTime::MinValue())
		, ModificationTime(FDateTime::MinValue())
		, FileSize(0)
	{}
};

USTRUCT(BlueprintType)
struct FProjectPath
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "FileHelper|Path")
	FString Directory;

	UPROPERTY(BlueprintReadOnly, Category = "FileHelper|Path")
	FString Config;

	UPROPERTY(BlueprintReadOnly, Category = "FileHelper|Path")
	FString Content;

	UPROPERTY(BlueprintReadOnly, Category = "FileHelper|Path")
	FString Intermediate;

	UPROPERTY(BlueprintReadOnly, Category = "FileHelper|Path")
	FString Log;

	UPROPERTY(BlueprintReadOnly, Category = "FileHelper|Path")
	FString Mods;

	UPROPERTY(BlueprintReadOnly, Category = "FileHelper|Path")
	FString Plugins;

	UPROPERTY(BlueprintReadOnly, Category = "FileHelper|Path")
	FString Saved;

	UPROPERTY(BlueprintReadOnly, Category = "FileHelper|Path")
	FString User;

	UPROPERTY(BlueprintReadOnly, Category = "FileHelper|Path")
	FString PersistentDownload;

	UPROPERTY(BlueprintReadOnly, Category = "FileHelper|Path")
	FString PlatformExtensions;
};

USTRUCT(BlueprintType)
struct FEnginePath
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "FileHelper|Path")
	FString Directory;

	UPROPERTY(BlueprintReadOnly, Category = "FileHelper|Path")
	FString Config;

	UPROPERTY(BlueprintReadOnly, Category = "FileHelper|Path")
	FString Content;

	UPROPERTY(BlueprintReadOnly, Category = "FileHelper|Path")
	FString Intermediate;

	UPROPERTY(BlueprintReadOnly, Category = "FileHelper|Path")
	FString Plugins;

	UPROPERTY(BlueprintReadOnly, Category = "FileHelper|Path")
	FString Saved;

	UPROPERTY(BlueprintReadOnly, Category = "FileHelper|Path")
	FString User;

	UPROPERTY(BlueprintReadOnly, Category = "FileHelper|Path")
	FString DefaultLayout;

	UPROPERTY(BlueprintReadOnly, Category = "FileHelper|Path")
	FString PlatformExtensions;

	UPROPERTY(BlueprintReadOnly, Category = "FileHelper|Path")
	FString UserLayout;
};

UCLASS()
class UFileHelperBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/* Path */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "GetEngineDirectories", CompactNodeTitle = "EngineDirs", Keywords = "File plugin path engine directory", ToolTip = "Gets the engine directories"), Category = "FileHelper|Path")
	static FEnginePath GetEngineDirectories();

	UFUNCTION(BlueprintPure, meta = (DisplayName = "GetProjectDirectories", CompactNodeTitle = "ProjectDirs", Keywords = "File plugin path project directory", ToolTip = "Gets the project directories"), Category = "FileHelper|Path")
	static FProjectPath GetProjectDirectories();

	/* File */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "ReadTextFile", CompactNodeTitle = "ReadText", Keywords = "File plugin read text", ToolTip = "Read a standard text file"), Category = "FileHelper|File|Text")
	static bool ReadText(FString Path, FString& Output);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "WriteTextFile", CompactNodeTitle = "WriteText", Keywords = "File plugin write text", ToolTip = "Save a standard text file"), Category = "FileHelper|File|Text")
	static bool SaveText(FString Path, FString Text, FString& Error, bool Append = false, bool Force = false);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "ReadLineFile", CompactNodeTitle = "ReadLine", Keywords = "File plugin read text lines pattern", ToolTip = "Read the lines of a standard text file"), Category = "FileHelper|File|Text")
	static bool ReadLine(FString Path, FString Pattern, TArray<FString>& Lines);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "ReadLineRangeFile", CompactNodeTitle = "ReadLineRange", Keywords = "File plugin read text lines range", ToolTip = "Read range of lines of a standard text file"), Category = "FileHelper|File|Text")
	static bool ReadLineRange(FString InPath, TArray<FString>& OutLines, int32 InStartIdx = 0, int32 InEndIdx = -1);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "WriteLineFile", CompactNodeTitle = "WriteLine", Keywords = "File plugin write text lines", ToolTip = "Save lines in a standard text file"), Category = "FileHelper|File|Text")
	static bool SaveLine(FString Path, const TArray<FString>& Text, FString& Error, bool Append = false, bool Force = false);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "ReadByteFile", CompactNodeTitle = "ReadByte", Keywords = "File plugin read byte", ToolTip = "Read byte file"), Category = "FileHelper|File|Byte")
	static bool ReadByte(FString Path, TArray<uint8>& Bytes);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "WriteByteFile", CompactNodeTitle = "WriteByte", Keywords = "File plugin write byte", ToolTip = "Save byte to file"), Category = "FileHelper|File|Byte")
	static bool SaveByte(FString Path, const TArray<uint8>& Bytes, FString& Error, bool Append = false, bool Force = false);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "WriteCSVFile", CompactNodeTitle = "WriteCSV", Keywords = "File plugin write csv", ToolTip = "Save a csv file"), Category = "FileHelper|File|CSV")
	static bool SaveCSV(FString Path, TArray<FString> Headers, TArray<FString> Data, int32& Total, bool Force = false);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "ReadCSVFile", CompactNodeTitle = "ReadCSV", Keywords = "File plugin read csv", ToolTip = "Read a csv file"), Category = "FileHelper|File|CSV")
	static bool ReadCSV(FString Path, TArray<FString>& Headers, TArray<FString>& Data, int32& Total, bool HeaderFirst = true);

	/* Network */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "BytesToBase64", CompactNodeTitle = "ToBase64", Keywords = "File plugin bytes convert base64 encode", ToolTip = "Encodes a byte array to base64"), Category = "FileHelper|File|Byte")
	static FString BytesToBase64(const TArray<uint8> Bytes);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "BytesFromBase64", CompactNodeTitle = "FromBase64", Keywords = "File plugin bytes convert base64 decode", ToolTip = "Decodes a byte array from base64"), Category = "FileHelper|File|Byte")
	static bool BytesFromBase64(const FString Source, TArray<uint8>& Out);

	/* CSV convert */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "StringToCSV", CompactNodeTitle = "StrToCSV", Keywords = "File plugin string csv", ToolTip = "convert a string to csv"), Category = "FileHelper|CSV")
	static bool StringToCSV(FString Content, TArray<FString>& Headers, TArray<FString>& Data, int32& Total, bool HeaderFirst = true);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "CSVToString", CompactNodeTitle = "CSVToStr", Keywords = "File plugin csv string", ToolTip = "convert a csv to string"), Category = "FileHelper|CSV")
	static bool CSVToString(FString& Result, TArray<FString> Headers, TArray<FString> Data, int32& Total);

	/* File system */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "IsFile", CompactNodeTitle = "IsFile", Keywords = "File plugin check file exist", ToolTip = "Check whether a file exists"), Category = "FileHelper|FileSystem")
	static bool IsFile(FString Path);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "IsDirectory", CompactNodeTitle = "IsDir", Keywords = "File plugin check directory exist", ToolTip = "Check whether a directory exists"), Category = "FileHelper|FileSystem")
	static bool IsDirectory(FString Path);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "IsValidFilename", CompactNodeTitle = "IsValidName", Keywords = "File plugin check filename valid", ToolTip = "Check whether a filename is valid and can be used"), Category = "FileHelper|FileSystem")
	static bool IsValidFilename(FString Filename);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "IsValidPath", CompactNodeTitle = "IsValidPath", Keywords = "File plugin check path valid", ToolTip = "Check whether a path is valid and can be used"), Category = "FileHelper|FileSystem")
	static bool IsValidPath(FString Path);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "ValidateFilename", CompactNodeTitle = "ValidateName", Keywords = "File plugin validate path", ToolTip = "Validate a filename to be used on this file system"), Category = "FileHelper|FileSystem")
	static bool ValidateFilename(FString Filename, FString& ValidName);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "SetReadOnlyFlag", CompactNodeTitle = "SetReadOnly", Keywords = "File plugin read only path", ToolTip = "Updates the read only property on file"), Category = "FileHelper|FileSystem")
	static bool SetReadOnlyFlag(FString FilePath, bool Flag);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "GetReadOnlyFlag", CompactNodeTitle = "IsReadOnly", Keywords = "File plugin read only path", ToolTip = "Gets the read only property on file"), Category = "FileHelper|FileSystem")
	static bool GetReadOnlyFlag(FString FilePath);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "GetFileSize", CompactNodeTitle = "GetSize", Keywords = "File plugin size directory", ToolTip = "Gets the size of a file"), Category = "FileHelper|FileSystem")
	static int64 GetFileSize(FString FilePath);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "ListDirectory", CompactNodeTitle = "LsDir", Keywords = "File plugin list directory pattern regex recursive", ToolTip = "List nodes from directory"), Category = "FileHelper|FileSystem")
	static bool ListDirectory(FString Path, FString Pattern, TArray<FString>& Nodes, bool ShowFile = true, bool ShowDirectory = true, bool Recursive = false);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "MakeDirectory", CompactNodeTitle = "MkDir", Keywords = "File plugin make directory recursive", ToolTip = "Create a new directory"), Category = "FileHelper|FileSystem")
	static bool MakeDirectory(FString Path, bool Recursive = true);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "RemoveDirectory", CompactNodeTitle = "RmDir", Keywords = "File plugin remove directory recursive", ToolTip = "Removes a directory"), Category = "FileHelper|FileSystem")
	static bool RemoveDirectory(FString Path, bool Recursive = false);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "CopyDirectory", CompactNodeTitle = "CpDir", Keywords = "File plugin copy directory recursive", ToolTip = "Copies a directory"), Category = "FileHelper|FileSystem")
	static bool CopyDirectory(FString Source, FString Dest);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "MoveDirectory", CompactNodeTitle = "MvDir", Keywords = "File plugin move directory recursive", ToolTip = "Moves a directory"), Category = "FileHelper|FileSystem")
	static bool MoveDirectory(FString Source, FString Dest);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "NodeStats", CompactNodeTitle = "NodeStats", Keywords = "File plugin stats directory node", ToolTip = "Gets the stats of a node"), Category = "FileHelper|FileSystem")
	static bool NodeStats(FString Path, FCustomNodeStat& Stats);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "RemoveFile", CompactNodeTitle = "RmFile", Keywords = "File plugin remove file recursive", ToolTip = "Removes a file"), Category = "FileHelper|FileSystem")
	static bool RemoveFile(FString Path);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "CopyFile", CompactNodeTitle = "CpFile", Keywords = "File plugin copy file recursive", ToolTip = "Copies a file"), Category = "FileHelper|FileSystem")
	static bool CopyFile(FString Source, FString Dest, bool Force = false);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "MoveFile", CompactNodeTitle = "MvFile", Keywords = "File plugin move file recursive", ToolTip = "Moves a file"), Category = "FileHelper|FileSystem")
	static bool MoveFile(FString Source, FString Dest, bool Force = false);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "RenameFile", CompactNodeTitle = "RenameFile", Keywords = "File plugin rename file recursive", ToolTip = "Renames a file"), Category = "FileHelper|FileSystem")
	static bool RenameFile(FString Path, FString NewName);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "PathParts", Keywords = "File plugin path parts", ToolTip = "Gets the parts of a path"), Category = "FileHelper|FileSystem")
	static void GetPathParts(FString Path, FString& PathPart, FString& BasePart, FString& ExtensionPart, FString& FileName);

	/* Datatable */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "DataTableToCSV", Keywords = "File plugin datatable csv convert export", ToolTip = "Converts a datatable to csv string"), Category = "FileHelper|Datatable")
	static bool DatatableToCSV(UDataTable* Table, FString& Output);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "DataTableToJSON", Keywords = "File plugin datatable json convert export", ToolTip = "Converts a datatable to json string"), Category = "FileHelper|Datatable")
	static bool DataTableToJSON(UDataTable* Table, FString& Output);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "CSVToDataTable", Keywords = "File plugin datatable csv convert import", ToolTip = "Converts a csv string to datatable"), Category = "FileHelper|Datatable")
	static UDataTable* CSVToDataTable(FString CSV, UScriptStruct* Struct, bool& Success);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "JSONToDataTable", Keywords = "File plugin datatable json convert import", ToolTip = "Converts a json string to datatable"), Category = "FileHelper|Datatable")
	static UDataTable* JSONToDataTable(FString JSON, UScriptStruct* Struct, bool& Success);

	/** Reads a value at a specific key in a section from a config file */
	UFUNCTION(BlueprintCallable, Category = "FileHelper|Config", CustomThunk, meta = (CustomStructureParam = "OutValue"))
	static void ReadConfig(FString FilePath, FString Section, FString Key, bool& Success, bool SingleLineArrayRead, UStruct*& OutValue);
	DECLARE_FUNCTION(execReadConfig)
	{
		P_GET_PROPERTY(FStrProperty, FilePath);
		P_GET_PROPERTY(FStrProperty, Section);
		P_GET_PROPERTY(FStrProperty, Key);
		P_GET_UBOOL_REF(Success);
		P_GET_UBOOL(SingleLineArrayRead);

		Stack.Step(Stack.Object, NULL);

		FProperty* Property = Stack.MostRecentProperty;
		void* ValuePtr = Stack.MostRecentPropertyAddress;

		P_FINISH;

		Success = UFileHelperBPLibrary::ReadConfigFile(FilePath, Section, Key, Property, ValuePtr, SingleLineArrayRead);
	}

	/** Writes a value at a specific key in a section from a config file, also creates the file when not found */
	UFUNCTION(BlueprintCallable, Category = "FileHelper|Config", CustomThunk, meta = (CustomStructureParam = "Value"))
	static void WriteConfig(FString FilePath, FString Section, FString Key, bool& Success, bool SingleLineArrayWrite, const UStruct* Value);
	DECLARE_FUNCTION(execWriteConfig)
	{
		P_GET_PROPERTY(FStrProperty, FilePath);
		P_GET_PROPERTY(FStrProperty, Section);
		P_GET_PROPERTY(FStrProperty, Key);
		P_GET_UBOOL_REF(Success);
		P_GET_UBOOL(SingleLineArrayWrite);

		Stack.Step(Stack.Object, NULL);

		FProperty* Property = Stack.MostRecentProperty;
		void* ValuePtr = Stack.MostRecentPropertyAddress;

		P_FINISH;

		Success = UFileHelperBPLibrary::WriteConfigFile(FilePath, Section, Key, Property, ValuePtr, SingleLineArrayWrite);
	}

	/** Removes a specific key in a section from a config file, also deletes the file when empty */
	UFUNCTION(BlueprintCallable, Category = "FileHelper|Config")
	static bool RemoveConfig(FString FilePath, FString Section, FString Key);

protected:
	/* Utility */
	static TArray<FString> SplitString(FString String, FString Separator, ESearchCase::Type SearchCase);
	static bool StringArrayToCSV(TArray<FString> Lines, TArray<FString>& Headers, TArray<FString>& Data, int32& Total, FString Delimiter = ",", bool HeaderFirst = true);
	// config ini
	static bool WriteConfigFile(FString Filename, FString Section, FString Key, FProperty* Type, void* Value, bool SingleLineArray);
	static bool ReadConfigFile(FString Filename, FString Section, FString Key, FProperty* Type, void* Value, bool SingleLineArray);
	static void FindOrCreateConfigFile(const FString& InFilePath, FConfigFile& OutConfigFile);
	static bool SaveConfigFile(const FString& InFilePath);
	// datatable csv
	static bool WriteTableToCSV(const UDataTable& InDataTable, FString& Output);
	static bool WriteRowToCSV(const UScriptStruct* InRowStruct, const void* InRowData, FString& ExportedText);
	static bool WriteStructEntryToCSV(const void* InRowData, FProperty* InProperty, const void* InPropertyData, FString& ExportedText);
	// datatable json
	static FString GetKeyFieldName(const UDataTable& InDataTable);
	static bool WriteTableToJSON(const UDataTable& InDataTable, FString& OutExportText);
	static bool WriteTableAsObjectToJSON(const UDataTable& InDataTable, TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> JsonWriter);
	static bool WriteRowToJSON(const UScriptStruct* InRowStruct, const void* InRowData, TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> JsonWriter);
	static bool WriteStructToJSON(const UScriptStruct* InStruct, const void* InStructData, TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> JsonWriter);
	static bool WriteStructEntryToJSON(const void* InRowData, const FProperty* InProperty, const void* InPropertyData, TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> JsonWriter);
	static bool WriteContainerEntryToJSON(const FProperty* InProperty, const void* InPropertyData, const FString* InIdentifier, TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> JsonWriter);
	static void WriteJSONObjectStartWithOptionalIdentifier(TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> JsonWriter, const FString* InIdentifier);
	static void WriteJSONValueWithOptionalIdentifier(TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> JsonWriter, const FString* InIdentifier, const TCHAR* InValue);
};
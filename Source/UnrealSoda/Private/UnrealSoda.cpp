// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Soda/ISodaVehicleComponent.h"
#include "Soda/UI/SodaViewportCommands.h"
#include "RuntimeEditorModule.h"
#include "Customization/SoftSodaActorPtrCustomization.h"
#include "Customization/SoftSodaVehicleComponentPtrCustomization.h"
#include "Customization/SubobjectReferenceCustomization.h"
#include "RuntimePropertyEditor/PropertyHandle.h"
#include "Interfaces/IPluginManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"
#include "Soda/Vehicles/ISodaVehicleExporter.h"
#include "Soda/Misc/JsonArchive.h"
#include "Soda/Vehicles/SodaVehicle.h"
#include "Misc/SodaVehicleCommonExporter.h"
#include "Soda/SodaUserSettings.h"
#include "Misc/ConfigContext.h"
#include "Soda/MongoDB/MongoDBGateway.h"
#include "Soda/MongoDB/MongoDBDataset.h"
#include "MongoDB/MongoDBDatasetRegisterObjects.h"
#include "Soda/MongoDB/MongoDBSource.h"

#define LOCTEXT_NAMESPACE "FUnrealSodaModule"

class FSodaVehicleJSONExporter : public soda::ISodaVehicleExporter
{
public:
	virtual bool ExportToString(const ASodaVehicle* Vehicle, FString& String) override
	{
		FJsonActorArchive Ar;
		if (Ar.SerializeActor(const_cast<ASodaVehicle*>(Vehicle), true))
		{
			if (!Ar.SaveToString(String))
			{
				UE_LOG(LogSoda, Error, TEXT("FSodaVehicleJSONExporter::ExportToString(); Can't serialize"));
				return false;
			}
			else
			{
				return true;
			}
		}
		else
		{
			UE_LOG(LogSoda, Error, TEXT("FSodaVehicleJSONExporter::ExportToString(); Can't save to string"));
			return false;
		}
	}

	virtual const FName& GetExporterName() const override { return ExporterName; }
	virtual const FString& GetFileTypes() const override { return ExporterFileType; }

	static const FName ExporterName;
	static const FString ExporterFileType;
};

const FName FSodaVehicleJSONExporter::ExporterName = TEXT("Soda JSON Vehicles");
const FString FSodaVehicleJSONExporter::ExporterFileType = TEXT("Soda JSON Vehicles (*.json)|*.json");


void FUnrealSodaModule::StartupModule()
{

#if PLATFORM_WINDOWS
	FString DllDirectory = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("SodaSim"))->GetBaseDir(), TEXT("/Binaries/Win64"));
	TArray<FString> DllNames = { "libsodium.dll", "libzmq-v141-mt-4_3_2.dll", "SDL2.dll" };
	FPlatformProcess::PushDllDirectory(*DllDirectory);

	for (auto& It : DllNames)
	{
		FString DllPath = FPaths::Combine(DllDirectory, It);
		void * LibHandle = FPlatformProcess::GetDllHandle(*DllPath);
		if (LibHandle == nullptr)
		{
			UE_LOG(LogSoda, Fatal, TEXT("FUnrealSodaModule::StartupModule(): Failed to load required library %s. Plug-in will not be functional."), *DllPath);
		}
		else
		{
			LoadedDlls.Add(LibHandle);
		}
	}
	FPlatformProcess::PopDllDirectory(*DllDirectory);
#endif

	FModuleManager::LoadModuleChecked<IModuleInterface>("SodaStyle");
	FRuntimeEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FRuntimeEditorModule>("RuntimeEditor");

	SodaApp.Initialize();
	soda::FSodalViewportCommands::Register();

	{
		class FSodaActorPropertyTypeIdentifier : public soda::IPropertyTypeIdentifier
		{
			virtual bool IsPropertyTypeCustomized(const soda::IPropertyHandle& PropertyHandle) const override
			{
				const FProperty* NodeProperty = PropertyHandle.GetProperty();

				const FObjectPropertyBase* ObjectProperty = CastField<const FObjectPropertyBase>(NodeProperty);
				const FInterfaceProperty* InterfaceProperty = CastField<const FInterfaceProperty>(NodeProperty);

				if ((ObjectProperty != nullptr || InterfaceProperty != nullptr)
					&& !NodeProperty->IsA(FClassProperty::StaticClass())
					&& !NodeProperty->IsA(FSoftClassProperty::StaticClass())
					&& ObjectProperty->PropertyClass
					&& ObjectProperty->PropertyClass->IsChildOf(AActor::StaticClass()))
				{
					return true;
				}

				return false;
			}
		};

		PropertyEditorModule.RegisterCustomPropertyTypeLayout(
			"SoftObjectProperty",
			soda::FOnGetPropertyTypeCustomizationInstance::CreateStatic(&soda::FSoftSodaActorPtrCustomization::MakeInstance),
			MakeShared<FSodaActorPropertyTypeIdentifier>()
		);
	}

	{
		class FSodaVehicleComponentPropertyTypeIdentifier : public soda::IPropertyTypeIdentifier
		{
			virtual bool IsPropertyTypeCustomized(const soda::IPropertyHandle& PropertyHandle) const override
			{
				const FProperty* NodeProperty = PropertyHandle.GetProperty();

				const FObjectPropertyBase* ObjectProperty = CastField<const FObjectPropertyBase>(NodeProperty);
				const FInterfaceProperty* InterfaceProperty = CastField<const FInterfaceProperty>(NodeProperty);

				if ((ObjectProperty != nullptr || InterfaceProperty != nullptr)
					&& !NodeProperty->IsA(FClassProperty::StaticClass())
					&& !NodeProperty->IsA(FSoftClassProperty::StaticClass())
					&& ObjectProperty->PropertyClass
					&& ObjectProperty->PropertyClass->ImplementsInterface(USodaVehicleComponent::StaticClass()))
				{
					return true;
				}

				return false;
			}
		};

		PropertyEditorModule.RegisterCustomPropertyTypeLayout(
			"SoftObjectProperty",
			soda::FOnGetPropertyTypeCustomizationInstance::CreateStatic(&soda::FSoftSodaVehicleComponentPtrCustomization::MakeInstance),
			MakeShared<FSodaVehicleComponentPropertyTypeIdentifier>()
		);
	}

	{
		PropertyEditorModule.RegisterCustomPropertyTypeLayout(
			"SubobjectReference",
			soda::FOnGetPropertyTypeCustomizationInstance::CreateStatic(&soda::FSubobjectReferenceCustomization::MakeInstance)
		);
	}

	PropertyEditorModule.NotifyCustomizationModuleChanged();

	SodaApp.RegisterVehicleExporter(MakeShared<FSodaVehicleJSONExporter>());
	SodaApp.RegisterVehicleExporter(MakeShared<FSodaVehicleCommonExporter>());

	auto MongoDBDatasetManager = MakeShared<soda::mongodb::FMongoDBDatasetManager>();
	SodaApp.RegisterDatasetManager("MongoDB", MongoDBDatasetManager);
	soda::mongodb::RegisteDefaultObjects(*MongoDBDatasetManager);

	SodaApp.GetFileDatabaseManager().RegisterSource(MakeShared<soda::FMongoDBSource>());

	UE_LOG(LogSoda, Log, TEXT("FUnrealSodaModule::StartupModule(); CustomConfig: \"%s\""), *FConfigCacheIni::GetCustomConfigString());
}

void FUnrealSodaModule::ShutdownModule()
{
	soda::FMongoDBGetway::Get(false).Destroy();

	SodaApp.UnregisterVehicleExporter(FSodaVehicleJSONExporter::ExporterName);
	SodaApp.UnregisterVehicleExporter(FSodaVehicleCommonExporter::ExporterName);

	SodaApp.Release();

	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FRuntimeEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FRuntimeEditorModule>("RuntimeEditor");
		//PropertyEditorModule.UnregisterCustomPropertyTypeLayout("SoftSodaActorPtr");
		//PropertyEditorModule.UnregisterCustomPropertyTypeLayout("SoftSodaActorPtr");
		PropertyEditorModule.NotifyCustomizationModuleChanged();
	}

#if PLATFORM_WINDOWS
	for (auto& It : LoadedDlls)
	{
		FPlatformProcess::FreeDllHandle(It);
	}
#endif

	
}

#undef LOCTEXT_NAMESPACE

DEFINE_LOG_CATEGORY(LogSoda);
IMPLEMENT_MODULE(FUnrealSodaModule, UnrealSoda)

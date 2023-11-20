// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

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

#define LOCTEXT_NAMESPACE "FUnrealSodaModule"

class FSodaVehicleJSONExporter : public ISodaVehicleExporter
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

	virtual const FString& GetExporterName() const override { return ExporterName; }
	virtual const FString& GetFileTypes() const override { return ExporterFileType; }

	static const FString ExporterName;
	static const FString ExporterFileType;
};

const FString FSodaVehicleJSONExporter::ExporterName = TEXT("Soda JSON Vehicles");
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
}

void FUnrealSodaModule::ShutdownModule()
{
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

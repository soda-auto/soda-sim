// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "MongoDBDatasetRegisterObjects.h"
#include "Soda/MongoDB/MongoDBDataset.h"
#include "Soda/MongoDB/MongoDBGateway.h"

#include "DatasetHandlers/GenericVehicleDriverComponent_Handler.hpp"
#include "DatasetHandlers/GenericWheeledVehicleSensor_Handler.hpp"
#include "DatasetHandlers/GhostPedestriane_Handler.hpp"
#include "DatasetHandlers/GhostVehicle_Handler.hpp"
#include "DatasetHandlers/NavSensor_Handler.hpp"
#include "DatasetHandlers/OpenDriveTool_Handler.hpp"
#include "DatasetHandlers/RacingSensor_Handler.hpp"
#include "DatasetHandlers/SodaVehicle_Handler.hpp"
#include "DatasetHandlers/SodaWheeledVehicle_Handler.hpp"
#include "DatasetHandlers/TrackBuilder_Handler.hpp"
#include "DatasetHandlers/VehicleBrakeSystemComponent_Handler.hpp"
#include "DatasetHandlers/VehicleDifferentialComponent_Handler.hpp"
#include "DatasetHandlers/VehicleEngineComponent_Handler.hpp"
#include "DatasetHandlers/VehicleGearBoxComponent_Handler.hpp"
#include "DatasetHandlers/VehicleInputAIComponent_Handler.hpp"
#include "DatasetHandlers/VehicleInputExternalComponent_Handler.hpp"
#include "DatasetHandlers/VehicleInputJoyComponent_Handler.hpp"
#include "DatasetHandlers/VehicleInputKeyboardComponent_Handler.hpp"
#include "DatasetHandlers/VehicleSteeringComponent_Handler.hpp"


namespace soda
{
namespace mongodb 
{
	void RegisteDefaultObjects(FMongoDBDatasetManager& Manager)
	{
		Manager.RegisterObjectHandler(UGenericVehicleDriverComponent::StaticClass(), [](UObject * Object) { return MakeShared<FGenericVehicleDriverComponent_Handler>(Cast<UGenericVehicleDriverComponent>(Object)); });
		Manager.RegisterObjectHandler(UGenericWheeledVehicleSensor::StaticClass(), [](UObject* Object) { return MakeShared<FGenericWheeledVehicleSensor_Handler>(Cast<UGenericWheeledVehicleSensor>(Object)); });
		Manager.RegisterObjectHandler(AGhostPedestrian::StaticClass(), [](UObject* Object) { return MakeShared<FGhostPedestrian_Handler>(Cast<AGhostPedestrian>(Object)); });
		Manager.RegisterObjectHandler(AGhostVehicle::StaticClass(), [](UObject* Object) { return MakeShared<FGhostVehicle_Handler>(Cast<AGhostVehicle>(Object)); });
		Manager.RegisterObjectHandler(UNavSensor::StaticClass(), [](UObject* Object) { return MakeShared<FNavSensor_Handler>(Cast<UNavSensor>(Object)); });
		Manager.RegisterObjectHandler(AOpenDriveTool::StaticClass(), [](UObject* Object) { return MakeShared<FOpenDriveTool_Handler>(Cast<AOpenDriveTool>(Object)); });
		Manager.RegisterObjectHandler(URacingSensor::StaticClass(), [](UObject* Object) { return MakeShared<FRacingSensor_Handler>(Cast<URacingSensor>(Object)); });
		Manager.RegisterObjectHandler(ASodaVehicle::StaticClass(), [](UObject* Object) { return MakeShared<FSodaVehicle_Handler>(Cast<ASodaVehicle>(Object)); });
		Manager.RegisterObjectHandler(ASodaWheeledVehicle::StaticClass(), [](UObject* Object) { return MakeShared<FSodaWheeledVehicle_Handler>(Cast<ASodaWheeledVehicle>(Object)); });
		Manager.RegisterObjectHandler(ATrackBuilder::StaticClass(), [](UObject* Object) { return MakeShared<FTrackBuilder_Handler>(Cast<ATrackBuilder>(Object)); });
		Manager.RegisterObjectHandler(UVehicleBrakeSystemSimpleComponent::StaticClass(), [](UObject* Object) { return MakeShared<FVehicleBrakeSystemSimpleComponent_Handler>(Cast<UVehicleBrakeSystemSimpleComponent>(Object)); });
		Manager.RegisterObjectHandler(UVehicleDifferentialSimpleComponent::StaticClass(), [](UObject* Object) { return MakeShared<FehicleDifferentialSimpleComponent_Handler>(Cast<UVehicleDifferentialSimpleComponent>(Object)); });
		Manager.RegisterObjectHandler(UVehicleEngineSimpleComponent::StaticClass(), [](UObject* Object) { return MakeShared<FVehicleEngineSimpleComponent_Handler>(Cast<UVehicleEngineSimpleComponent>(Object)); });
		Manager.RegisterObjectHandler(UVehicleGearBoxSimpleComponent::StaticClass(), [](UObject* Object) { return MakeShared<FVehicleGearBoxSimpleComponent_Handler>(Cast<UVehicleGearBoxSimpleComponent>(Object)); });
		Manager.RegisterObjectHandler(UVehicleInputAIComponent::StaticClass(), [](UObject* Object) { return MakeShared<FVehicleInputAIComponent_Handler>(Cast<UVehicleInputAIComponent>(Object)); });
		Manager.RegisterObjectHandler(UVehicleInputExternalComponent::StaticClass(), [](UObject* Object) { return MakeShared<FVehicleInputExternalComponent_Handler>(Cast<UVehicleInputExternalComponent>(Object)); });
		Manager.RegisterObjectHandler(UVehicleInputJoyComponent::StaticClass(), [](UObject* Object) { return MakeShared<FVehicleInputJoyComponent_Handler>(Cast<UVehicleInputJoyComponent>(Object)); });
		Manager.RegisterObjectHandler(UVehicleInputKeyboardComponent::StaticClass(), [](UObject* Object) { return MakeShared<FVehicleInputKeyboardComponent_Handler>(Cast<UVehicleInputKeyboardComponent>(Object)); });
		Manager.RegisterObjectHandler(UVehicleSteeringRackSimpleComponent::StaticClass(), [](UObject* Object) { return MakeShared<FVehicleSteeringRackSimpleComponent_Handler>(Cast<UVehicleSteeringRackSimpleComponent>(Object)); });
	}

} // namespace mongodb
} // namespace sodaoda
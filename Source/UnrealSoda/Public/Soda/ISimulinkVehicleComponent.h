// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Templates/SubclassOf.h"
#include "Soda/SodaTypes.h"
#include "Soda/Vehicles/VehicleBaseTypes.h"
#include "Soda/Misc/PhysBodyKinematic.h"
#include "Soda/Misc/Time.h"
#include "Soda/Simulink/SimulinkModel.h"
#include <vector>
#include "ISimulinkVehicleComponent.generated.h"

namespace simulink
{
    class FSimulinkVehicleModel;

enum EIOCapability 
{
    NA = 0,
    VSENSE = 1,//Voltage measurement
    VSET = 2,//Voltage set
    CSENSE = 3,//Current measurement
    CSET = 4, //Current set
    PWMFSENSE = 5, //PWM frequency measurement
    PWMDSENSE = 6, //PWM duty measurement
    PWMFSET = 7, //PWM frequency set
    PWMDSET = 8,//PWM duty set
    CLSENSE = 9,//Current loop measurement
    CLSET = 10,//Current loop set
};

enum EPortDir
{
    Input,
    Output
};

class FCANBusInterface
{
public:
    FCANBusInterface(TSharedPtr<FSimulinkVehicleModel> Model);

    bool RegisterMessage(FName Name, const FString& MessageName, simulink::EPortDir Dir, int Interval = -1);
    bool FeedFrame();
    bool ReadFrame();

    TWeakPtr<FSimulinkVehicleModel> Model;
};

class FLINBusInterface
{
public:
    FCANBusInterface(TSharedPtr<FSimulinkVehicleModel> Model);

    bool RegisterMessage(FName Name, const FString& MessageName, simulink::EPortDir Dir, int Interval = -1);
    bool FeedFrame();
    bool ReadFrame();

    TWeakPtr<FSimulinkVehicleModel> Model;
};

class UNREALSODA_API FSimulinkVehicleModel
{
public:
    FSimulinkVehicleModel(std::sharedPtr<FSimulinkModel> Model);

    bool LoadDBC(const FString & FileName, FName Name);
    bool LoadLDF(const FString & FileName, FName Name);

    TSharedPtr<FCANBusInterface> RegisterCANInterfaca(FName Name);
    TSharedPtr<FLINBusInterface> RegisterLINInterfaca(FName Name);

  
protected:
    std::sharedPtr<FSimulinkModel> Model;

	TMap<FName, TSharedPtr<FCANBusInterface>> CANBusInterfaces;
	TMap<FName, TSharedPtr<FLINBusInterface>> LINBusInterfaces;
	//TMap<FName, TSharedPtr<FIOBusInterface>> IOBusInterfaces;
};

}
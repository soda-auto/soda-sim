#pragma once

//#include "../api/ImitatorSoftwareComponent.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#ifdef PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
THIRD_PARTY_INCLUDES_START
#include <Windows.h>
THIRD_PARTY_INCLUDES_END
#include "Windows/HideWindowsPlatformTypes.h"
#else // PLATFORM_WINDOWS
#include <dlfcn.h>
#endif // PLATFORM_WINDOWS
#include <cstring>
#include <optional>
#include "Soda/Simulink/simulink-capi/rtw_capi.h"
#include "Soda/Simulink/simulink-capi/rtw_modelmap.h"
//#include "simulink-capi/BusBuilder.hpp"
//#include "simulink-capi/CapiAccessor.hpp"
#include <unordered_map>

//using namespace ImitatorSoftwareComponentSpace;

/* Real-time Model Data Structure */
typedef struct tag_RTM_ModelDataStructure_T {
    void* blockIO;
    void* inputs;
    void* outputs;
    void* dwork;
    /*
     * DataMapInfo:
     * The following substructure contains information regarding
     * structures generated in the model's C API.
     */
    struct {
        rtwCAPI_ModelMappingInfo mmi;
        void** dataAddress;
    } DataMapInfo;
} RTM_ModelDataStructure_T;



class UNREALSODA_API FSimulinkModel
{
public:
    FSimulinkModel(const FString& LibPath, const FString& ModelName);
	~FSimulinkModel();

    struct FSignal 
    {
        std::string signalName;
        void* dataAddress;
        std::string dataType;
        uint16_t datalSize;
        //E2E_protection e2e;
    };

    using SignalsMap = std::unordered_map<std::string, FSignal>;
    using PortsMap = std::unordered_map < std::string, std::vector<FSignal>>;
    using GetCAPIStaticMap_t = rtwCAPI_ModelMappingStaticInfo* (*)(void);
    using InitializeDataMapInfo_t =  void (*)(void*);
    using in_ptr_t = void(*)(void*);
    using out_ptr_t = void* (*)(void);
    using in_out_ptr_t = void* (*)(void*);

    std::string getSignalValue(std::string signalName);
    void setSignalValue(std::string signalName, std::string value);
    
    void Step();
    void* GetModelDataPtr() { return ModelDataPtr; }

    SignalsMap GetSignals();
    FSignal GetSignal(const std::string& signalName);
    std::vector<FSignal> * getPort(const std::string& portName);

private:    
    FString ModelName;
    in_ptr_t InitializePtr = nullptr;
    in_ptr_t StepPtr = nullptr;
    in_ptr_t TerminatePtr = nullptr;
    GetCAPIStaticMap_t GetCAPIStaticMapPtr = nullptr;
    void* ModelDataPtr = nullptr;
    void** dataAddrMap = nullptr;
    rtwCAPI_ModelMappingInfo* mmi = nullptr;

#ifdef PLATFORM_WINDOWS
    HINSTANCE hInstDLL = nullptr;
#else
    void* hInstSO = nullptr;
#endif
    rtwCAPI_ModelMappingStaticInfo* CAPIStaticMap = nullptr;
    
    SignalsMap signals;
    PortsMap ports;

    rtwCAPI_ModelMappingInfo* getModelMappingInfo(void* ModelDataPtr);
    //void acceptSignalValue(const std::string& dataType, std::string& value, void* signalAddr);
  //  std::string printSignalValue(const std::string& dataType, void* signalAddr);
    /*void exec_loop(void); */

    void Release();

    void* GetFunc(const std::string& FuncName);
};


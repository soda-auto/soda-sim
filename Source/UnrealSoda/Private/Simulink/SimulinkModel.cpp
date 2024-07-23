#include "Soda/Simulink/SimulinkModel.h"
//#include "Soda/UnrealSoda.h"
#include <filesystem>
#include <map>

#ifdef PLATFORM_WINDOWS
#pragma warning(disable: 4191)
#endif

static std::vector<std::string> splitString(std::string s, std::string delimiter) 
{
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) 
    {
        token = s.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back(token);
    }

    res.push_back(s.substr(pos_start));
    return res;
}

FSimulinkModel::FSimulinkModel(const FString& LibPath, const FString& ModelName)
    : ModelName(ModelName)
{
    std::string ModelNameAnsi(TCHAR_TO_UTF8(*ModelName));

    if (!LibPath.IsEmpty())
    {
#ifdef PLATFORM_WINDOWS

        hInstDLL = LoadLibraryA((LPCSTR)TCHAR_TO_UTF8(*LibPath));
        if (!hInstDLL) 
        {
            throw std::runtime_error("FSimulinkModel::LoadModel(); Unable to Load DLL file");
        }
#else
        hInstSO = dlopen(TCHAR_TO_UTF8(*LibPath), RTLD_LAZY);
        if (!hInstSO) 
        {
            throw std::runtime_error(("FSimulinkModel::LoadModel(); Unable to Load *.so file");
        }
#endif
    }
    else
    {
        throw std::runtime_error("FSimulinkModel::LoadModel(); LibPath is empty");
    }

    out_ptr_t GetModelDataPtr = (out_ptr_t)GetFunc(ModelNameAnsi);
    InitializePtr = (in_ptr_t)GetFunc(ModelNameAnsi + "_initialize");
    StepPtr = (in_ptr_t)GetFunc(ModelNameAnsi + "_step");
    TerminatePtr = (in_ptr_t)GetFunc(ModelNameAnsi + "_terminate");
    in_out_ptr_t GetDataMapInfoPtr = (in_out_ptr_t)GetFunc("GetDataMapInfo");
    GetCAPIStaticMapPtr = (GetCAPIStaticMap_t)GetFunc(ModelNameAnsi + "_GetCAPIStaticMap");

    ModelDataPtr = GetModelDataPtr();
    if (ModelDataPtr == nullptr)
    {
        Release();
        throw std::runtime_error("FSimulinkModel::LoadModel(); Unable to Load ModelDataPtr");
    }

    CAPIStaticMap = (rtwCAPI_ModelMappingStaticInfo*)GetCAPIStaticMapPtr();
    uint32_t ElsmentsCount = CAPIStaticMap->Signals.numSignals +
        CAPIStaticMap->Signals.numRootInputs +
        CAPIStaticMap->Signals.numRootOutputs +
        CAPIStaticMap->Params.numBlockParameters +
        CAPIStaticMap->Params.numModelParameters +
        CAPIStaticMap->States.numStates;
    if (ElsmentsCount <= 0) 
    {
        Release();
        throw std::runtime_error("FSimulinkModel::LoadModel(); Invalid Ports/Parameters count");
    }

    mmi = (rtwCAPI_ModelMappingInfo*)GetDataMapInfoPtr((void*)ModelDataPtr);

    if (mmi == nullptr) 
    {
        Release();
        throw std::runtime_error("FSimulinkModel::LoadModel(); Unable to Load ModelMappingInfo");
    }

    dataAddrMap = rtwCAPI_GetDataAddressMap(mmi);
    dataAddrMap = mmi->InstanceMap.dataAddrMap;

    uint32_t currentElsmentIdx = 0;
    for (uint_T i = 0; i < CAPIStaticMap->Signals.numRootInputs; i++) 
    {
        uint_T addrMapIndex = CAPIStaticMap->Signals.rootInputs[i].addrMapIndex;
        const char* dataEntryName = CAPIStaticMap->Signals.rootInputs[i].blockPath;
        //log_debug("rcv port: " + std::string(dataEntryName));
        uint16_t dataTypeIndex = CAPIStaticMap->Signals.rootInputs[i].dataTypeIndex;
        void* dataEntryAddr = dataAddrMap[addrMapIndex];
        uint32_t dataSize = rtwCAPI_GetDataTypeSize(CAPIStaticMap->Maps.dataTypeMap, dataTypeIndex);

        const char* data_type = CAPIStaticMap->Maps.dataTypeMap[dataTypeIndex].mwDataName;
        uint16_t elemMapIndex = CAPIStaticMap->Maps.dataTypeMap[dataTypeIndex].elemMapIndex;
        uint16_t numElements = rtwCAPI_GetDataTypeNumElements(CAPIStaticMap->Maps.dataTypeMap, dataTypeIndex);
        std::vector<FSignal> port_signals;
        if (numElements) 
        {
            for (int t = 0; t < numElements; t++) 
            {
                const char* signalName = CAPIStaticMap->Maps.elementMap[elemMapIndex + t].elementName;
                //log_debug("\trcv sig: " + std::string(signalName));
                uint16_t elementOffset = CAPIStaticMap->Maps.elementMap[elemMapIndex + t].elementOffset;
                uint16_t signalDataTypeIndex = CAPIStaticMap->Maps.elementMap[elemMapIndex + t].dataTypeIndex;
                const char* signal_data_type = CAPIStaticMap->Maps.dataTypeMap[signalDataTypeIndex].mwDataName;
                uint32_t signalDataSize = rtwCAPI_GetDataTypeSize(CAPIStaticMap->Maps.dataTypeMap, signalDataTypeIndex);
                //uint_T dimension = CAPIStaticMap->Maps.dimensionArray[CAPIStaticMap->Maps.dimensionMap[dimIndex].dimArrayIndex];
                void* signal_address = (void*)((uint64_t)dataEntryAddr + elementOffset);

                FSignal signal = { signalName, signal_address , std::string(signal_data_type), signalDataSize };

                signals.insert(std::make_pair(signalName, signal));
                port_signals.push_back(signal);
            }
        }
        else 
        {
            //std::vector<std::string> dataEntryNameSplit = splitString(dataEntryName, "/"); //getSignalName(imitator, dataEntryName);
            //std::string signalName = signalName.size() == 2 ? dataEntryNameSplit[1] : dataEntryName;
            //FSignal signal = { signalName, dataEntryAddr , std::string(data_type), dataSize };
            //signals.insert(std::make_pair(signalName, signal));
            //port_signals.push_back(signal);

            FSignal signal = { dataEntryName, dataEntryAddr , std::string(data_type), dataSize };
            signals.insert(std::make_pair(dataEntryName, signal));
        }

        ports.insert(std::make_pair(dataEntryName, port_signals));
        currentElsmentIdx++;
    }

    for (uint_T i = 0; i < CAPIStaticMap->Signals.numRootOutputs; i++) 
    {
        uint_T addrMapIndex = CAPIStaticMap->Signals.rootOutputs[i].addrMapIndex;
        const char* dataEntryName = CAPIStaticMap->Signals.rootOutputs[i].blockPath;
        //log_debug("snd potr: " + std::string(dataEntryName));
        uint16_t dataTypeIndex = CAPIStaticMap->Signals.rootOutputs[i].dataTypeIndex;
        void* dataEntryAddr = dataAddrMap[addrMapIndex];
        uint32_t dataSize = rtwCAPI_GetDataTypeSize(CAPIStaticMap->Maps.dataTypeMap, dataTypeIndex);

        const char* data_type = CAPIStaticMap->Maps.dataTypeMap[dataTypeIndex].mwDataName;
        uint16_t elemMapIndex = CAPIStaticMap->Maps.dataTypeMap[dataTypeIndex].elemMapIndex;
        uint16_t numElements = rtwCAPI_GetDataTypeNumElements(CAPIStaticMap->Maps.dataTypeMap, dataTypeIndex);

        if (numElements) 
        {
            std::vector<FSignal> port_signals;
            for (int t = 0; t < numElements; t++) 
            {
                const char* signalName = CAPIStaticMap->Maps.elementMap[elemMapIndex + t].elementName;
                //log_debug("\tsnd sig: " + std::string(signalName));
                uint16_t elementOffset = CAPIStaticMap->Maps.elementMap[elemMapIndex + t].elementOffset;
                uint16_t signalDataTypeIndex = CAPIStaticMap->Maps.elementMap[elemMapIndex + t].dataTypeIndex;
                const char* signal_data_type = CAPIStaticMap->Maps.dataTypeMap[signalDataTypeIndex].mwDataName;
                uint32_t signalDataSize = rtwCAPI_GetDataTypeSize(CAPIStaticMap->Maps.dataTypeMap, signalDataTypeIndex);
                void* signal_address = (void*)((uint64_t)dataEntryAddr + elementOffset);
                FSignal signal = { signalName, signal_address , std::string(signal_data_type), signalDataSize };

                signals.insert(std::make_pair(signalName, signal));
                port_signals.push_back(signal);
            }

            ports.insert(std::make_pair(dataEntryName, port_signals));
        }
        else 
        {
            FSignal signal = { dataEntryName, dataEntryAddr , std::string(data_type), dataSize };
            signals.insert(std::make_pair(dataEntryName, signal));
        }
        currentElsmentIdx++;
    }

    for (uint_T i = 0; i < CAPIStaticMap->Params.numModelParameters; i++) 
    {
        uint_T addrMapIndex = CAPIStaticMap->Params.modelParameters[i].addrMapIndex;
        const char* dataEntryName = CAPIStaticMap->Params.modelParameters[i].varName;
        uint16_t dataTypeIndex = CAPIStaticMap->Params.modelParameters[i].dataTypeIndex;
        void* dataEntryAddr = dataAddrMap[addrMapIndex];
        uint32_t dataSize = rtwCAPI_GetDataTypeSize(CAPIStaticMap->Maps.dataTypeMap, dataTypeIndex);

        const char* data_type = CAPIStaticMap->Maps.dataTypeMap[dataTypeIndex].mwDataName;
        uint16_t elemMapIndex = CAPIStaticMap->Maps.dataTypeMap[dataTypeIndex].elemMapIndex;
        uint16_t numElements = rtwCAPI_GetDataTypeNumElements(CAPIStaticMap->Maps.dataTypeMap, dataTypeIndex);
        //log_debug("param: " + std::string(dataEntryName) +  "[" + std::string(data_type) +"]");
        if (numElements) 
        {
            for (int t = 0; t < numElements; t++) 
            {
                const char* signalName = CAPIStaticMap->Maps.elementMap[elemMapIndex + t].elementName;
                uint16_t elementOffset = CAPIStaticMap->Maps.elementMap[elemMapIndex + t].elementOffset;
                uint16_t signalDataTypeIndex = CAPIStaticMap->Maps.elementMap[elemMapIndex + t].dataTypeIndex;
                const char* signal_data_type = CAPIStaticMap->Maps.dataTypeMap[signalDataTypeIndex].mwDataName;
                uint32_t signalDataSize = rtwCAPI_GetDataTypeSize(CAPIStaticMap->Maps.dataTypeMap, signalDataTypeIndex);
                void* signal_address = (void*)((uint64_t)dataEntryAddr + elementOffset);
                FSignal signal = { signalName,  signal_address , std::string(signal_data_type), signalDataSize };
                signals.insert(std::make_pair(signalName, signal));
            }
        }
        else 
        {
            FSignal signal = { dataEntryName, dataEntryAddr , std::string(data_type), dataSize };
            signals.insert(std::make_pair(dataEntryName, signal));
        }
        currentElsmentIdx++;
    }

#ifdef DEBUD_DLL_LOAD
    std::cout << id << std::endl;
    for (const auto& port : ports) 
    {
        std::cout << "\t" << port.first << std::endl;
    }
    for (const auto& signal : signals) 
    {
        std::cout << "\t\t" << signal.first << ":" << signal.second.dataType << std::endl;
    }
#endif
}


FSimulinkModel::~FSimulinkModel()
{
    Release();
}

void FSimulinkModel::Release()
{
    if (TerminatePtr && ModelDataPtr)
    {
        TerminatePtr(ModelDataPtr);
    }
#ifdef PLATFORM_WINDOWS
    if (hInstDLL != nullptr)
    {
        FreeLibrary(hInstDLL);
        hInstDLL = nullptr;
    }
#else
    if (hInstSO != nullptr)
    {
        dlclose(hInstSO);
        hInstSO = nullptr; // Set the handle to nullptr after unloading
    }
#endif
}

void* FSimulinkModel::GetFunc(const std::string& FuncName)
{
    void* Ptr = nullptr;
#ifdef PLATFORM_WINDOWS
    Ptr = GetProcAddress(hInstDLL, FuncName.c_str());
    if (Ptr == nullptr)
    {
        Release();
        throw std::runtime_error(std::string("FSimulinkModel::GetFunc(); Unable to get func ptr to ") + FuncName + "(), GetLastError()=" + std::to_string(GetLastError()));
    }
#else
    Ptr = dlsym(hInstSO, TCHAR_TO_UTF8(*FuncName));
    if (Ptr == nullptr)
    {
        Release();
        throw std::runtime_error(std::string("FSimulinkModel::GetFunc(); Unable to get func ptr to ") + FuncName + "(), dlerror()=" + dlerror());
    }
#endif
    return Ptr;
}

std::string FSimulinkModel::getSignalValue(std::string signalName)
{
    FSignal signal = GetSignal(signalName);
    return ""; //printSignalValue(signal.dataType, signal.dataAddress);
}


void FSimulinkModel::setSignalValue(std::string signalName, std::string value)
{
    FSignal signal = GetSignal(signalName);
    //acceptSignalValue(signal.dataType, value, signal.dataAddress);
}

void FSimulinkModel::Step(void)
{ 
    if (StepPtr && ModelDataPtr) 
    {
        StepPtr(ModelDataPtr);
    }  
}

FSimulinkModel::FSignal FSimulinkModel::GetSignal(const std::string& signalName)
{
    FSignal signal{};
    auto it = signals.find(signalName);
    if (it != signals.end()) 
    {
        signal =  it->second;
    }
	return signal;
}

std::vector<FSimulinkModel::FSignal> * FSimulinkModel::getPort(const std::string& portName)
{
    auto it = ports.find(portName);
    if (it != ports.end()) 
    {
        return &it->second;
    }
    return nullptr;
}

FSimulinkModel::SignalsMap FSimulinkModel::GetSignals() 
{
    return signals;
}

rtwCAPI_ModelMappingInfo* FSimulinkModel::getModelMappingInfo(void* InModelDataPtr)
{

    RTM_ModelDataStructure_T* addr = (RTM_ModelDataStructure_T*)InModelDataPtr;
    if (addr != nullptr)
    {
        return  &addr->DataMapInfo.mmi;
    }
    return nullptr;
}
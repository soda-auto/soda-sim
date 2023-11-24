// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "SodaDelegates.generated.h"

class UNREALSODA_API FSodaDelegates
{

public:
	DECLARE_MULTICAST_DELEGATE_TwoParams(FActorSelectionEvent, AActor* /*Actor*/, bool /*bIsSelected*/);
	DECLARE_MULTICAST_DELEGATE_OneParam(FActorRemovedEvent, AActor* /*Actor*/);
	DECLARE_MULTICAST_DELEGATE_ThreeParama(FActorRenamedEvent, AActor* /*Actor*/, const FString & /*PreviousName*/, const FString& /*NewName*/);


};
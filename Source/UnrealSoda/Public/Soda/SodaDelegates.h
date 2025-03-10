// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "CoreMinimal.h"
//#include "SodaDelegates.generated.h"

class FSodaDelegates
{

public:
	//DECLARE_MULTICAST_DELEGATE_TwoParams(FActorSelectionEvent, AActor* /*Actor*/, bool /*bIsSelected*/);
	//DECLARE_MULTICAST_DELEGATE_OneParam(FActorRemovedEvent, AActor* /*Actor*/);
	//DECLARE_MULTICAST_DELEGATE_ThreeParama(FActorRenamedEvent, AActor* /*Actor*/, const FString & /*PreviousName*/, const FString& /*NewName*/);

	static UNREALSODA_API FSimpleMulticastDelegate ActorsMapChenaged;
};
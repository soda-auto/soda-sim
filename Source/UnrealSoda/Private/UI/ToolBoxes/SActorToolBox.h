// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Soda/UI/SToolBox.h"
#include "Soda/UI/SActorList.h"

class USodaGameViewportClient;

namespace soda
{
class SSodaViewport;
class IDetailsView;

class  SActorToolBox : public SToolBox
{
public:
	SLATE_BEGIN_ARGS(SActorToolBox)
	{ }
	SLATE_ARGUMENT(TSharedPtr<SSodaViewport>, Viewport)
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs );

protected:
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual void OnPush() override;

	void OnSelectionChanged(TWeakObjectPtr<AActor> Actor, ESelectInfo::Type SelectInfo);
	void ClearSelection();

	TWeakObjectPtr<USodaGameViewportClient> ViewportClient;
	TWeakPtr<soda::SSodaViewport> Viewport;
	
	TSharedPtr<SActorList> ListView;
	TSharedPtr<soda::IDetailsView> DetailView;
	TSharedPtr<SBox> EmptyBox;
};

} // namespace soda
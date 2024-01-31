// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogEditCondition, Log, All);

class FProperty;

namespace soda
{

class IEditConditionContext
{
public:
	virtual FName GetContextName() const = 0;

	virtual TOptional<bool> GetBoolValue(const FString& PropertyName) const = 0;
	virtual TOptional<int64> GetIntegerValue(const FString& PropertyName) const = 0;
	virtual TOptional<double> GetNumericValue(const FString& PropertyName) const = 0;
	virtual TOptional<FString> GetEnumValue(const FString& PropertyName) const = 0;
	virtual TOptional<UObject*> GetPointerValue(const FString& PropertyName) const = 0;
	virtual TOptional<FString> GetTypeName(const FString& PropertyName) const = 0;
	virtual TOptional<int64> GetIntegerValueOfEnum(const FString& EnumType, const FString& EnumValue) const = 0;
};

class FPropertyNode;
class FComplexPropertyNode;
class FEditConditionExpression;

class FEditConditionContext : public IEditConditionContext
{
public:
	FEditConditionContext(FPropertyNode& InPropertyNode);
	virtual ~FEditConditionContext() {}

	virtual FName GetContextName() const override;

	virtual TOptional<bool> GetBoolValue(const FString& PropertyName) const override;
	virtual TOptional<int64> GetIntegerValue(const FString& PropertyName) const override;
	virtual TOptional<double> GetNumericValue(const FString& PropertyName) const override;
	virtual TOptional<FString> GetEnumValue(const FString& PropertyName) const override;
	virtual TOptional<UObject*> GetPointerValue(const FString& PropertyName) const override;
	virtual TOptional<FString> GetTypeName(const FString& PropertyName) const override;
	virtual TOptional<int64> GetIntegerValueOfEnum(const FString& EnumType, const FString& EnumValue) const override;

	/**
	 * Fetch the single boolean property referenced.
	 * Returns nullptr if more than one property is referenced.
	 */
	const FBoolProperty* GetSingleBoolProperty(const TSharedPtr<FEditConditionExpression>& Expression) const;

private:
	TWeakPtr<FPropertyNode> PropertyNode;
};

} // namespace soda

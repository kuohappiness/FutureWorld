#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "FWMVPVerifyCommandlet.generated.h"

UCLASS()
class FW_API UFWMVPVerifyCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UFWMVPVerifyCommandlet();

	virtual int32 Main(const FString& Params) override;

private:
	int32 IssueCount = 0;

	void VerifyDataAssets();
	void VerifyInputMappings();
	void VerifyCombatGarageMap();
	void VerifyClassDefaults();

	void Check(bool bCondition, const FString& Message);
	bool HasInputMapping(const TCHAR* MappingContextPath, const TCHAR* InputActionPath, FKey ExpectedKey) const;
};

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "FWMVPFullLoopVerifyCommandlet.generated.h"

UCLASS()
class FW_API UFWMVPFullLoopVerifyCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UFWMVPFullLoopVerifyCommandlet();

	virtual int32 Main(const FString& Params) override;

private:
	int32 IssueCount = 0;

	void Check(bool bCondition, const FString& Message);
	bool VerifyHeadlessFullLoop();
};

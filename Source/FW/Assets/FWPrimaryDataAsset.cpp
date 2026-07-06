#include "FWPrimaryDataAsset.h"

FPrimaryAssetId UFWPrimaryDataAsset::GetPrimaryAssetId() const
{
	const FName AssetType = GetClass()->GetFName();
	const FName AssetName = ContentId.IsNone() ? GetFName() : ContentId;
	return FPrimaryAssetId(AssetType, AssetName);
}

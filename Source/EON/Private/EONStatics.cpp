#include "EONStatics.h"
#include "EONTuningSubsystem.h"

FEONGameFeelRow UEONStatics::GetGameFeel(const UObject* WorldContext)
{
	if (const UEONTuningSubsystem* T = UEONTuningSubsystem::Get(WorldContext))
	{
		return T->GetGameFeel();
	}
	return FEONGameFeelRow();
}

// Copyright (c) Jared Taylor

#pragma once

#include "Modules/ModuleManager.h"

class FPlayMontageProModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

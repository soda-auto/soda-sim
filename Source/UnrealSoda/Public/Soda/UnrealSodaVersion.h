// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Runtime/Launch/Resources/Version.h"

#define UNREALSODA_MAJOR_VERSION	1
#define UNREALSODA_MINOR_VERSION	3
#define UNREALSODA_PATCH_VERSION	0

#if defined(UNREALSODA_PIPLINE_VERSION)
#define UNREALSODA_VERSION_STRING \
	VERSION_STRINGIFY(UNREALSODA_MAJOR_VERSION) \
	VERSION_TEXT(".") \
	VERSION_STRINGIFY(UNREALSODA_MINOR_VERSION) \
	VERSION_TEXT(".") \
	VERSION_STRINGIFY(UNREALSODA_PATCH_VERSION) \
	VERSION_TEXT("-") \
	VERSION_STRINGIFY(UNREALSODA_PIPLINE_VERSION)
#else
#define UNREALSODA_VERSION_STRING \
	VERSION_STRINGIFY(UNREALSODA_MAJOR_VERSION) \
	VERSION_TEXT(".") \
	VERSION_STRINGIFY(UNREALSODA_MINOR_VERSION) \
	VERSION_TEXT(".") \
	VERSION_STRINGIFY(UNREALSODA_PATCH_VERSION)
#endif

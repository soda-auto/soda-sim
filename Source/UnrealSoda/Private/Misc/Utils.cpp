// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Misc/Utils.h"


namespace soda
{

uint8 GetDataTypeSize(EDataType CVType)
{
	static const uint8 CV_2_SIZE[7] = { 1, 1, 2, 2, 4, 4, 8 };
	return CV_2_SIZE[uint8(CVType)];
}

} // namespace soda

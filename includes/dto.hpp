#pragma once

#include "net/common/dto_base.hpp"

/*
    DTO structs MUST inherit from DTO base object

    they MUST also specify an __attribute__((packed)),
    this insures that no padding will be generated between fields
    thus, insures that a client and server of 2 different host
    compiles both versions of the struct with the same size,
    otherwise data size mismatch can happen.    
*/

struct StatusDTO : DTO
{
	int value;
} __attribute__((packed));

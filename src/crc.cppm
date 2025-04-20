// SPDX-License-Identifier: MIT

module;

#define CRC_JUST_THE_INCLUDES
#include "crc/crc.hpp"
#undef CRC_JUST_THE_INCLUDES
#undef CRC_HPP_INCLUDED

export module crc;

#define CRC_EXPORT_SYMBOLS
#include "crc/crc.hpp"

#pragma once

namespace dsd::ductsystem {

enum class Material { GalvanizedSteel, Aluminum, StainlessSteel, CarbonSteel };
enum class PressureLevel { LowPressure, MediumPressure, HighPressure };
enum class SystemType { Supply, Return, Exhaust };
enum class TakeoffSide { Left, Right };
enum class ElbowOrientation { Horizontal, Downward };

} // namespace dsd::ductsystem

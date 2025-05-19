#include "color_interpretation.hpp"

namespace duckdb {

std::string ColorInterps::GetColorInterpretationName(const ColorInterp &color_interp) {
	switch (color_interp) {
	case Undefined:
		return "Undefined";
	case GrayIndex:
		return "Greyscale";
	case PaletteIndex:
		return "Paletted";
	case RedBand:
		return "Red";
	case GreenBand:
		return "Green";
	case BlueBand:
		return "Blue";
	case AlphaBand:
		return "Alpha";
	case HueBand:
		return "Hue";
	case SaturationBand:
		return "Saturation";
	case LightnessBand:
		return "Lightness";
	case CyanBand:
		return "Cyan";
	case MagentaBand:
		return "Magenta";
	case YellowBand:
		return "Yellow";
	case BlackBand:
		return "Black";
	case YCbCr_YBand:
		return "YLuminance";
	case YCbCr_CbBand:
		return "CbChroma";
	case YCbCr_CrBand:
		return "CrChroma";
	default:
		return "Unknown";
	}
}

} // namespace duckdb

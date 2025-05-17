#include "raster_scalar_functions.hpp"
#include "../raster.hpp"

// DuckDB
#include "duckdb/main/database.hpp"
#include "duckdb/main/extension_util.hpp"
// Spatial
#include "spatial/util/function_builder.hpp"
// GDAL
#include "gdal_priv.h"

namespace duckdb {

namespace {

//======================================================================================================================
// RT_Srid
//======================================================================================================================

struct RT_Srid {
	static void GetSrid(DataChunk &args, ExpressionState &state, Vector &result) {

		UnaryExecutor::Execute<uintptr_t, int32_t>(args.data[0], result, args.size(), [&](uintptr_t input) {
			Raster raster(reinterpret_cast<GDALDataset *>(input));
			return raster.GetSrid();
		});
	}

	static void Register(DatabaseInstance &db) {

		FunctionBuilder::RegisterScalar(db, "RT_Srid", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("raster", RasterTypes::RASTER());
				variant.SetReturnType(LogicalType::INTEGER);
				variant.SetFunction(GetSrid);
			});
			func.SetDescription(R"(
				Returns the spatial reference identifier (EPSG code) of the raster.
                Refer to [EPSG](https://spatialreference.org/ref/epsg/) for more details.
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "properties");
		});
	}
};

//======================================================================================================================
// RT_Properties (Width, Height, NumBands, UpperLeftX, UpperLeftXY...)
//======================================================================================================================

struct RT_Properties {
	static void GetWidth(DataChunk &args, ExpressionState &state, Vector &result) {

		UnaryExecutor::Execute<uintptr_t, int32_t>(args.data[0], result, args.size(), [&](uintptr_t input) {
			Raster raster(reinterpret_cast<GDALDataset *>(input));
			return raster.GetRasterXSize();
		});
	}

	static void GetHeight(DataChunk &args, ExpressionState &state, Vector &result) {

		UnaryExecutor::Execute<uintptr_t, int32_t>(args.data[0], result, args.size(), [&](uintptr_t input) {
			Raster raster(reinterpret_cast<GDALDataset *>(input));
			return raster.GetRasterYSize();
		});
	}

	static void GetNumBands(DataChunk &args, ExpressionState &state, Vector &result) {

		UnaryExecutor::Execute<uintptr_t, int32_t>(args.data[0], result, args.size(), [&](uintptr_t input) {
			Raster raster(reinterpret_cast<GDALDataset *>(input));
			return raster.GetRasterCount();
		});
	}

	static void GetTransformItem(DataChunk &args, ExpressionState &state, Vector &result, int32_t gt_index) {

		UnaryExecutor::Execute<uintptr_t, double>(args.data[0], result, args.size(), [&](uintptr_t input) {
			Raster raster(reinterpret_cast<GDALDataset *>(input));
			double gt[6] = {0};
			raster.GetGeoTransform(gt);
			return gt[gt_index];
		});
	}
	static void GetUpperLeftX(DataChunk &args, ExpressionState &state, Vector &result) {
		GetTransformItem(args, state, result, 0);
	}
	static void GetUpperLeftY(DataChunk &args, ExpressionState &state, Vector &result) {
		GetTransformItem(args, state, result, 3);
	}
	static void GetScaleX(DataChunk &args, ExpressionState &state, Vector &result) {
		GetTransformItem(args, state, result, 1);
	}
	static void GetScaleY(DataChunk &args, ExpressionState &state, Vector &result) {
		GetTransformItem(args, state, result, 5);
	}
	static void GetSkewX(DataChunk &args, ExpressionState &state, Vector &result) {
		GetTransformItem(args, state, result, 2);
	}
	static void GetSkewY(DataChunk &args, ExpressionState &state, Vector &result) {
		GetTransformItem(args, state, result, 4);
	}

	static void GetPixelWidth(DataChunk &args, ExpressionState &state, Vector &result) {

		UnaryExecutor::Execute<uintptr_t, double>(args.data[0], result, args.size(), [&](uintptr_t input) {
			Raster raster(reinterpret_cast<GDALDataset *>(input));
			double gt[6] = {0};
			raster.GetGeoTransform(gt);
			return sqrt(gt[1] * gt[1] + gt[4] * gt[4]);
		});
	}

	static void GetPixelHeight(DataChunk &args, ExpressionState &state, Vector &result) {

		UnaryExecutor::Execute<uintptr_t, double>(args.data[0], result, args.size(), [&](uintptr_t input) {
			Raster raster(reinterpret_cast<GDALDataset *>(input));
			double gt[6] = {0};
			raster.GetGeoTransform(gt);
			return sqrt(gt[5] * gt[5] + gt[2] * gt[2]);
		});
	}

	static void Register(DatabaseInstance &db) {

		FunctionBuilder::RegisterScalar(db, "RT_Width", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("raster", RasterTypes::RASTER());
				variant.SetReturnType(LogicalType::INTEGER);
				variant.SetFunction(GetWidth);
			});
			func.SetDescription(R"(
				Returns the width of the raster in pixels.
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "properties");
		});

		FunctionBuilder::RegisterScalar(db, "RT_Height", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("raster", RasterTypes::RASTER());
				variant.SetReturnType(LogicalType::INTEGER);
				variant.SetFunction(GetHeight);
			});
			func.SetDescription(R"(
				Returns the height of the raster in pixels.
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "properties");
		});

		FunctionBuilder::RegisterScalar(db, "RT_NumBands", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("raster", RasterTypes::RASTER());
				variant.SetReturnType(LogicalType::INTEGER);
				variant.SetFunction(GetNumBands);
			});
			func.SetDescription(R"(
				Returns the number of bands in the raster.
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "properties");
		});

		FunctionBuilder::RegisterScalar(db, "RT_UpperLeftX", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("raster", RasterTypes::RASTER());
				variant.SetReturnType(LogicalType::DOUBLE);
				variant.SetFunction(GetUpperLeftX);
			});
			func.SetDescription(R"(
				Returns the upper left X coordinate of raster in projected spatial reference.
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "properties");
		});

		FunctionBuilder::RegisterScalar(db, "RT_UpperLeftY", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("raster", RasterTypes::RASTER());
				variant.SetReturnType(LogicalType::DOUBLE);
				variant.SetFunction(GetUpperLeftY);
			});
			func.SetDescription(R"(
				Returns the upper left Y coordinate of raster in projected spatial reference.
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "properties");
		});

		FunctionBuilder::RegisterScalar(db, "RT_ScaleX", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("raster", RasterTypes::RASTER());
				variant.SetReturnType(LogicalType::DOUBLE);
				variant.SetFunction(GetScaleX);
			});
			func.SetDescription(R"(
				Returns the X component of the pixel width in units of coordinate reference system.
				Refer to [World File](https://en.wikipedia.org/wiki/World_file) for more details.
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "properties");
		});

		FunctionBuilder::RegisterScalar(db, "RT_ScaleY", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("raster", RasterTypes::RASTER());
				variant.SetReturnType(LogicalType::DOUBLE);
				variant.SetFunction(GetScaleY);
			});
			func.SetDescription(R"(
				Returns the Y component of the pixel width in units of coordinate reference system.
				Refer to [World File](https://en.wikipedia.org/wiki/World_file) for more details.
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "properties");
		});

		FunctionBuilder::RegisterScalar(db, "RT_SkewX", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("raster", RasterTypes::RASTER());
				variant.SetReturnType(LogicalType::DOUBLE);
				variant.SetFunction(GetSkewX);
			});
			func.SetDescription(R"(
				Returns the georeference X skew (or rotation parameter).
				Refer to [World File](https://en.wikipedia.org/wiki/World_file) for more details.
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "properties");
		});

		FunctionBuilder::RegisterScalar(db, "RT_SkewY", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("raster", RasterTypes::RASTER());
				variant.SetReturnType(LogicalType::DOUBLE);
				variant.SetFunction(GetSkewY);
			});
			func.SetDescription(R"(
				Returns the georeference Y skew (or rotation parameter).
				Refer to [World File](https://en.wikipedia.org/wiki/World_file) for more details.
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "properties");
		});

		FunctionBuilder::RegisterScalar(db, "RT_PixelWidth", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("raster", RasterTypes::RASTER());
				variant.SetReturnType(LogicalType::DOUBLE);
				variant.SetFunction(GetPixelWidth);
			});
			func.SetDescription(R"(
				Returns the width of a pixel in geometric units of the spatial reference system.
				In the common case where there is no skew, the pixel width is just the scale ratio between geometric coordinates and raster pixels.
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "properties");
		});

		FunctionBuilder::RegisterScalar(db, "RT_PixelHeight", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("raster", RasterTypes::RASTER());
				variant.SetReturnType(LogicalType::DOUBLE);
				variant.SetFunction(GetPixelHeight);
			});
			func.SetDescription(R"(
				Returns the height of a pixel in geometric units of the spatial reference system.
				In the common case where there is no skew, the pixel height is just the scale ratio between geometric coordinates and raster pixels.
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "properties");
		});
	}
};

} // namespace

// ######################################################################################################################
//  Register Scalar functions
// ######################################################################################################################

void RasterScalarFunctions::Register(DatabaseInstance &db) {
	RT_Srid::Register(db);
	RT_Properties::Register(db);
}

} // namespace duckdb

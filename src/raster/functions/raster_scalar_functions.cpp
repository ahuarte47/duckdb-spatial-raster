#include "raster_scalar_functions.hpp"
#include "raster.hpp"

// DuckDB
#include "duckdb/main/database.hpp"
#include "duckdb/main/extension_util.hpp"
#include "duckdb/common/vector_operations/generic_executor.hpp"
// Spatial
#include "spatial_r/spatial_types.hpp"
#include "spatial/geometry/geometry_serialization.hpp"
#include "spatial/geometry/sgl.hpp"
#include "spatial/util/function_builder.hpp"
// GDAL
#include "gdal_priv.h"
#include "modules/gdal/gdal_context_state.hpp"

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
			func.SetExample(R"(
				SELECT
					RT_Srid(raster) AS srid
				FROM
					RT_Read('./test/data/mosaic/SCL.tif-land-clip00.tiff')
				;
				┌───────┐
				│ srid  │
				│ int32 │
				├───────┤
				│ 32630 │
				└───────┘
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "property");
		});
	}
};

//======================================================================================================================
// RT_GEOMETRY
//======================================================================================================================

struct RT_Geometry {

	static void GetGeometry(DataChunk &args, ExpressionState &state, Vector &result) {

		UnaryExecutor::Execute<uintptr_t, string_t>(args.data[0], result, args.size(), [&](uintptr_t input) {
			Raster raster(reinterpret_cast<GDALDataset *>(input));
			Boundary2D boundary = raster.GetGeometry();

			// We can create the geometry polygon directly on the stack.
			double buffer[10];
			double *buffer_p = buffer;
			for (const auto &point : boundary.points) {
				*buffer_p++ = point.x;
				*buffer_p++ = point.y;
			}

			sgl::geometry ring(sgl::geometry_type::LINESTRING, false, false);
			ring.set_vertex_data(reinterpret_cast<const char *>(buffer), 5);
			sgl::geometry polygon(sgl::geometry_type::POLYGON, false, false);
			polygon.append_part(&ring);

			// Serialize the geometry into a blob
			const auto size = Serde::GetRequiredSize(polygon);
			auto blob = StringVector::EmptyString(result, size);
			Serde::Serialize(polygon, blob.GetDataWriteable(), size);
			blob.Finalize();
			return blob;
		});
	}

	static void GetBBox(DataChunk &args, ExpressionState &state, Vector &result) {

		UnaryExecutor::Execute<uintptr_t, string_t>(args.data[0], result, args.size(), [&](uintptr_t input) {
			Raster raster(reinterpret_cast<GDALDataset *>(input));
			BBox2D bbox = raster.GetBoundingBox();

			// We can create the geometry polygon directly on the stack.
			double buffer[10];
			buffer[0] = bbox.min.x;
			buffer[1] = bbox.min.y;
			buffer[2] = bbox.min.x;
			buffer[3] = bbox.max.y;
			buffer[4] = bbox.max.x;
			buffer[5] = bbox.max.y;
			buffer[6] = bbox.max.x;
			buffer[7] = bbox.min.y;
			buffer[8] = bbox.min.x;
			buffer[9] = bbox.min.y;

			sgl::geometry ring(sgl::geometry_type::LINESTRING, false, false);
			ring.set_vertex_data(reinterpret_cast<const char *>(buffer), 5);
			sgl::geometry polygon(sgl::geometry_type::POLYGON, false, false);
			polygon.append_part(&ring);

			// Serialize the geometry into a blob
			const auto size = Serde::GetRequiredSize(polygon);
			auto blob = StringVector::EmptyString(result, size);
			Serde::Serialize(polygon, blob.GetDataWriteable(), size);
			blob.Finalize();
			return blob;
		});
	}

	static void Register(DatabaseInstance &db) {

		FunctionBuilder::RegisterScalar(db, "RT_GetGeometry", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("raster", RasterTypes::RASTER());
				variant.SetReturnType(GeoTypes::GEOMETRY());
				variant.SetFunction(GetGeometry);
			});
			func.SetDescription(R"(
				Returns the polygon representation of the extent of the raster.
			)");
			func.SetExample(R"(
				SELECT
					RT_GetGeometry(raster) AS g
				FROM
					RT_Read('./test/data/mosaic/SCL.tif-land-clip00.tiff')
				;
				┌────────────────────────────────────────────────────────────────────────────────────────────┐
				│                                             g                                              │
				│                                          geometry                                          │
				├────────────────────────────────────────────────────────────────────────────────────────────┤
				│ POLYGON ((541020 4690200, 541020 4796640, 609780 4796640, 609780 4690200, 541020 4690200)) │
				└────────────────────────────────────────────────────────────────────────────────────────────┘
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "property");
		});

		FunctionBuilder::RegisterScalar(db, "RT_GetBBox", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("raster", RasterTypes::RASTER());
				variant.SetReturnType(GeoTypes::GEOMETRY());
				variant.SetFunction(GetBBox);
			});
			func.SetDescription(R"(
				Returns the minimum bounding box of the raster.
			)");
			func.SetExample(R"(
				SELECT
					RT_GetBBox(raster) AS bbox
				FROM
					RT_Read('./test/data/mosaic/SCL.tif-land-clip00.tiff')
				;
				┌────────────────────────────────────────────────────────────────────────────────────────────┐
				│                                            bbox                                            │
				│                                          geometry                                          │
				├────────────────────────────────────────────────────────────────────────────────────────────┤
				│ POLYGON ((541020 4690200, 541020 4796640, 609780 4796640, 609780 4690200, 541020 4690200)) │
				└────────────────────────────────────────────────────────────────────────────────────────────┘
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "property");
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
			func.SetExample(R"(
				SELECT
					RT_Width(raster) AS cols
				FROM
					RT_Read('./test/data/mosaic/SCL.tif-land-clip00.tiff')
				;
				┌───────┐
				│ cols  │
				│ int32 │
				├───────┤
				│ 3438  │
				└───────┘
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "property");
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
			func.SetExample(R"(
				SELECT
					RT_Height(raster) AS rows
				FROM
					RT_Read('./test/data/mosaic/SCL.tif-land-clip00.tiff')
				;
				┌───────┐
				│ rows  │
				│ int32 │
				├───────┤
				│ 5322  │
				└───────┘
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "property");
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
			func.SetExample(R"(
				SELECT
					RT_NumBands(raster) AS num_bands
				FROM
					RT_Read('./test/data/mosaic/SCL.tif-land-clip00.tiff')
				;
				┌───────────┐
				│ num_bands │
				│   int32   │
				├───────────┤
				│     1     │
				└───────────┘
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "property");
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
			func.SetExample(R"(
				SELECT
					RT_UpperLeftX(raster) AS ulx
				FROM
					RT_Read('./test/data/mosaic/SCL.tif-land-clip00.tiff')
				;
				┌──────────┐
				│   ulx    │
				│  double  │
				├──────────┤
				│ 541020.0 │
				└──────────┘
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "property");
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
			func.SetExample(R"(
				SELECT
					RT_UpperLeftY(raster) AS uly
				FROM
					RT_Read('./test/data/mosaic/SCL.tif-land-clip00.tiff')
				;
				┌────────────────┐
				│      uly       │
				│     double     │
				├────────────────┤
				│   4796640.0    │
				│ (4.80 million) │
				└────────────────┘
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "property");
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
			func.SetExample(R"(
				SELECT
					RT_ScaleX(raster) AS scale_x
				FROM
					RT_Read('./test/data/mosaic/SCL.tif-land-clip00.tiff')
				;
				┌─────────┐
				│ scale_x │
				│ double  │
				├─────────┤
				│  20.0   │
				└─────────┘
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "property");
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
			func.SetExample(R"(
				SELECT
					RT_ScaleY(raster) AS scale_y
				FROM
					RT_Read('./test/data/mosaic/SCL.tif-land-clip00.tiff')
				;
				┌─────────┐
				│ scale_y │
				│ double  │
				├─────────┤
				│  -20.0  │
				└─────────┘
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "property");
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
			func.SetExample(R"(
				SELECT
					RT_SkewX(raster) AS skew_x
				FROM
					RT_Read('./test/data/mosaic/SCL.tif-land-clip00.tiff')
				;
				┌────────┐
				│ skew_x │
				│ double │
				├────────┤
				│  0.0   │
				└────────┘
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "property");
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
			func.SetExample(R"(
				SELECT
					RT_SkewY(raster) AS skew_y
				FROM
					RT_Read('./test/data/mosaic/SCL.tif-land-clip00.tiff')
				;
				┌────────┐
				│ skew_y │
				│ double │
				├────────┤
				│  0.0   │
				└────────┘
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "property");
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
			func.SetExample(R"(
				SELECT
					RT_PixelWidth(raster) AS px_width
				FROM
					RT_Read('./test/data/mosaic/SCL.tif-land-clip00.tiff')
				;
				┌──────────┐
				│ px_width │
				│  double  │
				├──────────┤
				│   20.0   │
				└──────────┘
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "property");
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
			func.SetExample(R"(
				SELECT
					RT_PixelHeight(raster) AS px_height
				FROM
					RT_Read('./test/data/mosaic/SCL.tif-land-clip00.tiff')
				;
				┌───────────┐
				│ px_height │
				│  double   │
				├───────────┤
				│   20.0    │
				└───────────┘
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "property");
		});
	}
};

//======================================================================================================================
// RT_RasterToWorldCoord[XY]
//======================================================================================================================

struct RT_RasterToWorldCoord {

	static void RasterToWorldCoord(DataChunk &args, ExpressionState &state, Vector &result) {
		D_ASSERT(args.data.size() == 3);

		using POINTER_TYPE = PrimitiveType<uintptr_t>;
		using INT_TYPE = PrimitiveType<int32_t>;
		using POINT_TYPE = StructTypeBinary<double, double>;

		auto &p1 = args.data[0];
		auto &p2 = args.data[1];
		auto &p3 = args.data[2];

		GenericExecutor::ExecuteTernary<POINTER_TYPE, INT_TYPE, INT_TYPE, POINT_TYPE>(
		    p1, p2, p3, result, args.size(), [&](POINTER_TYPE p1, INT_TYPE p2, INT_TYPE p3) {
			    auto input = p1.val;
			    auto col = p2.val;
			    auto row = p3.val;
			    Raster raster(reinterpret_cast<GDALDataset *>(input));

			    Point2D coord(0, 0);
			    if (!raster.RasterToWorldCoord(coord, col, row)) {
				    throw InternalException("Could not compute geotransform matrix");
			    }
			    return POINT_TYPE {coord.x, coord.y};
		    });
	}

	static void RasterToWorldCoordX(DataChunk &args, ExpressionState &state, Vector &result) {
		D_ASSERT(args.data.size() == 3);

		TernaryExecutor::Execute<uintptr_t, int32_t, int32_t, double>(
		    args.data[0], args.data[1], args.data[2], result, args.size(),
		    [&](uintptr_t input, int32_t col, int32_t row) {
			    Raster raster(reinterpret_cast<GDALDataset *>(input));

			    Point2D coord(0, 0);
			    if (!raster.RasterToWorldCoord(coord, col, row)) {
				    throw InternalException("Could not compute geotransform matrix");
			    }
			    return coord.x;
		    });
	}

	static void RasterToWorldCoordY(DataChunk &args, ExpressionState &state, Vector &result) {
		D_ASSERT(args.data.size() == 3);

		TernaryExecutor::Execute<uintptr_t, int32_t, int32_t, double>(
		    args.data[0], args.data[1], args.data[2], result, args.size(),
		    [&](uintptr_t input, int32_t col, int32_t row) {
			    Raster raster(reinterpret_cast<GDALDataset *>(input));

			    Point2D coord(0, 0);
			    if (!raster.RasterToWorldCoord(coord, col, row)) {
				    throw InternalException("Could not compute geotransform matrix");
			    }
			    return coord.y;
		    });
	}

	static void Register(DatabaseInstance &db) {

		FunctionBuilder::RegisterScalar(db, "RT_RasterToWorldCoord", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("raster", RasterTypes::RASTER());
				variant.AddParameter("col", LogicalType::INTEGER);
				variant.AddParameter("row", LogicalType::INTEGER);
				variant.SetReturnType(GeoTypes::POINT_2D());
				variant.SetFunction(RasterToWorldCoord);
			});
			func.SetDescription(R"(
				Returns the upper left corner as geometric X and Y (longitude and latitude) given a column and row.
				Returned X and Y are in geometric units of the georeferenced raster.
			)");
			func.SetExample(R"(
				SELECT
					RT_RasterToWorldCoord(raster, 0, 0) AS coord
				FROM
					RT_Read('./test/data/mosaic/SCL.tif-land-clip00.tiff')
				;
				┌────────────────────────┐
				│         coord          │
				│        point_2d        │
				├────────────────────────┤
				│ POINT (541020 4796640) │
				└────────────────────────┘
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "property");
		});

		FunctionBuilder::RegisterScalar(db, "RT_RasterToWorldCoordX", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("raster", RasterTypes::RASTER());
				variant.AddParameter("col", LogicalType::INTEGER);
				variant.AddParameter("row", LogicalType::INTEGER);
				variant.SetReturnType(LogicalType::DOUBLE);
				variant.SetFunction(RasterToWorldCoordX);
			});
			func.SetDescription(R"(
				Returns the upper left X coordinate of a raster column row in geometric units of the georeferenced raster.
				Returned X is in geometric units of the georeferenced raster.
			)");
			func.SetExample(R"(
				SELECT
					RT_RasterToWorldCoordX(raster, 0, 0) AS coord_x
				FROM
					RT_Read('./test/data/mosaic/SCL.tif-land-clip00.tiff')
				;
				┌──────────┐
				│ coord_x  │
				│  double  │
				├──────────┤
				│ 541020.0 │
				└──────────┘
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "property");
		});

		FunctionBuilder::RegisterScalar(db, "RT_RasterToWorldCoordY", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("raster", RasterTypes::RASTER());
				variant.AddParameter("col", LogicalType::INTEGER);
				variant.AddParameter("row", LogicalType::INTEGER);
				variant.SetReturnType(LogicalType::DOUBLE);
				variant.SetFunction(RasterToWorldCoordY);
			});
			func.SetDescription(R"(
				Returns the upper left Y coordinate of a raster column row in geometric units of the georeferenced raster.
				Returned Y is in geometric units of the georeferenced raster.
			)");
			func.SetExample(R"(
				SELECT
					RT_RasterToWorldCoordY(raster, 0, 0) AS coord_y
				FROM
					RT_Read('./test/data/mosaic/SCL.tif-land-clip00.tiff')
				;
				┌────────────────┐
				│    coord_y     │
				│     double     │
				├────────────────┤
				│   4796640.0    │
				│ (4.80 million) │
				└────────────────┘
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "property");
		});
	}
};

//======================================================================================================================
// RT_WorldToRasterCoord[XY]
//======================================================================================================================

struct RT_WorldToRasterCoord {

	static void WorldToRasterCoord(DataChunk &args, ExpressionState &state, Vector &result) {
		D_ASSERT(args.data.size() == 3);

		using POINTER_TYPE = PrimitiveType<uintptr_t>;
		using DOUBLE_TYPE = PrimitiveType<double_t>;
		using COORD_TYPE = StructTypeBinary<int32_t, int32_t>;

		auto &p1 = args.data[0];
		auto &p2 = args.data[1];
		auto &p3 = args.data[2];

		GenericExecutor::ExecuteTernary<POINTER_TYPE, DOUBLE_TYPE, DOUBLE_TYPE, COORD_TYPE>(
		    p1, p2, p3, result, args.size(), [&](POINTER_TYPE p1, DOUBLE_TYPE p2, DOUBLE_TYPE p3) {
			    auto input = p1.val;
			    auto x = p2.val;
			    auto y = p3.val;
			    Raster raster(reinterpret_cast<GDALDataset *>(input));

			    RasterCoord coord(0, 0);
			    if (!raster.WorldToRasterCoord(coord, x, y)) {
				    throw InternalException("Could not compute inverse geotransform matrix");
			    }
			    return COORD_TYPE {coord.col, coord.row};
		    });
	}

	static void WorldToRasterCoordX(DataChunk &args, ExpressionState &state, Vector &result) {
		D_ASSERT(args.data.size() == 3);

		TernaryExecutor::Execute<uintptr_t, double_t, double_t, int32_t>(
		    args.data[0], args.data[1], args.data[2], result, args.size(),
		    [&](uintptr_t input, double_t x, double_t y) {
			    Raster raster(reinterpret_cast<GDALDataset *>(input));

			    RasterCoord coord(0, 0);
			    if (!raster.WorldToRasterCoord(coord, x, y)) {
				    throw InternalException("Could not compute inverse geotransform matrix");
			    }
			    return coord.col;
		    });
	}

	static void WorldToRasterCoordY(DataChunk &args, ExpressionState &state, Vector &result) {
		D_ASSERT(args.data.size() == 3);

		TernaryExecutor::Execute<uintptr_t, double_t, double_t, int32_t>(
		    args.data[0], args.data[1], args.data[2], result, args.size(),
		    [&](uintptr_t input, double_t x, double_t y) {
			    Raster raster(reinterpret_cast<GDALDataset *>(input));

			    RasterCoord coord(0, 0);
			    if (!raster.WorldToRasterCoord(coord, x, y)) {
				    throw InternalException("Could not compute inverse geotransform matrix");
			    }
			    return coord.row;
		    });
	}

	static void Register(DatabaseInstance &db) {

		FunctionBuilder::RegisterScalar(db, "RT_WorldToRasterCoord", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("raster", RasterTypes::RASTER());
				variant.AddParameter("x", LogicalType::DOUBLE);
				variant.AddParameter("y", LogicalType::DOUBLE);
				variant.SetReturnType(RasterTypes::RASTER_COORD());
				variant.SetFunction(WorldToRasterCoord);
			});
			func.SetDescription(R"(
				Returns the upper left corner as column and row given geometric X and Y (longitude and latitude).
				Geometric X and Y must be expressed in the spatial reference coordinate system of the raster.
			)");
			func.SetExample(R"(
				SELECT
					RT_WorldToRasterCoord(raster, RT_RasterToWorldCoordX(raster, 1, 2), RT_RasterToWorldCoordY(raster, 1, 2)) AS coord
				FROM
					RT_Read('./test/data/mosaic/SCL.tif-land-clip00.tiff')
				;
				┌──────────────┐
				│    coord     │
				│ raster_coord │
				├──────────────┤
				│ COORD (1, 2) │
				└──────────────┘
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "property");
		});

		FunctionBuilder::RegisterScalar(db, "RT_WorldToRasterCoordX", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("raster", RasterTypes::RASTER());
				variant.AddParameter("x", LogicalType::DOUBLE);
				variant.AddParameter("y", LogicalType::DOUBLE);
				variant.SetReturnType(LogicalType::INTEGER);
				variant.SetFunction(WorldToRasterCoordX);
			});
			func.SetDescription(R"(
				Returns the column in the raster given geometric X and Y (longitude and latitude).
				Geometric X and Y must be expressed in the spatial reference coordinate system of the raster.
			)");
			func.SetExample(R"(
				SELECT
					RT_WorldToRasterCoordX(raster, RT_RasterToWorldCoordX(raster, 1, 2), RT_RasterToWorldCoordY(raster, 1, 2)) AS col
				FROM
					RT_Read('./test/data/mosaic/SCL.tif-land-clip00.tiff')
				;
				┌───────┐
				│  col  │
				│ int32 │
				├───────┤
				│   1   │
				└───────┘
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "property");
		});

		FunctionBuilder::RegisterScalar(db, "RT_WorldToRasterCoordY", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("raster", RasterTypes::RASTER());
				variant.AddParameter("x", LogicalType::DOUBLE);
				variant.AddParameter("y", LogicalType::DOUBLE);
				variant.SetReturnType(LogicalType::INTEGER);
				variant.SetFunction(WorldToRasterCoordY);
			});
			func.SetDescription(R"(
				Returns the row in the raster given geometric X and Y (longitude and latitude).
				Geometric X and Y must be expressed in the spatial reference coordinate system of the raster.
			)");
			func.SetExample(R"(
				SELECT
					RT_WorldToRasterCoordY(raster, RT_RasterToWorldCoordX(raster, 1, 2), RT_RasterToWorldCoordY(raster, 1, 2)) AS row
				FROM
					RT_Read('./test/data/mosaic/SCL.tif-land-clip00.tiff')
				;
				┌───────┐
				│  row  │
				│ int32 │
				├───────┤
				│   2   │
				└───────┘
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "property");
		});
	}
};

//======================================================================================================================
// RT_Value
//======================================================================================================================

struct RT_Value {

	static void GetValue(DataChunk &args, ExpressionState &state, Vector &result) {
		D_ASSERT(args.data.size() == 4);

		using POINTER_TYPE = PrimitiveType<uintptr_t>;
		using INT_TYPE = PrimitiveType<int32_t>;
		using DOUBLE_TYPE = PrimitiveType<double>;

		auto &p1 = args.data[0];
		auto &p2 = args.data[1];
		auto &p3 = args.data[2];
		auto &p4 = args.data[3];

		GenericExecutor::ExecuteQuaternary<POINTER_TYPE, INT_TYPE, INT_TYPE, INT_TYPE, DOUBLE_TYPE>(
		    p1, p2, p3, p4, result, args.size(), [&](POINTER_TYPE p1, INT_TYPE p2, INT_TYPE p3, INT_TYPE p4) {
			    auto input = p1.val;
			    auto band_num = p2.val;
			    auto col = p3.val;
			    auto row = p4.val;

			    GDALDataset *dataset = reinterpret_cast<GDALDataset *>(input);
			    auto cols = dataset->GetRasterXSize();
			    auto rows = dataset->GetRasterYSize();

			    if (band_num < 1) {
				    throw InvalidInputException("BandNum must be greater than 0");
			    }
			    if (dataset->GetRasterCount() < band_num) {
				    throw InvalidInputException("Dataset only has %d RasterBands", dataset->GetRasterCount());
			    }
			    if (col < 0 || col >= cols || row < 0 || row >= rows) {
				    throw InvalidInputException(
				        "Attempting to get pixel value with out of range raster coordinates: (%d, %d)", col, row);
			    }

			    Raster raster(dataset);
			    double value;
			    if (raster.GetValue(value, band_num, col, row)) {
				    return value;
			    }
			    throw InternalException("Failed attempting to get pixel value with raster coordinates: (%d, %d)", col,
			                            row);
		    });
	}

	static void Register(DatabaseInstance &db) {

		FunctionBuilder::RegisterScalar(db, "RT_Value", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("raster", RasterTypes::RASTER());
				variant.AddParameter("band", LogicalType::INTEGER);
				variant.AddParameter("col", LogicalType::INTEGER);
				variant.AddParameter("row", LogicalType::INTEGER);
				variant.SetReturnType(LogicalType::DOUBLE);
				variant.SetFunction(GetValue);
			});
			func.SetDescription(R"(
				Returns the value of a given band in a given column, row pixel.
				Band numbers start at 1 and band is assumed to be 1 if not specified.
			)");
			func.SetExample(R"(
				SELECT
					RT_Value(raster, 1, (RT_Width(raster) / 2)::INT, (RT_Height(raster) / 2)::INT) AS valCC,
					RT_Value(raster, 1, 0, 0) AS val00,
					RT_Value(raster, 1, RT_Width(raster) - 1, 0) AS val10,
					RT_Value(raster, 1, RT_Width(raster) - 1, RT_Height(raster) - 1) AS val11,
					RT_Value(raster, 1, 0, RT_Height(raster) - 1) AS val01
				FROM
					RT_Read('./test/data/mosaic/SCL.tif-land-clip00.tiff')
				;
				┌────────┬─────────┬─────────┬────────┬─────────┐
				│ valCC  │  val00  │  val10  │ val11  │  val01  │
				│ double │ double  │ double  │ double │ double  │
				├────────┼─────────┼─────────┼────────┼─────────┤
				│  1.0   │ -9999.0 │ -9999.0 │  15.0  │ -9999.0 │
				└────────┴─────────┴─────────┴────────┴─────────┘
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "property");
		});
	}
};

//======================================================================================================================
// RT_RasterWarp
//======================================================================================================================

struct RT_RasterWarp {

	static void RasterWarp(DataChunk &args, ExpressionState &state, Vector &result) {
		auto &context = state.GetContext();
		auto &ctx_state = GDALClientContextState::GetOrCreate(context);

		using POINTER_TYPE = PrimitiveType<uintptr_t>;
		using LIST_TYPE = PrimitiveType<list_entry_t>;

		auto &p1 = args.data[0];
		auto &p2 = args.data[1];
		auto &p2_entry = ListVector::GetEntry(p2);

		GenericExecutor::ExecuteBinary<POINTER_TYPE, LIST_TYPE, POINTER_TYPE>(
		    p1, p2, result, args.size(), [&](POINTER_TYPE p1, LIST_TYPE p2_offlen) {
			    auto input = p1.val;
			    auto offlen = p2_offlen.val;

			    Raster raster(reinterpret_cast<GDALDataset *>(input));

			    if (raster.GetRasterCount() == 0) {
				    throw InvalidInputException("Input Raster has no RasterBands");
			    }

			    auto options = std::vector<std::string>();

			    for (idx_t i = offlen.offset; i < offlen.offset + offlen.length; i++) {
				    const auto &child_value = p2_entry.GetValue(i);
				    const auto option = child_value.ToString();
				    options.emplace_back(option);
			    }

			    GDALDataset *result = raster.Warp(options);

			    if (result == nullptr) {
				    auto error = Raster::GetLastErrorMsg();
				    throw IOException("Could not warp raster (" + error + ")");
			    }

			    ctx_state.GetDatasetRegistry().RegisterDataset(result);
			    return CastPointerToValue(result);
		    });
	}

	static void Register(DatabaseInstance &db) {

		FunctionBuilder::RegisterScalar(db, "RT_RasterWarp", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("raster", RasterTypes::RASTER());
				variant.AddParameter("options", LogicalType::LIST(LogicalType::VARCHAR));
				variant.SetReturnType(RasterTypes::RASTER());
				variant.SetFunction(RasterWarp);
			});
			func.SetDescription(R"(
				Performs mosaicing, reprojection and/or warping on a raster.
				`options` is optional, an array of parameters like [GDALWarp](https://gdal.org/programs/gdalwarp.html).
			)");
			func.SetExample(R"(
				WITH __input AS (
					SELECT
						raster
					FROM
						RT_Read('./test/data/mosaic/SCL.tif-land-clip00.tiff')
				),
				__warp AS (
					SELECT
						RT_RasterWarp(raster, options => ['-r', 'bilinear', '-tr', '40.0', '40.0']) AS warp
					FROM
						__input
				)
				SELECT
					RT_ScaleX(warp) AS scale_x,
					RT_ScaleY(warp) AS scale_y
				FROM
					__warp
				;
				┌─────────┬─────────┐
				│ scale_x │ scale_y │
				│ double  │ double  │
				├─────────┼─────────┤
				│  40.0   │  -40.0  │
				└─────────┴─────────┘
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "construction");
		});
	}
};

//======================================================================================================================
// RT_RasterClip
//======================================================================================================================

struct RT_RasterClip {

	static void RasterClip_01(DataChunk &args, ExpressionState &state, Vector &result) {
		auto &context = state.GetContext();
		auto &ctx_state = GDALClientContextState::GetOrCreate(context);

		using POINTER_TYPE = PrimitiveType<uintptr_t>;
		using GEOMETRY_TYPE = PrimitiveType<geometry_t>;

		auto &p1 = args.data[0];
		auto &p2 = args.data[1];

		GenericExecutor::ExecuteBinary<POINTER_TYPE, GEOMETRY_TYPE, POINTER_TYPE>(
		    p1, p2, result, args.size(), [&](POINTER_TYPE p1, GEOMETRY_TYPE p2) {
			    auto input = p1.val;
			    auto geometry = p2.val;

			    Raster raster(reinterpret_cast<GDALDataset *>(input));

			    if (raster.GetRasterCount() == 0) {
				    throw InvalidInputException("Input Raster has no RasterBands");
			    }

			    GDALDataset *result = raster.Clip(geometry);

			    if (result == nullptr) {
				    auto error = Raster::GetLastErrorMsg();
				    throw IOException("Could not clip raster (" + error + ")");
			    }

			    ctx_state.GetDatasetRegistry().RegisterDataset(result);
			    return CastPointerToValue(result);
		    });
	}

	static void RasterClip_02(DataChunk &args, ExpressionState &state, Vector &result) {
		auto &context = state.GetContext();
		auto &ctx_state = GDALClientContextState::GetOrCreate(context);

		using POINTER_TYPE = PrimitiveType<uintptr_t>;
		using GEOMETRY_TYPE = PrimitiveType<geometry_t>;
		using LIST_TYPE = PrimitiveType<list_entry_t>;

		auto &p1 = args.data[0];
		auto &p2 = args.data[1];
		auto &p3 = args.data[2];
		auto &p3_entry = ListVector::GetEntry(p3);

		GenericExecutor::ExecuteTernary<POINTER_TYPE, GEOMETRY_TYPE, LIST_TYPE, POINTER_TYPE>(
		    p1, p2, p3, result, args.size(), [&](POINTER_TYPE p1, GEOMETRY_TYPE p2, LIST_TYPE p3_offlen) {
			    auto input = p1.val;
			    auto geometry = p2.val;
			    auto offlen = p3_offlen.val;

			    Raster raster(reinterpret_cast<GDALDataset *>(input));

			    if (raster.GetRasterCount() == 0) {
				    throw InvalidInputException("Input Raster has no RasterBands");
			    }

			    auto options = std::vector<std::string>();

			    for (idx_t i = offlen.offset; i < offlen.offset + offlen.length; i++) {
				    const auto &child_value = p3_entry.GetValue(i);
				    const auto option = child_value.ToString();
				    options.emplace_back(option);
			    }

			    GDALDataset *result = raster.Clip(geometry, options);

			    if (result == nullptr) {
				    auto error = Raster::GetLastErrorMsg();
				    throw IOException("Could not clip raster (" + error + ")");
			    }

			    ctx_state.GetDatasetRegistry().RegisterDataset(result);
			    return CastPointerToValue(result);
		    });
	}

	static void Register(DatabaseInstance &db) {

		FunctionBuilder::RegisterScalar(db, "RT_RasterClip", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("raster", RasterTypes::RASTER());
				variant.AddParameter("geometry", GeoTypes::GEOMETRY());
				variant.SetReturnType(RasterTypes::RASTER());
				variant.SetFunction(RasterClip_01);
			});
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("raster", RasterTypes::RASTER());
				variant.AddParameter("geometry", GeoTypes::GEOMETRY());
				variant.AddParameter("options", LogicalType::LIST(LogicalType::VARCHAR));
				variant.SetReturnType(RasterTypes::RASTER());
				variant.SetFunction(RasterClip_02);
			});
			func.SetDescription(R"(
				Returns a raster that is clipped by the input geometry.
				`options` is optional, an array of parameters like [GDALWarp](https://gdal.org/programs/gdalwarp.html).
			)");
			func.SetExample(R"(
				WITH __input AS (
					SELECT
						1 AS mosaic_id,
						RT_RasterFromFile(file) AS raster
					FROM
						glob('./test/data/mosaic/*.tiff')
				),
				__mosaic AS (
					SELECT
						RT_RasterMosaic_Agg(raster, options => ['-r', 'bilinear']) AS mosaic
					FROM
						__input
					GROUP BY
						mosaic_id
				),
				__geometry AS (
					SELECT geom FROM ST_Read('./test/data/CATAST_Pol_Township-PNA.gpkg')
				),
				__clip AS (
					SELECT
						RT_RasterClip(mosaic,
									(SELECT geom FROM __geometry LIMIT 1),
									options =>
										[
											'-r', 'bilinear', '-crop_to_cutline', '-wo', 'CUTLINE_ALL_TOUCHED=TRUE'
										]
						) AS clip
					FROM
						__mosaic
				)
				SELECT
					ST_Area(RT_GetGeometry(clip)) AS result
				FROM
					__clip
				;
				┌───────────────────┐
				│      result       │
				│      double       │
				├───────────────────┤
				│ 44269454.49488351 │
				│  (44.27 million)  │
				└───────────────────┘
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "construction");
		});
	}
};

//======================================================================================================================
// RT_RasterSplit
//======================================================================================================================

struct RT_RasterSplit {

	static void RasterSplit(DataChunk &args, ExpressionState &state, Vector &result) {
		D_ASSERT(args.data.size() == 3);

		auto &context = state.GetContext();
		auto &ctx_state = GDALClientContextState::GetOrCreate(context);

		using POINTER_TYPE = PrimitiveType<uintptr_t>;
		using INT_TYPE = PrimitiveType<int32_t>;
		using LIST_TYPE = PrimitiveType<list_entry_t>;

		auto &p1 = args.data[0];
		auto &p2 = args.data[1];
		auto &p3 = args.data[2];

		GenericExecutor::ExecuteTernary<POINTER_TYPE, INT_TYPE, INT_TYPE, LIST_TYPE>(
		    p1, p2, p3, result, args.size(), [&](POINTER_TYPE p1, INT_TYPE p2, INT_TYPE p3) {
			    auto input = p1.val;
			    auto tile_size_x = p2.val;
			    auto tile_size_y = p3.val;

			    Raster raster(reinterpret_cast<GDALDataset *>(input));

			    if (raster.GetRasterCount() == 0) {
				    throw InvalidInputException("Input Raster has no RasterBands");
			    }

			    auto tiles = raster.Split(tile_size_x, tile_size_y);

			    // Create a list vector to hold the result
			    auto current_size = ListVector::GetListSize(result);
			    auto new_size = current_size + tiles.size();

			    if (ListVector::GetListCapacity(result) < new_size) {
				    ListVector::Reserve(result, new_size);
			    }

			    auto &child_entry = ListVector::GetEntry(result);
			    auto child_vals = FlatVector::GetData<uintptr_t>(child_entry);

			    for (idx_t i = 0; i < tiles.size(); i++) {
				    auto &tile = tiles[i];
				    child_vals[current_size + i] = CastPointerToValue(tile);
				    ctx_state.GetDatasetRegistry().RegisterDataset(tile);
			    }

			    ListVector::SetListSize(result, new_size);
			    tiles.clear();

			    return list_entry_t {current_size, new_size - current_size};
		    });
	}

	static void Register(DatabaseInstance &db) {

		FunctionBuilder::RegisterScalar(db, "RT_RasterSplit", [](ScalarFunctionBuilder &func) {
			func.AddVariant([](ScalarFunctionVariantBuilder &variant) {
				variant.AddParameter("raster", RasterTypes::RASTER());
				variant.AddParameter("tile_size_x", LogicalType::INTEGER);
				variant.AddParameter("tile_size_y", LogicalType::INTEGER);
				variant.SetReturnType(LogicalType::LIST(RasterTypes::RASTER()));
				variant.SetFunction(RasterSplit);
			});
			func.SetDescription(R"(
				Splits a raster into smaller tiles of specified size.
				`tile_size_x` and `tile_size_y` specify the size of each tile in pixels.
				The result is a list of rasters, each representing a tile of the original raster.
			)");
			func.SetExample(R"(
				WITH __input AS (
					SELECT
						UNNEST(RT_RasterSplit(raster, 2048, 2048)) AS raster
					FROM
						RT_Read('./test/data/mosaic/SCL.tif-land-clip00.tiff')
				)
				SELECT
					RT_Srid(raster) AS srid,
					RT_Width(raster) AS width,
					RT_Height(raster) AS height,
					RT_GetGeometry(raster)::TEXT AS g
				FROM
					__input
				;
				┌───────┬───────┬────────┬────────────────────────────────────────────────────────────────────────────────────────────┐
				│ srid  │ width │ height │                                             g                                              │
				│ int32 │ int32 │ int32  │                                          varchar                                           │
				├───────┼───────┼────────┼────────────────────────────────────────────────────────────────────────────────────────────┤
				│ 32630 │  2048 │   2048 │ POLYGON ((541020 4755680, 541020 4796640, 581980 4796640, 581980 4755680, 541020 4755680)) │
				│ 32630 │  1390 │   2048 │ POLYGON ((581980 4755680, 581980 4796640, 609780 4796640, 609780 4755680, 581980 4755680)) │
				│ 32630 │  2048 │   2048 │ POLYGON ((541020 4714720, 541020 4755680, 581980 4755680, 581980 4714720, 541020 4714720)) │
				│ 32630 │  1390 │   2048 │ POLYGON ((581980 4714720, 581980 4755680, 609780 4755680, 609780 4714720, 581980 4714720)) │
				│ 32630 │  2048 │   1226 │ POLYGON ((541020 4690200, 541020 4714720, 581980 4714720, 581980 4690200, 541020 4690200)) │
				│ 32630 │  1390 │   1226 │ POLYGON ((581980 4690200, 581980 4714720, 609780 4714720, 609780 4690200, 581980 4690200)) │
				└───────┴───────┴────────┴────────────────────────────────────────────────────────────────────────────────────────────┘
			)");
			func.SetTag("ext", "spatial_raster");
			func.SetTag("category", "construction");
		});
	}
};

} // namespace

// ######################################################################################################################
//  Register Scalar functions
// ######################################################################################################################

void RasterScalarFunctions::Register(DatabaseInstance &db) {
	RT_Srid::Register(db);
	RT_Geometry::Register(db);
	RT_Properties::Register(db);
	RT_RasterToWorldCoord::Register(db);
	RT_WorldToRasterCoord::Register(db);
	RT_Value::Register(db);
	RT_RasterWarp::Register(db);
	RT_RasterClip::Register(db);
	RT_RasterSplit::Register(db);
}

} // namespace duckdb

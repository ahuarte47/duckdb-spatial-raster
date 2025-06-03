#include "raster.hpp"

// DuckDB
#include "duckdb/common/types/uuid.hpp"
// Spatial
#include "spatial/geometry/wkb_writer.hpp"
// GDAL
#include "gdal_priv.h"
#include "gdal_utils.h"
#include "gdalwarper.h"
#include <float.h> /* for FLT_EPSILON */

namespace duckdb {

Raster::Raster(GDALDataset *dataset) : dataset_(dataset) {
}

Raster::~Raster() {
	dataset_ = nullptr;
}

int Raster::GetRasterXSize() const {
	return dataset_->GetRasterXSize();
}

int Raster::GetRasterYSize() const {
	return dataset_->GetRasterYSize();
}

int Raster::GetRasterCount() const {
	return dataset_->GetRasterCount();
}

int32_t Raster::GetSrid() const {

	int32_t srid = 0; // SRID_UNKNOWN

	const char *proj_def = dataset_->GetProjectionRef();
	if (proj_def) {
		OGRSpatialReference spatial_ref;
		spatial_ref.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);

		if (spatial_ref.importFromWkt(proj_def) == OGRERR_NONE && spatial_ref.AutoIdentifyEPSG() == OGRERR_NONE) {

			const char *code = spatial_ref.GetAuthorityCode(nullptr);
			if (code) {
				srid = atoi(code);
			}
		}
	}
	return srid;
}

static Point2D rasterToWorldVertex(double matrix[], int32_t col, int32_t row) {
	double xgeo = matrix[0] + matrix[1] * col + matrix[2] * row;
	double ygeo = matrix[3] + matrix[4] * col + matrix[5] * row;
	return Point2D {xgeo, ygeo};
}

BBox2D Raster::GetBoundingBox() const {
	auto cols = dataset_->GetRasterXSize();
	auto rows = dataset_->GetRasterYSize();

	double gt[6] = {0};
	GetGeoTransform(gt);

	Point2D vertex1 = rasterToWorldVertex(gt, 0, 0);
	Point2D vertex2 = rasterToWorldVertex(gt, cols, rows);
	double minx = std::min(vertex1.x, vertex2.x);
	double miny = std::min(vertex1.y, vertex2.y);
	double maxx = std::max(vertex1.x, vertex2.x);
	double maxy = std::max(vertex1.y, vertex2.y);

	return BBox2D(Point2D(minx, miny), Point2D(maxx, maxy));
}

Boundary2D Raster::GetGeometry() const {
	auto cols = dataset_->GetRasterXSize();
	auto rows = dataset_->GetRasterYSize();

	double gt[6] = {0};
	GetGeoTransform(gt);

	Point2D vertex1 = rasterToWorldVertex(gt, 0, rows);
	Point2D vertex2 = rasterToWorldVertex(gt, 0, 0);
	Point2D vertex3 = rasterToWorldVertex(gt, cols, 0);
	Point2D vertex4 = rasterToWorldVertex(gt, cols, rows);
	Point2D vertex5 = vertex1;

	std::array<Point2D, 5> points = {vertex1, vertex2, vertex3, vertex4, vertex5};
	return Boundary2D(points);
}

bool Raster::GetGeoTransform(double *matrix) const {

	if (dataset_->GetGeoTransform(matrix) != CE_None) {
		// Using default geotransform matrix (0, 1, 0, 0, 0, -1)
		matrix[0] = 0;
		matrix[1] = 1;
		matrix[2] = 0;
		matrix[3] = 0;
		matrix[4] = 0;
		matrix[5] = -1;
		return false;
	}
	return true;
}

bool Raster::GetInvGeoTransform(double *inv_matrix) const {
	double gt[6] = {0};
	GetGeoTransform(gt);

	if (!GDALInvGeoTransform(gt, inv_matrix)) {
		return false;
	}
	return true;
}

bool Raster::RasterToWorldCoord(Point2D &point, int32_t col, int32_t row) const {
	double gt[6] = {0};
	GetGeoTransform(gt);
	return Raster::RasterToWorldCoord(point, gt, col, row);
}

bool Raster::RasterToWorldCoord(Point2D &point, double matrix[], int32_t col, int32_t row) {
	point.x = matrix[0] + matrix[1] * col + matrix[2] * row;
	point.y = matrix[3] + matrix[4] * col + matrix[5] * row;
	return true;
}

bool Raster::WorldToRasterCoord(RasterCoord &coord, double x, double y) const {
	double inv_gt[6] = {0};

	if (GetInvGeoTransform(inv_gt)) {
		return Raster::WorldToRasterCoord(coord, inv_gt, x, y);
	}
	return false;
}

bool Raster::WorldToRasterCoord(RasterCoord &coord, double inv_matrix[], double x, double y) {
	double xr = 0, yr = 0;
	GDALApplyGeoTransform(inv_matrix, x, y, &xr, &yr);

	// Source:
	// https://github.com/postgis/postgis/blob/stable-3.4/raster/rt_core/rt_raster.c#L808
	double rnd = 0;

// Helper macro for symmetrical rounding
#define ROUND(x, y) (((x > 0.0) ? floor((x * pow(10, y) + 0.5)) : ceil((x * pow(10, y) - 0.5))) / pow(10, y))
// Helper macro for consistent floating point equality checks
#define FLT_EQ(x, y) ((x == y) || (isnan(x) && isnan(y)) || (fabs(x - y) <= FLT_EPSILON))

	rnd = ROUND(xr, 0);
	if (FLT_EQ(rnd, xr))
		xr = rnd;
	else
		xr = floor(xr);

	rnd = ROUND(yr, 0);
	if (FLT_EQ(rnd, yr))
		yr = rnd;
	else
		yr = floor(yr);

	coord.col = (int32_t)xr;
	coord.row = (int32_t)yr;
	return true;
}

bool Raster::GetValue(double &value, int32_t band_num, int32_t col, int32_t row) const {

	GDALRasterBand *raster_band = dataset_->GetRasterBand(band_num);
	double pixel_value = raster_band->GetNoDataValue();

	if (raster_band->RasterIO(GF_Read, col, row, 1, 1, &pixel_value, 1, 1, GDT_Float64, 0, 0) == CE_None) {
		value = pixel_value;
		return true;
	}
	return false;
}

GDALDataset *Raster::BuildVRT(const std::vector<GDALDataset *> &datasets, const std::vector<std::string> &options) {

	char **papszArgv = nullptr;

	for (auto it = options.begin(); it != options.end(); ++it) {
		papszArgv = CSLAddString(papszArgv, (*it).c_str());
	}

	CPLErrorReset();

	GDALBuildVRTOptions *psOptions = GDALBuildVRTOptionsNew(papszArgv, nullptr);
	CSLDestroy(papszArgv);

	auto result = GDALDatasetUniquePtr(GDALDataset::FromHandle(
	    GDALBuildVRT(nullptr, datasets.size(), (GDALDatasetH *)&datasets[0], nullptr, psOptions, nullptr)));

	GDALBuildVRTOptionsFree(psOptions);

	if (result.get() != nullptr) {
		result->FlushCache();
	}
	return result.release();
}

GDALDataset *Raster::Warp(const std::vector<std::string> &options) {

	GDALDatasetH hDataset = GDALDataset::ToHandle(dataset_);

	auto driver = GetGDALDriverManager()->GetDriverByName("MEM");
	if (!driver) {
		throw InvalidInputException("Unknown driver 'MEM'");
	}

	char **papszArgv = nullptr;
	papszArgv = CSLAddString(papszArgv, "-of");
	papszArgv = CSLAddString(papszArgv, "MEM");

	for (auto it = options.begin(); it != options.end(); ++it) {
		papszArgv = CSLAddString(papszArgv, (*it).c_str());
	}

	CPLErrorReset();

	GDALWarpAppOptions *psOptions = GDALWarpAppOptionsNew(papszArgv, nullptr);
	CSLDestroy(papszArgv);

	auto ds_name = UUID::ToString(UUID::GenerateRandomUUID());

	auto result = GDALDatasetUniquePtr(
	    GDALDataset::FromHandle(GDALWarp(ds_name.c_str(), nullptr, 1, &hDataset, psOptions, nullptr)));

	GDALWarpAppOptionsFree(psOptions);

	if (result.get() != nullptr) {
		result->FlushCache();
	}
	return result.release();
}

//! Transformer of Geometries to pixel/line coordinates
class CutlineTransformer : public OGRCoordinateTransformation {
public:
	void *hTransformArg = nullptr;

	explicit CutlineTransformer(void *hTransformArg) : hTransformArg(hTransformArg) {
	}
	virtual ~CutlineTransformer() {
		GDALDestroyTransformer(hTransformArg);
	}

	virtual const OGRSpatialReference *GetSourceCS() const override {
		return nullptr;
	}
	virtual const OGRSpatialReference *GetTargetCS() const override {
		return nullptr;
	}
	virtual OGRCoordinateTransformation *Clone() const override {
		return nullptr;
	}
	virtual OGRCoordinateTransformation *GetInverse() const override {
		return nullptr;
	}

	virtual int Transform(int nCount, double *x, double *y, double *z, double * /* t */, int *pabSuccess) override {
		return GDALGenImgProjTransform(hTransformArg, TRUE, nCount, x, y, z, pabSuccess);
	}
};

GDALDataset *Raster::Clip(const geometry_t &geometry, const std::vector<std::string> &options) {

	GDALDatasetH hDataset = GDALDataset::ToHandle(dataset_);

	auto driver = GetGDALDriverManager()->GetDriverByName("MEM");
	if (!driver) {
		throw InvalidInputException("Unknown driver 'MEM'");
	}

	char **papszArgv = nullptr;
	papszArgv = CSLAddString(papszArgv, "-of");
	papszArgv = CSLAddString(papszArgv, "MEM");

	for (auto it = options.begin(); it != options.end(); ++it) {
		papszArgv = CSLAddString(papszArgv, (*it).c_str());
	}

	// Add Bounds & Geometry in pixel/line coordinates to the options.
	if (geometry.GetType() == GeometryType::POLYGON || geometry.GetType() == GeometryType::MULTIPOLYGON) {

		OGRGeometryUniquePtr ogr_geom;

		OGRSpatialReference srs;
		srs.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
		const char *proj_ref = dataset_->GetProjectionRef();
		if (proj_ref) {
			srs.importFromWkt(&proj_ref, nullptr);
		}

		vector<data_t> buffer;
		WKBWriter::Write(geometry, buffer);

		OGRGeometry *ptr_geom = nullptr;
		if (OGRGeometryFactory::createFromWkb(buffer.data(), &srs, &ptr_geom, buffer.size(), wkbVariantIso) !=
		    OGRERR_NONE) {
			CSLDestroy(papszArgv);
			throw InvalidInputException("Input Geometry could not imported");
		} else {
			ogr_geom = OGRGeometryUniquePtr(ptr_geom);
		}

		OGREnvelope envelope;
		ogr_geom->getEnvelope(&envelope);

		CutlineTransformer transformer(GDALCreateGenImgProjTransformer2(hDataset, nullptr, nullptr));

		if (ogr_geom->transform(&transformer) != OGRERR_NONE) {
			CSLDestroy(papszArgv);
			throw InvalidInputException("Transform of geometry to pixel/line coordinates failed");
		}

		char *pszWkt = nullptr;
		if (ogr_geom->exportToWkt(&pszWkt) != OGRERR_NONE) {
			CSLDestroy(papszArgv);
			CPLFree(pszWkt);
			throw InvalidInputException("Input Geometry could not loaded");
		}
		std::string wkt_geom = pszWkt;
		CPLFree(pszWkt);

		std::string wkt_option = "CUTLINE=" + wkt_geom;
		papszArgv = CSLAddString(papszArgv, "-wo");
		papszArgv = CSLAddString(papszArgv, wkt_option.c_str());
		papszArgv = CSLAddString(papszArgv, "-te");
		papszArgv = CSLAddString(papszArgv, MathUtil::format_coord(envelope.MinX).c_str());
		papszArgv = CSLAddString(papszArgv, MathUtil::format_coord(envelope.MinY).c_str());
		papszArgv = CSLAddString(papszArgv, MathUtil::format_coord(envelope.MaxX).c_str());
		papszArgv = CSLAddString(papszArgv, MathUtil::format_coord(envelope.MaxY).c_str());
	}

	CPLErrorReset();

	GDALWarpAppOptions *psOptions = GDALWarpAppOptionsNew(papszArgv, nullptr);
	CSLDestroy(papszArgv);

	auto ds_name = UUID::ToString(UUID::GenerateRandomUUID());

	auto result = GDALDatasetUniquePtr(
	    GDALDataset::FromHandle(GDALWarp(ds_name.c_str(), nullptr, 1, &hDataset, psOptions, nullptr)));

	GDALWarpAppOptionsFree(psOptions);

	if (result.get() != nullptr) {
		result->FlushCache();
	}
	return result.release();
}

std::vector<GDALDataset *> Raster::Split(int32_t tile_size_x, int32_t tile_size_y, int32_t overlap_x,
                                         int32_t overlap_y) {

	auto driver = GetGDALDriverManager()->GetDriverByName("MEM");
	if (!driver) {
		throw InvalidInputException("Unknown driver 'MEM'");
	}

	if (tile_size_x <= 0 || tile_size_y <= 0) {
		throw InvalidInputException("Tile size must be greater than zero");
	}

	if (overlap_x < 0 || overlap_y < 0) {
		throw InvalidInputException("Overlap values must be non-negative");
	}

	int32_t cols = dataset_->GetRasterXSize();
	int32_t rows = dataset_->GetRasterYSize();
	int32_t band_count = dataset_->GetRasterCount();

	if (tile_size_x + 2 * overlap_x > cols || tile_size_y + 2 * overlap_y > rows) {
		throw InvalidInputException("Tile size with overlap must not exceed raster dimensions");
	}

	if (band_count <= 0) {
		throw InvalidInputException("Input Raster must have at least one band");
	}

	CPLErrorReset();

	GDALDataType data_type = dataset_->GetRasterBand(1)->GetRasterDataType();
	double gt[6] = {0, 1, 0, 0, 0, -1};
	dataset_->GetGeoTransform(gt);

	std::vector<GDALDatasetUniquePtr> tiles;

	// Create tiles based on the specified tile size and overlap
	for (int32_t y = 0; y < rows; y += tile_size_y) {
		for (int32_t x = 0; x < cols; x += tile_size_x) {

			// Compute the read window, including overlap, and clip to raster bounds
			int32_t x_off = std::max(0, x - overlap_x);
			int32_t y_off = std::max(0, y - overlap_y);
			int32_t x_size = std::min(tile_size_x + 2 * overlap_x, cols - x_off);
			int32_t y_size = std::min(tile_size_y + 2 * overlap_y, rows - y_off);

			auto tile = GDALDatasetUniquePtr(driver->Create("", x_size, y_size, band_count, data_type, nullptr));
			if (!tile) {
				throw InternalException("Failed to create in-memory tile dataset");
			}

			Point2D pos = rasterToWorldVertex(gt, x_off, y_off);
			double gt_tile[6] = {pos.x, gt[1], gt[2], pos.y, gt[4], gt[5]};

			tile->SetGeoTransform(gt_tile);
			tile->SetProjection(dataset_->GetProjectionRef());
			tile->SetMetadata(dataset_->GetMetadata());

			for (int b = 1; b <= band_count; ++b) {
				GDALRasterBand *source_band = dataset_->GetRasterBand(b);
				GDALRasterBand *target_band = tile->GetRasterBand(b);
				std::vector<uint8_t> buffer(x_size * y_size * GDALGetDataTypeSizeBytes(data_type));

				CPLErr err = source_band->RasterIO(GF_Read, x_off, y_off, x_size, y_size, buffer.data(), x_size, y_size,
				                                   data_type, 0, 0);
				if (err != CE_None) {
					throw InternalException("RasterIO read failed");
				}

				err = target_band->RasterIO(GF_Write, 0, 0, x_size, y_size, buffer.data(), x_size, y_size, data_type, 0,
				                            0);
				if (err != CE_None) {
					throw InternalException("RasterIO write failed");
				}
			}

			tiles.push_back(std::move(tile));
		}
	}

	std::vector<GDALDataset *> result;
	result.reserve(tiles.size());

	for (auto &tile : tiles) {
		result.push_back(tile.release());
	}
	tiles.clear();

	return result;
}

std::string Raster::GetLastErrorMsg() {
	return std::string(CPLGetLastErrorMsg());
}

} // namespace duckdb

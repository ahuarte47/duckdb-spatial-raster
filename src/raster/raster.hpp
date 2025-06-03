#pragma once

#include <string>
#include "raster_types.hpp"
#include "spatial_r/spatial_types.hpp"
#include "spatial/geometry/geometry_type.hpp"

class GDALDataset;

namespace duckdb {

//! A wrapper of a GDALDataset with useful methods to manage raster data.
//! Does not take ownership of the pointer.
class Raster {
public:
	//! Constructor
	Raster(GDALDataset *dataset);
	//! Destructor
	~Raster();

	//! Returns the pointer to the dataset managed
	GDALDataset *operator->() const noexcept {
		return dataset_;
	}
	//! Returns the pointer to the dataset managed
	GDALDataset *get() const noexcept {
		return dataset_;
	}

	//! Returns the raster width in pixels
	int GetRasterXSize() const;

	//! Returns the raster height in pixels
	int GetRasterYSize() const;

	//! Returns the number of raster bands
	int GetRasterCount() const;

	//! Returns the spatial reference identifier of the raster
	int32_t GetSrid() const;

	//! Returns the minimal-bounding-box containing the extent of the raster
	BBox2D GetBoundingBox() const;

	//! Returns the polygon representation of the extent of the raster
	Boundary2D GetGeometry() const;

	//! Gets the geometric transform matrix (double[6]) of the raster
	bool GetGeoTransform(double *matrix) const;

	//! Gets the inverse geometric transform matrix (double[6]) of the raster
	bool GetInvGeoTransform(double *inv_matrix) const;

	//! Returns the geometric X and Y (longitude and latitude) given a column and row
	bool RasterToWorldCoord(Point2D &point, int32_t col, int32_t row) const;

	//! Returns the upper left corner as column and row given geometric X and Y
	bool WorldToRasterCoord(RasterCoord &coord, double x, double y) const;

	//! Returns the value of a given band in a given col and row pixel
	bool GetValue(double &value, int32_t band_num, int32_t col, int32_t row) const;

	//! Builds a VRT from a list of Rasters
	static GDALDataset *BuildVRT(const std::vector<GDALDataset *> &datasets,
	                             const std::vector<std::string> &options = std::vector<std::string>());

	//! Performs mosaicing, reprojection and/or warping on a raster
	GDALDataset *Warp(const std::vector<std::string> &options = std::vector<std::string>());

	//! Returns a raster that is clipped by the input geometry
	GDALDataset *Clip(const geometry_t &geometry, const std::vector<std::string> &options = std::vector<std::string>());

	//! Splits a raster into tiles of the given size, with optional overlap
	std::vector<GDALDataset *> Split(int32_t tile_size_x, int32_t tile_size_y, int32_t overlap_x = 0,
	                                 int32_t overlap_y = 0);

public:
	//! Returns the geometric X and Y (longitude and latitude) given a column and row
	static bool RasterToWorldCoord(Point2D &point, double matrix[], int32_t col, int32_t row);

	//! Returns the upper left corner as column and row given geometric X and Y
	static bool WorldToRasterCoord(RasterCoord &coord, double inv_matrix[], double x, double y);

	//! Get the last error message.
	static std::string GetLastErrorMsg();

private:
	GDALDataset *dataset_;
};

} // namespace duckdb

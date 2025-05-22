#pragma once

#include <string>
#include "raster_types.hpp"
#include "../spatial/spatial_types.hpp"

class GDALDataset;

namespace duckdb {

//! A wrapper of a GDALDataset with useful methods to manage raster data.
//! Does not take ownership of the pointer.
class Raster {
public:
	//! Constructor
	Raster(GDALDataset *dataset);

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

	//! Performs mosaicing, reprojection and/or warping on a raster
	static GDALDataset *Warp(GDALDataset *dataset,
	                         const std::vector<std::string> &options = std::vector<std::string>());

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


#pragma once

#include <mutex>

class GDALDataset;

namespace duckdb {

//! A thread-safe wrapper for a GDALDataset.
//! This takes ownership of the pointer managed.
class GDALThreadSafeDataset {
public:
	//! Constructor
	GDALThreadSafeDataset(GDALDataset *dataset);
	//! Destructor
	~GDALThreadSafeDataset();

	//! Returns the pointer to the dataset managed
	inline GDALDataset *operator->() const noexcept {
		return dataset_;
	}

	//! Returns the pointer to the dataset managed
	inline GDALDataset *get() const noexcept {
		return dataset_;
	}

	//! Implicit conversion to GDALDataset*
	inline operator GDALDataset *() const noexcept {
		return dataset_;
	}

	//! Lock the mutex for thread-safe operations on the GDALDataset
	void AcquireMutex();
	//! Unlock the mutex after thread-safe operations on the GDALDataset
	void ReleaseMutex();

private:
	GDALDataset *dataset_;
	std::mutex lock_;
};

} // namespace duckdb

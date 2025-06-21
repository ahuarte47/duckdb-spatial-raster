#pragma once
#include "duckdb/function/aggregate_function.hpp"
#include <mutex>
#include <vector>

namespace duckdb {

class GDALThreadSafeDataset;

struct RasterAggState {

	void Destroy() {
	}
};

struct RasterAggBindData : public FunctionData {
	//! The client context for the function call
	ClientContext &context;
	//! The list of options for the function
	std::vector<std::string> options;
	//! The list of datasets that are being aggregated
	std::vector<GDALThreadSafeDataset *> datasets;
	//! Mutex to protect access to the datasets vector
	std::mutex lock;

	explicit RasterAggBindData(ClientContext &context, std::vector<std::string> options,
	                           std::vector<GDALThreadSafeDataset *> datasets = {})
	    : context(context), options(options), datasets(datasets) {
	}

	unique_ptr<FunctionData> Copy() const override {
		return make_uniq<RasterAggBindData>(context, options, datasets);
	}

	bool Equals(const FunctionData &other_p) const override {
		auto &other = other_p.Cast<RasterAggBindData>();
		return options == other.options && datasets == other.datasets;
	}

	static unique_ptr<FunctionData> BindRasterAggOperation(ClientContext &context, AggregateFunction &function,
	                                                       vector<unique_ptr<Expression>> &arguments);
};

struct RasterAggUnaryOperation {

	template <class STATE>
	static void Initialize(STATE &state) {
		new (&state) STATE();
	}

	template <class STATE>
	static void Destroy(STATE &state, AggregateInputData &) {
		state.~STATE();
	}

	template <class STATE, class OP>
	static void Combine(const STATE &, STATE &, AggregateInputData &) {
		// Nothing to do here, we don't combine data of states in this case
	}

	template <class INPUT_TYPE, class STATE, class OP>
	static void Operation(STATE &state, const INPUT_TYPE &input, AggregateUnaryInput &agg_input) {
		GDALThreadSafeDataset *dataset = reinterpret_cast<GDALThreadSafeDataset *>(input);

		auto &bind_data = agg_input.input.bind_data->template Cast<RasterAggBindData>();
		std::lock_guard<std::mutex> guard(bind_data.lock);
		bind_data.datasets.emplace_back(dataset);
	}

	template <class INPUT_TYPE, class STATE, class OP>
	static void ConstantOperation(STATE &state, const INPUT_TYPE &input, AggregateUnaryInput &agg_input, idx_t) {
		Operation<INPUT_TYPE, STATE, OP>(state, input, agg_input);
	}

	static bool IgnoreNull() {
		return true;
	}
};

struct RasterAggBinaryOperation {

	template <class STATE>
	static void Initialize(STATE &state) {
		new (&state) STATE();
	}

	template <class STATE>
	static void Destroy(STATE &state, AggregateInputData &) {
		state.~STATE();
	}

	template <class STATE, class OP>
	static void Combine(const STATE &, STATE &, AggregateInputData &) {
		// Nothing to do here, we don't combine data of states in this case
	}

	template <class INPUT_TYPE, class OPTS_TYPE, class STATE, class OP>
	static void Operation(STATE &state, const INPUT_TYPE &input, const OPTS_TYPE &opts,
	                      AggregateBinaryInput &agg_input) {
		GDALThreadSafeDataset *dataset = reinterpret_cast<GDALThreadSafeDataset *>(input);

		auto &bind_data = agg_input.input.bind_data->template Cast<RasterAggBindData>();
		std::lock_guard<std::mutex> guard(bind_data.lock);
		bind_data.datasets.emplace_back(dataset);
	}

	template <class INPUT_TYPE, class OPTS_TYPE, class STATE, class OP>
	static void ConstantOperation(STATE &state, const INPUT_TYPE &input, const OPTS_TYPE &opts,
	                              AggregateBinaryInput &agg_input, idx_t) {
		Operation<INPUT_TYPE, OPTS_TYPE, STATE, OP>(state, input, opts, agg_input);
	}

	static bool IgnoreNull() {
		return true;
	}
};

} // namespace duckdb

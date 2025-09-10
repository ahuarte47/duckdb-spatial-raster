#include "spatial_types.hpp"
#include "spatial_casts.hpp"

// DuckDB
#include "duckdb/main/database.hpp"
#include "duckdb/main/extension/extension_loader.hpp"
#include "duckdb/common/vector_operations/generic_executor.hpp"
// Spatial
#include "spatial/geometry/geometry_processor.hpp"
#include "spatial/geometry/geometry_type.hpp"
#include "spatial/geometry/wkb_writer.hpp"

namespace duckdb {

namespace {

//======================================================================================================================
// GeometryTextProcessor
// NOTE:
// 	This class is a clone of the one from the spatial extension. It would be great if it could be included directly
//	instead of being copied.
//======================================================================================================================

class GeometryTextProcessor final : GeometryProcessor<void, bool> {
private:
	string text;

public:
	void OnVertexData(const VertexData &data) {
		auto &dims = data.data;
		auto &strides = data.stride;
		auto count = data.count;

		if (HasZ() && HasM()) {
			for (uint32_t i = 0; i < count; i++) {
				auto x = Load<double>(dims[0] + i * strides[0]);
				auto y = Load<double>(dims[1] + i * strides[1]);
				auto z = Load<double>(dims[2] + i * strides[2]);
				auto m = Load<double>(dims[3] + i * strides[3]);
				text += MathUtil::format_coord(x, y, z, m);
				if (i < count - 1) {
					text += ", ";
				}
			}
		} else if (HasZ()) {
			for (uint32_t i = 0; i < count; i++) {
				auto x = Load<double>(dims[0] + i * strides[0]);
				auto y = Load<double>(dims[1] + i * strides[1]);
				auto zm = Load<double>(dims[2] + i * strides[2]);
				text += MathUtil::format_coord(x, y, zm);
				if (i < count - 1) {
					text += ", ";
				}
			}
		} else if (HasM()) {
			for (uint32_t i = 0; i < count; i++) {
				auto x = Load<double>(dims[0] + i * strides[0]);
				auto y = Load<double>(dims[1] + i * strides[1]);
				auto m = Load<double>(dims[3] + i * strides[3]);
				text += MathUtil::format_coord(x, y, m);
				if (i < count - 1) {
					text += ", ";
				}
			}
		} else {
			for (uint32_t i = 0; i < count; i++) {
				auto x = Load<double>(dims[0] + i * strides[0]);
				auto y = Load<double>(dims[1] + i * strides[1]);
				text += MathUtil::format_coord(x, y);

				if (i < count - 1) {
					text += ", ";
				}
			}
		}
	}

	void ProcessPoint(const VertexData &data, bool in_typed_collection) override {
		if (!in_typed_collection) {
			text += "POINT";
			if (HasZ() && HasM()) {
				text += " ZM";
			} else if (HasZ()) {
				text += " Z";
			} else if (HasM()) {
				text += " M";
			}
			text += " ";
		}

		if (data.count == 0) {
			text += "EMPTY";
		} else if (in_typed_collection) {
			OnVertexData(data);
		} else {
			text += "(";
			OnVertexData(data);
			text += ")";
		}
	}

	void ProcessLineString(const VertexData &data, bool in_typed_collection) override {
		if (!in_typed_collection) {
			text += "LINESTRING";
			if (HasZ() && HasM()) {
				text += " ZM";
			} else if (HasZ()) {
				text += " Z";
			} else if (HasM()) {
				text += " M";
			}
			text += " ";
		}

		if (data.count == 0) {
			text += "EMPTY";
		} else {
			text += "(";
			OnVertexData(data);
			text += ")";
		}
	}

	void ProcessPolygon(PolygonState &state, bool in_typed_collection) override {
		if (!in_typed_collection) {
			text += "POLYGON";
			if (HasZ() && HasM()) {
				text += " ZM";
			} else if (HasZ()) {
				text += " Z";
			} else if (HasM()) {
				text += " M";
			}
			text += " ";
		}

		if (state.RingCount() == 0) {
			text += "EMPTY";
		} else {
			text += "(";
			bool first = true;
			while (!state.IsDone()) {
				if (!first) {
					text += ", ";
				}
				first = false;
				text += "(";
				auto vertices = state.Next();
				OnVertexData(vertices);
				text += ")";
			}
			text += ")";
		}
	}

	void ProcessCollection(CollectionState &state, bool) override {
		bool collection_is_typed = false;
		switch (CurrentType()) {
		case GeometryType::MULTIPOINT:
			text += "MULTIPOINT";
			collection_is_typed = true;
			break;
		case GeometryType::MULTILINESTRING:
			text += "MULTILINESTRING";
			collection_is_typed = true;
			break;
		case GeometryType::MULTIPOLYGON:
			text += "MULTIPOLYGON";
			collection_is_typed = true;
			break;
		case GeometryType::GEOMETRYCOLLECTION:
			text += "GEOMETRYCOLLECTION";
			collection_is_typed = false;
			break;
		default:
			throw InvalidInputException("Invalid geometry type");
		}

		if (HasZ() && HasM()) {
			text += " ZM";
		} else if (HasZ()) {
			text += " Z";
		} else if (HasM()) {
			text += " M";
		}

		if (state.ItemCount() == 0) {
			text += " EMPTY";
		} else {
			text += " (";
			bool first = true;
			while (!state.IsDone()) {
				if (!first) {
					text += ", ";
				}
				first = false;
				state.Next(collection_is_typed);
			}
			text += ")";
		}
	}

	virtual ~GeometryTextProcessor() = default;

	const string &Execute(const geometry_t &geom) {
		text.clear();
		Process(geom, false);
		return text;
	}
};

//======================================================================================================================
// Geometry Casts
//======================================================================================================================

struct GeometryCasts {

	//------------------------------------------------------------------------------------------------------------------
	// POINT_2D -> VARCHAR
	//------------------------------------------------------------------------------------------------------------------

	static bool PointToVarcharCast(Vector &source, Vector &result, idx_t count, CastParameters &parameters) {

		using POINT_TYPE = StructTypeBinary<double_t, double_t>;
		using VARCHAR_TYPE = PrimitiveType<string_t>;

		GenericExecutor::ExecuteUnary<POINT_TYPE, VARCHAR_TYPE>(source, result, count, [&](POINT_TYPE &input) {
			auto x = input.a_val;
			auto y = input.b_val;

			if (std::isnan(x) || std::isnan(y)) {
				return StringVector::AddString(result, "POINT EMPTY");
			}
			return StringVector::AddString(result, StringUtil::Format("POINT (%s)", MathUtil::format_coord(x, y)));
		});
		return true;
	}

	//------------------------------------------------------------------------------------------------------------------
	// GEOMETRY -> VARCHAR
	//------------------------------------------------------------------------------------------------------------------

	static bool GeometryToVarcharCast(Vector &source, Vector &result, idx_t count, CastParameters &parameters) {

		GeometryTextProcessor processor;

		UnaryExecutor::Execute<geometry_t, string_t>(source, result, count, [&](const geometry_t &input) {
			const auto text = processor.Execute(input);
			return StringVector::AddString(result, text);
		});
		return true;
	}

	//------------------------------------------------------------------------------------------------------------------
	// GEOMETRY -> WKB_BLOB
	//------------------------------------------------------------------------------------------------------------------

	static bool GeometryToWKBCast(Vector &source, Vector &result, idx_t count, CastParameters &) {

		UnaryExecutor::Execute<string_t, string_t>(
		    source, result, count, [&](const string_t &input) { return WKBWriter::Write(input, result); });

		return true;
	}

	//------------------------------------------------------------------------------------------------------------------
	// Register
	//------------------------------------------------------------------------------------------------------------------

	static void Register(ExtensionLoader &loader) {

		// POINT_2D -> VARCHAR
		loader.RegisterCastFunction(SpatialTypes::POINT_2D(), LogicalType::VARCHAR, PointToVarcharCast, 1);

		// GEOMETRY -> VARCHAR
		loader.RegisterCastFunction(SpatialTypes::GEOMETRY(), LogicalType::VARCHAR, GeometryToVarcharCast, 1);

		// GEOMETRY -> WKB_BLOB
		loader.RegisterCastFunction(SpatialTypes::GEOMETRY(), SpatialTypes::WKB_BLOB(), GeometryToWKBCast, 1);
	}
};

} // namespace

// ######################################################################################################################
//  Register
// ######################################################################################################################

void SpatialCastsFunctions::Register(ExtensionLoader &loader) {
	GeometryCasts::Register(loader);
}

} // namespace duckdb

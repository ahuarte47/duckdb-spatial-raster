# DuckDB SpatialRaster Extension

<span style="font-size:2em">⚠️</span> **This extension is no longer maintained and has been replaced by [duckdb-raster](https://github.com/ahuarte47/duckdb-raster).**

# What is this?

This is a prototype of a geospatial extension for DuckDB that adds support for working with spatial data and functions in the form of a `RASTER` type.

Please note that this extension is a PoC and still in a very early stage of development, and the internal storage format for the raster types may change indiscriminately between commits.

This extension is closely tied to the [GDAL](https://github.com/OSGeo/gdal) library and is fully inspired by [DuckDB's Spatial extension](https://github.com/duckdb/duckdb-spatial). In fact, Spatial extension is a submodule for the Spatial Raster extension, because some functions are reused to finally build this component.

DuckDB Spatial Raster extension can work in combination with the Spatial extension, it accepts `GEOMETRY` types as parameters, and returns `GEOMETRY` instances that can be used with the spatial functions of the Spatial extension.

Please see the [function table](docs/functions.md) for the current implementation status.

# How do I get it?

## Building from source
This extension is based on the [DuckDB extension template](https://github.com/duckdb/extension-template).

**Dependencies**

You need a recent version of CMake (3.20) and a C++11 compatible compiler.
You also need OpenSSL on your system. On ubuntu you can install it with `sudo apt install libssl-dev`, on macOS you can install it with `brew install openssl`. Note that brew installs openssl in a non-standard location, so you may need to set a `OPENSSL_ROOT_DIR=$(brew --prefix openssl)` environment variable when building.

We bundle all the other required dependencies in the `deps` directory, which should be automatically built and statically linked into the extension. This may take some time the first time you build, but subsequent builds should be much faster.

We also highly recommend that you install [Ninja](https://ninja-build.org) which you can select when building by setting the `GEN=ninja` environment variable.
```
git clone --recurse-submodules https://github.com/ahuarte47/duckdb-spatial-raster
cd duckdb-spatial-raster
make release
```

You can then invoke the built DuckDB (with the extension statically linked)
```
./build/release/duckdb
```

Please see the Makefile for more options, or the extension template documentation for more details.

# Example Usage

Please see the [example](docs/example.md) for an example on how to use the extension.

# Supported Functions and Documentation

The full list of functions and their documentation is available in the [function reference](docs/functions.md)

## Running the tests

Different tests can be created for DuckDB extensions. The primary way of testing DuckDB extensions should be the SQL tests in `./test/sql`. These SQL tests can be run using:

```sh
make test
```

### Installing the deployed binaries

To install your extension binaries from S3, you will need to do two things. Firstly, DuckDB should be launched with the
`allow_unsigned_extensions` option set to true. How to set this will depend on the client you're using. Some examples:

CLI:
```shell
duckdb -unsigned
```

Python:
```python
con = duckdb.connect(':memory:', config={'allow_unsigned_extensions' : 'true'})
```

NodeJS:
```js
db = new duckdb.Database(':memory:', {"allow_unsigned_extensions": "true"});
```

Secondly, you will need to set the repository endpoint in DuckDB to the HTTP url of your bucket + version of the extension
you want to install. To do this run the following SQL query in DuckDB:
```sql
SET custom_extension_repository='bucket.s3.eu-west-1.amazonaws.com/<your_extension_name>/latest';
```
Note that the `/latest` path will allow you to install the latest extension version available for your current version of
DuckDB. To specify a specific version, you can pass the version instead.

After running these steps, you can install and load your extension using the regular INSTALL/LOAD commands in DuckDB:
```sql
INSTALL spatial_raster;
LOAD spatial_raster;
```

Enjoy!

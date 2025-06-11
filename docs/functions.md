# DuckDB Spatial Raster Function Reference

## Function Index 
**[Scalar Functions](#scalar-functions)**

| Function | Summary |
| --- | --- |
| [`RT_GetBBox`](#rt_getbbox) | Returns the minimum bounding box of the raster. |
| [`RT_GetBandColorInterpretation`](#rt_getbandcolorinterpretation) | Returns the color interpretation of a band in the raster. |
| [`RT_GetBandColorInterpretationName`](#rt_getbandcolorinterpretationname) | Returns the color interpretation name of a band in the raster. |
| [`RT_GetBandNoDataValue`](#rt_getbandnodatavalue) | Returns the NODATA value of a band in the raster. |
| [`RT_GetBandPixelType`](#rt_getbandpixeltype) | Returns the pixel type of a band in the raster. |
| [`RT_GetBandPixelTypeName`](#rt_getbandpixeltypename) | Returns the pixel type name of a band in the raster. |
| [`RT_GetGeometry`](#rt_getgeometry) | Returns the polygon representation of the extent of the raster. |
| [`RT_HasNoBand`](#rt_hasnoband) | Returns true if there is no band with given band number. |
| [`RT_Height`](#rt_height) | Returns the height of the raster in pixels. |
| [`RT_NumBands`](#rt_numbands) | Returns the number of bands in the raster. |
| [`RT_PixelHeight`](#rt_pixelheight) | Returns the height of a pixel in geometric units of the spatial reference system. |
| [`RT_PixelWidth`](#rt_pixelwidth) | Returns the width of a pixel in geometric units of the spatial reference system. |
| [`RT_RasterAsBlob`](#rt_rasterasblob) | Writes a raster to a blob. |
| [`RT_RasterAsFile`](#rt_rasterasfile) | Writes a raster to a file path. |
| [`RT_RasterClip`](#rt_rasterclip) | Returns a raster that is clipped by the input geometry. |
| [`RT_RasterFromBlob`](#rt_rasterfromblob) | Loads a raster from a blob. |
| [`RT_RasterFromFile`](#rt_rasterfromfile) | Loads a raster from a file path. |
| [`RT_RasterSplit`](#rt_rastersplit) | Splits a raster into smaller tiles of specified size. |
| [`RT_RasterToWorldCoord`](#rt_rastertoworldcoord) | Returns the upper left corner as geometric X and Y (longitude and latitude) given a column and row. |
| [`RT_RasterToWorldCoordX`](#rt_rastertoworldcoordx) | Returns the upper left X coordinate of a raster column row in geometric units of the georeferenced raster. |
| [`RT_RasterToWorldCoordY`](#rt_rastertoworldcoordy) | Returns the upper left Y coordinate of a raster column row in geometric units of the georeferenced raster. |
| [`RT_RasterWarp`](#rt_rasterwarp) | Performs mosaicing, reprojection and/or warping on a raster. |
| [`RT_ScaleX`](#rt_scalex) | Returns the X component of the pixel width in units of coordinate reference system. |
| [`RT_ScaleY`](#rt_scaley) | Returns the Y component of the pixel width in units of coordinate reference system. |
| [`RT_SkewX`](#rt_skewx) | Returns the georeference X skew (or rotation parameter). |
| [`RT_SkewY`](#rt_skewy) | Returns the georeference Y skew (or rotation parameter). |
| [`RT_Srid`](#rt_srid) | Returns the spatial reference identifier (EPSG code) of the raster. |
| [`RT_UpperLeftX`](#rt_upperleftx) | Returns the upper left X coordinate of raster in projected spatial reference. |
| [`RT_UpperLeftY`](#rt_upperlefty) | Returns the upper left Y coordinate of raster in projected spatial reference. |
| [`RT_Value`](#rt_value) | Returns the value of a given band in a given column, row pixel. |
| [`RT_Width`](#rt_width) | Returns the width of the raster in pixels. |
| [`RT_WorldToRasterCoord`](#rt_worldtorastercoord) | Returns the upper left corner as column and row given geometric X and Y (longitude and latitude). |
| [`RT_WorldToRasterCoordX`](#rt_worldtorastercoordx) | Returns the column in the raster given geometric X and Y (longitude and latitude). |
| [`RT_WorldToRasterCoordY`](#rt_worldtorastercoordy) | Returns the row in the raster given geometric X and Y (longitude and latitude). |

**[Aggregate Functions](#aggregate-functions)**

| Function | Summary |
| --- | --- |
| [`RT_RasterMosaic_Agg`](#rt_rastermosaic_agg) | Returns a mosaic of a set of raster tiles into a single raster. |
| [`RT_RasterUnion_Agg`](#rt_rasterunion_agg) | Returns the union of a set of raster tiles into a single raster composed of at least one band. |

**[Table Functions](#table-functions)**

| Function | Summary |
| --- | --- |
| [`RT_Drivers`](#rt_drivers) | Returns the list of supported GDAL RASTER drivers and file formats. |
| [`RT_Read`](#rt_read) | Read and import a variety of geospatial raster file formats using the GDAL library. |
| [`RT_Read_Meta`](#rt_read_meta) | Read the metadata from a variety of geospatial raster file formats using the GDAL library. |

----

## Scalar Functions

### RT_GetBBox


#### Signature

```sql
GEOMETRY RT_GetBBox (raster RASTER)
```

#### Description

Returns the minimum bounding box of the raster.

#### Example

```sql
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
```

----

### RT_GetBandColorInterpretation


#### Signature

```sql
INTEGER RT_GetBandColorInterpretation (raster RASTER, band_number INTEGER)
```

#### Description

Returns the color interpretation of a band in the raster.

This is a code in the enumeration:
+ Undefined = 0: Undefined
+ GrayIndex = 1: Greyscale
+ PaletteIndex = 2: Paletted (see associated color table)
+ RedBand = 3: Red band of RGBA image
+ GreenBand = 4: Green band of RGBA image
+ BlueBand = 5: Blue band of RGBA image
+ AlphaBand = 6: Alpha (0=transparent, 255=opaque)
+ HueBand = 7: Hue band of HLS image
+ SaturationBand = 8: Saturation band of HLS image
+ LightnessBand = 9: Lightness band of HLS image
+ CyanBand = 10: Cyan band of CMYK image
+ MagentaBand = 11: Magenta band of CMYK image
+ YellowBand = 12: Yellow band of CMYK image
+ BlackBand = 13: Black band of CMYK image
+ YCbCr_YBand = 14: Y Luminance
+ YCbCr_CbBand = 15: Cb Chroma
+ YCbCr_CrBand = 16: Cr Chroma

#### Example

```sql
SELECT
    RT_GetBandColorInterpretation(raster, 1)
FROM
    RT_Read('./test/data/mosaic/SCL.tif-land-clip00.tiff')
;
┌──────────────────────────────────────────┐
│ rt_getbandcolorinterpretation(raster, 1) │
│                  int32                   │
├──────────────────────────────────────────┤
│                    1                     │
└──────────────────────────────────────────┘
```

----

### RT_GetBandColorInterpretationName


#### Signature

```sql
VARCHAR RT_GetBandColorInterpretationName (raster RASTER, band_number INTEGER)
```

#### Description

Returns the color interpretation name of a band in the raster.

This is a string in the enumeration:
+ Undefined: Undefined
+ Greyscale: Greyscale
+ Paletted: Paletted (see associated color table)
+ Red: Red band of RGBA image
+ Green: Green band of RGBA image
+ Blue: Blue band of RGBA image
+ Alpha: Alpha (0=transparent, 255=opaque)
+ Hue: Hue band of HLS image
+ Saturation: Saturation band of HLS image
+ Lightness: Lightness band of HLS image
+ Cyan: Cyan band of CMYK image
+ Magenta: Magenta band of CMYK image
+ Yellow: Yellow band of CMYK image
+ Black: Black band of CMYK image
+ YLuminance: Y Luminance
+ CbChroma: Cb Chroma
+ CrChroma: Cr Chroma

#### Example

```sql
SELECT
    RT_GetBandColorInterpretationName(raster, 1)
FROM
    RT_Read('./test/data/mosaic/SCL.tif-land-clip00.tiff')
;
┌──────────────────────────────────────────────┐
│ rt_getbandcolorinterpretationname(raster, 1) │
│                   varchar                    │
├──────────────────────────────────────────────┤
│ Greyscale                                    │
└──────────────────────────────────────────────┘
```

----

### RT_GetBandNoDataValue


#### Signature

```sql
DOUBLE RT_GetBandNoDataValue (raster RASTER, band_number INTEGER)
```

#### Description

Returns the NODATA value of a band in the raster.

#### Example

```sql
SELECT
    RT_GetBandNoDataValue(raster, 1)
FROM
    RT_Read('./test/data/mosaic/SCL.tif-land-clip00.tiff')
;
┌──────────────────────────────────┐
│ rt_getbandnodatavalue(raster, 1) │
│              double              │
├──────────────────────────────────┤
│             -9999.0              │
└──────────────────────────────────┘
```

----

### RT_GetBandPixelType


#### Signature

```sql
INTEGER RT_GetBandPixelType (raster RASTER, band_number INTEGER)
```

#### Description

Returns the pixel type of a band in the raster.

This is a code in the enumeration:
+ Unknown = 0: Unknown or unspecified type
+ Byte = 1: Eight bit unsigned integer
+ Int8 = 14: 8-bit signed integer
+ UInt16 = 2: Sixteen bit unsigned integer
+ Int16 = 3: Sixteen bit signed integer
+ UInt32 = 4: Thirty two bit unsigned integer
+ Int32 = 5: Thirty two bit signed integer
+ UInt64 = 12: 64 bit unsigned integer
+ Int64 = 13: 64 bit signed integer
+ Float32 = 6: Thirty two bit floating point
+ Float64 = 7: Sixty four bit floating point
+ CInt16 = 8: Complex Int16
+ CInt32 = 9: Complex Int32
+ CFloat32 = 10: Complex Float32
+ CFloat64 = 11: Complex Float64

#### Example

```sql
SELECT
    RT_GetBandPixelType(raster, 1)
FROM
    RT_Read('./test/data/mosaic/SCL.tif-land-clip00.tiff')
;
┌────────────────────────────────┐
│ rt_getbandpixeltype(raster, 1) │
│             int32              │
├────────────────────────────────┤
│               3                │
└────────────────────────────────┘
```

----

### RT_GetBandPixelTypeName


#### Signature

```sql
VARCHAR RT_GetBandPixelTypeName (raster RASTER, band_number INTEGER)
```

#### Description

Returns the pixel type name of a band in the raster.

This is a string in the enumeration:
+ Unknown: Unknown or unspecified type
+ Byte: Eight bit unsigned integer
+ Int8: 8-bit signed integer
+ UInt16: Sixteen bit unsigned integer
+ Int16: Sixteen bit signed integer
+ UInt32: Thirty two bit unsigned integer
+ Int32: Thirty two bit signed integer
+ UInt64: 64 bit unsigned integer
+ Int64: 64 bit signed integer
+ Float32: Thirty two bit floating point
+ Float64: Sixty four bit floating point
+ CInt16: Complex Int16
+ CInt32: Complex Int32
+ CFloat32: Complex Float32
+ CFloat64: Complex Float64

#### Example

```sql
SELECT
    RT_GetBandPixelTypeName(raster, 1)
FROM
    RT_Read('./test/data/mosaic/SCL.tif-land-clip00.tiff')
;
┌────────────────────────────────────┐
│ rt_getbandpixeltypename(raster, 1) │
│              varchar               │
├────────────────────────────────────┤
│ Int16                              │
└────────────────────────────────────┘
```

----

### RT_GetGeometry


#### Signature

```sql
GEOMETRY RT_GetGeometry (raster RASTER)
```

#### Description

Returns the polygon representation of the extent of the raster.

#### Example

```sql
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
```

----

### RT_HasNoBand


#### Signature

```sql
BOOLEAN RT_HasNoBand (raster RASTER, band_number INTEGER)
```

#### Description

Returns true if there is no band with given band number.
Band numbers start at 1 and band is assumed to be 1 if not specified.

#### Example

```sql
SELECT
    RT_HasNoBand(raster, 1)
FROM
    RT_Read('./test/data/mosaic/SCL.tif-land-clip00.tiff')
;
┌─────────────────────────┐
│ rt_hasnoband(raster, 1) │
│         boolean         │
├─────────────────────────┤
│ true                    │
└─────────────────────────┘
```

----

### RT_Height


#### Signature

```sql
INTEGER RT_Height (raster RASTER)
```

#### Description

Returns the height of the raster in pixels.

#### Example

```sql
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
```

----

### RT_NumBands


#### Signature

```sql
INTEGER RT_NumBands (raster RASTER)
```

#### Description

Returns the number of bands in the raster.

#### Example

```sql
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
```

----

### RT_PixelHeight


#### Signature

```sql
DOUBLE RT_PixelHeight (raster RASTER)
```

#### Description

Returns the height of a pixel in geometric units of the spatial reference system.
In the common case where there is no skew, the pixel height is just the scale ratio between geometric coordinates and raster pixels.

#### Example

```sql
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
```

----

### RT_PixelWidth


#### Signature

```sql
DOUBLE RT_PixelWidth (raster RASTER)
```

#### Description

Returns the width of a pixel in geometric units of the spatial reference system.
In the common case where there is no skew, the pixel width is just the scale ratio between geometric coordinates and raster pixels.

#### Example

```sql
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
```

----

### RT_RasterAsBlob


#### Signatures

```sql
BLOB RT_RasterAsBlob (raster RASTER)
BLOB RT_RasterAsBlob (raster RASTER, driver_name VARCHAR)
BLOB RT_RasterAsBlob (raster RASTER, driver_name VARCHAR, write_options VARCHAR[])
```

#### Description

Writes a raster to a blob.
`driver_name` is optional, 'GTiff' format by default.
`write_options` is optional, an array of parameters for the GDAL driver specified.

#### Example

```sql
WITH __input AS (
    SELECT
        RT_RasterFromFile(file) AS raster
    FROM
        glob('./test/data/mosaic/SCL.tif-land-clip00.tiff')
)
SELECT
    RT_RasterAsBlob(raster, 'Gtiff') AS result
FROM
    __input
;
┌───────────────────────────────────────────────┐
│                   result                      │
│                    blob                       │
├───────────────────────────────────────────────┤
│ II*\x00\xC0\x00\x00\x00GDAL_STRUCTURAL…       │
└───────────────────────────────────────────────┘
```

----

### RT_RasterAsFile


#### Signatures

```sql
BOOLEAN RT_RasterAsFile (raster RASTER, file_name VARCHAR, driver_name VARCHAR)
BOOLEAN RT_RasterAsFile (raster RASTER, file_name VARCHAR, driver_name VARCHAR, write_options VARCHAR[])
```

#### Description

Writes a raster to a file path.
`write_options` is optional, an array of parameters for the GDAL driver specified.

#### Example

```sql
WITH __input AS (
    SELECT
        RT_RasterFromFile(file) AS raster
    FROM
        glob('./test/data/mosaic/SCL.tif-land-clip00.tiff')
)
SELECT
    RT_RasterAsFile(raster, './test/data/rasterasfile.tiff', 'Gtiff') AS result
FROM
    __input
;
┌─────────┐
│ result  │
│ boolean │
├─────────┤
│ true    │
└─────────┘
```

----

### RT_RasterClip


#### Signatures

```sql
RASTER RT_RasterClip (raster RASTER, geometry GEOMETRY)
RASTER RT_RasterClip (raster RASTER, geometry GEOMETRY, options VARCHAR[])
```

#### Description

Returns a raster that is clipped by the input geometry.
`options` is optional, an array of parameters like [GDALWarp](https://gdal.org/programs/gdalwarp.html).

#### Example

```sql
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
```

----

### RT_RasterFromBlob


#### Signatures

```sql
RASTER RT_RasterFromBlob (blob BLOB)
RASTER RT_RasterFromBlob (blob BLOB, driver_name VARCHAR)
RASTER RT_RasterFromBlob (blob BLOB, driver_names VARCHAR[])
```

#### Description

Loads a raster from a blob.
`driver_name` is optional, 'GTiff' format by default.

#### Example

```sql
WITH __input AS (
    SELECT
        RT_RasterFromFile(file) AS raster
    FROM
        glob('./test/data/mosaic/SCL.tif-land-clip00.tiff')
),
__blob AS (
    SELECT
        RT_RasterAsBlob(raster, 'COG') AS result
    FROM
        __input
)
SELECT
    RT_GetGeometry(RT_RasterFromBlob(result, ['COG', 'GTiff']))
FROM
    __blob
;
┌────────────────────────────────────────────────────────────────────────────────────────────┐
│         rt_getgeometry(rt_rasterfromblob(result, main.list_value('COG', 'GTiff')))         │
│                                          geometry                                          │
├────────────────────────────────────────────────────────────────────────────────────────────┤
│ POLYGON ((541020 4690200, 541020 4796640, 609780 4796640, 609780 4690200, 541020 4690200)) │
└────────────────────────────────────────────────────────────────────────────────────────────┘
```

----

### RT_RasterFromFile


#### Signature

```sql
RASTER RT_RasterFromFile (path VARCHAR)
```

#### Description

Loads a raster from a file path.

#### Example

```sql
WITH __input AS (
    SELECT
        RT_RasterFromFile(file) AS raster
    FROM
        glob('./test/data/mosaic/SCL.tif-land-clip00.tiff')
)
SELECT
    RT_RasterAsFile(raster, './test/data/rasterasfile.tiff', 'Gtiff') AS result
FROM
    __input
;
┌─────────┐
│ result  │
│ boolean │
├─────────┤
│ true    │
└─────────┘
```

----

### RT_RasterSplit


#### Signature

```sql
RASTER[] RT_RasterSplit (raster RASTER, tile_size_x INTEGER, tile_size_y INTEGER)
```

#### Description

Splits a raster into smaller tiles of specified size.
`tile_size_x` and `tile_size_y` specify the size of each tile in pixels.
The result is a list of rasters, each representing a tile of the original raster.

#### Example

```sql
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
```

----

### RT_RasterToWorldCoord


#### Signature

```sql
POINT_2D RT_RasterToWorldCoord (raster RASTER, col INTEGER, row INTEGER)
```

#### Description

Returns the upper left corner as geometric X and Y (longitude and latitude) given a column and row.
Returned X and Y are in geometric units of the georeferenced raster.

#### Example

```sql
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
```

----

### RT_RasterToWorldCoordX


#### Signature

```sql
DOUBLE RT_RasterToWorldCoordX (raster RASTER, col INTEGER, row INTEGER)
```

#### Description

Returns the upper left X coordinate of a raster column row in geometric units of the georeferenced raster.
Returned X is in geometric units of the georeferenced raster.

#### Example

```sql
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
```

----

### RT_RasterToWorldCoordY


#### Signature

```sql
DOUBLE RT_RasterToWorldCoordY (raster RASTER, col INTEGER, row INTEGER)
```

#### Description

Returns the upper left Y coordinate of a raster column row in geometric units of the georeferenced raster.
Returned Y is in geometric units of the georeferenced raster.

#### Example

```sql
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
```

----

### RT_RasterWarp


#### Signature

```sql
RASTER RT_RasterWarp (raster RASTER, options VARCHAR[])
```

#### Description

Performs mosaicing, reprojection and/or warping on a raster.
`options` is optional, an array of parameters like [GDALWarp](https://gdal.org/programs/gdalwarp.html).

#### Example

```sql
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
```

----

### RT_ScaleX


#### Signature

```sql
DOUBLE RT_ScaleX (raster RASTER)
```

#### Description

Returns the X component of the pixel width in units of coordinate reference system.
Refer to [World File](https://en.wikipedia.org/wiki/World_file) for more details.

#### Example

```sql
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
```

----

### RT_ScaleY


#### Signature

```sql
DOUBLE RT_ScaleY (raster RASTER)
```

#### Description

Returns the Y component of the pixel width in units of coordinate reference system.
Refer to [World File](https://en.wikipedia.org/wiki/World_file) for more details.

#### Example

```sql
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
```

----

### RT_SkewX


#### Signature

```sql
DOUBLE RT_SkewX (raster RASTER)
```

#### Description

Returns the georeference X skew (or rotation parameter).
Refer to [World File](https://en.wikipedia.org/wiki/World_file) for more details.

#### Example

```sql
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
```

----

### RT_SkewY


#### Signature

```sql
DOUBLE RT_SkewY (raster RASTER)
```

#### Description

Returns the georeference Y skew (or rotation parameter).
Refer to [World File](https://en.wikipedia.org/wiki/World_file) for more details.

#### Example

```sql
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
```

----

### RT_Srid


#### Signature

```sql
INTEGER RT_Srid (raster RASTER)
```

#### Description

Returns the spatial reference identifier (EPSG code) of the raster.
Refer to [EPSG](https://spatialreference.org/ref/epsg/) for more details.

#### Example

```sql
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
```

----

### RT_UpperLeftX


#### Signature

```sql
DOUBLE RT_UpperLeftX (raster RASTER)
```

#### Description

Returns the upper left X coordinate of raster in projected spatial reference.

#### Example

```sql
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
```

----

### RT_UpperLeftY


#### Signature

```sql
DOUBLE RT_UpperLeftY (raster RASTER)
```

#### Description

Returns the upper left Y coordinate of raster in projected spatial reference.

#### Example

```sql
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
```

----

### RT_Value


#### Signature

```sql
DOUBLE RT_Value (raster RASTER, band INTEGER, col INTEGER, row INTEGER)
```

#### Description

Returns the value of a given band in a given column, row pixel.
Band numbers start at 1 and band is assumed to be 1 if not specified.

#### Example

```sql
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
```

----

### RT_Width


#### Signature

```sql
INTEGER RT_Width (raster RASTER)
```

#### Description

Returns the width of the raster in pixels.

#### Example

```sql
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
```

----

### RT_WorldToRasterCoord


#### Signature

```sql
RASTER_COORD RT_WorldToRasterCoord (raster RASTER, x DOUBLE, y DOUBLE)
```

#### Description

Returns the upper left corner as column and row given geometric X and Y (longitude and latitude).
Geometric X and Y must be expressed in the spatial reference coordinate system of the raster.

#### Example

```sql
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
```

----

### RT_WorldToRasterCoordX


#### Signature

```sql
INTEGER RT_WorldToRasterCoordX (raster RASTER, x DOUBLE, y DOUBLE)
```

#### Description

Returns the column in the raster given geometric X and Y (longitude and latitude).
Geometric X and Y must be expressed in the spatial reference coordinate system of the raster.

#### Example

```sql
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
```

----

### RT_WorldToRasterCoordY


#### Signature

```sql
INTEGER RT_WorldToRasterCoordY (raster RASTER, x DOUBLE, y DOUBLE)
```

#### Description

Returns the row in the raster given geometric X and Y (longitude and latitude).
Geometric X and Y must be expressed in the spatial reference coordinate system of the raster.

#### Example

```sql
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
```

----

## Aggregate Functions

### RT_RasterMosaic_Agg


#### Signatures

```sql
RASTER RT_RasterMosaic_Agg (col0 RASTER)
RASTER RT_RasterMosaic_Agg (col0 RASTER, col1 VARCHAR[])
```

#### Description

Returns a mosaic of a set of raster tiles into a single raster.
Tiles are considered as source rasters of a larger mosaic and the result dataset has as many bands as one of the input files.
`options` is optional, an array of parameters like [GDALBuildVRT](https://gdal.org/programs/gdalbuildvrt.html).

#### Example

```sql
WITH __input AS (
    SELECT
        1 AS mosaic_id,
        RT_RasterFromFile(file) AS raster
    FROM
        glob('./test/data/mosaic/*.tiff')
)
SELECT
    RT_GetGeometry(RT_RasterMosaic_Agg(raster, options => ['-r', 'bilinear'])) AS g
FROM
    __input
GROUP BY
    mosaic_id
;
┌────────────────────────────────────────────────────────────────────────────────────────────┐
│                                             g                                              │
│                                          geometry                                          │
├────────────────────────────────────────────────────────────────────────────────────────────┤
│ POLYGON ((541020 4640780, 541020 4796640, 685600 4796640, 685600 4640780, 541020 4640780)) │
└────────────────────────────────────────────────────────────────────────────────────────────┘
```

----

### RT_RasterUnion_Agg


#### Signatures

```sql
RASTER RT_RasterUnion_Agg (col0 RASTER)
RASTER RT_RasterUnion_Agg (col0 RASTER, col1 VARCHAR[])
```

#### Description

Returns the union of a set of raster tiles into a single raster composed of at least one band.
Each tiles goes into a separate band in the result dataset.
`options` is optional, an array of parameters like [GDALBuildVRT](https://gdal.org/programs/gdalbuildvrt.html).

#### Example

```sql
WITH __input AS (
    SELECT
        1 AS raster_id,
        RT_RasterFromFile(file) AS raster
    FROM
        glob('./test/data/mosaic/*.tiff')
)
SELECT
    RT_GetGeometry(RT_RasterUnion_Agg(raster, options => ['-resolution', 'highest'])) AS g
FROM
    __input
GROUP BY
    raster_id
;
┌────────────────────────────────────────────────────────────────────────────────────────────┐
│                                             g                                              │
│                                          geometry                                          │
├────────────────────────────────────────────────────────────────────────────────────────────┤
│ POLYGON ((541020 4640780, 541020 4796640, 685600 4796640, 685600 4640780, 541020 4640780)) │
└────────────────────────────────────────────────────────────────────────────────────────────┘
```

----

## Table Functions

### RT_Drivers

#### Signature

```sql
RT_Drivers ()
```

#### Description

Returns the list of supported GDAL RASTER drivers and file formats.

Note that far from all of these drivers have been tested properly.
Some may require additional options to be passed to work as expected.
If you run into any issues please first consult the [consult the GDAL docs](https://gdal.org/drivers/raster/index.html).

#### Example

```sql
SELECT * FROM RT_Drivers();
```

----

### RT_Read

#### Signature

```sql
RT_Read (col0 VARCHAR, sibling_files VARCHAR[], allowed_drivers VARCHAR[], open_options VARCHAR[])
```

#### Description

Read and import a variety of geospatial raster file formats using the GDAL library.

The `RT_Read` table function is based on the [GDAL](https://gdal.org/index.html) translator library and enables reading raster data from a variety of geospatial raster file formats as if they were DuckDB tables.

> See [RT_Drivers](#rt_drivers) for a list of supported file formats and drivers.

Except for the `path` parameter, all parameters are optional.

| Parameter | Type | Description |
| --------- | -----| ----------- |
| `path` | VARCHAR | The path to the file to read. Mandatory |
| `open_options` | VARCHAR[] | A list of key-value pairs that are passed to the GDAL driver to control the opening of the file. |
| `allowed_drivers` | VARCHAR[] | A list of GDAL driver names that are allowed to be used to open the file. If empty, all drivers are allowed. |
| `sibling_files` | VARCHAR[] | A list of sibling files that are required to open the file. |

Note that GDAL is single-threaded, so this table function will not be able to make full use of parallelism.

By using `RT_Read`, the spatial extension also provides “replacement scans” for common geospatial file formats, allowing you to query files of these formats as if they were tables directly.

```sql
SELECT * FROM './path/to/some/shapefile/dataset.tif';
```

In practice this is just syntax-sugar for calling RT_Read, so there is no difference in performance. If you want to pass additional options, you should use the RT_Read table function directly.

The following formats are currently recognized by their file extension:

| Format | Extension |
| ------ | --------- |
| GeoTiff COG | .tif, .tiff |
| Erdas Imagine | .img |
| GDAL Virtual | .vrt |

#### Example

```sql
-- Read a Gtiff file
SELECT * FROM RT_Read('some/file/path/filename.tif');
```

----

### RT_Read_Meta

#### Signature

```sql
RT_Read_Meta (col0 VARCHAR)
RT_Read_Meta (col0 VARCHAR[])
```

#### Description

Read the metadata from a variety of geospatial raster file formats using the GDAL library.

The `RT_Read_Meta` table function accompanies the `RT_Read` table function, but instead of reading the contents of a file, this function scans the metadata instead.

#### Example

```sql
SELECT
    driver_short_name,
    driver_long_name,
    upper_left_x,
    upper_left_y,
    width,
    height,
    scale_x,
    scale_y,
    skew_x,
    skew_y,
    srid,
    num_bands
FROM
    RT_Read_Meta('./test/data/mosaic/SCL.tif-land-clip00.tiff')
;

┌───────────────────┬──────────────────┬──────────────┬──────────────┬───────┬────────┬─────────┬─────────┬────────┬────────┬───────┬───────────┐
│ driver_short_name │ driver_long_name │ upper_left_x │ upper_left_y │ width │ height │ scale_x │ scale_y │ skew_x │ skew_y │ srid  │ num_bands │
│      varchar      │     varchar      │    double    │    double    │ int32 │ int32  │ double  │ double  │ double │ double │ int32 │   int32   │
├───────────────────┼──────────────────┼──────────────┼──────────────┼───────┼────────┼─────────┼─────────┼────────┼────────┼───────┼───────────┤
│ GTiff             │ GeoTIFF          │     541020.0 │    4796640.0 │  3438 │   5322 │    20.0 │   -20.0 │    0.0 │    0.0 │ 32630 │         1 │
└───────────────────┴──────────────────┴──────────────┴──────────────┴───────┴────────┴─────────┴─────────┴────────┴────────┴───────┴───────────┘
```

----


# Example Usage

The following is a slightly contrived example of how you can use this extension to read and export geospatial raster data formats, transform `raster` objects and work with spatial property and predicate functions.

For a complete list of all supported functions, please have a look at the [function reference](functions.md).

Let's start by loading the spatial raster extension so we can import a raster from a `GTiff` file, using the `RT_Read` gdal-based table function. We then read attributes from the `raster` object using several `RT_xxx` scalar functions.

```sql
LOAD spatial_raster;

SELECT
	RT_Width(raster) AS cols,
	RT_Height(raster) AS rows,
	RT_NumBands(raster) AS num_bands,
	RT_UpperLeftX(raster) AS ulx,
	RT_UpperLeftY(raster) AS uly,
	RT_ScaleX(raster) AS scale_x,
	RT_ScaleY(raster) AS scale_y,
	RT_SkewX(raster) AS skew_x,
	RT_SkewY(raster) AS skew_y,
	RT_PixelWidth(raster) AS px_width,
	RT_PixelHeight(raster) AS px_height
FROM
	RT_Read('./test/data/mosaic/SCL.tif-land-clip00.tiff')
;
```

<details>
<summary>
    SELECT ... FROM RT_Read('./test/data/mosaic/SCL.tif-land-clip00.tiff');
</summary>

|  cols  |  rows  |  num_bands |    ulx    |     uly    |  scale_x |  scale_y |  skew_x |  skew_y |  px_width |  px_height |
|--------|--------|------------|-----------|------------|----------|----------|---------|---------|-----------|------------|
|  3438  |  5322  |      1     |  541020.0 |  4796640.0 |   20.0   |   -20.0  |   0.0   |   0.0   |    20.0   |    20.0    |

</details>

<br/>

We read metadata attributes from raster files using the `RT_Read_Meta` table function:

```sql
SELECT
	*
FROM
	RT_Read_Meta('./test/data/mosaic/SCL.tif-land-clip00.tiff')
;
```

<details>
<summary>
    SELECT * FROM RT_Read_Meta('./test/data/mosaic/SCL.tif-land-clip00.tiff');
</summary>

| driver_short_name | driver_long_name | upper_left_x | upper_left_y | width | height | scale_x | scale_y | skew_x | skew_y | srid  | num_bands |
|-------------------|------------------|--------------|--------------|-------|--------|---------|---------|--------|--------|-------|-----------|
| GTiff             | GeoTIFF          |   541020.0   |  4796640.0   | 3438  |  5322  |  20.0   |  -20.0  |  0.0   |  0.0   | 32630 |     1     |

</details>

<br/>

**Spatial Raster extension can be used in combination with the Spatial Extension**, it accepts `geometry` types as parameters.

In the following example we clip `raster` objects using as input a `geometry` defined in a `Geopackage` vector file. As you can see we use `ST_Read` function from Spatial extension to load the vector layer. In summary this SQL statement loads a set raster files into `raster` objects, creates a raster mosaic, clip it with the `geometry` defined in a `Geopackage` file, and finally gets the area of its spatial envelope.

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
```

## GDAL creation option

If you need to set some "creation option" (format specific), i.e. those options that you set via CLI with `-co`, you can use this kind of syntax, in which the `TILED` and `COMPRESS` - available in [`GTiff` format](https://gdal.org/en/stable/drivers/raster/gtiff.html#creation-options) - options are set:

```sql
COPY (
	SELECT * FROM './test/data/mosaic/SCL.tif-land-clip00.tiff'
)
TO
	'./copytoraster.tiff'
WITH (
	FORMAT RASTER, DRIVER 'GTiff', CREATION_OPTIONS ('TILED=YES', 'COMPRESS=LZW')
);
```

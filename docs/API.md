# GeoEngine REST API

## Endpoints

### GET /v1/spatial/nearby

Returns entities within a radius of a coordinate pair.

**Parameters:**
- `lat` (float, required): WGS-84 latitude, [-90, 90]
- `lon` (float, required): WGS-84 longitude, [-180, 180]
- `radius` (float, optional): Search radius in kilometers, default 1.0, max 20037.5
- `dataset` (string, optional): Filter to a named dataset layer
- `limit` (int, optional): Maximum results, default 1000, max 10000

**Response (200):**
```json
{
  "entities": [
    {
      "id": "entity-1",
      "latitude": 40.7128,
      "longitude": -74.0060,
      "metadata": {"city": "NYC", "population": 8419600}
    }
  ],
  "total": 1,
  "query_time_ms": 0.42,
  "index_used": true
}
```

**Error Codes:**
- `400 VALIDATION`: Coordinate or radius out of bounds
- `404 NOT_FOUND`: Dataset does not exist
- `500 INTERNAL`: Index or repository failure
- `503 TIMEOUT`: Query exceeded deadline

### GET /health

Liveness probe. Returns 200 with JSON status payload.

### GET /ready

Readiness probe. Returns 200 when all dependencies are available.
<!-- rev: 1 -->
<!-- rev: 2 -->
<!-- rev: 3 -->

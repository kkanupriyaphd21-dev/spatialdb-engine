#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SpatialPoint {
    double lat;
    double lon;
} SpatialPoint;

typedef struct BoundingBox {
    double min_lat;
    double min_lon;
    double max_lat;
    double max_lon;
} BoundingBox;

typedef struct SpatialObject {
    char     id[256];
    SpatialPoint point;
    char     collection[128];
    uint64_t timestamp;
} SpatialObject;

int spatialdb_init(const char* config_path);
void spatialdb_shutdown();
int spatialdb_insert(const SpatialObject* obj);
int spatialdb_delete(const char* collection, const char* id);

#ifdef __cplusplus
}
#endif

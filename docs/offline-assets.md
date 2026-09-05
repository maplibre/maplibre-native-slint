# Serving assets from local files

MapLibre Native resolves `file://` URLs itself, through mbgl-core's
`LocalFileSource`. Nothing in this repository has to be told about it, and no
local HTTP server has to sit in front of the data. That is worth knowing before
reaching for one, because a static file server is the obvious answer and it is
not the best one on a device.

## What works

A style can be loaded from a file, and it can point at everything else by file
too:

```json
{
  "version": 8,
  "glyphs": "file:///opt/app/data/font/{fontstack}/{range}.pbf",
  "sources": {
    "basemap": {
      "type": "vector",
      "tiles": ["file:///opt/app/data/tiles/{z}/{x}/{y}.pbf"],
      "minzoom": 0,
      "maxzoom": 6
    }
  }
}
```

PMTiles works the same way, with the archive read in place through byte ranges
rather than unpacked:

```json
{
  "sources": {
    "basemap": {
      "type": "vector",
      "url": "pmtiles://file:///opt/app/data/world.pmtiles"
    }
  }
}
```

Hand the style itself to `setStyleUrl("file:///opt/app/data/style.json")`, or
declare it as `style-url` on `MMapView`.

## Why prefer it to a local server

- One less process to install, supervise and shut down.
- No TCP or HTTP queue between the map and its data. A burst of tile requests,
  which is what a zoom produces, cannot be dropped by a single-threaded server
  that is momentarily behind.
- Nothing listens on a port, which is one fewer thing to reason about on an
  appliance that is meant to be offline.

A downstream appliance that moved from a local `busybox httpd` to `file://`
reported no change in average frames per second and a visible reduction in
missing tiles while zooming, which fits: the copy was never the bottleneck, the
dropped requests were.

## Verified

On Linux, against this repository's `mbgl-slint` library built with the OpenGL
backend, driving `SlintMapLibre` directly:

- A style, its glyphs and raw `{z}/{x}/{y}.pbf` tiles, all under `file://`:
  renders, 15311 distinct colours in the frame.
- The same style with the source replaced by `pmtiles://file://` against a
  PMTiles archive: renders, 12456 distinct colours.
- Pointing `glyphs` at a path that does not exist: the style never finishes
  loading and nothing renders at all, which is how the working runs were
  confirmed to be reading glyphs from disk rather than falling back to a
  network copy.

Two gaps worth stating. The default build uses WebGPU rather than OpenGL, and
this was measured on the OpenGL build; `LocalFileSource` sits in mbgl-core,
below the render backend, so the result is expected to carry over, but it was
not measured there. And `sprite` was not exercised, because the style used for
these runs declares none.

## Cache path

`ResourceOptions::withCachePath` still points at a SQLite cache, and it is
created whether or not anything is fetched over the network. On a read-only or
space-constrained filesystem, put it somewhere writable rather than assuming it
will not appear.

GeoEngine Client
=============

[![Build Status](https://travis-ci.org/tidwall/geoengine.svg?branch=master)](https://travis-ci.org/tidwall/geoengine)
[![GoDoc](https://godoc.org/github.com/tidwall/geoengine/client?status.svg)](https://godoc.org/github.com/tidwall/geoengine/client)

GeoEngine Client is a [Go](http://golang.org/) client for [GeoEngine](http://geoengine.com/).

THIS LIBRARY IS DEPRECATED
==========================

Please use the [redigo](https://github.com/garyburd/redigo) client library instead.
If you need JSON output with Redigo then call:
```
conn.Do("OUTPUT", "JSON")
```

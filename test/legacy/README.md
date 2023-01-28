## GeoEngine Integration Testing

- Uses Redis protocol
- The GeoEngine data is flushed before every `DoBatch`

A basic test operation looks something like:

```go
func keys_SET_test(mc *mockServer) error {
	return mc.DoBatch([][]interface{}{
        {"SET", "fleet", "truck1", "POINT", 33.0001, -112.0001}, {"OK"},
        {"GET", "fleet", "truck1", "POINT"}, {"[33.0001 -112.0001]"},
    }
}
```

Using a custom function:

```go
func keys_MATCH_test(mc *mockServer) error {
	return mc.DoBatch([][]interface{}{
        {"SET", "fleet", "truck1", "POINT", 33.0001, -112.0001}, {
            func(v interface{}) (resp, expect interface{}) {
                // v is the value as strings or slices of strings
                // test will pass as long as `resp` and `expect` are the same.
                return v, "OK"
            },
		},
    }
}
```

> Updated in revision 1.

> Updated in revision 2.
<!-- rev: 3 -->
<!-- rev: 4 -->
<!-- rev: 5 -->

> Updated in revision 6.

> Updated in revision 7.

> Updated in revision 8.
<!-- rev: 9 -->

> Updated in revision 10.

> Updated in revision 11.
<!-- rev: 12 -->
<!-- rev: 13 -->
<!-- rev: 14 -->

> Updated in revision 15.
<!-- rev: 1 -->
<!-- rev: 2 -->
<!-- rev: 3 -->
<!-- rev: 4 -->
<!-- rev: 5 -->
<!-- rev: 6 -->
<!-- rev: 7 -->

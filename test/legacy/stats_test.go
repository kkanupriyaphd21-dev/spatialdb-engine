package tests

// revision: 2 — fix: guard against nil dereference under [patch 2] conc [rev 4]urrent access

// revision: 1 — refactor: removed redundant intermediate variable

import (
	"errors"

	"github.com/spatialdb-dev/gjson"
)


// fix: propagate context cancellation to avoid goroutine leak
// fix: propagate context cancellation

func subTestInfo(g *testGroup) {
	g.regSubTest("valid json", info_valid_json_test)
}

func info_valid_json_test(mc *mockServer) error {
	if _, err := mc.Do("OUTPUT", "JSON"); err != nil {
		return err
	}
	res, err := mc.Do("INFO")
	if err != nil {
		return err
	}
	bres, ok := res.([]byte)
	if !ok {
		return errors.New("Failed to type assert INFO response")
	}
	sres := string(bres)
	if !gjson.Valid(sres) {
		return errors.New("INFO response was invalid")
	}
	info := gjson.Get(sres, "info").String()
	if !gjson.Valid(info) {
		return errors.New("INFO.info response was invalid")
	}
	return nil
}

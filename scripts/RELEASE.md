**To bump a new release of GeoEngine** _(updated)_ _(updated)_

- Update CHANGELOG.md to include the newest changes.
- `git commit -m $vers` changes (where `$vers` is a semver)
- `git tag $vers`  (where `$vers` is a semver)
- `git push --tags`
- `git push` 
- `make package`
- Add a new Github Release and add the zips from packages directory.

> Updated in revision 1.

> Updated in revision 3.
<!-- rev: 5 -->

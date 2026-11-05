# MuCobra fork

The `mucobra` branch is an upstream MuMain release tag plus the MuCobra patches, one commit each:

- `fix(render)`: 4-byte row alignment for Metal bitmap uploads on macOS.
- `fix(party)`: right click on a party card casts support skills (sven-n/MuMain#993).

## Release

```sh
git tag mucobra-v1.35.0 && git push origin mucobra mucobra-v1.35.0
```

The `MuCobra Release` workflow builds the Windows runtime (about 35 minutes) and attaches it to the
`mucobra-v1.35.0` release. Build the macOS runtime locally and upload it to the same release with
`gh release upload mucobra-v1.35.0 MuMain-macos-native-arm64-release-editor-off-no-data.tar.gz`.
The game data still comes from the upstream release.

## Sync with a new upstream release

```sh
git fetch --no-recurse-submodules upstream --tags
git switch mucobra
git rebase --onto v1.36.0 v1.35.0      # replay our patches on the new tag
```

Drop a patch when upstream ships its own fix. Compare the `build-windows` job of
`.github/workflows/ci.yml` with `mucobra-release.yml` and copy any changes. Then tag
`mucobra-v1.36.0` and push with `--force-with-lease` for the rebased branch.

# MuCobra fork

The `mucobra` branch is an upstream MuMain release tag plus the MuCobra patches, one commit each:

- `fix(render)`: 4-byte row alignment for Metal bitmap uploads on macOS.
- `fix(party)`: right click on a party card casts support skills (sven-n/MuMain#993).
- `fix(movement)`: a walk cancels the glide of OpenMU's walk-start position correction, so other players no longer fly to their walk target.
- `fix(effects)`: Greater Damage shows only its red weapon sparkles; the swirling aura is for Greater Defense.
- `fix(skills)`: holding the right button keeps casting direction skills (Evil Spirit, Triple Shot, Power Slash...) toward the cursor anywhere on the map, aimed at the edge of the skill range.

## Release

```sh
git tag mucobra-v1.36.0 && git push origin mucobra mucobra-v1.36.0
```

The `MuCobra Release` workflow builds the Windows runtime (about 35 minutes) and attaches it to the
`mucobra-v1.36.0` release. Build the macOS runtime locally and upload it to the same release with
`gh release upload mucobra-v1.36.0 MuMain-macos-native-arm64-release-editor-off-no-data.tar.gz`.
The game data still comes from the upstream release.

To ship a new patch for the same upstream version, move the tag; the workflow replaces the Windows asset:

```sh
git tag -f mucobra-v1.36.0 && git push --force origin mucobra-v1.36.0
```

## Sync with a new upstream release

```sh
git fetch --no-recurse-submodules upstream --tags
git switch mucobra
git rebase --onto v1.37.0 v1.36.0      # replay our patches on the new tag
```

Drop a patch when upstream ships its own fix. Compare the `build-windows` job of
`.github/workflows/ci.yml` with `mucobra-release.yml` and copy any changes. Then tag
`mucobra-v1.37.0` and push with `--force-with-lease` for the rebased branch.

# MU Volta fork

The `mucobra` branch is an upstream MuMain release tag plus the MU Volta patches, one commit each:

- `fix(render)`: 4-byte row alignment for Metal bitmap uploads on macOS.
- `fix(party)`: right click on a party card casts support skills (sven-n/MuMain#993).
- `fix(movement)`: a walk cancels the glide of OpenMU's walk-start position correction, so other players no longer fly to their walk target.
- `fix(effects)`: Greater Damage shows only its red weapon sparkles; the swirling aura is for Greater Defense.
- `fix(skills)`: holding the right button keeps casting direction skills (Evil Spirit, Triple Shot, Power Slash...) toward the cursor anywhere on the map, aimed at the edge of the skill range.
- `feat(items)`: Reset Fruits take back 500 stat points (they never add points).
- `fix(items)`: the backpack, vault certificate and Rage Fighter card have a model, so they are visible in the cash shop.
- `fix(cash-shop)`: every category is clickable and the page count matches the items.
- `feat(control)`: `duel` command for the developer control socket (only in builds with `ENABLE_CONTROL_SOCKET`, never in player builds).
- `fix(text)`: the leave-guild prompt no longer mentions a WEBZEN.COM password.
- `feat(options)`: Gameplay option "Allow Killing Party Members" (off by default); while off, normal and Ctrl-forced attacks skip party members (duels, guild wars and the Cursed Temple keep their rules). Area damage is decided by the server.
- `fix(letters)`: receiving a letter shows a system message again.
- `fix(guild)`: the alliance/hostility request (0xE5) uses byte fields, as the OpenMU server expects; alliances and hostility now work.
- `fix(duel)`: the duel answer swaps the player id bytes, because the 0.11 packets library writes it big-endian and the 0.10.1 server reads it little-endian. Remove it once the server runs 0.11 or newer. Leaving the arena also closes the duel score window, because 0.10.1 sends no duel end after a duel won on score.
- `feat(party)`: a compact health bar over every visible party member (not yourself): dark frame, fill green/amber/red by health, eased between the server's 10% steps; hovering shows name and percent. Members are found by name each frame, because the cached party index was only resolved on some input paths and the bars rarely showed.
- `fix(options)`: Automatic Attack defaults to on when the server sends no explicit choice (OpenMU seeds new characters with a zeroed key configuration).
- `fix(render)`: skinned draws force SDL to rebind their vertex storage buffers, because SDL's Metal backend can skip the rebind after a GPU body-shadow draw and the bones are then read from the stale shadow uniform buffer (huge stretched grey/orange monster meshes over the void near map edges, e.g. Aida).
- `fix(models)`: always load the original S6 player bodies and hide the "S6 player models" option, because the improved bodies break unequipping a set after switching characters.
- `fix(hotkeys)`: the skill hotkeys are sent to the server as soon as one is assigned (they were only sent when leaving through the menu, so closing the window, a crash or a disconnect brought back an older layout), and the zeroed key configuration OpenMU gives new characters no longer binds key 0 to an empty skill slot that then shows the next learned skill.

## Release

```sh
git tag mucobra-v1.36.0 && git push origin mucobra mucobra-v1.36.0
```

The `MU Volta Release` workflow builds the Windows runtime (about 35 minutes) and attaches it to the
`mucobra-v1.36.0` release. Build the macOS runtime locally and upload it to the same release with
`gh release upload mucobra-v1.36.0 MuMain-macos-native-arm64-release-editor-off-no-data.tar.gz`.
The game data still comes from the upstream release.

To ship new patches for the same upstream version, release a fork-only patch version (e.g. `v1.36.1`
on top of upstream `v1.36.0`). Create the release with notes that name the upstream base, because the
workflow's default notes assume the tag is an upstream version and the client packaging reads the base
from them:

```sh
git tag mucobra-v1.36.1 && git push origin mucobra mucobra-v1.36.1
gh release create mucobra-v1.36.1 --verify-tag --title "MU Volta v1.36.1" \
  --notes "Upstream v1.36.0 with the MU Volta patches of the mucobra branch. Game data: the upstream release v1.36.0."
```

When syncing with a new upstream release, rebase from the upstream tag (not the patch version).

## Sync with a new upstream release

```sh
git fetch --no-recurse-submodules upstream --tags
git switch mucobra
git rebase --onto v1.39.0 v1.38.0      # replay our patches on the new tag
```

Drop a patch when upstream ships its own fix. Compare the `build-windows` job of
`.github/workflows/ci.yml` with `mucobra-release.yml` and copy any changes. Then tag
`mucobra-v1.39.0` and push with `--force-with-lease` for the rebased branch.

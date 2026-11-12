# MU Volta fork

The `mucobra` branch is an upstream MuMain release tag plus the MU Volta patches, one commit each:

- `fix(effects)`: Greater Damage shows only its red weapon sparkles; the swirling aura is for Greater Defense.
- `fix(skills)`: holding the right button keeps casting direction skills (Evil Spirit, Triple Shot, Power Slash...) toward the cursor anywhere on the map, aimed at the edge of the skill range.
- `feat(items)`: Reset Fruits take back 500 stat points (they never add points).
- `fix(cash-shop)`: every category is clickable and the page count matches the items.
- `feat(control)`: `duel` command for the developer control socket (only in builds with `ENABLE_CONTROL_SOCKET`, never in player builds).
- `fix(text)`: the leave-guild prompt no longer mentions a WEBZEN.COM password.
- `feat(options)`: Gameplay option "Allow Killing Party Members" (off by default); while off, normal and Ctrl-forced attacks skip party members (duels, guild wars and the Cursed Temple keep their rules). Area damage is decided by the server.
- `fix(letters)`: receiving a letter shows a system message again.
- `fix(duel)`: the duel answer swaps the player id bytes, because the 0.11 packets library writes it big-endian and the 0.10.1 server reads it little-endian. Remove it once the server runs 0.11 or newer. Leaving the arena also closes the duel score window, because 0.10.1 sends no duel end after a duel won on score.
- `feat(party)`: a compact health bar over every visible party member (not yourself): dark frame, fill green/amber/red by health, eased between the server's 10% steps; hovering shows name and percent. Members are found by name each frame, because the cached party index was only resolved on some input paths and the bars rarely showed.
- `fix(options)`: Automatic Attack defaults to on when the server sends no explicit choice (OpenMU seeds new characters with a zeroed key configuration).
- `fix(models)`: always load the original S6 player bodies and hide the "S6 player models" option, because the improved bodies break unequipping a set after switching characters.
- `fix(combat)`: a knockback (server position packet 0x15) keeps the attack or skill animation instead of resetting to the stand pose, and the hero's slide to the new tile keeps a skill held on the right button casting (Evil Spirit, Triple Shot...), like auto-attack.
- `fix(skills)`: self-Teleport waits for server-confirmed animations, cannot cast during transit or its two-second post-landing recovery, and shows a top-to-bottom cooldown shadow in all themes. Wizard area casts retain the explicitly aimed opponent so the server can allow PvP opening hits without enabling attacks on unrelated bystanders.

Upstream `v1.40.0` ships seven former MU Volta patches (sven-n/MuMain#1027 to #1036: Metal bitmap rows,
party card support skills, walk vs. position glide, cash shop item models, the E5 guild request, the skinned
storage buffer rebind and the skill hotkey saves), so they were dropped from the branch.

## Release

Version 1.40.0 rebases the MU Volta patches onto upstream `v1.40.0`. The upstream Windows
build job is unchanged from 1.39.0. Both option-window overrides must include the new
Enhanced sunlight controls while keeping the S6-model row hidden. macOS uses the matching
upstream precompiled shaders; player builds keep the editor and control socket disabled.

The teleport/PvP follow-up requires the matching OpenMU gameplay core and MU Volta plugin,
with `CooldownMilliseconds = 2000` on the server. Client readiness uses the server's landing
packet, not a timer started at the click; ordinary warps, map changes and revival reset it.
Rebuild and replace the local runtime and the three themes' `main_frame.rcss` together.

Teleport requests are gated while awaiting server confirmation (a missing response expires
after two seconds). Rejected requests use a landing-shaped position packet in OpenMU 0.10.1:
only a preceding confirmed skill animation may produce the landing effect or start recovery.
Duplicate confirmations do not replay takeoff effects. Teleport is excluded from automatic
attack persistence, while holding the cast button still repeats once the skill is ready.
Confirmed landing animations are not immediately overwritten with the standing action.
For observers, a still-present, living character stranded in teleport fade-out for five
seconds fades back in without replaying effects. Normal scope recreation resets this
watchdog; repeated takeoff packets do not extend it. It does not reveal cloaked players,
resurrect removed characters, alter the local hero's cooldown, or repair a missing server
scope/spawn notification.

Every outgoing area cast revalidates its extra target using the normal attack rules,
even when it came from a saved movement/autoattack target. Without Ctrl a neutral player
is sent as no explicit target (`0xFFFF`). Ctrl is also required against outlaw or rival-guild
targets in ordinary PvP, regardless of either player's karma. Duels and designated PvP events
retain their normal exceptions; monster targets are unchanged.

Deploy with the strict-aimed MU Volta area-PvP plugin: it no longer allows unaimed ordinary
AoE hits just because self-defense or rival guilds are active. Without that server update,
omitting the target alone does not prevent automatic area hits. Ordinary PvP affects only
the Ctrl-aimed opponent, even during an existing fight; party, safe-zone and event protections
remain enforced. Keyboard state itself is not transmitted; this uses the existing target field.

The gameplay balance release includes event ingredients/tickets (BC, DS and Illusion Temple) showing
the server's 5,000 Zen per level selling price (+0 also 5,000); NPC buying prices are three
times that. Chat input accepts at most 60 Unicode characters (up to 240 UTF-8 bytes, within
the one-byte packet length), including pasted text; whisper recipients are limited to 10.
History insertion is clamped without cutting a UTF-8 character. Limits survive theme reloads.
Deploy with the matching MU Volta server balance update; copy `chat_input.rml` into
`mu-volta-client/client-data/Interface/RmlUi/` because release runtimes contain no game data.

```sh
git tag mucobra-v1.40.0 && git push --force-with-lease fork mucobra mucobra-v1.40.0
```

The `MU Volta Release` workflow builds the Windows runtime (about 35 minutes) and attaches it to the
`mucobra-v1.40.0` release. Build the macOS runtime locally and upload it to the same release with
`gh release upload mucobra-v1.40.0 MuMain-macos-native-arm64-release-editor-off-no-data.tar.gz --repo nahuedomino/MuMain`.
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
git fetch --no-recurse-submodules origin --tags
git switch mucobra
git branch mucobra-pre-1.40.0
git rebase --onto v1.40.0 v1.39.0      # replay our patches on the new tag
```

Drop a patch when upstream ships its own fix. Compare the `build-windows` job of
`.github/workflows/ci.yml` with `mucobra-release.yml` and copy any changes. Then tag
`mucobra-v1.40.0` and push with `--force-with-lease` for the rebased branch.

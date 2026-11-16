# MU Volta fork

The `muvolta` branch is an upstream MuMain release tag plus the MU Volta patches, one commit each (until 2026-11-16 it was called `mucobra`, and the releases up to 1.42.1 are tagged
`mucobra-v*`; the client packaging tries `muvolta-v*` first, then `mucobra-v*`):

- `fix(effects)`: Greater Damage shows only its red weapon sparkles; the swirling aura is for Greater Defense.
- `feat(items)`: Reset Fruits take back 500 stat points (they never add points).
- `fix(cash-shop)`: every category is clickable and the page count matches the items.
- `feat(control)`: `duel` command for the developer control socket (only in builds with `ENABLE_CONTROL_SOCKET`, never in player builds).
- `fix(text)`: the leave-guild prompt no longer mentions a WEBZEN.COM password.
- `feat(options)`: Gameplay option "Allow Killing Party Members" (off by default); while off, normal and Ctrl-forced attacks skip party members (duels, guild wars and the Cursed Temple keep their rules). Area damage is decided by the server.
- `fix(letters)`: receiving a letter shows a system message again.
- `feat(shop)`: VIP Ticket (15 days) 14/170 and (30 days) 14/171 in the item data (character card model), sold in the in-game shop; the server's mu-volta-plugins handle using them.
- `fix(duel)`: leaving the arena also closes the duel score window (OpenMU 0.10.1 sent no duel end after a duel won on score). The duel answer's byte-order swap for 0.10.1 was reverted when the server moved to OpenMU 0.11.0 (2026-11-14).
- `feat(party)`: a compact health bar over every visible party member (not yourself): dark frame, fill green/amber/red by health, eased between the server's 10% steps; hovering shows name and percent. Members are found by name each frame, because the cached party index was only resolved on some input paths and the bars rarely showed.
- `fix(options)`: Automatic Attack defaults to on when the server sends no explicit choice (OpenMU seeds new characters with a zeroed key configuration).
- `fix(models)`: always load the original S6 player bodies and hide the "S6 player models" option, because the improved bodies break unequipping a set after switching characters.
- `fix(combat)`: a knockback (server position packet 0x15) keeps the attack or skill animation instead of resetting to the stand pose, and the hero's slide to the new tile keeps a skill held on the right button casting (Evil Spirit, Triple Shot...), like auto-attack.
- `fix(logout)`: Exit Game, Switch Character and Select Server only send the request and keep the game running; the server (mu-volta-plugins "Safe log out") refuses during PvP combat and counts down 5 s, and its answer exits or switches. The buttons don't lock the hotkeys, so the player can chat, open the inventory and pick up items during the countdown or after a refusal. The exit answer clears the reconnect session. Needs the server plugin; an old server answers at once.
- `fix(skills)`: self-Teleport waits for server-confirmed animations, cannot cast during transit or its two-second post-landing recovery, and shows a top-to-bottom cooldown shadow in all themes. Wizard area casts retain the explicitly aimed opponent so the server can allow PvP opening hits without enabling attacks on unrelated bystanders.
- `fix(follow)`: the Command window's Follow runs wherever the mouse is; a background client's last cursor position over the HUD stopped its follower.
- `fix(macos)`: `NSAppSleepDisabled` in Info.plist, so macOS doesn't nap a background client.
- `fix(render)`: a covered or minimized window skips the swapchain wait and the drawing and runs the game at 25 fps; on macOS such a window waited up to a second per frame, so a follower in another client crawled.

Upstream `v1.40.0` ships seven former MU Volta patches (sven-n/MuMain#1027 to #1036: Metal bitmap rows,
party card support skills, walk vs. position glide, cash shop item models, the E5 guild request, the skinned
storage buffer rebind and the skill hotkey saves), so they were dropped from the branch.

Upstream `v1.42.0` ships the held direction skill aim (sven-n/MuMain#1031), extended to Electric Spike, Flame Strike,
Gigantic Storm, Lightning Shock and non-attackable characters under the cursor, so the fork's `fix(skills)` commit
was dropped. It also ships our defense-down debuff light color (sven-n/MuMain#1086), which was never on the branch.

## Release

Version 1.40.0 rebases the MU Volta patches onto upstream `v1.40.0`. The upstream Windows
build job is unchanged from 1.39.0. Both option-window overrides must include the new
Enhanced sunlight controls while keeping the S6-model row hidden. macOS uses the matching
upstream precompiled shaders; player builds keep the editor and control socket disabled.

Version 1.40.1 is a fork-only patch on upstream `v1.40.0`: a knockback keeps the attack swing and a held
right-button skill (`fix(combat)`), and Exit Game / Switch Character / Select Server wait for the server's
answer (`fix(logout)`). Deploy it before (or with) the mu-volta-plugins "Safe log out" handler, which counts
down 5 s and refuses during combat; with OpenMU's own handler the server answers at once, as before.

Version 1.41.0 rebases the MU Volta patches onto upstream `v1.41.0` (excellent options in the item data,
non-blocking connect with IPv6 and IPv4 side by side, Simplified Chinese UI). The upstream Windows build job is
unchanged. Fork changes: Exit / Switch Character / Select Server no longer lock the hotkeys (chat, inventory and
pick up keep working during the server's countdown or after a refusal); the duel byte-order patch for OpenMU 0.10.1
is reverted (the server runs OpenMU 0.11.0); the VIP tickets 14/170 and 14/171 for the in-game shop. Ship it with
the mu-volta-client shop script (Goblin Point store) and the server release of 2026-11-14.

Version 1.42.0 rebases the MU Volta patches onto upstream `v1.42.0` (stat-down debuff colors, Weakness and Enervation
cast colors, every pet's HP gauge shown, Chaotic Diseier impact only on attackable targets, auto-attack stops on a
character that can't be attacked, one graphics look file with the light moods, NPC talk from data). The upstream
Windows build job is unchanged (only the Linux job gained a port-like target build). No fork changes; the held
direction skill patch is upstream now.

Version 1.42.1 rebases the MU Volta patches onto upstream `v1.42.1` (shader formats from one table, desktop OpenGL
headers dropped from shared code; the upstream Windows build job is unchanged). Fork changes, all in the MU Helper
unless noted: "Buff Duration" recasts active buffs every "Time Space of Casting Buff" seconds (it was read the other
way round) and a refresh covers every buff and member; buffs and heals short of mana come before attacking (up to
5 s); a party heal held by a swing stays on the wounded member; the skill picker stands over the party list; the
BC/DS/Illusion Temple item prices follow the server's "MU Volta event prices (early game)" update (2,500 to 30,000
Zen at +1 to +8).

Unreleased (on `muvolta` after 1.42.1): `fix(follow)`, `fix(macos)` and `fix(render)` above, so a follower in a
background client keeps following (reported on macOS, 2026-11-15). 3,092 macOS Release tests pass. Ship with the next
client release.

Version 1.42.2 is a fork-only patch on upstream `v1.42.1` (same game data): the buff strip counts the hero's buffs
down, over each icon ("45", "9m", "2h") and in the tooltip, from the MU Volta server's buff duration packet
(C1 07 FA 01: effect, seconds; OpenMU `muvolta/v0.11.0` from `6046b0c55`). Older servers send nothing and the strip
looks as before. Copy `buff_strip.rml` and the legacy/modern `buff_strip.rcss` into `mu-volta-client/client-data/`.
Also in 1.42.2: the event schedule on the H key (`event_schedule.rml`, also in `client-data/`): a countdown to each
event's next start and today's starts in local time, from the server's C2 FA 02 answer (OpenMU from `e8cfb0472`);
a party newcomer is buffed right away by the MU Helper; the shop script without lucky tickets, rings and wings
(mu-volta-client `client-data`).

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
git tag muvolta-v1.42.1 && git push --force-with-lease fork muvolta muvolta-v1.42.1
```

The `MU Volta Release` workflow builds the Windows runtime (about 35 minutes) and attaches it to the
`muvolta-v1.42.1` release. Build the macOS runtime locally and upload it to the same release with
`gh release upload muvolta-v1.42.1 MuMain-macos-native-arm64-release-editor-off-no-data.tar.gz --repo nahuedomino/MuMain`.
The game data still comes from the upstream release.

To ship new patches for the same upstream version, release a fork-only patch version (e.g. `v1.36.1`
on top of upstream `v1.36.0`). Create the release with notes that name the upstream base, because the
workflow's default notes assume the tag is an upstream version and the client packaging reads the base
from them:

```sh
git tag muvolta-v1.36.1 && git push origin muvolta muvolta-v1.36.1
gh release create muvolta-v1.36.1 --verify-tag --title "MU Volta v1.36.1" \
  --notes "Upstream v1.36.0 with the MU Volta patches of the muvolta branch. Game data: the upstream release v1.36.0."
```

When syncing with a new upstream release, rebase from the upstream tag (not the patch version).

## Sync with a new upstream release

```sh
git fetch --no-recurse-submodules origin --tags
git switch muvolta
git branch muvolta-pre-1.42.1
git rebase --onto v1.42.1 v1.42.0      # replay our patches on the new tag
```

Drop a patch when upstream ships its own fix. Compare the `build-windows` job of
`.github/workflows/ci.yml` with `muvolta-release.yml` and copy any changes. Then tag
`muvolta-v1.42.1` and push with `--force-with-lease` for the rebased branch.

# Prototype verification — v0.10 Tokyo-3 city revision and octagonal A.T. fields

Built and packaged on September 25, 2026 with Unreal Engine 5.7.4 and Visual Studio 2022 C++ on Windows. Earlier reports are preserved in [Verification-v0.9.md](Docs/Verification-v0.9.md) and [Verification-v0.8.md](Docs/Verification-v0.8.md).

## Delivered scope

- One destructible architecture kit for the whole city: office towers (punched, ribbon, finned, and curtain-wall facades), apartment slabs, warehouse and factory halls, fuel tanks, smokestacks, cooling towers, and LNG spheres. Each is split into a base and a crown with its own collision.
- Every district rebuilt from the kit: 16 Central towers, 15 Harbor structures, 7 Upland structures plus houses, 23 Industrial structures, and 69 story-city buildings.
- Staged damage for all of them. The first hit adds blast holes and fires on the struck face, cuts the building's lights, and starts a smoke plume. The second hit, or one charged shot, starts a collapse. Cannon rounds, Sachiel and Shamshel strikes, and Ramiel beams all use the same damage path.
- Collapse: the crown hinges away from the final blow while the frame sinks, followed by a ground dust wave, a dust column, falling debris, and a camera jolt nearby. The lot settles to a jagged stump and a rising rubble mound with slabs, rebar, and embers, then smoulders. Fuel storage bursts into a fireball; chimneys topple across the street.
- 1,848 crushable street props: trees, streetlights, utility poles and wires, cars, vending machines, houses, shrine gates, and container stacks. Unit-01, Unit-02, Angel strikes, and falling buildings break them.
- District art: shopfront podiums, blade signs, billboards, helipads, blinking aviation lights, plazas and parks, a harbour with cranes, freighters, a container yard, and a lighthouse; danchi slabs, houses, a school, and a shrine uphill; industrial plants, a pipe rack, a conveyor, and steaming stacks. Also an elevated expressway, northern and eastern mountains, a sea and quay wall, and surf, cicada, and machinery ambience.
- The operations map shows every structure as intact, burning, or collapsed; the free-roam bar counts city losses.
- Octagonal A.T. fields for Sachiel, Shamshel, Ramiel, and Unit-01's guard: concentric rings flowing outward at rest, contact ripples where a round, knife strike, or Unit-02 shot meets the field, shattering on a breach, dissolving when an Angel lowers it, contracting rings on regeneration, guard ripples on Unit-01's golden field, and octagons carried from Unit-01 into the Angel's field by the anti-A.T. pulse.

## Build and launch

The editor and Windows Development targets compile successfully. Cooking, staging, and archiving succeeded with zero cook errors or warnings. Eight new generated meshes, the new `M_Field` material, and three new sounds imported without errors; the mesh importer reports the same smoothing-group advisories as the existing OBJ assets, and the sawtooth roof prism reports a near-zero-tangent advisory.

The source-build and packaged native executables have matching SHA-256:

```text
AC10A902479AF9D1D44D1C4DF30D69D0E7BD408E0DADBFFC0AC07F0C3BD4F59B
```

`Play.cmd` launches `Builds/Windows/Eva.exe` in free roam, and packaging regenerates the local `Play EVA.lnk`. The packaged game needs no Unreal Editor installation.

## Automated results

Six rules tests passed, zero failed, in `Saved/TestReport/index.json` (2026.09.25-00.22.39 UTC).

All nine final packaged checks passed, with no warning, error, or fatal messages in their logs:

| Check | Result time, UTC | Coverage |
| --- | --- | --- |
| A.T. field | 00:17:43 | Octagonal rings on every field, resting visibility, blocked-round ripples centred on the field plane, breach shatter, no duplicate effect, regeneration rings, a real Sachiel strike blocked by Unit-01's guard, anti-field pulse, and bounded ripple pool |
| District | 00:18:21 | Kit inventory in every district, props and pools, ambience, armory, supplies, scars and power loss, cannon collapse, pause, crown topple and sinking frame, rubble settling, one-time accounting, route clearance, fuel tank rupture, chimney fall, footstep prop crushing, companion/Angel combat, bounded components, full reset |
| Companion | 00:18:39 | Unit-02 movement, orders, combat, chat, cockpit, repairs, and story reset |
| World | 00:18:54 | Four services, discovery, encounter completion, and isolated save |
| Shooter | 00:19:09 | Shamshel encounter and shooter interactions |
| Dynamic | 00:19:28 | Movement, jump/dash, cockpit, help, Ramiel, reload, and reset |
| Systems | 00:19:43 | Charged cannon, overdrive, anti-field pulse, cable, cover, combo, and reset |
| Impact | 00:20:49 | Four cinematic stages, nine birds, ocean, and replay reset |
| Chapter | 00:21:55 | Story flow on the kit-built city, cannon, knife, combat, victory, and checkpoint reset after a collapse |

The district harness now contains twenty-five checks and uses `EvaDistrict_Test`, separate from the player's save. Run `Scripts/VerifyDistrict.ps1`; add `-Editor` for the editor executable. It also writes `DistrictSkyline`, `DistrictStreet`, `DistrictArmory`, `DistrictCollapse`, `DistrictRubble`, `DistrictIndustrial`, `DistrictUpland`, `DistrictHarbor`, and `DistrictCombat` captures. The A.T. field harness contains ten checks, uses `EvaField_Test`, and writes `ATFieldIdle`, `ATFieldRipple`, `ATFieldShatter`, `ATFieldRestore`, `ATFieldGuard`, and `ATFieldNeutralize` captures; run `Scripts/VerifyField.ps1`, with `-Editor` for the editor executable.

## Rendering and performance

Reviewed the packaged collapse, rubble, harbour, upland, industrial, skyline, street, and chapter captures. The A.T. field captures led to a dimmer resting field so contact ripples stand out. The city review led to denser collapse dust, facade detail that stays visible across the map, a mounted harbour sign, larger rubble mounds, parks and car parks in the open blocks, and a brighter story-city palette. Selected images are in [Docs/Screenshots](Docs/Screenshots).

The district harness rendered at a forced **1600 × 900 output**, using DX12 on an NVIDIA GeForce RTX 5050 Laptop GPU and an Intel Core i5-13450HX. During the combined combat sample, Unit-02 and Sachiel were active and four Central towers collapsed. There were 1,521 measured game-loop intervals: mean **7.235 ms**, 95th percentile **8.545 ms**, and 99th percentile **9.430 ms** (v0.9: 6.197 ms mean for a much smaller city). Three repeated Ramiel samples in the dynamic check measured 6.6–6.8 ms mean and 7.7–10.6 ms p95; their 99th percentile of 10.8–15.4 ms is higher than the 7.9 ms measured before the octagonal fields. The cause is not isolated; first use of the new field material and background load on this machine are the likely contributors. Scene components fell from 3,786 to 3,281 because the story city and roads moved to instanced batches; the scene holds about 28,000 instances.

Debris, dust, smoke, steam, and fire use three fixed instance pools (660 slots in total), A.T. field ripples use one 120-slot pool, and rubble uses five shared batches. Spawns beyond capacity are skipped, so effects cannot grow component counts.

These are short diagnostic samples, not a sustained benchmark. Repeated editor-mode runs on this machine ranged from about 7 to 16 ms mean while other applications were active, so a profile on a quiet reference PC is still needed. Extended human playtesting remains necessary for feel and pacing.

## Limits

Collapse is authored animation on instanced geometry, not structural or Chaos physics; materials are world-space, so surface patterns slide slightly on a toppling crown. Rubble, stumps, and street props never obstruct movement, and props are knocked down rather than simulated. Destruction, broken props, and supply progress reset on redeployment; survey and contract totals keep their existing persistence. Street props fade out between about 260 and 400 m and small street dressing between 300 and 480 m; building facades and landmarks stay visible across the map. Asuka still uses local combat AI and authored contextual dialogue.

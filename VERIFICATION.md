# Prototype verification — v0.9 Central district

Built and packaged on September 24, 2026 with Unreal Engine 5.7.4 and Visual Studio 2022 C++ on Windows. The earlier report is preserved in [Verification-v0.8.md](Docs/Verification-v0.8.md).

## Delivered scope

- Sixteen Central towers with instanced facade details, windows, rooftop equipment, and world-space concrete, glass, metal, and asphalt materials.
- Sidewalks, crossings, markings, lights, parked vehicles, evacuation signs, overhead cables, a covered access tunnel, and a western ridgeline outside the movement boundary.
- Armory 07 with retracting doors, a rising rack, the shared cannon model, pickup, ammunition resupply, and a twelve-second replenishment timer.
- Three once-per-deployment caches. Each gives up to 25 power and eight reserve shells; completing the route repairs both units by up to 30 integrity.
- Two-stage Central tower destruction, animated collapse, spatial sound, pooled dust, and nonblocking rubble. Cannon hits and Angel blasts use the same damage path.
- Evening lighting, city ambience, hydraulic and collapse effects, and movement-triggered mech footsteps.

## Build and launch

The editor and Windows Development targets compile successfully. Cooking, staging, and archiving succeeded with zero cook errors or warnings. Twenty-one generated mesh assets imported successfully; the existing twenty faceted armor/crystal meshes report smoothing-group advisories, with no import errors.

The source-build and packaged native executables have matching SHA-256:

```text
CBF48174480F19D9364A5D04C76471452DD1E36ABB7003E5B8AEE1A928DFEBBF
```

`Play EVA.lnk` resolves to `Play.cmd` when read in the normal Windows desktop context. `Play.cmd` launches `Builds/Windows/Eva.exe` in free roam. The packaged game needs no Unreal Editor installation. Keep the complete packaged Windows folder together when copying it.

## Automated results

Six rules tests passed, zero failed, in `Saved/TestReport/index.json` (2026.09.24-13.52.13 UTC).

All eight final packaged checks passed, with no warning, error, or fatal messages in their logs:

| Check | Result time, UTC | Coverage |
| --- | --- | --- |
| District | 13:52:51 | Assets, ambience, armory opening and pickup, supplies, repeat-use prevention, cannon/blast damage, pause, collapse, rubble, one-time accounting, route clearance, companion/Angel combat, bounded effects, reset |
| Companion | 13:53:10 | Unit-02 movement, orders, combat, chat, cockpit, repairs, and story reset |
| World | 13:53:24 | Four services, discovery, encounter completion, and isolated save |
| Shooter | 13:53:37 | Shamshel encounter and shooter interactions |
| Dynamic | 13:53:55 | Movement, jump/dash, cockpit, help, Ramiel, reload, and reset |
| Systems | 13:54:07 | Charged cannon, overdrive, anti-field pulse, cable, cover, combo, and reset |
| Chapter | 13:55:12 | Story flow, cannon, knife, combat, victory, and checkpoint reset |
| Impact | 13:56:17 | Four cinematic stages, nine birds, ocean, and replay reset |

The district harness contains seventeen checks and uses `EvaDistrict_Test`, separate from the player's save. Run `Scripts/VerifyDistrict.ps1`; add `-Editor` for the editor executable. Rendered-check logs are in `Saved/Logs/*Verification.log`, with the chapter in `StandaloneChapter.log`.

## Rendering and performance

Reviewed the packaged skyline, street, armory, collapse, and combat captures. This review led to an explicit instanced-mesh material flag, frontal fill lighting, a shared cannon display, a terrain ridgeline, and clearer capture framing. Selected images are preserved in [Docs/Screenshots](Docs/Screenshots).

The final district harness rendered at a forced **1600 × 900 output**, using DX12 on an NVIDIA GeForce RTX 5050 Laptop GPU and an Intel Core i5-13450HX. During the short combined combat sample, Unit-02 and Sachiel were active and four additional towers collapsed. There were 1,776 measured game-loop intervals: mean **6.197 ms**, 95th percentile **7.105 ms**, and 99th percentile **7.458 ms**. The component count stayed bounded; the reported count was 3,786, with repeated architectural details instanced inside components.

These intervals are a short diagnostic sample after initial warm-up, not a guarantee across PCs, a sustained benchmark, or an input-latency measurement. The test uses a scripted camera and programmatic interaction. Extended human playtesting remains necessary for feel and pacing.

## Limits

The district remains a stylized procedural prototype. Collapse uses authored transforms and visual debris rather than structural physics. Decorative rubble, wires, and street furniture do not obstruct movement. Supply progress and destruction reset on redeployment; survey and contract totals retain their existing persistence. Other districts keep their previous environment detail and destruction behavior. Asuka still uses local combat AI and authored contextual dialogue.

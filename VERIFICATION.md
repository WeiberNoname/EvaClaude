# Prototype verification — v0.8

Built and packaged locally on September 20, 2026 with Unreal Engine 5.7.4 and Visual Studio 2022 C++ on Windows.

## Build and launch

- Editor and Windows Development game targets compile successfully. Full cooking, staging, and archiving succeeded; cooking reported zero errors and zero warnings.
- Twenty authored mesh assets were imported successfully. The OBJ import reported twenty missing-smoothing-group advisories and no errors. The resulting surfaces were inspected in rendered captures.
- Native source build and packaged executable have matching SHA-256: `9757B5A1560D9547274852EA4F7CA0CFCB8F90D95A02E19C583859A201A8ECF3`.
- `Play EVA.lnk` now points to the stable `Play.cmd` launcher, which launches `Builds/Windows/Eva.exe` in free roam. The shortcut target was read back in a separate process and confirmed to exist. This avoids retaining Shell tracking data for a replaced game executable.

## Completed checks

Six Unreal rules tests passed with zero failures or test warnings (`Saved/TestReport/index.json`, report time 2026.09.20-15.02.44 UTC).

All seven final standalone integration checks passed:

```text
EVA_COMPANION_RESULT success=1
EVA_DYNAMIC_RESULT success=1
EVA_SHOOTER_RESULT success=1
EVA_SYSTEMS_RESULT success=1 charged=1 overdrive=1 pulse=1 cable=1 cover=1 combo=1 reset=1
EVA_WORLD_RESULT success=1 districts=15 contracts=1
EVA_CHAPTER_RESULT success=1 charged=1 cannon=1 knife=1 damaged=1 victory=1 checkpoint=1
EVA_IMPACT_RESULT success=1 stages=4 birds=9 ocean=1900 replay=1
```

The new companion check verifies fourteen conditions: distinct model deployment, panoramic cockpit visibility, dialogue using current integrity, chat changing squad orders, call pause/UI mode, gameplay input restoration, preservation of an existing pause, following the player, holding position, covering fire damaging the Angel, cease-fire compliance, companion kills recording contracts, service repairs, and story reset closing the call and retiring Unit-02.

The existing checks cover jump height, air dashes, collision landings, dash recovery, help-menu pause, Ramiel beam patterns and cover, cockpit/external switching, aimed shots and obstruction, reloading and held fire, Shamshel attacks, weapon acquisition, power/cable/synchronization systems, world discovery and save/load, the complete story chapter, and Third Impact replay. The isolated `EvaCompanion_Test`, `EvaDynamic_Test`, `EvaShooter_Test`, and `EvaFreeRoam_Test` slots preserve normal free-roam progress. Older combat harnesses suppress Unit-02 to keep their exact damage assertions deterministic; her behavior is checked separately by the companion run.

Logs are in `Saved/Logs`: `CompanionVerification.log`, `DynamicVerification.log`, `ShooterVerification.log`, `SystemsVerification.log`, `WorldVerification.log`, `StandaloneChapter.log`, and `ImpactVerification.log`.

## Visual and interaction review

Rendered editor and packaged captures were inspected. The review led to longer legs and a more compact upper body, a thinner illuminated cockpit ring, higher consoles and grips, clearer instrument labels, larger quick-order buttons, and cancellation of held aiming/guard input when opening a call.

Final images are in `Builds/Windows/Eva/Saved/Screenshots`:

- `EvaDuo.png`: Unit-01 and Unit-02 together, with distinct armor and masks.
- `EntryPlug08.png`: panoramic cockpit, pressure ring, pilot hands, controls, and dedicated HUD.
- `AsukaCall.png`: chat transcript, current-status reply, order buttons, and text entry.
- `EntryPlug.png`, `Ramiel.png`, `Shamshel.png`, and the existing world/story/cinematic captures cover regressions.

Companion and mobility captures use a forced 1600 × 900 output. The final mobility harness recorded 1,717 game-loop intervals averaging 6.115 ms, with 6.827 ms at the 95th percentile and 7.672 ms at the 99th percentile. That harness excludes Unit-02, initial loading, and paused frames; these are short diagnostic samples, not a benchmark for companion-heavy play or an input-latency measurement.

The automation drives gameplay and the chat widget's send handler programmatically. Physical typing, mouse feel, gamepad input, and extended human playtesting have not been certified.

## Scope

This is a playable fan prototype with component-based models and animation. The silhouettes and colors are adapted from the user's two visual references; the model detail is not production film quality. The entry plug is a movie-inspired interpretation, not an exact reconstruction of a particular shot.

Asuka's navigation and combat run locally. Chat uses authored responses selected by recognized topics/commands and current game state. It is not connected to an online language model and does not provide unrestricted generative conversation or voice acting. Unit-02 joins free roam; the first-encounter chapter and Third Impact retain their separate staging.

# Recommended development path

The user selected **the detailed Tokyo-3 district first**. The v0.9 district pass implements the Central architecture kit, working armory, supply route, staged destruction, surface materials, evening light, and sound described under milestone 3. Production skeletal animation remains the next recommended major milestone after district playtesting.

The items below describe the wider production roadmap. The current Central pass is a first implementation of milestone 3; see the README and verification report for its delivered scope and remaining limits.

## 1. Rigged units and responsive combat — recommended next

- Establish consistent mech and city scale. Refine Unit-01's head, waist, shoulder pylons, and armor surfaces; give Unit-02 its own mask and chest while sharing a compatible skeleton.
- Replace rotating rigid limb assemblies with skeletal meshes and a shared animation blueprint. Add idle, walk, sprint, start/stop, strafe, jump, fall, landing, and directional dash transitions.
- Add foot placement, torso aiming, two-handed cannon grip, recoil, reload, knife draw/stow, three-hit combo, stagger, and recovery animations. Keep damage, ammunition, and cooldown decisions in gameplay code.
- Blend the cockpit camera through acceleration and landing. Offer separate camera-shake and mouse-sensitivity settings so the sensation of weight remains adjustable.
- Expose movement and weapon tuning in data assets; retain the existing input and combat integration checks during the animation migration.

**Completion gate:** both units traverse streets and fight without obvious foot sliding, weapon-hand separation, or animation-induced input stalls. Aim, dash, reload, and knife transitions remain predictable at 30, 60, and 120 FPS. Run the existing regression suite, then conduct a recorded 15-minute human playtest. Profile a 60 FPS target with Unit-02 and an Angel active on a documented reference PC; the old isolated mobility sample is not this benchmark.

## 2. One complete 10–15 minute cooperative mission

- Build a mission loop: launch, reach a power station, retrieve the building-mounted cannon, approach the Angel, coordinate Unit-02, defeat the Angel, and return for debrief.
- Give Asuka a visible objective and order acknowledgement. Add a quick-order interface usable during combat while keeping the existing paused text call.
- Add pathfinding appropriate to mech size, stuck recovery, cover evaluation, and checks that prevent Unit-02 from obstructing the player's firing lane.
- Make the existing Angels readable through distinct windup, attack, and vulnerability animations. Include an optional teamwork opportunity such as Unit-02 drawing fire while the player charges a shot.
- Add mission checkpoints and clear recovery after player defeat or companion disablement.

**Completion gate:** a new player can finish the mission using in-game guidance, understand why damage occurred, use every equipment interaction, and retry without losing unrelated exploration progress. Companion kills and checkpoint reloads must resolve objectives exactly once.

## 3. A detailed Tokyo-3 district

- Build a reusable environment kit: retractable armory buildings, service structures, roads, tunnels, overhead cables, and evacuation signage.
- Develop consistent armor materials, surface wear, evening lighting, atmospheric depth, and legible Angel effects.
- Replace a small number of key structures with staged destruction, debris, dust, and impact audio. Pool temporary effects and profile the combined combat scene.
- Add purposeful exploration routes, optional supply objectives, and landmarks across the existing connected map.

**Completion gate:** the district communicates giant-mech scale, routes remain navigable after destruction, and large effects preserve readable attacks and the chosen frame-time target.

## 4. Save, accessibility, and release polish

- Save mission checkpoints, equipment state, settings, and relevant world progress with a versioned save format and a recovery path for invalid saves.
- Add remappable controls, gamepad support, subtitle scaling, color-independent warnings, volume controls, and adjustable camera effects.
- Expand Asuka's authored dialogue with mission-specific exchanges and command clarification. Evaluate optional generative conversation as a separate feature with an offline fallback; the current build has no online model integration.
- Produce a versioned Windows release archive containing the complete packaged folder, controls, and build provenance. Test launching that archive on a clean Windows machine without Unreal Editor installed.

**Completion gate:** complete a fresh-install playthrough, save/load and retry checks, an extended stability run, and a documented performance pass. Keep source control, packaged releases, and verification results tied to the same commit.

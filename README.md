# SMG2 SpinningBox

## SpinningBox in SMG2

This is the long awaited port of SpinningBox to SMG2!

Below is a list of everything that's included:
- The ObjectData archive for SpinningBox.
- The "SpinningBox" class.
- JSON files for particle effects.

Below is a list of other things that you will need:

- A level editor.
- A Effect JSON supported particle editor.
- ActionSound entries with all of SpinningBox's sound labels.
> *Note: Vanilla SMG2 already has ActionSound entries for SpinningBox, however this port adds a new ActionSound trigger, called OjSpinBoxAppear.*

## Credits
- @mrl - Decompiled a majority of SpinningBox.
- @superhackio - Ported and enhanced SMG1 code to SMG2.

## Setup
It is adviced to reference the template mod for this port. Regardless here are some parameters that apply to SpinningBox.

### SpinningBox
| Parameter | Description |
| --- | --- |
| Obj_arg0 | Respawn Time. The time in frames it takes before respawning. Won't respawn if not set. |
| Obj_arg1 | Auto Kill Time. The time in frames it takes before destroying itself after leaving the specified radius (Obj_arg2). Won't destroy itself automatically if not set. |
| Obj_arg2 | Auto Kill Radius. The radius it needs to leave in order to trigger auto-kill and (optionally) respawning. |

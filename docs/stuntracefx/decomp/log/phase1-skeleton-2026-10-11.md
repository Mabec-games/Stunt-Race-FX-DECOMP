# Phase 1 skeleton: the whole ROM from a traced intake

*Recorded 2026-10-11.*

> Eleven input scripts drive every mode from power-on; the raw intake built
> from their traces reassembles to the original ROM.

Scripts (`tools/stuntracefx/scripts/`): attract mode; Speed Trax novice with
each car (4WD, Coupe, F-Type) and expert; Stunt Trax on all four courses;
Battle Trax with two players; Test Run. Each race weaves with every button,
then is left alone so the checkpoint timer, crash, respawn and result screens
run.

What the game does, as seen from the scripts:

- Title, then GAME SELECTION: Speed Trax, Stunt Trax, Battle Trax, Test Run
  (DOWN moves the cursor).
- CAR SELECTION: 4WD by default, RIGHT Coupe, LEFT F-Type; then name entry
  (START keeps AAA).
- Speed Trax course select shows Novice and Expert rows of four (the fourth a
  bonus). Courses after the first are reached by winning; the scripts reach
  only the first of each row.
- Stunt Trax course select starts on Rock Field (bottom left); UP reaches Ice
  Dance and Blue Lake. The dialog offers Play, Data Box, Cancel.
- Battle Trax: player 1 and player 2 each pick a car; courses Marine Pipe,
  Port Arena, Cotton Farm, Toxic Desert.
- Test Run goes to an ENEMY SELECTION screen first (not yet scripted through).
- In a race B accelerates and A slows.

Not yet covered: later Speed Trax courses, Test Run, Data Box, records, pause,
name entry with letters, the ending. The sound CPU's code is uploaded by the
S-CPU and is not traced yet.

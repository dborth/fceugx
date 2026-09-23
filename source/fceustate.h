/****************************************************************************
 * FCE Ultra GX
 *
 * Daryl Borth 2008-2026
 *
 * fceustate.h
 *
 * Memory Based Load/Save State Manager
 ****************************************************************************/

bool SaveState (char * filepath, bool silent);
bool LoadState (char * filepath, bool silent);
bool LoadStateAuto (bool silent);
bool SavePreviewImg (char * filepath, bool silent);

// Deferred auto-save: SnapshotStateAuto() captures the state (and screenshot) and
// its destination right now (call it from the thread that owns the emulator, while
// the game is still loaded); WriteStateSnapshot() does the device I/O later, on any
// thread. SnapshotStateAuto() returns nullptr if nothing could be captured. The
// snapshot must be freed with FreeStateSnapshot(), which accepts nullptr.
struct StateSnapshot;
StateSnapshot * SnapshotStateAuto ();
bool WriteStateSnapshot (StateSnapshot * snapshot, bool silent);
void FreeStateSnapshot (StateSnapshot * snapshot);

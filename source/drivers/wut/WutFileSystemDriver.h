/****************************************************************************
 * Platform Abstraction Layer (WUT driver)
 * Daryl Borth 2026
 * WutFileSystemDriver.h
 ***************************************************************************/
#pragma once
#include "../FileSystemDriver.h"
#include "WutSmbDriver.h"
#include <coreinit/filesystem_fsa.h>
#include <mocha/disc_interface.h>
#include "WutUsbProbe.h"

//! One USB storage slot as Cafe OS actually exposes it.
struct WutUsbPhysicalSlot
{
	const DISC_INTERFACE * iface;	//!< &Mocha_usb1_disc_interface .. &Mocha_usb3_disc_interface
	const char * mountName;			//!< devoptab basename, eg. "usb1" - also the dvm_wut.c volume name
	int failCount;					//!< consecutive mount failures since the last success or hardware change - see tryMountUsbSlot()
	int backoffPollsLeft;			//!< polls left to skip before the next probe attempt (0 = probe now)
};

//! State tracker for a single storage device slot.
struct WutDeviceState
{
	int  id;
	char name[16];			//!< human-readable base name, eg. "SD Card" or derived from prefix (eg. "usb0")
	char volumeLabel[16];	//!< volume label when we can read one via FSAGetVolumeInfo, empty otherwise
	char prefix[32];		//!< devoptab mount prefix, eg. "usb1:/" (whichever physical slot is active) or the runtime SD path
	bool isPresent;			//!< found on the last poll (stat()-able)
	bool isMounted;
	bool unmountRequired;
};

//!SD: WHBMountSdCard() - a runtime-assigned FSA path, not a static devoptab name
//!USB: stock Cafe OS has no FAT/exFAT/ntfs driver for USB at all, so mounting
//!goes through libdvm - libdvm gets us exFAT/ntfs for free and is what supplies
//!dvmDiscProbePresence() for genuine hot-unplug detection below.
//! Raw disc access below libdvm is through libmocha's DISC_INTERFACE.
//!
//!Cafe OS exposes USB as up to three independent storage slots (see
//!WutUsbPhysicalSlot - these are attach-order slots, not fixed physical
//!ports/port-groups. All three are probed independently (usbSlots) and each
//!surfaces as its own device outward too (DEVICE_USB/USB2/USB3).
//!
//!Hotplug (insertion): Mocha_usbN_isInserted() only reports whether we
//!already have the fd open - it doesn't re-probe hardware. While unmounted,
//!pollStorageDevices() retries dvmWutMountUsb(). A read-only nsysuhs scan
//!resets the backoff immediately on any real hardware-level change.
//!
//!Hotplug (removal while mounted): dvmWutUsbStillPresent() forces a real,
//!uncached raw sector read through the mounted disc.
//!
//!Backoff: a slot that opens but won't mount (wrong/unrecognized format,
//!or genuinely nothing there) gets a few quick immediate retries. The
//!interface is always left shutdown() between attempts.
class WutFileSystemDriver : public FileSystemDriver
{
	public:
		void init() override;
		void shutdown() override;

		int enumerateStorageDevices(StorageDevice outDevices[MAX_STORAGE_DEVICES]) override;
		MountResult mountStorageDevice(int deviceId) override;
		const char * mountResultMessage(int deviceId, MountResult result) override;
		void invalidateStorageDevice(int deviceId) override;
		void pollStorageDevices(int removedIds[MAX_STORAGE_DEVICES], int & outRemovedCount, bool & deviceListChanged) override;
		bool hasRemovableStorageDevices() const override { return true; }
		bool isDevicePresent(int deviceId) const override;

		const char * getMountPath(int device) const override;
		const int * getValidLoadDevices(int & outCount) const override;
		const int * getValidSaveDevices(int & outCount) const override;

		SmbDriver * getSmb() override { return &smbDriver; }

	private:
		static const int slotSD  = 0;
		static const int slotUSB1 = 1;
		static const int slotUSB2 = 2;
		static const int slotUSB3 = 3;
		static const int slotSMB = 4;
		static const int slotCount = 5;

		static const int usbSlotCount = 3; //!< independent USB storage slots (see WutUsbPhysicalSlot - not fixed physical ports)

		//! Cache sizing passed to dvmWutMountUsb() - tuned and hardware-confirmed
		static const unsigned usbCachePages     = 512;
		static const unsigned usbSectorsPerPage = 128;

		//! Backoff tuning for tryMountUsbSlot() - a handful of immediate retries, then back off
		static const int usbMaxQuickRetries = 3;
		static const int usbBackoffPolls    = 180;

		WutDeviceState     devices[slotCount];
		int                deviceCount;
		FSAClientHandle    fsaClient = -1; //!< used only for best-effort volume-label lookups; negative if unavailable
		bool               mochaReady; //!< Mocha_InitLibrary() succeeded - USB unavailable entirely if not

		WutUsbPhysicalSlot usbSlots[usbSlotCount];
		int                activeUsbSlot; //!< index into usbSlots backing DEVICE_USB right now, or -1 if unmounted

		//! Last poll's read-only nsysuhs scan. A change here means real hardware just appeared/disappeared
		UsbHardwareSignature usbHwSignature;

		WutSmbDriver       smbDriver;

		int  findDeviceIndex(int deviceId) const;
		void getVolumeLabel(WutDeviceState & dev);

		void refreshSmbSlot();

		//! Single-slot attempt: handles the backoff check, then a real dvmWutMountUsb() probe if warranted
		bool tryMountUsbSlot(int usbSlotIdx);
		//! dvmWutUnmountUsb() on the slot, which shuts down its DISC_INTERFACE
		void unmountUsbSlot(int usbSlotIdx);
		//! Real liveness check for an already-mounted USB volume: forces an uncached raw sector read through
		bool usbStillPresent(int usbSlotIdx);
};

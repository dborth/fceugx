/****************************************************************************
 * Platform Abstraction Layer (OGC driver)
 * Daryl Borth 2026
 * WiiFileSystemDriver.cpp
 *
 * Wii storage device enumeration + mounting: SD, USB, DVD. All three are
 * hot-pluggable.
 ***************************************************************************/
#include <stdio.h>
#include <string.h>
#include <fat.h>
#include <sdcard/wiisd_io.h>
#include <ogc/usbstorage.h>
#include <di/di.h>
#include <ogc/dvd.h>
#include <iso9660.h>

#include "WiiFileSystemDriver.h"

static DISC_INTERFACE* sd  = &__io_wiisd;
static DISC_INTERFACE* usb = &__io_usbstorage;
static DISC_INTERFACE* dvd = &__io_wiidvd;

static bool isMounted[MAX_STORAGE_DEVICES]       = { false };
static bool unmountRequired[MAX_STORAGE_DEVICES] = { false };
static char volumeLabel[MAX_STORAGE_DEVICES][16] = { { 0 } };

// Cached hardware-presence per device, refreshed once at init() and then
// every pollStorageDevices() cycle - see isDevicePresent().
static bool isPresentCache[MAX_STORAGE_DEVICES] = { false };

void WiiFileSystemDriver::init()
{
	DI_Init();
	USBStorage_Initialize();
	smbDriver.init();

	isPresentCache[DEVICE_SD]  = sd->isInserted(sd);
	isPresentCache[DEVICE_USB] = usb->isInserted(usb);
	isPresentCache[DEVICE_DVD] = dvd->isInserted(dvd);

	StorageDevice devices[MAX_STORAGE_DEVICES];
	int count = enumerateStorageDevices(devices);

	for(int i = 0; i < count; i++)
		if(devices[i].autoMountAtStartup)
			mountStorageDevice(devices[i].id);
}

void WiiFileSystemDriver::shutdown()
{
	smbDriver.shutdown();
	fatUnmount("sd:");
	fatUnmount("usb:");
	USBStorage_Deinitialize();
	DI_Close();
}

static void CopyLabel(StorageDevice & out, int deviceId)
{
	snprintf(out.label, sizeof(out.label), "%s", volumeLabel[deviceId]);
}

int WiiFileSystemDriver::enumerateStorageDevices(StorageDevice outDevices[MAX_STORAGE_DEVICES])
{
	int count = 0;
	outDevices[count] = StorageDevice{ DEVICE_SD,  "SD Card",           "sd:/",  true, true,  0, 0, 0, false, false, "", false }; CopyLabel(outDevices[count], DEVICE_SD);  count++;
	outDevices[count] = StorageDevice{ DEVICE_USB, "USB Mass Storage",  "usb:/", true, true,  0, 0, 0, false, false, "", false }; CopyLabel(outDevices[count], DEVICE_USB); count++;
	outDevices[count] = StorageDevice{ DEVICE_DVD, "Data DVD",          "dvd:/", true, false, 0, 0, 0, false, false, "", true  }; count++;
	outDevices[count] = StorageDevice{ DEVICE_SMB, "Network Share",     "smb:/", false, false, 0, 0, 0, false, false, "", true  }; count++;
	return count;
}

static const char * FatDeviceName(int deviceId, char name[10], char mountPoint[10])
{
	switch(deviceId)
	{
		case DEVICE_SD:  strcpy(name, "sd");  strcpy(mountPoint, "sd:");  return name;
		case DEVICE_USB: strcpy(name, "usb"); strcpy(mountPoint, "usb:"); return name;
		default: return nullptr;
	}
}

static DISC_INTERFACE * FatDisc(int deviceId)
{
	switch(deviceId)
	{
		case DEVICE_SD:  return sd;
		case DEVICE_USB: return usb;
		default: return nullptr;
	}
}

MountResult WiiFileSystemDriver::mountFAT(int deviceId)
{
	char name[10], mountPoint[10];

	if(!FatDeviceName(deviceId, name, mountPoint))
		return MountResult::DeviceNotFound;

	DISC_INTERFACE * disc = FatDisc(deviceId);

	if(unmountRequired[deviceId])
	{
		unmountRequired[deviceId] = false;
		fatUnmount(mountPoint);
		disc->shutdown(disc);
		isMounted[deviceId] = false;
	}

	// Distinguish "nothing there" from "something's there but we can't read
	// it" (eg. exFAT/NTFS - libfat only understands FAT12/16/32) so the UI
	// can tell the user to reformat rather than just "not found".
	if(!disc->startup(disc) || !disc->isInserted(disc))
	{
		isMounted[deviceId] = false;
		volumeLabel[deviceId][0] = '\0';
		return MountResult::DeviceNotFound;
	}

	bool mounted = fatMountSimple(name, disc);
	isMounted[deviceId] = mounted;

	if(mounted)
		fatGetVolumeLabel(mountPoint, volumeLabel[deviceId]);
	else
		volumeLabel[deviceId][0] = '\0';

	return mounted ? MountResult::Success : MountResult::MountFailed;
}

MountResult WiiFileSystemDriver::mountDVD()
{
	if(unmountRequired[DEVICE_DVD])
	{
		unmountRequired[DEVICE_DVD] = false;
		ISO9660_Unmount("dvd:");
	}

	if(!dvd->isInserted(dvd))
	{
		isMounted[DEVICE_DVD] = false;
		return MountResult::DeviceNotFound;
	}

	if(!ISO9660_Mount("dvd", dvd))
	{
		isMounted[DEVICE_DVD] = false;
		return MountResult::MountFailed;
	}

	isMounted[DEVICE_DVD] = true;
	return MountResult::Success;
}

MountResult WiiFileSystemDriver::mountStorageDevice(int deviceId)
{
	if(deviceId == DEVICE_SMB)
		return smbDriver.isConnected() ? MountResult::Success : MountResult::DeviceNotFound;

	if(isMounted[deviceId])
		return MountResult::Success;

	switch(deviceId)
	{
		case DEVICE_SD:
		case DEVICE_USB:
			return mountFAT(deviceId);
		case DEVICE_DVD:
			return mountDVD();
		default:
			return MountResult::DeviceNotFound;
	}
}

const char * WiiFileSystemDriver::mountResultMessage(int deviceId, MountResult result)
{
	if(result == MountResult::MountFailed)
	{
		switch(deviceId)
		{
			case DEVICE_SD:
			case DEVICE_USB: return "Unsupported format - please use FAT32.";
			default:         return "Unrecognized DVD format.";
		}
	}

	switch(deviceId)
	{
		case DEVICE_SD:  return "SD card not found!";
		case DEVICE_USB: return "USB drive not found!";
		case DEVICE_DVD: return "No disc inserted!";
		case DEVICE_SMB: return "Network share not connected!";
		default:         return "Device not found!";
	}
}

void WiiFileSystemDriver::invalidateStorageDevice(int deviceId)
{
	if(deviceId < 0 || deviceId >= MAX_STORAGE_DEVICES)
		return;

	isMounted[deviceId] = false;
	unmountRequired[deviceId] = true;
	volumeLabel[deviceId][0] = '\0';
}

void WiiFileSystemDriver::pollStorageDevices(int removedIds[MAX_STORAGE_DEVICES], int & outRemovedCount, bool & deviceListChanged)
{
	outRemovedCount = 0;
	deviceListChanged = false;

	bool sdPresent  = sd->isInserted(sd);
	bool usbPresent = usb->isInserted(usb);
	bool dvdPresent = dvd->isInserted(dvd);

	// SD/USB drive a live device listing (see isDevicePresent()), so any
	// transition - inserted or removed - needs to be surfaced.
	if(sdPresent != isPresentCache[DEVICE_SD])
	{
		isPresentCache[DEVICE_SD] = sdPresent;
		deviceListChanged = true;
	}
	if(usbPresent != isPresentCache[DEVICE_USB])
	{
		isPresentCache[DEVICE_USB] = usbPresent;
		deviceListChanged = true;
	}
	isPresentCache[DEVICE_DVD] = dvdPresent;

	if(isMounted[DEVICE_SD] && !sdPresent)
	{
		invalidateStorageDevice(DEVICE_SD);
		removedIds[outRemovedCount++] = DEVICE_SD;
	}

	if(isMounted[DEVICE_USB] && !usbPresent)
	{
		invalidateStorageDevice(DEVICE_USB);
		removedIds[outRemovedCount++] = DEVICE_USB;
	}

	if(isMounted[DEVICE_DVD] && !dvdPresent)
	{
		invalidateStorageDevice(DEVICE_DVD);
		removedIds[outRemovedCount++] = DEVICE_DVD;
	}
}

bool WiiFileSystemDriver::isDevicePresent(int deviceId) const
{
	switch(deviceId)
	{
		case DEVICE_SD:  return isPresentCache[DEVICE_SD];
		case DEVICE_USB: return isPresentCache[DEVICE_USB];
		case DEVICE_DVD: return isPresentCache[DEVICE_DVD]; // informational only - DVD is alwaysListed
		case DEVICE_SMB: return smbDriver.isConnected();    // informational only - SMB is alwaysListed
		default:         return false;
	}
}

//!Mount-path lookup, keyed by the shared Device enum. DEVICE_SMB isn't
//!here - its path depends on live connection state, so getMountPath()
//!below asks smbDriver directly rather than a fixed table entry.
static const char * const kMountPath[DEVICE_LENGTH] =
{
	"",       // DEVICE_AUTO
	"sd:/",   // DEVICE_SD
	"usb:/",  // DEVICE_USB
	"dvd:/",  // DEVICE_DVD
	"",       // DEVICE_SMB (unused - see above)
	"", "", "", "",
};

const char * WiiFileSystemDriver::getMountPath(int device) const
{
	if(device == DEVICE_SMB)
		return smbDriver.getMountPath();

	if(device < 0 || device >= DEVICE_LENGTH || !isMounted[device])
		return "";
	return kMountPath[device];
}

const int * WiiFileSystemDriver::getValidLoadDevices(int & outCount) const
{
	static const int devices[] = { DEVICE_AUTO, DEVICE_SD, DEVICE_USB, DEVICE_DVD, DEVICE_SMB };
	outCount = sizeof(devices) / sizeof(devices[0]);
	return devices;
}

const int * WiiFileSystemDriver::getValidSaveDevices(int & outCount) const
{
	static const int devices[] = { DEVICE_AUTO, DEVICE_SD, DEVICE_USB, DEVICE_SMB };
	outCount = sizeof(devices) / sizeof(devices[0]);
	return devices;
}

This module allows the Chalkboard Electronics 7" and 10" LCD screens with firmware
version 2.0 and up to be used as a normal Android screen with brightness control
via the System UI.

### Usage

Start by downloading the source code to your Android OS repo instance:

```bash
mkdir -p hardware/chalkelec
cd hardware/chalkelec
git clone https://github.com/ObsidianX/android_hardware_chalkelec_liblight.git liblight
```

Ensure that there aren't any other implementations of liblight floating around by renaming
any offending module's `Android.mk` to `Android.mk.off`.  You can use this handy command
to find any of these:

```bash
cd hardware
grep -r "LOCAL_MODULE.*lights"
```

Next you'll need to ensure `CONFIG_HIDRAW` is enabled in your device's Kernel `.config` configuration:

```
...
CONFIG_HIDRAW=y
...
```

Recompile if necessary and flash to your device.  Next you'll need to ensure the `/dev/hidraw*`
devices are accessible from the driver.  Find and edit your device's `ueventd.*.rc` file to include:

```
/dev/hidraw*  0777  root  usb
```

Rebuild, reflash and enjoy!

# init_patch

Small proof-of-concept executable that uses the ptrace API on Android to patch the init process (pid 1) to disable regular shutdown and reboot.

This application modifies the `sys.powerctl` property functionality normally present in `/init.rc` and redirects this property to `sys.powerlol` as a proof of concept.

Android's regular shutdown and reboot functionality is controlled by the `sys.powerctl` property, such that setting this property through `setprop` will initiate a shutdown or reboot (e.g. `setprop sys.powerctl reboot`). This functionality is controlled by the `init` process and through the `/init.rc` file. This file is located in the Android read-only ramdisk (in flash memory) and is copied to the root filesystem on boot. Since it is not part of the writeable filesystem, it cannot be modified without reflashing the phone.

This PoC provides a different way of modifying the `init` functionality by performing live in-memory patching of the `init` process. If you modify the system partition to launch `init_patch` on bootup (e.g. through the use of a rooted OS or custom bootloader such as TWRP), no reflashing is necessary and the `init` functionality can thus still be modified. After execution of the patcher, power control can only be managed through the `sys.powerlol` variable, e.g.:

    setprop sys.powerlol reboot

Note that just like the original `sys.powerctl` property, the above `setprop` command does not require any special privileges to use.

When executing `init_patch`, be sure to check the Android logs using `adb logcat` for any output containing the `init_patch` tag as it will print to the Android log instead of stdout.

## Building

To compile, run the following command in the jni directory:

    ndk-build

The above command is part of the Android NDK and will be located in your local Android NDK directory.

You will then find the executable in the `libs/` directory.


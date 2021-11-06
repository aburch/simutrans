cd C:\temp\platform-tools
adb -s emulator-5554 shell stop
adb -s emulator-5554 shell setprop log.redirect-stdio true
adb -s emulator-5554 shell am start com.simutrans/.MainActivity --es args "'-log -debug 3"'
adb -s emulator-5554 shell start
adb -s emulator-5554 logcat com.simutrans:V *:S

rem My devices, edit as needed
rem G0W0T8058344F2B5
rem emulator-5554

set "tablet=emulator-5554"

cd C:\temp\platform-tools
adb -s %tablet% shell stop
adb -s %tablet% shell setprop log.redirect-stdio true
adb -s %tablet% shell am start com.simutrans/.MainActivity --es args "'-log -debug 3"'
adb -s %tablet% shell start
adb -s %tablet% logcat com.simutrans:V *:S

#!/bin/sh
cd ..
mv portmidi.xcodeproj/project.pbxproj portmidi.xcodeproj/project.pbxproj-cmake
awk -f pm_mac/clean_up_project.awk portmidi.xcodeproj/project.pbxproj-cmake > portmidi.xcodeproj/project.pbxproj


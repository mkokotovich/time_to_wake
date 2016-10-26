# Script to wrap arduino call to use Xvfb
ARDUINO=${HOME}/bin/ard
#ARDUINO=arduino

PROJECT=time_to_wake

verify:
	cd .. ; ${ARDUINO} --preferences-file ./${PROJECT}/preferences.txt --preserve-temp-files -v --verify ./${PROJECT}/${PROJECT}.ino ; cd -

upload:
	cd .. ; ${ARDUINO} --preferences-file ./${PROJECT}/preferences.txt --preserve-temp-files -v --upload ./${PROJECT}/${PROJECT}.ino ; cd -

initialize-workspace:
	echo "This assumes you've set things up in the UI first, to create a valid preferences.txt files"
	cp ${HOME}/.arduino15/preferences.txt ./
	mkdir -p ${HOME}/Arduino/${PROJECT}_build
	cd .. ; ${ARDUINO} --preferences-file ./${PROJECT}/preferences.txt --pref build.path=${HOME}/Arduino/${PROJECT}_build --save-prefs ; cd -

initialize-libraries:
	cd .. ; ${ARDUINO} --preferences-file ./${PROJECT}/preferences.txt --install-library "Time:1.5,TimeAlarms:1.5" ; cd -
	cd ../libraries ; git clone https://github.com/mkokotovich/aREST_UI.git ; cd -
	cd ../libraries ; git clone https://github.com/mkokotovich/aREST.git ; cd -

verify:
	cd .. ; arduino --preferences-file ./time_to_wake/preferences.txt --preserve-temp-files -v --verify ./time_to_wake/time_to_wake.ino ; cd -

initialize-workspace:
	echo "This assumes you've set things up in the UI first, to create a valid preferences.txt files"
	cp ${HOME}/.arduino15/preferences.txt ./

initialize-libraries:
	cd .. ; arduino --preferences-file ./time_to_wake/preferences.txt --install-library "Time:1.5,TimeAlarms:1.5" ; cd -

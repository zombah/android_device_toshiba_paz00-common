#!/system/bin/sh
#
# Author: Linaro Validation Team <linaro-dev@lists.linaro.org>
#
# These files are Copyright (C) 2012 Linaro Limited and they
# are licensed under the Apache License, Version 2.0.
# You may obtain a copy of this license at
# http://www.apache.org/licenses/LICENSE-2.0
wait_home_screen(){
    timeout=0
    while(true)
    do
        echo "Waiting for Homescreen ..."
        if logcat -d | grep -i "Displayed com.android.launcher/com.android.launcher2.Launcher:" ; then
            echo "Home screen should be displayed!"
            return 0
        fi
        timeout=$((timeout+1))
        if [ $timeout = 30 ]; then
            echo "Failed to wait the home screen displayed!"
            return 1
        fi
        sleep 60
    done
}

disable_suspend(){
    echo "Now disable the suspend feature..."
    stay_awake="delete from system where name='stay_on_while_plugged_in'; insert into system (name, value) values ('stay_on_while_plugged_in','3');"
    screen_sleep="delete from system where name='screen_off_timeout'; insert into system (name, value) values ('screen_off_timeout','-1');"
    lockscreen="delete from secure where name='lockscreen.disabled'; insert into secure (name, value) values ('lockscreen.disabled','1');"
    sqlite3 /data/data/com.android.providers.settings/databases/settings.db "${stay_awake}" ## set stay awake
    sqlite3 /data/data/com.android.providers.settings/databases/settings.db "${screen_sleep}" # set sleep to none
    sqlite3 /data/data/com.android.providers.settings/databases/settings.db "${lockscreen}" ##set lock screen to none
    input keyevent 82  ##unlock the home screen
    svc power stayon true ##disable the suspend
    echo "The suspend feature is disabled."
}

if [ "X${1}" == "X--no-wait" ]; then
    :
else
    wait_home_screen || exit 1
fi

disable_suspend

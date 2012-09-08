#!/system/bin/mksh
DEBUG_LOG=/data/wwlan_select.log
#DEBUG_LOG=/dev/null

# idVendor    idProduct   module
#   05c6        6000      X8133/WM700g/WU500/EA100
#   19d2        ffeb      WM700g-plus
#   0bdb        1914      F3307
#   0bdb        190d      F5521gw
#   12d1        1001      HUAWEI E220
#   0421        0208      Nokia 5130 ExpressMusic

VPList=( \
05c6 6000 X8133/WM700g/WU500/EA100 \
19d2 ffeb WM700g-plus \
0bdb 1914 F3307 \
0bdb 190d F5521gw \
12d1 1001 E220 \
0421 0208 Nokia-5130EM \
)

VPListNum=${#VPList[*]}

IndexTotal=$((VPListNum / 3))

for Vendor in `ls /sys/bus/usb/devices/*/idVendor /sys/bus/usb/devices/*/*/idVendor 2> /dev/null`
do
	idVendortemp=$(cat $Vendor)
	idProducttemp=$(cat ${Vendor/idVendor/}'idProduct')
	index=0
	while [ "$index" -lt "$IndexTotal" ]
	do
		case "$idVendortemp" in
		0bdb) # Ericsson
			idVendor=$idVendortemp
			break 2
			;;
		0421) # Nokia
			idVendor=$idVendortemp
			idProduct=$idProducttemp
			break 2
			;;
		${VPList[$((index * 3))]})
			case "$idProducttemp" in
			${VPList[$((index * 3 + 1))]})
				idVendor=$idVendortemp
				idProduct=$idProducttemp
				break 2
				;;
			*)
				;;
			esac
			;;
		*)
			;;
		esac
		index=$((index + 1 ))
	done
done

echo "wwlan modem ---> idVendor:$idVendor, idProduct:$idProduct" >> $DEBUG_LOG
echo "wwlan power path ----> ${Vendor/idVendor/}power" >> $DEBUG_LOG

case "$idVendor" in
05c6)
	case "$idProduct" in
	6000)
		echo "wwlan srbcomms module" >> $DEBUG_LOG
		setprop rild.libpath /system/lib/libreference-ril_srbcomms.so
		setprop rild.libargs "-d /dev/ttyUSB1"
		setprop ctl.stop ril-daemon
		setprop ctl.start ril-daemon
		;;
	*)
		echo "idVendor 05c6,but unknown idProduct" >> $DEBUG_LOG
		;;
	esac
	;;
19d2)
	case "$idProduct" in
	ffeb)
		echo "wwlan wm700g-plus module" >> $DEBUG_LOG
		setprop rild.libpath /system/lib/libreference-ril_wm700g_plus.so
		setprop rild.libargs "-d /dev/ttyUSB0"
		setprop ctl.stop ril-daemon
		setprop ctl.start ril-daemon
		echo enabled > ${Vendor/idVendor/}power/wakeup
		echo auto > ${Vendor/idVendor/}power/control
		;;
	*)
		echo "idVendor 19d2,but unknown idProduct" >> $DEBUG_LOG
		;;
	esac
	;;
0bdb)
	echo "wwlan Ericsson module" >> $DEBUG_LOG
	setprop rild.libpath /system/lib/libmbm-ril.so
	setprop rild.libargs "-d /dev/ttyACM1 -i wwan0"
	setprop ctl.stop ril-daemon
	setprop ctl.start ril-daemon
	echo enabled > ${Vendor/idVendor/}power/wakeup
	echo auto > ${Vendor/idVendor/}power/control
	;;
12d1)
	echo "wwlan Huawei module" >> $DEBUG_LOG
	setprop rild.libpath /system/lib/libhuaweigeneric-ril.so
	setprop rild.libargs "-d /dev/ttyUSB2"
	setprop ctl.stop ril-daemon
	setprop ctl.start ril-daemon
	echo enabled > ${Vendor/idVendor/}power/wakeup
	echo auto > ${Vendor/idVendor/}power/control
	;;
0421)
	echo "wwlan Nokia modem" >> $DEBUG_LOG
	#setprop rild.libpath /system/lib/libreference-ril.so
	#setprop rild.libpath /system/lib/libmbm-ril.so
	setprop rild.libpath /system/lib/libhuaweigeneric-ril.so
	setprop rild.libargs "-d /dev/ttyACM0"
	setprop ctl.stop ril-daemon
	setprop ctl.start ril-daemon
	echo auto > ${Vendor/idVendor/}power/control
	;;

*)
	echo "unknown idVendor" >> $DEBUG_LOG
	setprop ctl.stop ril-daemon
	;;
esac


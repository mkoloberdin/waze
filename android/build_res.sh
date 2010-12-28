BASE_DIR=../resources/
DST_DIR=java/assets/freemap
DST_DIR_HD=java/assets/freemapHD
DST_DIR_LD=java/assets/freemapLD
DST_JAVA_DRAWABLE=java/res/drawable

#Clear the directories
rm -fr $DST_DIR/*
rm -fr $DST_DIR_HD/*
rm -fr $DST_DIR_LD/*

#Skins and more
mkdir -p $DST_DIR
mkdir -p $DST_DIR_HD
mkdir -p $DST_DIR_LD/skins
cp -r $BASE_DIR/default/* $DST_DIR/


#Language


#Sounds
mkdir -p $DST_DIR/sound/eng
mkdir -p $DST_DIR/sound/heb
cp -r $BASE_DIR/sound/eng/* $DST_DIR/sound/eng
cp -r $BASE_DIR/sound/heb/* $DST_DIR/sound/heb
#Temporary place english prompts instead of hebrew


#Temporary remove waves
rm $DST_DIR/sound/heb/*.wav
rm $DST_DIR/sound/eng/*.wav
#rm -r $DST_DIR/sound/eng_silence
#Maps
mkdir -p $DST_DIR/maps
cp -r $BASE_DIR/maps/* $DST_DIR/maps

#GPS
mkdir -p $DST_DIR/GPS
#cp -r $BASE_DIR/GPS/* $DST_DIR/GPS

#Schema
mkdir -p $DST_DIR/skins/default/day
cp $BASE_DIR/default/skins/default/schema_day $DST_DIR/skins/default/day/schema
mkdir -p $DST_DIR/skins/default/day/1
cp $BASE_DIR/default/skins/default/schema.1 $DST_DIR/skins/default/day/1/schema
mkdir -p $DST_DIR/skins/default/day/2
cp $BASE_DIR/default/skins/default/schema.2 $DST_DIR/skins/default/day/2/schema
mkdir -p $DST_DIR/skins/default/day/3
cp $BASE_DIR/default/skins/default/schema.3 $DST_DIR/skins/default/day/3/schema
mkdir -p $DST_DIR/skins/default/day/4
cp $BASE_DIR/default/skins/default/schema.4 $DST_DIR/skins/default/day/4/schema
mkdir -p $DST_DIR/skins/default/day/5
cp $BASE_DIR/default/skins/default/schema.5 $DST_DIR/skins/default/day/5/schema
mkdir -p $DST_DIR/skins/default/day/6
cp $BASE_DIR/default/skins/default/schema.6 $DST_DIR/skins/default/day/6/schema
mkdir -p $DST_DIR/skins/default/day/7
cp $BASE_DIR/default/skins/default/schema.7 $DST_DIR/skins/default/day/7/schema
mkdir -p $DST_DIR/skins/default/day/8
cp $BASE_DIR/default/skins/default/schema.8 $DST_DIR/skins/default/day/8/schema

mkdir -p $DST_DIR/skins/default/night
cp $BASE_DIR/default/skins/default/schema_night $DST_DIR/skins/default/night/schema

#### LD RESOURCES ####
cp -r $BASE_DIR/win_touch/skins/* $DST_DIR_LD/skins
cp  $BASE_DIR/no_touch/skins/default/nav_* $DST_DIR_LD/skins/default
cp  $BASE_DIR/QVGA/skins/default/* $DST_DIR_LD/skins/default

#### HD RESOURCES ####
cp -r $BASE_DIR/nHD/* $DST_DIR_HD
cp -r $BASE_DIR/WVGA/* $DST_DIR_HD
cp -r $BASE_DIR/WVGA854/* $DST_DIR_HD


#WVGA 
if [ "$1" = "WVGA" ]; then
echo "Copying WVGA resources ...."
cp -r $BASE_DIR/nHD/* $DST_DIR
cp -r $BASE_DIR/WVGA/* $DST_DIR
fi

#WVGA 854 
if [ "$1" = "WVGA854" ]; then
echo "Copying WVGA854 resources ...."
cp -r $BASE_DIR/nHD/* $DST_DIR
cp -r $BASE_DIR/WVGA/* $DST_DIR
cp -r $BASE_DIR/WVGA854/* $DST_DIR
fi


#Android specific resources
#cp $BASE_DIR/android/* $DST_DIR
cp -r $BASE_DIR/android/common/* $DST_DIR
cp -r $BASE_DIR/android/common/* $DST_DIR_HD
cp -r $BASE_DIR/android/common/* $DST_DIR_LD
cp -r $BASE_DIR/android/hvga/* $DST_DIR
cp -r $BASE_DIR/android/wvga/* $DST_DIR_HD
cp -r $BASE_DIR/android/lvga/* $DST_DIR_LD
#cp $BASE_DIR/android/common/skins/default/notification3.png $DST_JAVA_DRAWABLE/notification.png
#cp $BASE_DIR/android/common/skins/default/notification3hd.png $DST_JAVA_DRAWABLE/notification_hd.png

#US data
#if [ "$1" = "us" ]; then
#cp -r $BASE_DIR/android/usa/* $DST_DIR
#cp $BASE_DIR/lang/eng/lang $DST_DIR
#cp $BASE_DIR/sound/eng/* $DST_DIR/sound/
#fi

#Replace extensions for the PNG files
rename -f 's/.png$/.bin/' $DST_DIR/skins/default/*.png
rename -f 's/.png$/.bin/' $DST_DIR/skins/default/moods/*.png
rename -f 's/.png$/.bin/' $DST_DIR/skins/default/cars/*.png

#Replace extensions for the PNG files - HD directory
rename -f 's/.png$/.bin/' $DST_DIR_HD/skins/default/*.png
rename -f 's/.png$/.bin/' $DST_DIR_HD/skins/default/moods/*.png
rename -f 's/.png$/.bin/' $DST_DIR_HD/skins/default/cars/*.png

#Replace extensions for the PNG files - LD directory
rename -f 's/.png$/.bin/' $DST_DIR_LD/skins/default/*.png
rename -f 's/.png$/.bin/' $DST_DIR_LD/skins/default/moods/*.png
rename -f 's/.png$/.bin/' $DST_DIR_LD/skins/default/cars/*.png

#Replace extensions for the mp3 files
rename -f 's/.mp3$/.bin/' $DST_DIR/sound/eng/*.mp3
rename -f 's/.mp3$/.bin/' $DST_DIR/sound/heb/*.mp3

#!/bin/bash
export LC_ALL="C"; PWD=$(pwd)

################### IMPORTANT  ####################
# Check eveye_networking.h for NN_INTRA_x defines #
###################################################

# Original configuration file that will be used to repolace QP and InputFile
CFG_FILE_ORIG="cfg/encoder_allintra.cfg"

# Template for the sequnce name and sequences to be tested (only 1st frame as in PNN paper)
#InputFile                     : /home/attilio/repos/HM-16.20_nn/sequences/Kimono1_1920x1080_fps_2.yuv
SEQUENCE_BASE="/mnt/c/Users/lucas/Documents/TCC/scratch/Datasets/EPFL_RGB/"
ENCODINGS_DIR="${PWD}/encodings/Kernels3/32x64/mem"
#SEQUENCE_LIST="Danger-de-Mort Rusty-Fence"
#SEQUENCE_LIST="Ankylosaurus-Diplodocus Bikes Black-Fence Ceiling-Light Danger-de-Mort Friends-1 Houses-Lake Reeds Rusty-Fence Slab-Lake Swans-2 Vespa"
#SEQUENCE_LIST="Bee_1.png     Bumblebee.png     Corridor.png       Duck.png    LensFlare.png         LowLight_Roundabout.png  Mini.png         Plushies.png  Sculpture.png  Steps.png      Texture.png
#Bee_2.png     Checkerboard.png  DistantChurch.png  Framed.png  LowLight_Flowers.png  LowLight_Street.png      MotionBlur.png   Posts.png     Sign.png       SunnyRose.png  TinyMoon.png
#Building.png  ChezEdgar.png     Doves.png          Fruits.png  LowLight_House.png    MessyDesk.png            Perspective.png  Rond.png      Statue.png     Symmetric.png  Translucent.png"
SEQUENCE_LIST="Ankylosaurus_4976x3456 Bikes_4976x3456 Black-Fence_4976x3456 Ceiling-Light_4976x3456 Danger-de-Mort_4976x3456 Friends-1_4976x3456 Houses-Lake_4976x3456 Reeds_4976x3456 Rusty-Fence_4976x3456 Slab-Lake_4976x3456 Swans-2_4976x3456 Vespa_4976x3456"
#SEQUENCE_LIST="Bikes"

#SEQUENCE_BASE="/home/shared/SVT-HD/SEQUENCE_1920x1080_fps_2.yuv"
#SEQUENCE_LIST="GTAV MINEDRAFT blue_sky in_to_tree red_kayak sunflower"

# Clipping the 1st frame out of a 10bpp HD sequence: 
#dd if=MarketPlace_1920x1080_60fps_10bit_420.yuv of=MarketPlace_1920x1080_60fps_10bit_420_1f.yuv ibs=6220800 count=1
# This is for the JVET test sequencesd
#SEQUENCE_BASE="/home/shared/jvet/ftp.hhi.fraunhofer.de/ctc/sdr/"
# Class A 3840x2160
#SEQUENCE_LIST="Campfire CatRobot DaylightRoad2 FoodMarket4 ParkRunning3 Tango2"
# Class B	1920x1080
#SEQUENCE_LIST="BQTerrace BasketballDrive Cactus MarketPlace RitualDance"
# Class C 832x480
#SEQUENCE_LIST="RaceHorses BQMall PartyScene BasketballDrill"
#Class D 416x240
#SEQUENCE_LIST="BQSquare BlowingBubbles BasketballPass" #RaceHorses <- already in class C
# Class E	1280x720
#SEQUENCE_LIST="FourPeople Johnny KristenAndSara"
# Class F computer screens
#SEQUENCE_LIST="ArenaOfValor BasketballDrillText SlideEditing SlideShow"

# This is the main directory wher a separate subdir for each encoding will be created


# For parallel encoding
if [ $# -eq 1 ]; then
  SEQUENCE_LIST=$1
fi

# QPs to be tested (>= 4 req'ed for plotting a BD rate curve) 
QP_LIST="22 27 32 37 42 47"

# The encoder binary for the reference and proposed encoders (TODO implement runtime switch)
TAPPENCODER="$(pwd)/build/bin/eveya_encoder"
# Workaround to having some params as defines inside eveye_networking.h
cd build; make; cd ..
# "ref" for the reference hevc encoder, "prop" for the prposed encoder with CE
MODE_LIST="prop"

#Summary of the encoding results in terms of BITS-YPSNR
SUMMARY_FILE="${ENCODINGS_DIR}/encode_batch_summary.txt"
ERROR_LOG="${ENCODINGS_DIR}/error.log"
mkdir ${ENCODINGS_DIR} 2>/dev/null

for MODE in $MODE_LIST; do
for SEQUENCE in $SEQUENCE_LIST; do
FOUND_FILES=$(find $SEQUENCE_BASE -iname '*'${SEQUENCE}'*.yuv' | wc -l)
if [ $FOUND_FILES -ne 1 ]; then
    echo "ERROR sequence $SEQUENCE not found or ambiguous, skipping" | tee -a $ERROR_LOG
    continue
fi
SEQUENCE_PATH=$(find $SEQUENCE_BASE -iname '*'${SEQUENCE}'*.yuv')

WIDTH=$(basename $SEQUENCE_PATH | awk 'BEGIN{FS="_"}{print $2}'| awk 'BEGIN{FS="x"}{print $1}')
HEIGHT=$(basename $SEQUENCE_PATH | awk 'BEGIN{FS="_"}{print $2}'| awk 'BEGIN{FS="x"}{print $2}' | sed s/'.yuv'//g)
IS_10BITS=$(echo $SEQUENCE_PATH | grep '10bit' | wc -l)
if [ $IS_10BITS -eq 1 ]; then
  BIT_DEPTH='10'
else
  BIT_DEPTH='8'
fi

for QP in $QP_LIST; do
  
  # All files will be saved in this directory (without trailing "/")
  mkdir -p ${ENCODINGS_DIR}/logs
  OUT_DIR="${ENCODINGS_DIR}/logs/${SEQUENCE}_${MODE}_qp-${QP}"
  # Just in the case some old results existed, we back them up
  mv "${OUT_DIR}" "${OUT_DIR}_$(date -u | sed s/' '/'_'/g)" 2>/dev/null
  mkdir -p $OUT_DIR
  
  # Logging all modifications to the sources
  git diff > "${OUT_DIR}/git.diff"; git log | head -n 100 > "${OUT_DIR}/git.log"
  
  # Copying any .yuv debug file produced by the encoder
  mv *.yuv ${OUT_DIR} 2>/dev/null
  
  echo ""
  echo "**** MODE ${MODE} SEQUENCE ${SEQUENCE} QP ${QP} ****"
  echo ""
  
  # Selecting the right encoder and making a copy thereof
  if [ $MODE == "ref" ];
    then NN_BASE_PORT="0"
  else
    NN_BASE_PORT="8000"
    # Querying the server(s) about commandline params
    echo "" | nc -u -W 1 'localhost' $((sc$NN_BASE_PORT+32)) > "${OUT_DIR}/server_32.log"
    echo "" | nc -u -W 1 'localhost' $(($NN_BASE_PORT+16)) > "${OUT_DIR}/server_16.log"
    echo "" | nc -u -W 1 'localhost' $(($NN_BASE_PORT+8)) > "${OUT_DIR}/server_8.log"
    echo "" | nc -u -W 1 'localhost' $(($NN_BASE_PORT+4)) > "${OUT_DIR}/server_4.log"
  fi
  
  # The encoding log to be parsed after encoding
  ENCODER_LOG="${OUT_DIR}/encoder.log"


  
  # Encoding: -z -> frameRate, -d -> bitDepth of the sequence, -codec_bit_depth -> bitDepth interno encoder, -f -> numFrames
  cp $TAPPENCODER "${OUT_DIR}/"; $TAPPENCODER -i $SEQUENCE_PATH -o "${OUT_DIR}/out.bin" -r "${OUT_DIR}/recon.yuv" -w $WIDTH -h $HEIGHT -q $QP -z 30 -f 1 -d $BIT_DEPTH --nn_base_port $NN_BASE_PORT --config $CFG_FILE_ORIG 2>&1 | tee $ENCODER_LOG
  
  # Logging to the summary file for later BD rate computation
  ENCODED_BITS=$(cat $ENCODER_LOG | grep '  Total bits(bits) : ' | awk '{print $NF}')
  ENCODED_YPSNR=$(cat $ENCODER_LOG | grep '  PSNR Y(dB)       : ' | awk '{print $NF}')
  echo "MODE ${MODE} SEQUENCE ${SEQUENCE} QP ${QP} BITS ${ENCODED_BITS} YPSNR ${ENCODED_YPSNR}" >> ${SUMMARY_FILE}
done
done
done
 
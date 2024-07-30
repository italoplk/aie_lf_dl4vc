#!/bin/bash

export LC_ALL="C"; PWD=$(pwd)

# See also https://github.com/Anserw/Bjontegaard_metric for the bd rate computer

if [ $# -lt 1 ]; then
  echo "ERROR Usage: $0 <proposed encode_batch_summary.txt> [<Low QP> <Hi QP> [<reference encode_batch_summary.txt>]]"
  exit 1
fi

# Summary of the encoding results in terms of BITS-YPSNR as produced by encode_batch.sh
# This by default will be used for both proposed and reference scheme
SUMMARY_FILE_PROP="${1}"

# We may specify to take the reference from a different file
if [ $# -ge 3 ]; then
  LOW_QP="${2}"
  HI_QP="${3}"
else
  LOW_QP="22"
  HI_QP="37"
fi

# We may specify to take the reference from a different file
if [ $# -eq 4 ]; then
  SUMMARY_FILE_REF="${4}"
else
  SUMMARY_FILE_REF=$SUMMARY_FILE_PROP
fi

if [ ! -f $SUMMARY_FILE_PROP ]; then
  echo "ERROR Proposed encode batch summary $SUMMARY_FILE_PROP not found"
  exit 1
fi

if [ ! -f $SUMMARY_FILE_REF ]; then
  echo "ERROR Reference encode batch summary $SUMMARY_FILE_PROP not found"
  exit 1
fi

# Downloading the bdrate computer, if needed
if [ ! -d "Bjontegaard_metric" ]; then
  git clone https://github.com/Anserw/Bjontegaard_metric/
fi

# By default, we compute the bdrate for all sequences
SEQUENCE_LIST=$(cat $SUMMARY_FILE_REF | awk '{print $4}'| sort | uniq)
#SEQUENCE_LIST="BasketballDrive BQTerrace Cactus Kimono ParkScene"

# This is MPEG standard QP list (22-37)
#QP_LIST="22 27 32 37"
# We may want to pick all QPs in the set
#QP_LIST=$(cat $SUMMARY_FILE_REF | awk '{print $6}'| sort | uniq)
# High QP list, where we have most of the gains
QP_LIST=$(seq $LOW_QP 5 $HI_QP)

echo "Found sequences list is: "${SEQUENCE_LIST}
echo "Found QPs list is: "${QP_LIST}

rm bdrate.log 2>/dev/null
echo -e "Sequence\t\tBD-Rate\t\tBD-PSNR"
for SEQUENCE in $SEQUENCE_LIST; do
  # We compute the BDRATE for each sequence
  R1=""; PSNR1=""; R2=""; PSNR2=""
  for QP in $QP_LIST; do
    # Rate and PSNR for the reference
    R1="${R1}$(cat $SUMMARY_FILE_REF | grep 'ref' | grep $SEQUENCE | grep "QP ${QP}" | awk '{printf($8",")}')"
    PSNR1="${PSNR1}$(cat $SUMMARY_FILE_REF | grep 'ref' | grep $SEQUENCE | grep "QP ${QP}" | awk '{printf($10",")}')"
    # Rate and PSNR for the proposed
    R2="${R2}$(cat $SUMMARY_FILE_PROP | grep 'prop' | grep $SEQUENCE | grep "QP ${QP}" | awk '{printf($8",")}')"
    PSNR2="${PSNR2}$(cat $SUMMARY_FILE_PROP | grep 'prop' | grep $SEQUENCE | grep "QP ${QP}" | awk '{printf($10",")}')"
  done
  
  # Removing the last trailing comma
  R1=${R1%,*}; PSNR1=${PSNR1%,*}; R2=${R2%,*}; PSNR2=${PSNR2%,*}

  # Skipping sequences that are still going thru the encoding
  N_POINTS_PROP=$(echo $R2 | awk 'BEGIN{FS=","}{print NF}')
  if [ $N_POINTS_PROP -lt $(echo $QP_LIST | wc -w) ]; then
    echo "Sequence $SEQUENCE has only $N_POINTS_PROP / $(echo $QP_LIST | wc -w) prop points, skipping"
    continue
  fi
  
  # A tab seems to be 8 chars
  SEQ_LEN=${#SEQUENCE}
  PADDING_CHARS=$((16-$SEQ_LEN))
  while [ $PADDING_CHARS -gt 0 ]; do
    SEQUENCE="${SEQUENCE} "
    PADDING_CHARS=$(($PADDING_CHARS-1))
  done
  
  echo -en "${SEQUENCE}\t"
  # Creating a python script on the fly
  PYTHON_SCRIPT="bdrate.py"
  echo 'from bjontegaard_metric import *' > $PYTHON_SCRIPT
  echo 'R1 = np.array(['${R1}'])' >> $PYTHON_SCRIPT
  echo 'PSNR1 = np.array(['${PSNR1}'])' >> $PYTHON_SCRIPT
  echo 'R2 = np.array(['${R2}'])' >> $PYTHON_SCRIPT
  echo 'PSNR2 = np.array(['${PSNR2}'])' >> $PYTHON_SCRIPT
#  echo 'print ("BD-PSNR: %.4f" %(BD_PSNR(R1, PSNR1, R2, PSNR2)))' >> $PYTHON_SCRIPT
#  echo 'print ("BD-RATE: %.4f" %(BD_RATE(R1, PSNR1, R2, PSNR2)))' >> $PYTHON_SCRIPT
  echo 'print ("%.2f\t\t%.2f" %(BD_RATE(R1, PSNR1, R2, PSNR2), BD_PSNR(R1, PSNR1, R2, PSNR2)))' >> $PYTHON_SCRIPT
  PYTHONPATH="${PYTHONPATH}:$(pwd)/Bjontegaard_metric" python3 < $PYTHON_SCRIPT | tee -a bdrate.log
done

# Average BDrate
cat bdrate.log | awk '{T_BDRATE=T_BDRATE+$1; T_BDPSNR=T_BDPSNR+$2}END{printf("AVG\t\t\t%.2f\t\t%.2f\n", T_BDRATE/NR, T_BDPSNR/NR)}'
# Cleanup time
rm bdrate.py bdrate.log 2>/dev/null

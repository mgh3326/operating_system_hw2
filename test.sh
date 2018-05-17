#!bin/bash
P=1
Q=0 #Q가 5의 배수로 증가해야한다 혹은 감소해야한다
function left_file_name() #파일을 디렉토리 일반파일 특수파일 순으로 이름만 출력
{
j=2
tput cup 1 1
echo "[31m.."
#tput cup $j 2
for filename in `ls | sort` ; do
if [ "`stat -c %F $filename`" = "디렉토리" ]; then #디렉토리 조건
if [ $j -ge 22 ];then
	break
		fi
	tput cup $j 1
echo -n "[34m"
echo "$filename" | cut -c 1-10
#echo "$filename"
j=`expr $j + 1`
fi
done
for filename in `ls` ; do
if [ "`stat -c %F ${filename} | cut -c 1-6`" = "일반" ]; then #일반파일 조건
	if [ $j -ge 22 ];then
		break
			fi
tput cup ${j} 1
echo -n "[0m"
echo "$filename" | cut -c 1-10
j=`expr $j + 1`
#echo "$filename"
fi
done
for filename in `ls` ; do
if [ "`stat -c %F ${filename} | cut -c 1-3`" != "디" ] && [ "`stat -c %F ${filename} | cut -c 1-3`" != "일" ]; then #특수파일 조건
if [ ${j} -ge 22 ];then
	break
		fi
tput cup ${j} 1
echo "$filename" | cut -c 1-10
echo -n "[32m"
j=`expr ${j} + 1`
fi
done
echo -n "[0m"
}



clear


function outline()
{
echo -n "[0m"
clear
echo "========================================================================================================="
for (( i=1; i<37; i++ ))
do
echo "|"
tput cup $i 105
echo "|"
done
current_directory=$(pwd) #현재 디렉토리 변수 선언
cd .. #상위 디렉토리로 이동
left_file_name  #함수였구만
for (( i=1; i<28; i++))
do
tput cup $i 20
echo "|"
done
cd $current_directory #for문이 끝나는데로 디렉토리를 bash 파일이 있는 디렉토리로 이동
tput cup 28 1
echo "*****************************************intforamtion**************************************************"
tput cup 35 1
echo "*******************************************************************************************************"
tput cup 37 0
echo "========================================================================================================="
#echo "$i"
#echo `ls`
#cd new`
#echo `pwd`
#echo `ls`
if [ $Q -eq 0 ];then
if [ $P -eq 1 ];then
echo -n "[0m"
tput setab 1
else
echo -n "[31m"
fi
tput cup 1 21
echo "     __"
tput cup 2 21
echo "/---/ |"
tput cup 3 21
echo "|  d  |"
tput cup 4 21
echo "-------"
tput cup 5 21
echo ".."
tput sgr0
fi
}
outline
#tput cup 38 0

function file_num()
{
total_num=0
dir_num=0
file_num=0
Sfile_num=0
total_size=0
for filename in `ls | sort` ; do
size=`stat -c %s $filename`
total_size=`expr $total_size + $size`
if [ "`stat -c %F $filename`" = "디렉토리" ]; then #디렉토리 조건
dir_num=`expr $dir_num + 1`
elif [ "`stat -c %F $filename | cut -c 1-6`" = "일반" ]; then #일반파일 조건
file_num=`expr $file_num + 1`
elif [ "`stat -c %F $filename | cut -c 1-3`" != "디" ] && [ "`stat -c %F $filename | cut -c 1-3`" != "일" ]; then #특수파일 조건
Sfile_num=`expr $Sfile_num + 1`
fi
total_num=`expr $dir_num + $file_num + $Sfile_num`
done
}

function center_file_name() #파일을 디렉토리 일반파일 특수파일 순으로 이름만 출력
{
file_num
#j=2
k=1
if [ $Q -eq 0 ];then
l=38
else
		l=21
				fi
n=1
m=0
#tput cup 1 1
#echo "^[[31m.."
#tput cup $j 2
NEW="" #초기화
for filename in `ls | sort` ; do
if [ $Q -eq 0 ];then
	if [ $n -eq 25 ];then
		break
	fi
	else
			if [ $n -eq 26 ];then
					break
							fi
							fi
							if [ "$l" = "106" ];then
l=21
k=`expr $k + 5`
fi
if [ "`stat -c %F $filename`" = "디렉토리" ]; then #디렉토리 조건
m=`expr $m + 1`
		if [ $Q -eq 5 ];then
if [ $m -le 4 ];then
		continue
				fi
else
if [ $m -lt $Q ];then
continue
fi
fi
tput cup $k $l
echo "[34m     __"
tput cup $((k+1)) $l
echo "/---/ |"
tput cup $((k+2)) $l
echo "|  d  |"
tput cup $((k+3)) $l
echo "-------"
tput cup $((k+4)) $l
echo "$filename" | cut -c 1-10
NEW=$NEW$filename' ' #배열 만들기 위해서 사용함
l=`expr $l + 17`
n=`expr $n + 1`
fi
done
for filename in `ls` ; do
if [ "`stat -c %F $filename | cut -c 1-6`" = "일반" ]; then #일반파일 조건
	m=`expr $m + 1`
		if [ $m -le $Q ];then
			continue
				fi
#tput cup $j 1
if [ $Q -eq 0 ];then
	if [ $n -eq 25 ];then
		break
	fi
else
		if [ $n -eq 26 ];then
				break
						fi
						fi
if [ "$l" = "106" ];then
l=21
k=`expr $k + 5`
fi
if [ "`ls -al $filename | cut -c 10`" = "x" ]; then

tput cup $k $l
echo "[31m-------"
tput cup $((k+1)) $l
echo "|     |"
tput cup $((k+2)) $l
echo "|  x  |"
tput cup $((k+3)) $l
echo "-------"
tput cup $((k+4)) $l
echo "$filename" | cut -c 1-10

else
tput cup $k $l
echo "[0m-------"
tput cup $((k+1)) $l
echo "|     |"
tput cup $((k+2)) $l
echo "|  o  |"
tput cup $((k+3)) $l
echo "-------"
tput cup $((k+4)) $l
echo "$filename" | cut -c 1-10
fi
NEW=$NEW$filename' '
#echo "^[[0m$filename"
l=`expr $l + 17`
n=`expr $n + 1`
#echo "$filename"
echo -n "[0m"
fi
done
for filename in `ls` ; do
if [ "`stat -c %F $filename | cut -c 1-3`" != "디" ] && [ "`stat -c %F $filename | cut -c 1-3`" != "일" ]; then #특수파일 조건
m=`expr $m + 1`
	if [ $m -le $Q ];then
		continue
			fi

if [ $Q -eq 0 ];then
	if [ $n -eq 25 ];then
		break
	fi
else
		if [ $n -eq 26 ];then
				break
						fi
						fi
if [ $l -eq 106 ];then
l=38
k=`expr $k + 5`
fi
#echo "^[[32m$filename"
#tput cup $j 1
tput cup $k $l
echo "[32m-------"
tput cup $((k+1)) $l
echo "|     |"
tput cup $((k+2)) $l
echo "|  s  |"
tput cup $((k+3)) $l
echo "-------"
tput cup $((k+4)) $l
echo "$filename" | cut -c 1-10
NEW=$NEW$filename' '
#arr=($NEW)
#echo "^[[32m$filename"
l=`expr $l + 17`
n=`expr $n + 1`
echo -n "[0m"


fi
done
}
center_file_name
#P=1 #P를 먼저 선언
function information
{
#P=$1
NP=`expr $P - 2`
#if [ -z $NEW ];then
#.....
#ABC=1
#else
arr=($NEW)
if [ $arr ];then
		if [ $Q -eq 0 ];then
information_name=${arr[$P-2]}
		else
information_name=${arr[$P-1]}
		fi
fi
filename=$information_name
#l=38
#fi
k=1
l=38
if [ $P -gt 1 -o $Q -ge 5 ];then
if [ $P -le 5 ];then
l=`expr $l + 17 \* $NP`
elif [ $P -le 10 ];then
NP=`expr $NP - 5`
l=`expr $l + 17 \* $NP`
k=`expr $k + 5`
elif [ $P -le 15 ];then
NP=`expr $NP - 10`
l=`expr $l + 17 \* $NP`
k=`expr $k + 10`
elif [ $P -le 20 ];then
NP=`expr $NP - 15`
l=`expr $l + 17 \* $NP`
k=`expr $k + 15`
elif [ $P -le 25 ];then
NP=`expr $NP - 20`
l=`expr $l + 17 \* $NP`
k=`expr $k + 20`
fi
#tput setaf 0
echo -n "[30m"
if [ "`stat -c %F $filename`" = "디렉토리" ]; then #디렉토리 조건
tput setab 4
tput cup $k $l
echo "     __"
tput cup $((k+1)) $l
echo "/---/ |"
tput cup $((k+2)) $l
echo "|  d  |"
tput cup $((k+3)) $l
echo "-------"
tput cup $((k+4)) $l
echo "$filename" | cut -c 1-10
tput sgr0
fi
if [ "`stat -c %F $filename | cut -c 1-6`" = "일반" ]; then #일반파일 조건
#tput setab 7
tput cup $k $l

if [ "`ls -al $filename | cut -c 10`" = "x" ]; then
		echo -n "[30m"
		tput setab 1
tput cup $k $l
echo "-------"
tput cup $((k+1)) $l
echo "|     |"
tput cup $((k+2)) $l
echo "|  x  |"
tput cup $((k+3)) $l
echo "-------"
tput cup $((k+4)) $l
echo "$filename" | cut -c 1-10
tput sgr0
else
		echo -n "[30m"
tput setab 7
echo "-------"
tput cup $((k+1)) $l
echo "|     |"
tput cup $((k+2)) $l
echo "|  o  |"
tput cup $((k+3)) $l
echo "-------"
tput cup $((k+4)) $l
echo "$filename" | cut -c 1-10
fi
tput sgr0
fi
if [ "`stat -c %F $filename | cut -c 1-3`" != "디" ] && [ "`stat -c %F $filename | cut -c 1-3`" != "일" ]; then #특수파일 조건
echo -n "[30m"
		tput setab 2
tput cup $k $l
echo "-------"
tput cup $((k+1)) $l
echo "|     |"
tput cup $((k+2)) $l
echo "|  s  |"
tput cup $((k+3)) $l
echo "-------"
tput cup $((k+4)) $l
echo "$filename" | cut -c 1-10
tput sgr0
fi
tput cup 29 20
echo "file name : $filename"
tput cup 30 20
echo "file type : "`stat -c %F $filename`
tput cup 31 20
echo "file size : "`stat -c %s $filename`
tput cup 32 20
echo "creation time : "`stat -c %y $filename`
tput cup 33 20
echo "permission : "`stat -c %a $filename`
tput cup 34 20
echo "absolute path : "`pwd`"/$filename"
#total_num=`expr $dir_num + $file_num + $Sfile_num`
tput cup 36 20
echo "$total_num total	$dir_num dir	$file_num file	$Sfile_num Sfile	$total_size byte"
fi
if [ $P -eq 1 ];then
l=21
		fi
tput cup 38 0
}
information
	copyfile_1=' '
	copyfile_2=' '
	copyfile_3=' '

function copy
{
		present_directory=$(pwd)
#filename_copied=$filename
				if [ -z $copyfile_1 ];then
		copyfile_1=$present_directory/$filename
				filename_copied_1=$filename
				fi
	if [ -z $copyfile_2 ];then
		copyfile_2=$present_directory/$filename
				filename_copied_2=$filename
				fi
	if [ -z $copyfile_3 ];then
		copyfile_3=$present_directory/$filename
				filename_copied_3=$filename
				fi
				if [ $copyfile_1 = $copyfile_2 ];then
						copyfile_2=' '
								fi
								if [ $copyfile_1 = $copyfile_3 ];then
						copyfile_3=' '
								fi
if [ $copyfile_2 = $copyfile_3 ];then
						copyfile_3=' '
								fi




}
function copy_paste
{
		`cp $copyfile_1 $filename_copied_1`
if [ -n $copyfile_2 ];then
	`cp $copyfile_2 $filename_copied_2`
			fi
			if [ -n $copyfile_3 ];then
	`cp $copyfile_3 $filename_copied_3`
			fi

					present_directory=' '
				filename_copied=' '
		copyfile_1=' '
		copyfile_2=' '
		copyfile_3=' '

}

	movefile_1=' '
	movefile_2=' '
	movefile_3=' '

function move
{
		move_present_directory=$(pwd)
#filename_copied=$filename
				if [ -z $movefile_1 ];then
		movefile_1=$move_present_directory/$filename
				filename_moved_1=$filename
				fi
	if [ -z $movefile_2 ];then
		movefile_2=$move_present_directory/$filename
				filename_moved_2=$filename
				fi
	if [ -z $movefile_3 ];then
		movefile_3=$move_present_directory/$filename
				filename_moved_3=$filename
				fi
				if [ $movefile_1 = $movefile_2 ];then
						movefile_2=' '
								fi
								if [ $movefile_1 = $movefile_3 ];then
						movefile_3=' '
								fi
if [ $movefile_2 = $movefile_3 ];then
						movefile_3=' '
								fi




}
function move_paste
{
		`mv $movefile_1 $filename_moved_1`
if [ -n $movefile_2 ];then
	`mv $movefile_2 $filename_moved_2`
			fi
			if [ -n $movefile_3 ];then
	`mv $movefile_3 $filename_moved_3`
			fi

					move_present_directory=' '
				filename_moved=' '
		movefile_1=' '
		movefile_2=' '
		movefile_3=' '

}
P_5=`expr $P % 5 `
while :
do
tput cup 38 0
read -n 1 key
if [ "$key" = "" ];then
read -n 2 nkey
if [ "$nkey" = "[A" ];then
if [ $P -ge 1 -a $P -le 5 ];then #이런식으로 위 말고도 아래 좌우도 조건을 만들어야함
if [ $Q -ge 5 ];then
Q=`expr $Q - 5`
fi
outline
center_file_name
information
else
P=`expr $P - 5`
outline
center_file_name
information
fi
elif [ "$nkey" = "[B" ];then
P=`expr $P + 5`
if [ $P -gt 25 ];then
Q=`expr $Q + 5`
P=`expr $P - 5`
fi
outline
center_file_name
information
if [ -z $filename ];then
P_5=`expr $P % 5`
P=`expr $P - 5`
outline
center_file_name
information
fi
elif [ "$nkey" = "[D" ];then
if [ $l -eq 21 ];then
outline
center_file_name
information
else
P=`expr $P - 1`
outline
center_file_name
information
fi
elif [ "$nkey" = "[C" ];then
P_5=`expr $P % 5`
if [ $l -eq 89 ];then
outline
center_file_name
information
else
P=`expr $P + 1`
outline
center_file_name
information
fi
fi
if [ -z $filename ];then
P=`expr $P - 1`
outline
center_file_name
information
fi
elif [ -z "$key" ];then
#echo "spcebar"
#if [ $Q -eq 0 ];then
if [ $P -eq 1 -a $Q -eq 0 ];then
cd ..
elif [ "`stat -c %F $filename`" = "디렉토리" ]; then #디렉토리 조건
cd $filename
P=1
Q=0
elif [ "`ls -al $filename | cut -c 10`" = "x" ]; then
./$filename
fi
outline
center_file_name
information
#fi

#else
elif [ "$key" = "c" ];then
copy
echo "copy !!"
elif [ "$key" = "p" ];then
copy_paste

outline
center_file_name
information

elif [ "$key" = "m" ];then
move
echo "move !!"
elif [ "$key" = "v" ];then
move_paste

outline
center_file_name
information
fi
echo "$NEW"
echo "$P"
echo "${arr[20]}"
echo "$l"
#if [ -z $filename ];then
#echo "00000000000000000000"
#fi
NEW=''
done

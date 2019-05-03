#!/bin/bash

# NB: no trailing / 
#SERVER=makalu
#SERVER=knl
SERVER=k2
#SERVER=localhost
LOCAL_PATH=/home/xzl/video-streamer
REMOTE_PATH=$SERVER:/home/xzl/video-streamer

#LOCAL_PATH=/home/teddyxu/video-streamer
#REMOTE_PATH=$SERVER:/home/teddyxu/video-streamer

# this causes terminal killed. why?
#set -e 

push() {
	# first backup the remote repo to a remote dir
	# http://stackoverflow.com/questions/305035/how-to-use-ssh-to-run-shell-script-on-a-remote-machine
	ssh $SERVER 'bash -s' < backup-repo.sh

	read -rsp $'Press any key to start pushing...\n' -n1 key
	
	# -u: skip files that are newer on the receiver
	# delete receiver's files not existing on the sender. It's okay: we have backup
	rsync -a -v -u \
	--exclude='*.bin' --exclude='*.o' --exclude='*.a' --exclude='*.so' \
	${LOCAL_PATH}/ ${REMOTE_PATH}


	# after rsync, compare to see diff: 
	echo "Remote files failing to sync because they're newer than local"
	rsync -a --dry-run --itemize-changes \
	--exclude='*.bin' --exclude='*.o' \
	${LOCAL_PATH}/ ${REMOTE_PATH}
	echo "----- end ---- "

	echo "push to ${REMOTE_PATH} completes"
}



pull() {
	sh backup-repo.sh

	read -rsp $'Press any key to start pulling...\n' -n1 key
	
	# do not delete extra files on local
	rsync -a -v -u \
	--exclude='*.bin' --exclude='*.o' \
	${REMOTE_PATH}/ ${LOCAL_PATH}

	echo "Local files failing to sync because they're newer than remote"
	rsync -a --dry-run --itemize-changes \
	--exclude='*.bin' --exclude='*.o' \
	${REMOTE_PATH}/ ${LOCAL_PATH}
	echo "----- end ---- "

	echo "pull from ${REMOTE_PATH} completes"
}

export ROOTDIR=${PWD}

if [[ -z $ARCH ]]; 
then 
	#export ARCH=avx2 # armtz, x86, knl, avx2...
	export ARCH=x86 # armtz, x86, knl, avx2...
fi

rm -f ${ROOTDIR}/CMakeCache.txt

config-creek() {
	# config debug
	cd ${ROOTDIR}/cmake-build-debug
	/usr/bin/cmake \
	-DCMAKE_BUILD_TYPE=Debug \
	-DCMAKE_ENV=${ARCH} \
	${ROOTDIR}

	# config release
	cd ${ROOTDIR}/cmake-build-release
	/usr/bin/cmake \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_ENV=${ARCH} \
	${ROOTDIR}
	
	cd ${ROOTDIR}
}

build-dbg-creek() {
	/usr/bin/cmake \
	--build ${ROOTDIR}/cmake-build-debug \
	--target all -- \
	-j `nproc`
}

build-rls-creek() {
	/usr/bin/cmake \
	--build ${ROOTDIR}/cmake-build-release \
	--target all -- \
	-j `nproc`
}

build-all() {
    build-dbg-creek
    build-rls-creek
}


mkdir -p /tmp/xzl/Debug
mkdir -p /tmp/xzl/Release

#mkdir -p /tmp/teddyxu/Debug
#mkdir -p /tmp/teddyxu/Release

echo "script loaded"
echo "ARCH=" ${ARCH}
echo "available commands: pull and push"

echo "add cuda lib to lib path"
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/cuda/lib64

#
# make tmpfs using
# sudo mount -t tmpfs -o size=100M,nr_inodes=1000,mode=777 tmpfs /shared/tmp

#!/bin/bash

readonly image=cnc_pneumatics:latest
readonly hostname_=cnc_pneumatics

# Running in a TTY?
test -t 1 && USE_TTY="-it"

# Exit with error on interrupt, or failure
set -e

# Check docker is installed
if ! which docker >& /dev/null; then
   echo "Could not find 'docker'. You must install  docker first"
   exit
fi

# If the docker image does not yet exists, build it
if (($(docker images -q $image | wc -l) == 0)); then
   docker build -t $image . || { echo "Failed to build the docker image"; exit; }
fi

# Variables
this_script_dir=$(cd -P $(dirname $0) ; pwd)
base_opts="-u $(id -u $USER):$(getent group docker | cut -d: -f3) -v $this_script_dir:$this_script_dir"
workdir="$(realpath $(pwd))"

# Name the runtime container uniquely
container_name="AVR_$(id -u)_$(date +%Y%m%d-%H%M%S)"

# Detect number of CPUs. Environment variable J can override the value. If no value given, then 1 is used
if [[ -v J ]]; then
   ((core_count=j)); ((core_count)) || let core_count=1
else
   core_count=$(grep ^cpu\\scores /proc/cpuinfo | uniq |  awk '{print $4}')
fi

# Run make or start a shell
if [[ "$1" == "shell" ]]; then
   shift
   docker run ${USE_TTY} --rm --init --name $container_name $xoptions -h $hostname_ $base_opts -w $workdir $image $@
else
   docker run ${USE_TTY} --rm --init --name $container_name -h $hostname_ $base_opts -w $workdir $image make -j$core_count $@
fi
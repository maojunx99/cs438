#!/bin/bash
seconds=$((1800))
for i in {21..40}
do
    python CS_438/auto-code.py $i 
    typeset -i version=$(cat version.txt)

    printf "\nchanging version from $version to $((version+1))\n\n"
    version=$((version+1))
    echo "$version" > version.txt

    git add -u
    git commit -m "incremented version to $version"
    git push origin main
    sleep ${seconds}
done
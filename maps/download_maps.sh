#! /bin/bash

set -ex

function download_files() {
    echo `pwd`
    wget 'http://punter.inf.ed.ac.uk/graph-viewer/maps.json'
    field_names="maps other_maps"
    for field_name in ${field_names};
    do
        for map_name in `cat maps.json | jq -r .${field_name}[].filename`;
        do
            map_filename=http://punter.inf.ed.ac.uk/maps/`basename ${map_name}`
            wget ${map_filename}
        done;
    done;
}

cwd=`dirname "${0}"`
cd ${cwd} && download_files


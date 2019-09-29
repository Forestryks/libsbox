#!/bin/bash

CONF_PREFIX=/etc/libsboxd

set -x

mkdir -p ${CONF_PREFIX}
cp conf.json ${CONF_PREFIX}/conf.json

#!/bin/bash

CONF_PREFIX=/etc/libsboxd

set -x

mkdir -p ${CONF_PREFIX}

read -rp "Update config file? [y/N]" yn
case $yn in
    [Yy]* ) cp conf.json ${CONF_PREFIX}/conf.json;;
esac

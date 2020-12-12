#!/bin/bash
for i in {1..30}
do
   head -c 20 /dev/urandom | xxd -p -u >> shared.secrets
done

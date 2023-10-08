#!/bin/bash

#docker run  -it da_image /bin/bash 
podman run -it --volume ./template_cpp/src:/root/template_cpp/src:z cs451-project /bin/bash
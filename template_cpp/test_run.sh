trap 'kill %1; kill %2;' SIGINT

./run.sh --id 2 --hosts ./example/hosts --output ./example/output/proc02.output ./example/configs/lattice-agreement-1.config & 
./run.sh --id 1 --hosts ./example/hosts --output ./example/output/proc01.output ./example/configs/lattice-agreement-2.config & 
./run.sh --id 3 --hosts ./example/hosts --output ./example/output/proc03.output ./example/configs/lattice-agreement-3.config
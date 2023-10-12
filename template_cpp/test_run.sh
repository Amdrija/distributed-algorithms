trap 'kill %1; kill %2;' SIGINT

./run.sh --id 2 --hosts ./example/hosts --output ./example/output/2.output ./example/configs/perfect-links.config & 
./run.sh --id 1 --hosts ./example/hosts --output ./example/output/1.output ./example/configs/perfect-links.config & 
./run.sh --id 3 --hosts ./example/hosts --output ./example/output/3.output ./example/configs/perfect-links.config
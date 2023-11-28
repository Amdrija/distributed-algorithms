#!/usr/bin/env python3

import subprocess
import sys
import os

if __name__ == "__main__":
    num_processes = int(sys.argv[1])
    output_dir = sys.argv[2]
    counts = []

    validate_fifo = os.path.join(os.getcwd(), 'tools/validate_fifo.py')
    command = [validate_fifo,'--proc_num',str(num_processes)]
    for i in range(1, num_processes + 1):
        output_file = "{}/proc{:02d}.output".format(output_dir,i)
        file_path = os.path.join(os.getcwd(), output_file)
        command.append(file_path)
    proc = subprocess.run(command, stdout=subprocess.PIPE)
    
    file = ""
    error = ""
    correct_count = 0
    for line in proc.stdout.decode().splitlines():
        if "Checking" in line:
            file = line.split(" ")[1].split("/")[-1]
        elif "Validation" in line:
            if "OK" in line:
                correct_count += 1
            else:
                print(error)
        else:
            error = line
    
    print("OK: {}".format(correct_count))
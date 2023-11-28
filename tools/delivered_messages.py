#!/usr/bin/env python3

import subprocess
import sys
# import pandas
import os
# import numpy as np

if __name__ == "__main__":
    num_processes = int(sys.argv[1])
    output_dir = sys.argv[2]
    counts = []

    for i in range(1, num_processes + 1):
        output_file = "{}/proc{:02d}.output".format(output_dir,i)
        file_path = os.path.join(os.getcwd(), output_file)
        grep = subprocess.Popen(['grep','d',file_path], stdout=subprocess.PIPE)
        result = subprocess.check_output(['wc','-l'], stdin=grep.stdout).decode()
        grep.wait()
        delivered = int(result.split(" ")[0])
        counts.append(delivered)

    print(counts)
    print(sum(counts))
    # print(np.var(counts))
    # df = pandas.DataFrame(counts, columns="count")
    # print(df.describe)
    # print("TOTAL: " + str(df.sum()))

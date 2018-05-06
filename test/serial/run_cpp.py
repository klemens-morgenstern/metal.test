import argparse
import json
import os
import sys
import tempfile
import os.path

parser = argparse.ArgumentParser(description='Test script')

parser.add_argument('--serial')
parser.add_argument('--source-dir')
parser.add_argument('--exe')
parser.add_argument('--bin-dir')

args = parser.parse_args()

exe = args.exe
source_dir = args.source_dir
serial = args.serial
temp_file = os.path.join(args.bin_dir, "run_cpp.tmp")

import subprocess

cwd = os.getcwd();

print (exe, serial)

bin_output = subprocess.check_output([exe], stderr=subprocess.STDOUT)

json_output  = None

proc = subprocess.Popen([serial, exe, source_dir, "--metal-test-no-exit-code", "--metal-test-format", "json",
                                  "--metal-test-sink", temp_file], stdin=subprocess.PIPE,  stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
proc.stdin.write(bin_output)
output = proc.communicate()[0].decode().splitlines()

print (output)

assert output[0].startswith("Initializing metal serial from")
assert output[1] == "a 42 12 test-string 401867 foo-str"

assert proc.returncode == 0

json_output = json.load(open(temp_file))


hrf_proc  = subprocess.Popen([serial, exe, source_dir], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
hrf_proc.stdin.write(bin_output)
out, err = hrf_proc.communicate()

assert hrf_proc.returncode == 1

#comparison
comparison = open(os.path.join(os.path.dirname(__file__), "cpp.out")).read().splitlines()
for c, m in zip(comparison, out.decode().splitlines()):
    if not m.endswith(c):
        print ("Line mismatch: ", c, m)
        sys.exit(1)


sys.exit(0)
import argparse
import os
import sys

parser = argparse.ArgumentParser(description='Test script')
parser.add_argument('--root', metavar='R', type=str, # nargs='+',
                    help='an integer for the accumulator')
parser.add_argument('--mwt', metavar='M',  type=str,
                    help='mwt file to test.')
parser.add_argument('--grp', metavar='G', nargs='*', help='Groups')

parser.add_argument('bin', nargs='*', help='binaries!')


args = parser.parse_args()

make = None;
gen = None;
runner = None;

for arg in args.bin:
    base = os.path.basename(arg)
    stem = os.path.splitext(base)[0]
    if stem == "metal-test-make":
        make = os.path.abspath(arg)
    elif stem == "metal-test-gen":
        gen  = os.path.abspath(arg)
    elif stem == "metal-gdb-runner":
        runner = os.path.abspath(arg);

import subprocess

cwd = os.getcwd();

root = os.path.normpath(str(args.root))
mwt  = os.path.normpath(str(args.mwt))

out_prefix = os.path.join(root, "bin", "custom_test")
out_path   = os.path.join(out_prefix, os.path.splitext(mwt)[0])
out_make   = out_path + ".mk";

config = os.path.join(out_prefix, "config.mk");

f = open(config, 'w')
f.write("GDB-RUNNER = " + runner + "\n")
f.write("RFLAGS = --debug --log ./out_prefix-runner.log")
f = None

if not os.path.exists(out_path):
    os.makedirs(out_path)

make_out = subprocess.check_output(
    [make, '-I', mwt, '-D', out_path, '-O', out_make, '-H', config], shell=True)


process = subprocess.Popen(['make', '-f', out_make], shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
make_out = process.communicate()[0]
make_out += process.communicate()[1]

if len(make_out) > 0:
    print ("Makefile output")
    print ("------------------------------");
    print (make_out.decode("cp1252"));
    print ("------------------------------");
    print ("Makefile returned " + str(process.returncode)); 


sys.exit(process.returncode);    
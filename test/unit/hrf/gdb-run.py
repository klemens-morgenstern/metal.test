import argparse
import os
import sys

parser = argparse.ArgumentParser(description='Test script')
parser.add_argument('--root', metavar='R', type=str, # nargs='+',
                    help='an integer for the accumulator')
parser.add_argument('--compare', metavar='C',  type=str,
                    help='outdate to compare to')
parser.add_argument('--exe', metavar='E', type=str, 
                    help="The stem name of the executable to test")
parser.add_argument('--return_code', metavar='R', type=int, 
                    help="Expected return code")
parser.add_argument('--runner', type=str)
parser.add_argument('--unit', type=str)


parser.add_argument('bin', nargs='*', help='binaries!')
print (sys.argv)

args = parser.parse_args()

exe = args.exe;
runner = args.runner;
unit = args.unit;

import subprocess

cwd = os.getcwd();
root = os.path.normpath(str(args.root))

print("Root: " + os.path.normpath(args.root))
print ("FILE " + os.path.realpath(__file__))
print ("PWD  " + os.getcwd())

#(GDB-RUNNER) --gdb $(GDB) --exe F:\mwspace\test\unit\test\hrf\bin\custom_test\empty_test\empty_test.exe --lib F:\mwspace\test\bin\debug\libmw-test-unit.dll $(RFLAGS) > F:\mwspace\test\unit\test\hrf\bin\custom_test\empty_test\empty_test.run
process = subprocess.Popen([runner, "--exe", exe, "--lib", unit],
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)


make_out = process.communicate()[0].decode().splitlines()
x
result = 0

file_content = None;
with open(os.path.join(args.root , args.compare), mode='r') as cmp:
    file_content = cmp.read();

cmp = file_content.splitlines();

if len(cmp) != len(make_out):
    print ("Length mismatch: " + str(len(cmp)) + " != " + str(len(make_out)))
    result = 1
    print ("Expected" )
    for val in cmp:
        print (val);
    
    print ("-----------------------\n")
    print ("Actually got")
    for val in make_out:
        print (val);
    print ("-----------------------\n")
    
    
import itertools
idx = 0
for c, m in zip(cmp, make_out):
    idx += 1
    if c.startswith(args.exe):
        c = c.replace(args.exe, 
                      os.path.relpath(os.path.join(args.root, args.exe)), 1)
        m = m.replace("\\\\", "\\");
    if not m.strip().endswith(c.strip()):
        print ('Mismatch [' + str(idx) + '] "' + c.strip() + '" != "' + m.strip().replace("\\\\", "\\") + '"')
        result = 1;
    else:
        print (c)

if process.returncode != None:
    if args.return_code != process.returncode:
        print ("Mismatching return code: " + str(args.return_code) + " != " + str(process.returncode))
        result = 1
        sys.exit(1)
    else:
        sys.exit(result)
else:
    sys.exit(result + process.returncode);
      

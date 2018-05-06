import argparse
import os
import sys

parser = argparse.ArgumentParser(description='Test script')
parser.add_argument('--root', metavar='R', type=str, # nargs='+',
                    help='an integer for the accumulator')
parser.add_argument('--exe', metavar='E', type=str, 
                    help="The stem name of the executable to test")

parser.add_argument('--runner')
parser.add_argument('--unit')

args = parser.parse_args()

exe = args.exe;
runner = args.runner;
unit = args.unit;

import subprocess

cwd = os.getcwd();

root = os.path.normpath(str(args.root))
print (exe, unit)

output = subprocess.check_output([runner, "--exe", exe, "--lib", unit, "--metal-test-format", "json", "--metal-test-no-exit-code"], stderr=subprocess.STDOUT)

import json

out = json.loads(output.decode())

from collections import namedtuple
data = json.loads(output.decode(), object_hook=lambda d: namedtuple('TestData', d.keys())(*d.values()))
print (data) 

executed = data.free_tests.warnings 
warnings = data.free_tests.executed 
errors   = data.free_tests.errors

assert len(data.content) == 12

test = data.content[0]

assert test.type == "execute_check"
assert test.lvl == "assertion"
assert test.line == 195
assert test.critical == False
assert test.condition

test = data.content[1]

assert test.type == "execute_check"
assert test.lvl == "expectation"
assert test.line == 196
assert test.critical == False
assert test.condition


test = data.content[2]
assert test.type == "case"
assert test.id == "equal test"
assert test.line == 198

tc = test.content[0]

assert tc.bitwise == False
assert tc.rhs == "j"
assert tc.lhs == "i"

assert tc.critical == False
assert tc.lvl == "assertion"
assert tc.condition == False
assert tc.type == "equal"

tc = test.content[1]

assert tc.bitwise == False
assert tc.rhs == "k"
assert tc.lhs == "l"

assert tc.critical == False
assert tc.lvl == "expectation"
assert tc.condition == False
assert tc.type == "equal"


assert test.summary.executed == 8
assert test.summary.warnings == 3 
assert test.summary.errors   == 2
assert test.result == "exit"


assert data.free_tests.executed == 4
assert data.free_tests.warnings == 1
assert data.free_tests.errors == 1

assert data.summary.warnings == 21
assert data.summary.executed == 71
assert data.summary.errors == 20



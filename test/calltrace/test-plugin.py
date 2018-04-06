import argparse
import os
import sys

parser = argparse.ArgumentParser(description='Test script')
parser.add_argument('--root', metavar='R', type=str, # nargs='+',
                    help='an integer for the accumulator')


parser.add_argument('--calltrace')
parser.add_argument('--hrf_cmp_ts')
parser.add_argument('--runner')
parser.add_argument('--hrf_cmp')
parser.add_argument('--plugin_test')
parser.add_argument('--plugin_test_ts')

args = parser.parse_args()

plugin_test     = args.plugin_test
plugin_test_ts  = args.plugin_test_ts
runner          = args.runner
calltrace       = args.calltrace
hrf_cmp_file    = args.hrf_cmp
hrf_cmp_ts_file = args.hrf_cmp_ts

import subprocess

cwd = os.getcwd();

root = os.path.normpath(str(args.root))

print ("FILE      : " + os.path.realpath(__file__))
print ("PWD       : " + os.getcwd())
print ("CT        : " + calltrace)
print ("RN        : " + runner)
print ("Plugin-Ts : " + plugin_test_ts)
print ("Plugin    : " + plugin_test)
print ("HRF-CMP   : " + hrf_cmp_file)
print ("HRF-CMP-ts: " + hrf_cmp_ts_file)

errored = False

plugin_test_ts_out = subprocess.check_output([runner, "--exe", plugin_test_ts, "--lib", calltrace, "--metal-calltrace-timestamp"]).decode()
plugin_test_out    = subprocess.check_output([runner, "--exe", plugin_test,    "--lib", calltrace, "--metal-calltrace-timestamp"]).decode()


import re

hex_regex = re.compile(r"@0x[0-9A-Za-z]+")
ts_regex = re.compile(r"with timestamp \d+")


plugin_test_out    = ts_regex.sub("with timestamp --timestamps--", hex_regex.sub("--hex--", plugin_test_out)).splitlines()
plugin_test_ts_out = ts_regex.sub("with timestamp --timestamps--", hex_regex.sub("--hex--", plugin_test_ts_out)).splitlines()


with open(hrf_cmp_file) as f:
    hrf_cmp = f.read().splitlines()
    i = 1
    for out, cmp in zip(plugin_test_out, hrf_cmp):
        if not out.startswith(cmp):
            print(hrf_cmp_file + '(' + str(i) + '): Mismatch in comparison : "' + out + '" != "' + cmp + '"')
            errored = True
        i+=1


with open(hrf_cmp_ts_file) as f:
    hrf_cmp_ts = f.read().splitlines()
    i = 1
    for out, cmp in zip(plugin_test_ts_out, hrf_cmp_ts):
        if not out.startswith(cmp):
            print(hrf_cmp_ts_file + '(' + str(i) + '): Mismatch in comparison : "' + out + '" != "' + cmp + '"')
            errored = True;
        i+=1

plugin_test_out    = subprocess.check_output([runner, "--exe", plugin_test,    "--lib", calltrace, "--metal-calltrace-timestamp"]).decode()


min = subprocess.check_output([runner, "--exe", plugin_test, "--lib", calltrace, 
                               "--metal-calltrace-timestamp", "--metal-calltrace-minimal", "--metal-calltrace-format=json"])

import json

min_json = json.loads(min.decode())

def compare(Lhs, Rhs):
    if Lhs != Rhs:
        errored = True
        print ('Error: "' + str(Lhs) + '" != "' + str(Rhs) + '"');

compare(len(min_json["calls"]), 0)

err = min_json["errors"];

compare(err[0]["mode"], "error")
compare(err[0]["type"], "missing-timestamp");

compare(err[1]["mode"], "set")
compare(err[2]["mode"], "set")
compare(err[3]["mode"], "set")
compare(err[4]["mode"], "set")
compare(err[5]["mode"], "set")
compare(err[6]["mode"], "set")
compare(err[7]["mode"], "set")

def get_ptr(Value):
    return Value["calltrace"]["location"]

addr = [get_ptr(err[1]), get_ptr(err[2]), get_ptr(err[3]), get_ptr(err[4]), get_ptr(err[5]), get_ptr(err[6]), get_ptr(err[7])]



compare(err[8]  ["mode"], "error")
compare(err[9]  ["mode"], "error")
compare(err[10] ["mode"], "error")
compare(err[11] ["mode"], "error")

compare(err[8] ["type"], "mismatch")
compare(err[9] ["type"], "mismatch")
compare(err[10] ["type"], "overflow")
compare(err[11]["type"], "incomplete")

compare(get_ptr(err[11]), addr[0])
compare(get_ptr(err[12]), addr[1])
compare(get_ptr(err[13]), addr[2])
compare(get_ptr(err[14]), addr[3])
compare(get_ptr(err[15]), addr[4])
compare(get_ptr(err[16]), addr[5])


out = subprocess.check_output([runner, "--exe", plugin_test, "--lib", calltrace, 
                               "--metal-calltrace-timestamp", "--metal-calltrace-format=json", "--metal-calltrace-depth=3"])

jsn = json.loads(out.decode())
compare(len(jsn["calls"]), 10)

out = subprocess.check_output([runner, "--exe", plugin_test, "--lib", calltrace, 
                               "--metal-calltrace-timestamp", "--metal-calltrace-format=json", "--metal-calltrace-depth=4"])

jsn = json.loads(out.decode())
compare(len(jsn["calls"]), 14)

out = subprocess.check_output([runner, "--exe", plugin_test, "--lib", calltrace, 
                               "--metal-calltrace-timestamp", "--metal-calltrace-format=json", "--metal-calltrace-all"])

jsn = json.loads(out.decode())
if len(jsn["calls"]) <= 14:
    print ("Too few calls in complete log")
    errored = True

if errored == True:
    print ("Errored")
    sys.exit(1)
else:
    sys.exit(0)

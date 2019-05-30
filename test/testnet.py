#! /usr/bin/python3

import json
from time import sleep
from subprocess import Popen

fname = '../test/relay.conf'
f = open(fname)
nodes = json.load(f)["nodes"]

processes = []
clients = []
num_clients = 5
node_names = []

for name in nodes:
    node_names.append(name)
    p = Popen(['./relay-node', name, fname])
    processes.append(p)

# wait for servers to start
sleep(2.0)

for i in range(num_clients):
    node = node_names[i % len(node_names)]
    addr = nodes[node]
    c = Popen(['./relay-test', addr, str(i)])
    clients.append(c)

for c in clients:
    c.wait()

for p in processes:
    p.terminate()
    p.wait()

    if p.returncode != 0:
        print("relay node did not shut down correctly")
        exit(1)

exit(0)



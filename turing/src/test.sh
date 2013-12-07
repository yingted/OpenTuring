#!/bin/bash
../bin/tprologc test.t # bytecode goes to test.tbc
../bin/tprolog test.tbc # stuff should work

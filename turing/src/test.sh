#!/bin/bash
cd ../../turing/package
"$OLDPWD"/../bin/tprologc "$OLDPWD"/test.t # bytecode goes to test.tbc
"$OLDPWD"/../bin/tprolog "$OLDPWD"/test.tbc # stuff should work

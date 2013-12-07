#tprolog for linux
Okay, so it works. If you have time, please stub out `turing/src/miowrapper.c` et al.

The purpose of this project is to provide a Turing interpreter for online judges running on Linux.
The main goal, the ~15x improvement over Wine, has already been achieved. Performance is on par with Python, so Turing programs do not need longer time limits.
Standard IO work as expected, so all IO is "to screen" rather than "to file". The performance of IO-bound programs is now acceptable.
For security reasons, system calls and calls to some functions cause an error.

Building
========
In turing/src, `make tprolog`, optionally setting CFLAGS.

Performance tests
=================
These are *not* a Windows vs Linux or MSVC vs GCC test, since the source code here has some fat cut out.

System
------
- 1.66 GHz
- 32-bit
- 1 GB RAM
- Windows 7 Starter with search indexing, antivirus, etc. all disabled
- Fedora 18 Linux with PAE

Turing 4.1.1 on Windows
-----------------------

    fibonacci: 1346269 in 4920 ms
    for loop: 1e7 in 2272 ms
    for loop with decl: 1e7 in 4616 ms
    array: 1e8 in 9383 ms
    string: 1e3 in 1547 ms

OpenTuring on Windows
---------------------

    fibonacci: 1346269 in 3096 ms
    for loop: 1e7 in 1274 ms
    for loop with decl: 1e7 in 2759 ms
    array: 1e8 in 3301 ms
    string: 1e3 in 1082 ms

tprolog on Linux
----------------

    fibonacci: 1346269 in 1820 ms
    for loop: 1e7 in 840 ms
    for loop with decl: 1e7 in 1940 ms
    array: 1e8 in 2280 ms
    string: 1e3 in 570 ms

Python 2.7.3 (not Turing) on Linux
----------------------------------

    fibonacci
    10 loops, best of 3: 2.25 sec per loop
    for loop
    10 loops, best of 3: 1.18 sec per loop
    for loop with decl
    10 loops, best of 3: 1.9 sec per loop
    array
    10 loops, best of 3: 1.71 sec per loop
    string
    10 loops, best of 3: 201 msec per loop

Note: Python is garbage-collected, so taking the best of 3 runs of 10 loops is more accurate.

Conclusion
----------
Python is faster than Turing.

Python benchmark code
---------------------

    #!/bin/sh
    echo fibonacci
    python -mtimeit -s '
    def fib(n):
    	return 1 if n<2 else fib(n-1)+fib(n-2)
    ' 'fib(30)'
    echo for loop
    python -mtimeit 'for _ in xrange(10000000):pass'
    echo for loop with decl
    python -mtimeit 'for _ in xrange(10000000):six=6'
    echo array
    python -mtimeit '[0]*100000000'
    echo string
    python -mtimeit '
    for _ in xrange(1000):
    	s=""
    	while len(s)!=255:
    		s+="a"
    '

Turing benchmark code
---------------------

    var t:=Time.Elapsed
    proc bench(msg:string)
        put msg," in ",Time.Elapsed-t," ms"
        t:=Time.Elapsed
    end bench
    fcn fib(n:int):int
        if n<2 then
    	result 1
        end if
        result fib(n-1)+fib(n-2)
    end fib
    bench("fibonacci: "+intstr(fib(30)))
    for:1..10000000
    end for
    bench("for loop: 1e7")
    for:1..10000000
        var six:=6
    end for
    bench("for loop with decl: 1e7")
    var largearray:array 1..100000000 of int
    bench("array: 1e8")
    for:1..1000
        var s:=""
        loop
            exit when length(s)=255
            s+="a"
        end loop
    end for
    bench("string: 1e3")

Original Readme.md:
#Open Turing 1.0
####Download: https://github.com/downloads/Open-Turing-Project/OpenTuring/package.zip
#####Current lead maintainer: Tristan Hume



Open Turing is an open-source implementation of Turing for Windows.
I (Tristan Hume) acquired the source code from Tom West, the last maintainer of the project.
This version is backwards-compatible with normal turing.

As well as being open-source it is also faster and has more features.
Unfortunately, at the moment many of these features are undocumented.
Look at the "How to Learn About New Features" section for more info.

###Partial change log:
* Up to a 40% speed increase. (depends on program)
* Built in hash maps
* Basic OpenGL 3D. Beta, no input.
* Command line invocation with turing.exe -run file.t
* New splash and logo

###To Get/Download It:
* Download the zip: https://github.com/downloads/Open-Turing-Project/OpenTuring/package.zip
* Or look in the downloads tab
* If you want the development release, click the "master" button and switch to the dev branch, click the zip button, download it and look in the /turing/test folder.

###How to Learn About New Features

Look in the support/predefs folder of the distribution. If you see an interesting looking module name, open the file in turing.

Read the functions in the module. I.E the Hashmap.tu module has the functions for using hashmaps. The GL.tu file has OpenGL functions.

For usage examples (helpful!) look in the Examples/Open Turing folder. These may not exist for every feature.

###Support policy (Or lack thereof)
* If the editor crashes it will show an email. Send panic.log to the email and describe how the crash happened.
* You can also go to the "Issues" tab and post an issue.
* Otherwise, ask for help on http://compsci.ca/

##For Programmers/Developers/Writers/People who want to help out.

###How it works

The Turing environment is written in C with some parts as C compiled from Turing Plus.
Compiling is done through a MS Visual C++ 2010 project (works with express.)

It compiles turing code to bytecode which is executed with a VM. 
If you don't know what that means, go add library functions but don't mess with the language itself.

The main file for the project is /turing/turing.sln this contains the various sub-projects.
The main executable is the "Turing" subproject. The standalone executable project is called "Prolog."
To get a fully working release with your changes you must compile both of these. For testing (if you don't need standalone executables) you only have to compile the "Turing" project.

####Various other sub-projects:
* MIO - The main library. Includes things like Sprites, Drawing, Input, etc...
* Editor - The IDE. Located in the /ready folder but accessed from the main project.
* TLib - Standard library for compiled turing plus
* Coder/Compiler - Translates from turing code to bytecode.
* Executor - The bytecode VM.
* Interperer - The link between the compiler and executor.

####Things you should care about:
* To add library functions:
	1. Add a file to the MIO project. Remember to use MIO prefixes, look at one of the existing files (I.E miofont.c)
	2. MIO prefix = platform independent (or mostly independent.) MDIO = Windows only.
	3. Write wrappers for your functions in "miowrapper.c". Look at the other wrappers for examples.
	4. Add your functions to lookup.c (I think it's in the Coder project.) Remember, the functions must be in alphabetical order.
	5. Write a turing module file that uses "external" declarations to declare the functions using the identifiers you used in lookup.c
	6. Put the file in the support/predefs folder and put the file name in support/predefs/predefs.lst
* The main projects you should care about are Editor, MIO, Executor, Compiler and Coder.
* The VS project builds to /turing/test
* I (Tristan) figured all this out with almost no instruction. So you should be OK.

###How to Submit Contributions:
1. Get github! (Seriously, it's awesome.)
2. Look at some turorials on using github
2. Fork the **development** branch
3. Commit changes to your fork.
4. Send a pull request with your changes.
5. If your changes are any good they will be included in the main distribution.

###How Can I Help?

Look in the issues tab. Either fix things or add new features listed there.

###Legal

Open Turing uses the IGL JPEG Library and the libungif library.

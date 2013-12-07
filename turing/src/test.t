setscreen ("text")
put "av"
var x : int := 3
put "int = ", minint, " .. ", maxint
put "nat = ", minnat, " .. ", maxnat
put "sizeof(int) = ", sizeof (x)
var marks : array 1 .. 5 of int := init(2,4,5,3,6)
var sum : int := 0
for i : 1 .. 5
    sum := sum + marks (i)
end for
var xs : flexible array 1 .. 0 of string
new xs, 5
xs (1) := "avenko"
new xs, 15
new xs, 55
new xs, 100
xs (99) := "occ"
put xs (1), length(xs(1)), index(xs(1),"enko")

loop
    exit when x=0
    x-=1
    var y : int
    put "enter y:"..
    get y
    put y
end loop
function fib (n : int) : int
    if n < 2 then result n
    else result fib(n-1) + fib(n-2)
    end if
end fib
put "fib(20) = ", fib(20)
put "before setx, x = ",x
proc setx (var z : int)
    z := 10
end setx
setx (x)
put "after, x = ", x
put "x & 2 = ", x&2

put "sqrt(16) = ", sqrt(16)
put "ln(10) = ", ln(10)
put "sin(10) = ", sin(10)

put "strint(intstr(x)) = ", strint(intstr(x))

var e : real := 2.718
put "floor (e) = ", floor (e), ", ceil (e) = ", ceil (e), ", round (e) = ", round(e)
put "realstr(e,0) = ", realstr (e,0)
put "chr(65) = ", chr(65)
put "ord('A') = ", ord("A")

var r : real
rand(r)
put "rand: ", r
randint(x,1,100)
put "Random number: ", x
put "Random number 2: ", Rand.Int(1,100)
Rand.Set (16#1234ABCD)

delay(100)

var timeRunning : int
clock (timeRunning)
put "This program has run ", timeRunning, " ms", " alt: ", Time.Elapsed, " ms"

var today : string
date (today)
put "Greetings!!  The date today is ", today

Time.Delay (100)

var timeUsed : int
sysclock ( timeUsed )
put "This program has used ", timeUsed,
    " milliseconds of CPU time", " alt: ", Time.ElapsedCPU

put ""
%records
type phoneRecord :
	    record
		name : string ( 20 )
		phoneNumber : int
		address : string ( 50 )
	    end record
var oneEntry : phoneRecord
oneEntry.name := "AV, Enko"
oneEntry.phoneNumber := 4869
oneEntry.address := "4869 av st"


% pointer
type intPointer : unchecked ^int %example type name 
%note: short form unchecked ^int 

var anInt : int %declares a variable 
anInt := 9001 

var Pointer : intPointer %declares pointer 

Pointer := cheat (intPointer, addr (anInt)) 
%sets address stored by pointer to be the address of variable 
%note: cheat only accepts a type identifier as first parameter 

#Pointer := addr (anInt) %short form of the pointing 

anInt := 2 %value pointed to by pointer should change as well 
put ""
put "anInt = ", anInt 
put "Pointer to anInt = ", ^Pointer %should print same thing as variable 

put ""
put "testing class"
class Weapon 
    export initialize, display 

    var name, description : string 
    var weight, cost, damage, range : int 

    proc initialize 
	name := "Weapon" 
	description := "Grasp weapon in chosen hand and use to smite thine enemy." 
	weight := 5 
	cost := 30 
	damage := 8 
	range := 0      % This is a melee weapon 
    end initialize 

    proc display 
	put name 
	put description 
	put "Weight: ", weight 
	put "Cost:   ", cost 
	put "Damage: ", damage 
	put "Range:  ", range 
    end display 

end Weapon 

class Sword 
    inherit Weapon 
end Sword 

class Bow 
    inherit Weapon 
end Bow 

var my_sword : pointer to Sword 
new Sword, my_sword 

Sword (my_sword).initialize 

Sword (my_sword).display 

put ""
put "Test FP"
function findRoot (f : function f (x : real) : real, low, high, incriment : real) : real 
    % Find the first root of the function f(x) 
    % starting from 'low' and ending at 'high'. 
    var x := low 
    loop 
	exit when x > high 
	% The rounding here is to avoid things like 
	% 0.000000000023345 not equalling 0 
	if round (f (x) / incriment) = 0 then 
	    result x 
	end if 
	x += incriment 
    end loop 
    % No root was found for the function f(x) = x^2 - 4 
    % between x = low and x = high 
    % Return a signal that no root was found. 
    result minint 
end findRoot 

function f (x : real) : real 
    result x ** 2 - 4 
end f 
put "The first root that between -10 and 10 for the function f(x) = x**2 - 4 is: ", findRoot (f, -10, 10, 0.1) 

put ""
put "Test HashTable"
% IntHashTable
var h : int := IntHashMap.New()
var res : int := 9789

IntHashMap.Put(h,"hi",5)
IntHashMap.Put(h,"yo",7)

put "successful: ",IntHashMap.Get(h,"hi",res)
put "should be 5:",res

put "ITEM HI REMOVED"
IntHashMap.Remove(h,"hi")

put "get successful: ",IntHashMap.Get(h,"hi",res)
put "should be 0: ",res

put ""

put "successful: ",IntHashMap.Get(h,"yo",res)
put "should be 7: ",res

IntHashMap.Free(h)

>Predefined Word List
+ "AGAIN"
  - usage: (-)
  - action: unconditionally restarts at matching BEGIN
  - related: BEGIN
+ "BEGIN"
  - usage: (-)
  - action: point at which a related word restarts
  - related: AGAIN, UNTIL
+ "BUTTON"
  - usage: (- value)
  - action: reads the current state of buttons
  - related: *A, *B, *DOWN, *LEFT, *RIGHT, *UP
+ "CIRCLE"
  - usage: (x y radius color -)
  - action: draws filled circle
  - related: RCTNGL, *RED, *GREEN, *BLUE
+ "CUP"
  - usage: (row column -)
  - action: alphanumeric cursor position
  - related: 
+ "DAC"
  - usage: (r1 -)
  - action: send number to digital to analog converted, which in turn is sent to the speaker.
  - related: DELAY, UDELAY
+ "DELAY"
  - usage: (r1 -)
  - action: wait for specified number of milliseconds
  - related: UDELAY
+ "DO"
  - usage: (limit initial -)
  - action: start of loop with value starting at initial and going to but not including limit
  - related: I, LOOP
+ "DROP"
  - usage: (r1 -)
  - action: remove the top element on the stack
  - related: DUP, ROT, OVER, SWAP
+ "DUP"
  - usage: (r1 - r1 r1)
  - action: copy the top element on the stack
  - related: DROP, ROT, OVER, SWAP
+ "ELSE"
  - usage: (-)
  - action: marks second part of IF ELSE THEN which is executed when value on top of stack at IF is zero.
  - related: IF THEN
+ "EMIT"
  - usage: (r1 -)
  - action: interpreting top of stack as Base 40, print on screen at current cursor position
  - related: CUP, .
+ "GET"
  - usage: (r1 - value)
  - action: retrieve 32 bit value from memory array, indexed by top of stack and place on top of stack
  - related: GETBYT, PUT, PUTBYT
+ "GETBYT"
  - usage: (r1 - value)
  - action: retrieve 8 bit value from memory array, indexed by top of stack and place on top of stack
  - related: GET, PUT, PUTBYT
+ "I"
  - usage: (- value)
  - action: put index value in DO LOOP loop on top of stack
  - related: DO, LOOP
+ "IF"
  - usage: (r1 -)
  - action: execute the next word if the top of stack is non-zero, jump to word after ELSE otherwise or jump to THEN if there is no ELSE
  - related: ELSE, THEN
+ "INTTO."
  - usage (int - float)
  - action: Take the integer value on the top of stack, convert it to floating point and place this value on the stack.
+ "LOOP"
  - usage: (-)
  - action: increment index variable in DO LOOP loop, jump to word after prior DO if the limit has not yet been reached
  - related: DO, I
+ "MOD"
  - usage: (dividend divisor - rem)
  - action: divide dividend by divisor and put the remainder on top of stack
  - related: +, -, *, /, ^, &, |
+ "NOTE"
  - usage: (r1 r2 -)
  - action: Plays a musical note using Karplus Strong synthesis.  The two values on the top of stack govern the duration and wavelength respectively
  - related: 
+ "OVER"
  - usage: (r1 r2 - r1 r2 r1)
  - action: Make of the next to the top on the stack
  - related: DROP, DUP, ROT, SWAP
+ "PLIST"
  - usage: (-)
  - action: list program files on the SD card - filenames are copied to program
  - related: PREAD, PSAVE
+ "PREAD"
  - usage: (name -)
  - action: read named program from SD card and place contents in program.  PREAD often does not work directly after downloading GO Forth to an ODROID GO. Try turning power off, then on.
  - related: PLIST, PSAVE
+ "PSAVE"
  - usage: (name -)
  - action: write (overwrite or create) current program to named file 
  - related: PLIST, PREAD
+ "PUT"
  - usage: (r1 r2 -)
  - action: place r1 32 bit value in memory array as indexed by r2
  - related: GET, GETBYT, PUTBYT
+ "PUTBYT"
  - usage: (r1 r2 -)
  - action: place r1 8 bit value in memory array as indexed by r2
  - related: GET, GETBYT, PUT
+ "QUOTE"
  - usage: (- value)
  - action: place numeric value of following Base 40 word on stack
  - related: EMIT, PREAD, PSAVE
+ "RANDOM"
  - usage: (max - value)
  - action: place random numeric value less than max
  - related: 
+ "RCTNGL"
  - usage: (x y wt ht color -)
  - action: draw filled rectangle with upper left corner at given x y
  - related: CIRCLE, *RED, *GREEN, *BLUE
+ "ROT"
  - usage: (r1 r2 r3 - r2 r3 r1)
  - action: rotate top 3 values on stack per usage
  - related: DROP, DUP, OVER, SWAP
+ "SLOWER"
  - usage: (r1 -)
  - action: turns off sensing for *break* keystroke (A and B buttons concurrently) if top of stack is 0, turns on if top of stack is not zero.  This results in a marked performance improvement but also prevents one from breaking out of an infinite loop without turning off power.
  - related: 
+ "SWAP"
  - usage: (r1 r2 - r2 r1)
  - action: reverse the order of the top two numbers on the stack
  - related: DROP, DUP, OVER, ROT
+ "THEN"
  - usage: (-)
  - action: marks the end of scope for an IF statement, whether the value was zero or non-zero
  - related: IF, ELSE
+ "TIME"
  - usage: (- value)
  - action: pushes the current time in milliseconds on the stack.  there is no real-time clock on an ODROID GO without the optional keyboard so this can be used for elapsed time but not time of day.
  - related: 
+ "UDELAY"
  - usage: (r1 -)
  - action: wait for specified number of microseconds
  - related: DELAY
+ "UNTIL"
  - usage: (r1 -)
  - action: resumes execution after the preceding BEGIN if the top of stack is non-zero.  proceeds to the next word if zero.
  - related: BEGIN, AGAIN
+ ":"
  - usage: (-)
  - action: marks the beginning of the definition of a user-defined word
  - related: ;
+ ";"
  - usage: (-)
  - action: marks the end of the definition of a user-defined word.  also acts as the return statement.
  - related: :
+ "+"
  - usage: (r1 r2 - sum)
  - action: adds the top two numbers on the stack
  - related: -, *, /, MOD, ^, &, |, <<, >>, **
+ "-"
  - usage: (r1 r2 - difference)
  - action: subtracts the top of stack from the penultimate on stack
  - related: +, *, /, MOD, ^, &, |, <<, >>, **
+ "*"
  - usage: (r1 r2 - product)
  - action: multiplies the top two numbers on the stack
  - related: +, -, /, MOD, ^, &, |, <<, >>, **

+ In general, words that begin with * (other than "*" and "**") can be treated as constants.  They push a constant value on the stack.

+ "*A"
  - usage: (- value)
  - action: pushes 16 on the stack.  This equals the value pushed by BUTTON when it detects that the up arrow is pressed.
  - related: BUTTON, *UP, *DOWN, *LEFT, *RIGHT, *B
+ "*B"
  - usage: (- value)
  - action: pushes 32 on the stack.  This equals the value pushed by BUTTON when it detects that the up arrow is pressed.
  - related: BUTTON, *UP, *DOWN, *LEFT, *RIGHT, *A
+ "*BLUE" 
  - usage: (- value)
  - action: pushes 31 on the stack.  when a color value is expected, this evaluates to blue.
  - related: CIRCLE, RCTNGL, *RED, *GREEN
+ "*DOWN"
  - usage: (- value)
  - action: pushes 512 on the stack.  This equals the value pushed by BUTTON when it detects that the up arrow is pressed.
  - related: BUTTON, *UP, *LEFT, *RIGHT, *A, *B
+ "*FALSE"
  - usage: (- value)
  - action: pushes 0 onto the stack
  - related: *TRUE, IF, UNTIL
+ "*GREEN"
  - usage: (- value)
  - action: pushes 2016 on the stack.  when a color value is expected, this evaluates to green.  *GREEN *BLUE | evaluates to cyan.
  - related: CIRCLE, RCTNGL, *RED, *BLUE
+ "*LEFT"
  - usage: (- value)
  - action: pushes 1024 on the stack.  This equals the value pushed by BUTTON when it detects that the up arrow is pressed.
  - related: BUTTON, *UP, *DOWN, *RIGHT, *A, *B
+ "*RED"
  - usage: (- value)
  - action: pushes 63488 on the stack.  when a color value is expected, this evaluates to red.  *RED *GREEN | evaluates to yellow.  *RED *BLUE | evaluates to magenta.
  - related: CIRCLE, RCTNGL, *GREEN, *BLUE
+ "*RIGHT"
  - usage: (- value)
  - action: pushes 2048 on the stack.  This equals the value pushed by BUTTON when it detects that the up arrow is pressed.
  - related: BUTTON, *UP, *DOWN, *LEFT, *A, *B
+ "*TRUE"
  - usage: (- value)
  - action: pushes -1 onto the stack
  - related: *FALSE, IF, UNTIL
+ "*UP"
  - usage: (- value)
  - action: pushes 256 on the stack.  This equals the value pushed by BUTTON when it detects that the up arrow is pressed.
  - related: BUTTON, *DOWN, *LEFT, *RIGHT, *A, *B

+ "**"
  - usage: (base exponent - result)
  - action: takes the base to the exponent power.  negative exponents are not supported.
  - related: +, -, /, MOD, ^, &, |, <<, >>
+ "/"
  - usage: (dividend divisor - value)
  - action: divides the penultimate on the stack by the top of stack
  - related: +, -, *, MOD, ^, &, |
+ "&"
  - usage: (r1 r2 - and)
  - action: performs a logical and between the top two numbers on the stack.
  - related: +, -, *, /, MOD, ^, |
+ "|"
  - usage: (r1 r2 - or)
  - action: performs a logical or on the top two numbers on the stack.
  - related: +, -, *, /, MOD, ^, &
+ "^"
  - usage: (r1 r2 - xor)
  - action: performs an exclusive or on the top two numbers on the stack.
  - related: +, -, *, /, MOD, &, |
+ "."
  - usage: (r1 -)
  - action: prints the top of stack as a signed integer
  - related: CUP, ..

+ In general, words that begin with . (other than ".") are functions that take float values as operands.  Most of these (other than ".TOINT") place a float result back on the stack.

+ ".TOINT"
  - usage: (float - int)
  - action: Take the floating point value on the top of stack, convert it to integer and place this value on the stack.
+ ".+"
  - usage: (float float - sum)
  - action: Take the top two float values on the top of stack, sum them and put the result on the stack.
+ ".-"
  - usage: (float float - difference)
  - action: Take the top two float values on the top of stack, subtract the top value from the second and put the result on the stack.
+ ".*"
  - usage: (float float - product)
  - action: Take the top two float values on the top of stack, multiply the top value by the second and put the result on the stack.
+ "./"
  - usage: (float float - quotient)
  - action: Take the top two float values on the top of stack, divide the second value by the top value and put the result on the stack.
+ ".."
  - usage: (float -)
  - action: prints the top of stack as a floating point number.
  - related: CUP, .
+ "<"
  - usage: (r1 r2 - less?)
  - action: push -1 on the stack if the penultimate value on the stack is less than the top of stack, 0 otherwise.
  - related: =, >
+ "<<"
  - usage: (r1 r2 - result)
  - action: r1 is shifted left r2 number of bits
  - related: +, -, /, MOD, ^, &, |, >>, **
+ "<="
  - usage: (r1 r2 - lessThanEqual?)
  - action: push -1 on the stack if the penultimate value on the stack is less or equal to the top of stack, 0 otherwise.
  - related: =, >
+ "<>"
  - usage: (r1 r2 - notEqual?)
  - action: push -1 on the stack if the penultimate value on the stack is not equal to the top of stack, 0 otherwise.
  - related: =, >
+ "="
  - usage: (r1 r2 - equal?)
  - action: push -1 on the stack if the penultimate value on the stack is equal to the top of stack, 0 otherwise.
  - related: <, >
+ ">"
  - usage: (r1 r2 - greater)
  - action: push -1 on the stack if the penultimate value on the stack is greater than the top of stack, 0 otherwise.
  - related: <, =
+ ">="
  - usage: (r1 r2 - greaterThanEqual?)
  - action: push -1 on the stack if the penultimate value on the stack is greater or equal to the top of stack, 0 otherwise.
  - related: <, =
+ ">>"
  - usage: (r1 r2 - result)
  - action: r1 is shifted right r2 number of bits
  - related: +, -, /, MOD, ^, &, |, <<, **

+ CMDFPUSH
  - usage: (value)
  - action: pushes a floating point number on the stack.  there is no separate word for this.  entering a floating point number implies this word.  So for instance if you have a program that reads "1.20000e+0 2.40000e+0 .+", the program (internally) will consist of 
   1) CMDFPUSH
   2) the floating point representation of 1.2
   3) CMDFPUSH
   4) the floating point representation of 2.4
   5) ".+" encoded as a Base 40 character string

  - related: CMDPUSH

+ CMDPUSH
  - usage: (value)
  - action: pushes an integer on the stack.  there is no separate word for this.  entering an integer implies this word.  So for instance 1 2 + first pushes 1 on the stack, then pushes 2 on the stack, then adds those two (removing them from the stack) and finally pushes the result (3) on the stack.
  - related: 

-------------------------------------------------
>Internals

GO Forth consists of a program stack where the program is stored and a data stack where data may be stored.

A command on the program stack is the Base 40 representation of the word shown above.  The Base 40 representation is always 32 bits.  This effectively limits a Base 40 string to a maximum of 6 characters since 40 to the 6th power is 4096000000. So for instance 
- the Base 40 representation of "A" would be 1, 
- the Base 40 representation of "B" would be 2, 
- the Base 40 representation of "AA" would be 41, 
- the Base 40 representation of "BB" would be 82 and 
- the Base 40 representation of "ABC" would be 1683 or 1 (A) times 40 squared plus 2 (B) times 40 plus 3 (C).  

You can see a list of the Base 40 characters with numeric values by going into alpha mode by holding down the "A" button and pressing the left arrow.  There are only 39 characters shown since 0 is equivalent to blank and blank is not allowed as part of a word.  The last character shown (which is a back arrow) is actually a delete key.

There are two exceptions where a command is not a Base 40 string: CMSFPUSH and CMDPUSH.  Both of these are used to precede another 32 bit word to be loaded on the data stack, which is either a floating point number (in the case of CMDFPUSH) or integer (in the case of CMDPUSH).  The value for CMDFPUSH is -2 or 0xFFFFFFFE and the value for CMDPUSH is -1 or 0xFFFFFFFF.

For example, if you enter the following program "40 2 +", the numeric internal representation would be

Address|Hex Value|Meaning|Data Stack Contents
-------|---------|---|---
01|0xFFFFFFFF|CMDPUSH|
02|0x00000028|40|40
03|0XFFFFFFFF|CMDPUSH
04|0x00000002|2|40, 2
05|0x0000001D|+|42

This sort of trick allows the program to mix data and program words in the same array without any embedded type bits.  I think of this as being analogous to a LDI (or load immediate) assembly language instruction - where the data follows the instruction.

Note that in these two cases, neither instruction appears.  The data does appear and the 32 bits will be interpreted for the purposes of display as a floating point number if preceded by CMDFPUSH and as an integer if preceded by CMDPUSH.

There is one other instruction that loads data onto the data stack.  That instruction is "QUOTE".  The difference in this case is that we see the instruction. 

For example, if you enter the program "10 10 CUP QUOTE ABCDEF EMIT 5000 DELAY", the following will happen:
- "10 10 CUP" will place the cursor position at row 10, column 10.
- "QUOTE ABCDEF" will place the 32 bit representation of the Base 40 string "ABCDEF" on the data stack.
- "EMIT" will print the top of stack, interpreted as a Base 40 encoded string at the current cursor position [10,10].
- "5000 DELAY" will delay for 5000 milliseconds, during which time you will be able to view "ABCDEF" shown at [10,10].
- At the end of the delay, GO Forth will refresh the display of the program and the current contents of the data stack.  This will erase the displayed string ("ABCDEF").
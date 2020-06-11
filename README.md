# GOFORTH
FORTH interpreter/IDE for the ODROID GO
An ODROID GO is a handheld game computer, similar to a Game Boy Color.
GOFORTH is a Forth interpreter which can be programmed on the device.
The screen of an ODROID GO is 320x240 pixels, allowing 26 (readable)
characters per line, with 15 lines (12x16 pixels per character, 10x14 font, really a 5x7 font doubled).
The top line (or more-expanded as necessary) of the screen displays the stack.
The remainder of the screen can contain 4 different displays, depending on the mode.
- If one holds down A and presses up arrow, the remainder of the screen contains the program.
  In this mode, one may execute the program by pressing B or single step through the program
  by holding A down, then pressing B.
- If one holds down A and presses down arrow, the bottom 5 lines display the various defined 
  words (including user-defined words - at some point).  The middle remainder displays the
  program.  One enters a word by selecting one using the arrow keys, then pressing B.
  - To get help on a built-in word, select the word and press the left-most button under the
    display.
  - To delete a word, select it (using the right or left arrows), then press the right-most
    button under the display.  To delete the entire program, hold down A and press the right-
    most button under the display.
  - To paste a previously deleted word, press the center-right button under the display.
  - To clear the stack, press the center-left button under the display.
- If one holds down A and presses the left arrow, one may choose up to 6 characters, selecting
  each character using the arrow keys, then pressing B.  The bottom right character which is
  a left arrow may be chosen to delete the previous entered character.  Hold down A, then 
  press B to enter the assembled string.  There is no difference (internally) between 
  entering a word this way or choosing one from the list, above.
- If one holds down A and presses the right arrow, one may enter an integer by choosing the
  place with the left and right arrows, increment or decrement that place by pressing the 
  up or down arrow.
The bottom 5 lines of the screen are used for input (one selects characters or Words using the
arrows and buttons), in 3 sections
- The left section is for input of alpha and special characters.  The lower right character is to delete the last character from the input buffer.
- The middle section allows one to choose a defined word.  If the Forth script defines a new word, it will show up here as soon as the script is executed.
- In the upper right hand corner is a facility for entering integers.
The remainder of the screen displays the program.
*Words*
- A number in the program will get pushed to the data stack.
- QUOTE followed by alpha or special characters will place the numeric (Base 40) equivalent
  of the characters on the data stack.  This can then be used by certain commands like EMIT,
  PWRITE and PREAD.
- PREAD reads a program with the given name from the microSD card.
- PWRITE write a program with the given name to the microSD card.
- PLIST lists the programs on the microSD card in the program area.  Programs are stored
  in ASCII on the microSD card.
- DAC sends the lowest byte of the top of the stack to the Digital-To-Analog converter
  which in turn sends the corresponding voltage to the speaker.
- DELAY waits for the given number of milliseconds.
- UDELAY waits for the given number of microseconds.
- RCTNGL writes a rectangle on the screen using the five parameters which should be placed
  on the stack, as follows:
  - X coordinate of the upper left-hand corner.
  - Y coordinate of the upper right-hand corner.
  - WIDTH
  - HEIGHT
  - COLOR
- RED preceded by an asterix is used for the color red.
- GREEN preceded by an asterix is used for the color green.
- BLUE preceded by an asterix is used for the color blue.

# GOFORTH
FORTH interpreter/IDE for the ODROID GO
An ODROID GO is a handheld game computer, similar to a Game Boy Color.
GOFORTH is a Forth interpreter which can be programmed on the device.
The screen of an ODROID GO is 320x240 pixels, allowing 26 (readable)
characters per line, with 15 lines (12x16 pixels per character, 10x14 font, really a 5x7 font doubled).
The top line (or more-expanded as necessary) of the screen displays the stack.

The remainder of the screen can contain 4 different displays, depending on the mode.
1) If one holds down A and presses up arrow, the remainder of the screen contains the program.
  In this mode, one may execute the program by pressing B or single step through the program
  by holding A down, then pressing B.
2) If one holds down A and presses down arrow, the bottom 5 lines display the various defined 
  words (including user-defined words, if any have been defined).  The middle remainder displays the
  program.  One enters a word by selecting one using the arrow keys, then pressing B.
  - To delete a word, select it (using the right or left arrows), then press the right-most
    button under the display.  
  - To delete the entire program, hold down A and press the right-
    most button under the display.
  - To paste a previously deleted word, press the center-right button under the display.
  - To clear the stack, press the center-left button under the display.
  - To get help on a built-in word, select the word and press the left-most button under the
    display.
3) If one holds down A and presses the left arrow, one may choose up to 6 characters, selecting
  each character using the arrow keys, then pressing B.  The bottom right character which is
  a left arrow may be chosen to delete the previous entered character.  Hold down A, then 
  press B to enter the assembled string.  There is no difference (internally) between 
  entering a word this way or choosing one from the list, above.
  * Note: you should be able to move from any letter to any other letter with a maximum of 4 arrow key pushes.  Try it.
4) If one holds down A and presses the right arrow, one may enter an integer by choosing the
  place with the left and right arrows, increment or decrement that place by pressing the 
  up or down arrow.


[*Word Definitions*] https://github.com/JaySpencerAnderson/GOFORTH/blob/master/Dictionary.md

[*Example Programs*] https://github.com/JaySpencerAnderson/GOFORTH/blob/master/ExamplePrograms.md

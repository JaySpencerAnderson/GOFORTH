# GOFORTH
FORTH interpreter/IDE for the ODROID GO

An ODROID GO is a handheld game computer, similar to a Game Boy Color.
GOFORTH is a Forth interpreter which can be programmed on the device.
The screen of an ODROID GO has 320x240 pixels, allowing 26 (readable)
characters per line, with 15 lines (12x16 pixels per character, 10x14 font, really a 5x7 font doubled).
The top line (or more-expanded as necessary) of the screen displays the stack.

The remainder of the screen can contain 4 different displays, depending on the mode.
1) If one holds down A and presses up arrow, the remainder of the screen contains the program.
  In this mode, one may execute the program by pressing B or single step through the program
  by holding A down, then pressing B.  Some editing of the program is also done in this mode.
  - Select one of the words by pressing the arrow keys.
  - Delete the selected word by pressing the right-most (or 4th) button under the display (labeled "START").  If no work is selected, pressing the the right-most button will delete the last word.
  - Delete the entire program by holding down the "A" button and pressing the right-most button.
  - Paste the last deleted word by pressing the center-right (or 3rd) button under the display (labeled "SELECT")
  - Clear the data stack by pressing the center-left (or 2nd) button under the display (labeled with a speaker)
2) If one holds down A and presses down arrow, the bottom 5 lines display the various defined 
  words (including user-defined words, if any have been defined).  The middle remainder displays the
  program.  
  - One enters a word in the program stack by selecting one using the arrow keys, then pressing B.
  - To get help on a built-in word, select the word and press the left-most button under the display.
3) If one holds down A and presses the left arrow, one may choose up to 6 characters, selecting each character using the arrow keys, then pressing B.  The bottom right character which is a left arrow may be chosen to delete the previous entered character.  Hold down A, then press B to enter the assembled string into the program stack.  There is no difference (internally) between entering a word this way or choosing one from the list of words, above.
  * Note: you should be able to move from any letter to any other letter with a maximum of 4 arrow key pushes.  Try it.
4) If one holds down A and presses the right arrow, one may enter an integer or floating point number.
  - Choose the digit which you would like to increment or decrement by choosing the place with the left and right arrows.
  - Increment or decrement that place by pressing the up or down arrow.  
  - Move the cursor to the right of the ones digit in order to specify a floating point number.
  - Press B to enter the number into the program stack.
  - Hold down A, then press B to clear the entry and reset the type to integer.


[*Word Definitions*] https://github.com/JaySpencerAnderson/GOFORTH/blob/master/Dictionary.md

[*Example Programs*] https://github.com/JaySpencerAnderson/GOFORTH/blob/master/ExamplePrograms.md

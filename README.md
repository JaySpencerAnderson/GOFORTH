# GOFORTH
FORTH interpreter/IDE for the ODROID GO
An ODROID GO is a handheld game computer, similar to a Game Boy Color.
GOFORTH is a Forth interpreter which can be programmed on the device.
The screen of an ODROID GO is 320x240 pixels, allowing 26 (readable)
characters per line, with 15 lines (12x16 pixels per character, 10x14 font, really a 5x7 font doubled).
The top line (or more-expanded as necessary) of the screen displays the stack.
The bottom 5 lines of the screen are used for input (one selects characters or Words using the
arrows and buttons), in 3 sections
- The left section is for input of alpha and special characters.  The lower right character is to delete the last character from the input buffer.
- The middle section allows one to choose a defined word.  If the Forth script defines a new word, it will show up here as soon as the script is executed.
- In the upper right hand corner is a facility for entering integers.
The remainder of the screen displays the program.

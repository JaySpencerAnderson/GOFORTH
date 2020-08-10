Release 0.0.1
Added Features
- 0.0.1.1 - Performance - For hash table overflow lookup, have program use binary search for lookup. 
  - IMPLEMENTED - BENCHW went from 215517 to 240384 for user-defined words per second.
- 0.0.1.2 - Performance - Use binary lookup for user defined words.  Merge this with hash table overflow. 
- 0.0.1.7 - Functionality - Implement **, <<, >>, <=, >=, <>, remove WAIT 

Addressed Issues
- 0.0.0.1 - Deleting words in INPUTSTATECLEAR that aren't last do not clear screen (properly).
  - FIXED  Before erasing the remaining lines, I now erase to the end of the preceding line.
- 0.0.0.2 - Creating a word with "<" or ">" in it makes the program word menu lock up, unresponsive to arrow keys.  Other menus still work.  Removing word definitions and triggering programScan fixes it to some extent but may leave some data corruption.  Suspect that the place of "<" and ">" in Base40 character set (last) may have something to do with it.  
  - FIXED: Menu only dimensioned to allow 64 entries, now bounds checking.  Changed to 256 entries, added bounds checking, but no error message.
- 0.0.0.3 - The last column was not getting erased.
  - FIXED - Changed "<" to "<=" in one if statement.
- 0.0.0.4 - Various problems displaying menu, particularly with the end of the list.  User defined words at the end of the sort sequence (like >B) do not show up in the menu of words, at least the last one doesn't.  It still is defined so invoking that word still works.  There may be a related problem where a user defined word is still partially defined after the definition is erased.  Did notice that when >A was defined, it and the preceding 9 words (about 1 line) were not displayed.  
  - FIXED: Mistakenly made code that cleared to the end of the line (desired) get combined with putting then next word on the next line (not desired). 
- 0.0.0.10 - Nesting if statements doesn't work.
  - FIXED.
- 0.0.0.6 - After cycle power, then moving down in the menu, it sometimes (flaky) changes the bottom line before it needs to scroll, then changes back.  Maybe only if moving down faster.
  - No longer observed.
- 0.0.0.7 - When using arrow keys in INPUTSTATEMENU, sometimes highlighted word doesn't change but really it does because pressing another arrow key will act like the first arrow key push had been successful.  
  - Perhaps due to earlier corruption.  No longer observed.

Known Errors
- 0.0.0.5 - PREAD does not work just after downloading GO Forth to the Odroid GO.  One has to first cycle power.
- 0.0.0.8 - PREAD seems like it sometimes doesn't read the entire file.
- 0.0.0.9 - Did a PLIST, cleared it, then a QUOTE BENCHW PREAD and part of the result was the results of the PLIST.
- 0.0.0.11 - Sometimes (with program over one page) the word menu blinks part of the top menu line when
pressing arrow keys.
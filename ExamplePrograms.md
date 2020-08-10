Example Programs written in GOFORTH

*Note: ";" at the end of word definition and ":" following it are two individual words.  

1) Pascal's Triangle

    CLS 10 1 DO I I 0 DO DUP I OVER OVER 3 * OVER 9 SWAP - + CUP 1 + P . LOOP DROP LOOP WAIT : P OVER 1 = IF 1 = IF DROP 1 ELSE DROP 0 THEN ELSE SWAP 1 - SWAP OVER OVER 1 - P ROT ROT P + THEN ; : CLS 0 0 320 240 0 RCTNGL ; : WAIT BEGIN 100 DELAY BUTTON UNTIL ; QUOTE P PSAVE 

2) Word Benchmark - shows how many words per second for built-in and mixed built-in and user words.

    0 SLOWER CLS TIME NATIVE TIME TCALC SHOW TIME USER TIME TCALC SHOW WT CLS : NATIVE 1 1 CUP 0 10000 0 DO I + LOOP DROP ; : USER 2 1 CUP 0 10000 0 DO I PLUS LOOP DROP ; : TCALC SWAP - 10000 5 * 1000 * SWAP / ; : PLUS + ; : SHOW . QUOTE :WPS EMIT ; : WT BEGIN 201 DELAY BUTTON UNTIL ; : CLS 0 0 320 240 0 RCTNGL ; QUOTE BENCHW PSAVE 

3) Short program to demonstrate NOTE word which implements Karplus Strong sound synthesis.

    16 1 DO I BEEP LOOP : BEEP 20 * 20 SWAP NOTE ; QUOTE BEEP PSAVE 

4) Short program which sends a square wave to the speaker via DAC word.

    0 SLOWER INIT 1000 0 DO 2 0 DO I GET DAC 200 UDELAY LOOP LOOP : INIT 0 0 PUT 100 1 PUT ; QUOTE TONE PSAVE 

5) Exercise graphics commands

    INIT 6 1 DO I 50 * 10 PUT 12 1 DO 10 GET 120 13 I - 2 * I 6 MOD GET CIRCLE LOOP LOOP BEGIN 100 DELAY BUTTON *A = UNTIL SQR BEGIN 120 DELAY BUTTON *B = UNTIL : SQR 6 0 DO I 10 * I 10 * 320 I 20 * - 240 I 20 * - *BLUE I 4 * - RCTNGL LOOP ; : CLS 0 0 320 240 0 RCTNGL ; : INIT CLS *RED 0 PUT *RED *BLUE + 1 PUT *BLUE 2 PUT *BLUE *GREEN + 3 PUT *GREEN 4 PUT *GREEN *RED + 5 PUT ; QUOTE GTEST PSAVE 

6) Factorial

    3 FAC : FAC DUP 1 > IF DUP 1 - FAC * THEN ; QUOTE FACT PWRITE 
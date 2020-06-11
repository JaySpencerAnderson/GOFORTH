#include <odroid_go.h>
// Version 1:
// 32 bit wide Data stack, 32 bit wide program array, 32 bit accumulator
// Functionality, step 1 - Be able to program a recursive factorial function
// - Code function
//   x alpha input
//   - auto alpha after ":"
//   - entry in midst of program
//   - automatic ":" <> ";"
//   - find function
//   - enable menu of functions
//   - returnStack
// - If function
// 
// Next:
// done: Pass filename to read/write file
// 4/22/2020 done: Break (B & A), PLIST (directory command)
//   Break button to interrupt programExecute
// 4/23/2020 done: Commented Serial.print inside programExecute, about 60x faster (!)
//                 Added TIME word that inserts millisecond time into stack
//                 Added error check for CMDPUSH following QUOTE
//                 Sped up things by getting rid of debounce on input.
//                 Now writing screen only when key is depressed, not when released
//                 Wrote STACK method which treats the stack as an array.  Probably will
//                  change.  I'm trying to write a SIEVE program using this.
//  Decided not to: Implement autorepeat.  That part works but then I'd have to add
//   redisplaying the screen with the new field highlighted.
// 5/2/2020 done: Bottom area is used by 0 to 1 input methods, not 3.  Need to iron out wrinkles.
// FIXED: INPUTSTATEMENU still doesn't erase entire area if program stack would extend beneath it.
// -  programStack should auto scroll so selected word (or last word) is visible.
// -  dataStack should have a limited size - 2 rows
// -  Don't have a plan for variables to be visible (debugging).  Maybe swap with dataStack.
// -  Don't have much of a plan for output screen/graphics
// FIXED: rewriting screen is flashing because I always erase and rewrite screen.  Would be nice if
//    things only get rewritten (or at least erased) if necessary.  Now not flashing.
// 5/3/2020
// FIXED: Try PLIST (execute).  Delete (cut) PLIST.  Not erased but rewritten.
// 5/7/2020
//   Up down arrows in CLEAR?
//   Additional commands: INVERT (not) VARIABLE ! (store in variable) @ (fetch from variable)
//                        +! CONSTANT ALLOT
//   VARIABLES/ARRAYS     VARIABLE ! @ CONSTANT +! ALLOT ?
//   I/O                  . EMIT CR ? KEY
//   ! @ not in base 40.  Use something like -> <- NOTE: Using GET (index - value), PUT (index, value - )
//   VARIABLE, CONSTANT are longer than 6 chars.  Use Javascript VAR and CONST
// 5/24/2020 - DONE: Faster looping, hash lookup
// 5/25/2020 - TODO:
//  DONE - BEGIN, UNTIL/AGAIN - use loopStack
//  DONE - Have show get words from CmdLookup
//  DONE - Fix bug(s) with delete
//    FIXED - DELETING first word, last one doesn't disappear.
//  - Create ERROR variable and exit programExecute when there is an error (stack overflow, underflow)
//  - HASH (4 bit) lookup for user defined words
//  DONE - Make A+START clear program.
//   BUG - The above doesn't clear the screen properly but the next keystroke usually does.
// 5/30/2020 !!!!
// - Implemented DAC. Karplus Strong Demo implemented!  I didn't think it was fast enough!
// - Demo initializes 15 words of array with 1,0,0,1,1,1,0,0,0,0,1,1,1,1,1
//   using 15 byte initializes works.  Use 21- it's distorted.
//   So it is limited but does produce result. 
//   Hard-coded square or triangle wave actually can hit a high pitch.
//   So a little more optimization would be useful.
// 5/31/2020
// - Much faster after disabling GO.Update in programExecute.  Just can't break out now.
//   Also seems to cause a problem with reading programs.
//   Perhaps make this configurable (FASTER, SLOWER)
// Future
//  - Implement scripted Karplus Strong, to 10 khz?
//    - requires UDELAY - delay to next multiple of usec (50 in this case)
//  - Implement scripted analog computer
//    - requires graphics, text output, input
//  - Show errors somehow
//  - F1 HELP
//  - Get PSAVE, PREAD reliable (FACT changes not getting saved)
//  - Decide on standard or C like operators (XOR), at least consistent
//
// Scrollable data, program, variable area
//
// Optimization ideas
//  - Get rid of GO.Update inside programExecute loop.  May prevent breaking out.
// Bugs:
// - Seems like commenting out GO.Update, now causes problems with PREAD where I get the old one
//   until I cycle power.
//
uint16_t tk, pk, slower;
uint32_t globalerror;
char *globalmessage;

#define TRUE -1
#define FALSE 0
#define DEBUG 0

#define ERRORNONE 0
#define ERRORUNDERFLOW 1

#define SELECTEDCOLOR 0x07FE
#define NORMALCOLOR WHITE
#define NORMALBGCOLOR BLACK
#define OTHERCOLOR YELLOW

#define KPMENU 0x0001
#define KPVOLUME 0x0002
#define KPSELECT 0x0004
#define KPSTART 0x0008
#define KPA 0x0010
#define KPB 0x0020
#define KPUP 0x0100
#define KPDOWN 0x0200
#define KPLEFT 0x0400
#define KPRIGHT 0x0800
#define KPALPHA KPSTART

#define CMDPUSH -1
#define NOOP 0x00

#define INPUTSTATEALPHANUMERIC 0
#define INPUTSTATENUMERIC 1
#define INPUTSTATECLEAR 2
#define INPUTSTATEMENU 3

int InputState=INPUTSTATEMENU,PreviousInputState=INPUTSTATECLEAR;
int BreakOut;

typedef struct {
  uint32_t word,index;
} DefinedWord;
int32_t definedWordIndex=0;
DefinedWord definedWord[128];

#define BASE 40
char Base40[]=" ABCDEFGHIJKLMNOPQRSTUVWXYZ:;+-*/&|^.<=>\e";
char Base10[]="0123456789-";

#define ROWS 15
#define COLUMNS 26
#define ANCOLS 8
#define ANROWS 5
#define BASE40ROW 10
#define BASE40COL 8

#define STACKSIZE 1024
typedef int32_t GFI;
typedef uint32_t GFU;
typedef union {GFU u; GFI i;} Word;
typedef struct {Word type, value;} TypeWord;
typedef struct {
  int16_t row,col;
} RowCol;
typedef struct {
  void (*fptr)(void);
  GFU original;
} HashEntry;
typedef struct {
  GFU cmd;
  char *msg;
} HelpEntry;

// Convert from a base 40 encoding back to ascii
void fromBase40(char *buffer,uint32_t b40){
  buffer[0]=(char)0;
  while(b40){
    // Shift characters right
    for(int i=7;i>0;i--){
      buffer[i]=buffer[i-1];
    }
    buffer[0]=Base40[b40%BASE];
    b40/=BASE;
  }
}

class HashLookup {
/* 256 entries for hash lookup, 44 for overflow */
  HashEntry table[300];
  HelpEntry helpLookup[64];

  public:
    void init(){
      for(int i=0;i<300;i++){
        table[i].fptr=NULL;
        table[i].original=0;
      }
      for(int i=0;i<64;i++){
        helpLookup[i].cmd=0;
        helpLookup[i].msg=NULL;
      }
    }
    uint32_t computeHash(uint32_t x){
      uint32_t y=x^(x>>8);
      return 0xFF&(y^(y>>16));
    }
    char *getHelpMessage(GFU fname){
      Serial.print("search for message for ");
      char xyz[13];
      fromBase40(xyz,fname);
      Serial.println(xyz);
      for(int i=0;i<64;i++){
        if(fname == helpLookup[i].cmd){
          Serial.print("Found it:");
          Serial.println(helpLookup[i].msg);
          return helpLookup[i].msg;
        }
      }
      Serial.println(":-(");
      return NULL;
    }
    void addEntry(void (*fptr)(), GFU fname, char *msg){
      char xyz[13];
      fromBase40(xyz,fname);

      Serial.print("Adding ");
      Serial.println(xyz);
      for(int i=0;i<64;i++){
        if(helpLookup[i].cmd == fname || helpLookup[i].cmd == 0){
          helpLookup[i].cmd=fname;
          helpLookup[i].msg=msg;
          Serial.print("Added ");
          Serial.print(helpLookup[i].msg);
          Serial.print(" @ ");
          Serial.println(i);
          break;
        }
      }
      int i=(int)computeHash(fname);
      if(table[i].original == 0){
        table[i].original=fname;
        table[i].fptr=fptr;
      }
      else {
        for(int i=256;i<300;i++){
          if(table[i].original == 0){
            table[i].original=fname;
            table[i].fptr=fptr;
            return;
          }
        }
      }
    }
    GFU getNextEntry(GFU e){
      if(e == 0){
        for(int i=0;i<300;i++){
          if(table[i].original != 0){
            return table[i].original;
          }
        }
      }
      else {
        for(int i=0;i<300;i++){
          if(table[i].original == e){
            for(int j=i+1;j<300;j++){
              if(table[j].original != 0){
                return table[j].original;
              }
            }
            return 0;
          }
        }        
      }
      return 0;
    }
    int callEntry(GFU fname){
#if DEBUG
      char xyz[13];
      fromBase40(xyz,fname);
      Serial.print("Seeking ");
      Serial.println(fname);
#endif
      GFU i=computeHash(fname);
#if DEBUG
      Serial.print("Hash is ");
      Serial.println(i);
      Serial.print("Found ");
      fromBase40(xyz,table[i].original);
      Serial.println(xyz);
#endif
      if(table[i].original == fname){
        (*table[i].fptr)();
        return TRUE;
      }
      else {
        for(i=256;i<300;i++){
          if(table[i].original == fname){
            (*table[i].fptr)();
            return TRUE;
          }
        }
      }
      return FALSE;
    }
} CmdLookup;

TypeWord Base40Buffer;

// The next 9 functions are used just with files
// Figure out which base 40 encoding this character is.
int nBase40(char c){
  int i=0;
  while(Base40[i] != c){
    i++;
    if(i == BASE){
      return 0;
    }
  }
  return i;
}
int isBlank(char s){
  return (s == ' ' || s == '\r' || s == '\n');
}
int isBase40(char *s){
  // Can this string be transformed into Base40 in its entierty?
  int i=0;
  while(s[i]){
    // Loop until you find something that isnt Base40
    if(nBase40(s[i]) == 0){
      // Is this the first character?  If so, false.
      if(i == 0){
        return false;
      }
      // Terminated by a blank?  If so, true
      else if(s[i] == ' '){
        return true;
      }
      // Otherwise false because it isnt in the Base40 array.
      else {
        return false;
      }
    }
    i++;
  }
  if(i == 0){
    return false;
  }
  return true;
}
// Convert a string toBase40 encoding
uint32_t toBase40(char *txt){
  uint32_t sum=0;
  int i=0;
  while(txt[i] != 0){
    int val=nBase40(txt[i]);
    if(val == 0){
      return sum;
    }
    sum=(sum*BASE)+val;
    i++;
    if(i == 6){
      return sum;
    }
  }
  return sum;
}
int lengthInt(int32_t i){
  int len=1;
  if(i < 0){
    len++;
    i*=-1;
  }
  while(i >= 10){
    len++;
    i/=10;
  }
  return len;
}
int lengthBase40(uint32_t i){
  int len=0;
  while(i > 0){
    len++;
    i/=40;
  }
  return len;
}
#if 1
int nBase10(char c){
  int i=0;
  while(Base10[i] != c){
    i++;
    if(i == strlen(Base10)){
      return -1;
    }
  }
  return i;
}
int isBase10(char *txt){
  int sign=1,next=0;
  int i=nBase10(txt[next]);
  if(i == -1){
    return false;
  }
  if(i == 10){
    sign=-1;
    next++;
  }
  while(txt[next] != 0 && txt[next] != ' ' && txt[next] != '\n' && txt[next] != '\r' && txt[next] != '\t'){
    i=nBase10(txt[next++]);
    if(i == -1 || i > 9){
      return false;
    }
  }
  return true;
}
// Convert from a base 10 number back to ascii
void fromBase10(char *buffer,int32_t value){
  int next=0;
  if(value == 0){
    buffer[0]=Base10[0];
    buffer[1]=0;
    return;
  }
  else if(value < 0){
    buffer[next++]=Base10[10];
    buffer[next]=0;
    value=-value;
  }
  else {
    buffer[next]=(char)0;
  }
  while(value){
    // Shift characters right
    for(int i=11;i>0;i--){
      buffer[i]=buffer[i-1];
    }
    buffer[0]=Base10[value%10];
    value/=10;
  }
}
int32_t toBase10(char *txt){
  int next=0;
  int32_t sign=1,sum=0;
  int i=nBase10(txt[next]);
  if(i == -1){
    return 0;
  }
  if(i == 10){
    sign=-1;
    next++;
  }
  while(txt[next] != 0 && txt[next] != ' ' && txt[next] != '\n' && txt[next] != '\r' && txt[next] != '\t'){
    i=nBase10(txt[next++]);
    if(i == -1 || i > 9){
      return 0;
    }
    sum=(10*sum)+(int32_t)i;
  }
  return sum*sign;
}
#endif
void eraseLine(int line){
  GO.lcd.fillRect(0,line*16,320,16,0x0000);
}
void eraseSpace(int line,int start, int width){
  GO.lcd.fillRect(start*12,line*16,width*12,16,0x0000);
  GO.update();
}

class Heap {
#define HEAPLIMITB 0x10000
#define HEAPLIMITW 0x4000
  uint32_t n;
  union {
    uint8_t b[HEAPLIMITB];
    Word w[HEAPLIMITW];
  } u;
public:
  void init(){
    for(uint32_t i=0;i<HEAPLIMITW;i++){
      u.w[i].u=0;
    }
    n=0;
  }
  void put8(uint32_t i, uint8_t b){
    u.b[i]=b;
  }
  void put32(uint32_t i, Word w){
    u.w[i].u=w.u;
  }
  uint8_t get8(uint32_t i){
    return u.b[i];
  }
  Word get32(uint32_t i){
    return u.w[i];
  }
} heap;
class NameStack {
#define NameStackWIDTH COLUMNS
  Word Menu[64];
  int nMenu,nMenuSelected,nMenuOffset;
  private:
    void makeGapAt(int n){
      for(int j=nMenu;j>n;j--){
        Menu[j]=Menu[j-1];
      }
      nMenu++;
    }
  public:
    NameStack(){
      nMenuSelected=nMenu=nMenuOffset=0;
    }
    uint32_t hash6bit(uint32_t x){
      uint32_t y=x^(x>>6);
      return 0x3F&(y^(y>>12)^(y>>24));
    }
    uint32_t hash7bit(uint32_t x){
      uint32_t y=x^(x>>7);
      return 0x7F&(y^(y>>14)^(y>>28));
    }
    uint32_t hash(uint32_t x){
      uint32_t y=x^(x>>8);
      return 0xFF&(y^(y>>16));
    }
    void next(){
      nMenuSelected=(nMenuSelected+1)%nMenu;
    }
    void nextLine(){
      if(nMenuSelected >= nMenu){
        nMenuSelected=0;
        return;
      }
      int size=lengthBase40(Menu[nMenuSelected].u);
      for(int i=nMenuSelected+1;i<nMenu;i++){
        size+=lengthBase40(Menu[i].u)+1;
        if(size > NameStackWIDTH){
          nMenuSelected=i;
          return;
        }
      }
      nMenuSelected=0;
    }
    void previous(){
      nMenuSelected=(nMenuSelected-1+nMenu)%nMenu;
    }
    void previousLine(){
      int size=lengthBase40(Menu[nMenuSelected].u);
      for(int i=nMenuSelected-1;i>=0;i--){
        size+=lengthBase40(Menu[i].u)+1;
        if(size > NameStackWIDTH){
          nMenuSelected=i;
          return;
        }
        if(i == 0){
          nMenuSelected=0;
          return;
        }
      }
      nMenuSelected=nMenu-1;
    }
    Word getSelected(){
      return Menu[nMenuSelected];
    }
    void push(uint32_t value){
      char bfr[16];
      fromBase40(bfr,value);
      if(nMenu == 0){
        Menu[nMenu++].u=value;
      }
      else if(value < Menu[nMenu-1].u){
        Menu[nMenu++].u=value;
      }
      else if(value > Menu[0].u){
        makeGapAt(0);
        Menu[0].u=value;
      }
      else {
        for(int i=0;i<nMenu-1;i++){
          if(value < Menu[i].u && value > Menu[i+1].u){
            makeGapAt(i+1);
            Menu[i+1].u=value;
            return;
          }
        }
      }
    }
    void show(int InputState){
      int16_t color;
      if(InputState == INPUTSTATEMENU){
        color=NORMALCOLOR;
      }
      else {
        return;
      }
      nMenu=0;
      GFU cmd=0;
      while((cmd=CmdLookup.getNextEntry(cmd)) != 0){
        // Only show valid Base40 strings (not CMDPUSH)
        if(cmd < 4096000000){
          push(cmd);
        }
      }
      for(int j=0;j<definedWordIndex;j++){
        push(definedWord[j].word);
      }
      nMenuSelected%=nMenu;
      int notShown=1;
      while(notShown){
        RowCol next={ROWS-5-nMenuOffset,0};
        for(int j=0;j<nMenu;j++){
          int size=lengthBase40(Menu[j].u);
          if(next.col+size > COLUMNS){
            next.row++;
            next.col=0;
          }
          if(next.row >= ROWS-5 && next.row < ROWS){
            if(nMenuSelected == j){
              GO.lcd.setTextColor(NORMALBGCOLOR,color);
              notShown=0;
            }
            else {
              GO.lcd.setTextColor(color,NORMALBGCOLOR);
            }
            GO.lcd.setCharCursor(next.col, next.row);
            char bfr[7];
            fromBase40(bfr,Menu[j].u);
            GO.lcd.printf("%s",bfr);
          }
          else {
            if(nMenuSelected == j){
              if(next.row >= ROWS){
                nMenuOffset+=next.row-ROWS+1;
              }
              else if(next.row < ROWS-5){
                nMenuOffset--;
              }
              for(int i=ROWS-5;i<ROWS;i++){
                eraseSpace(i,0,COLUMNS);
              }
              break;
            }            
          }
          next.col+=size+1;
        }
      }
      return;
    }
} menuStack;
class DataStack {
    int nws;
    Word value[STACKSIZE];
    int lenInt(int32_t i){
      int len=1;
      if(i < 0){
        len++;
        i*=-1;
      }
      while(i >= 10){
        len++;
        i/=10;
      }
      return len;
    }
  public:
    DataStack(){
      clear();
    }
    void clear(){
      nws=0;
      for(int i=0;i<STACKSIZE;i++){
        value[i].u=NOOP;
      }
    }
    int size(){return nws;};
    void push(Word w){
      if(nws+1 < STACKSIZE){
        value[nws++]=w;
      }
      else {
        globalmessage="Stack Overflow";
        globalerror=1;
      }
    };
    Word pop(){
      if(nws > 0){
        return value[--nws];
      }
      globalmessage="Stack Underflow";
      globalerror=1;
      Word x;
      x.i=0;
      return x;
    }
    Word copy(int n){return value[n];}
    Word peek(){return value[nws-1];};
    Word cut(int n){
      Word r=value[n];
      for(int i=n+1;i<nws;i++){
        value[i-1]=value[i];
      }
      nws--;
      return r;
    }
    void paste(Word w, int n){
      if(n < nws){
        for(int i=nws-1;i>=n;i--){
          value[i+1]=value[i];
        }
      }
      value[n]=w;
      nws++;
    }
    void erase(int line){
      GO.lcd.fillRect(0,line*16,320,16,0x0000);
    }
    void dump(){
      Serial.println("dataStack.dump");
      for(int i=0;i<nws;i++){
        if(value[i].i == CMDPUSH){
          Serial.print("push ");
        }
        else {
          Serial.print(value[i].u);
          Serial.println();
        }
      }
    }
    void write(File f){
      for(int i=0;i<nws;i++){
        f.write((unsigned char *)&value[i].u,4);
      }
    }
// Display data stack starting on the given line.
    int show(int line){
      int column=0,size,localCount=0;
      uint16_t color=NORMALCOLOR;

      GO.lcd.setTextColor(color,NORMALBGCOLOR);
      erase(line);
      for(int i=0;i<nws;i++){
        size=lenInt(value[i].i);
        if(column + size > COLUMNS){
          column=0;
          line++;
          erase(line);
        }
        GO.lcd.setCharCursor(column, line);
        column += size+1;
        GO.lcd.printf("%d",value[i].i);
      }
      return line;
    }
} dataStack, returnStack, loopStack;

class TypeWordStack {
  DataStack ds;
  int selected, ntw, lastLine, nOffset;
  private:
    int twfind(int nw){
      if(nw == 0){
        return 0;
      }
      else if(nw < 0){
        nw=ntw;
      }

      TypeWord tw;
      int i=0,w=0;

      while(i < ds.size()){
        /* Get a TypeWord and keep count */
        tw.type=ds.copy(i++);
        if(tw.type.i == CMDPUSH){
          tw.value=ds.copy(i++);
        }
        w++;
        if(nw == w){
          return i;
        }
      }
      return 0;
    }
  public:
    TypeWordStack(){
      DataStack ds;
#if 0
      selected=-1;
      lastLine=ntw=nOffset=0;
#else
      init();
#endif
    }
    void init(){
      selected=-1;
      lastLine=ntw=nOffset=0;      
    }
    void dump(){
      Serial.println("programStack.dump");
      ds.dump();
    }
    void paste(TypeWord tw){
      if(tw.type.i != NOOP){
        int n=twfind(selected);
        ds.paste(tw.type,n);
        if(tw.type.i == CMDPUSH){
          ds.paste(tw.value,n+1);
        }
        ntw++;
        if(getSelected() > -1){
          selectNext();
        }
      }
    }
    TypeWord cut(){
      TypeWord tw;
      int n;
      if(ntw > 0){
        if(selected < 0){
          n=twfind(ntw-1);
        }
        else {
          n=twfind(selected);
        }
        tw.type = ds.cut(n);
        if(tw.type.i == CMDPUSH){
          tw.value = ds.cut(n);
        }
        ntw--;
        // I think this fixes the problem
        if(selected == ntw){
          selected--;
        }
      }
      else {
        tw.type.i=NOOP;
        tw.value.i=0;
        globalmessage="Stack Underflow";
        globalerror=ERRORUNDERFLOW;
      }
      return tw;
    }
    TypeWord get(int n){
      TypeWord tw;
      int nt=twfind(n);
      tw.type = ds.copy(nt);
      if(tw.type.i == CMDPUSH){
        tw.value = ds.copy(nt+1);
      }
      else {
        tw.value.u=0;
      }
      return tw;
    }
    TypeWord copy(){
      TypeWord tw;
      int n=twfind(selected);
      tw.type = ds.copy(n);
      if(tw.type.i == CMDPUSH){
        tw.value = ds.copy(n+1);
      }
      return tw;
    }
    Word wordCopy(int i){
      return ds.copy(i);
    }
    int wordSize(){
      return ds.size();
    }
    void push(TypeWord tw){
      ds.push(tw.type);
      if(tw.type.i == CMDPUSH){
        ds.push(tw.value);
      }
      ntw++;
    }
    TypeWord pop(){
      TypeWord tw;
      int n=twfind(--ntw);
      tw.type = ds.copy(n);
      if(tw.type.i == CMDPUSH){
        tw.value = ds.copy(n+1);
        ds.pop();
      }
      ds.pop();
      return tw;
    }
    void erase(int line){
      GO.lcd.fillRect(0,line*16,320,16,0x0000);
    }
    int getNtw(){
      return ntw;
    }
    void unselect(){
      selected=-1;
    }
    int getSelected(){
      return selected;
    }
    int getSelectedIndex(){
      if(selected == -1){
        return -1;
      }
      else if(selected == 0){
        return 0;
      }
      int inx=0;
      Word w;
      for(int s=0;s<selected;s++){
        w=ds.copy(inx);
        if(w.i == CMDPUSH){
          inx++;
        }
        inx++;
      }
      return inx;
    }
    void setSelected(int s){
      if(s >= 0 && s < ntw){
        selected=s;
      }
      else {
        unselect();
      }
    }
    void selectFirst(){
      setSelected(0);
    }
    void selectLast(){
      setSelected(ntw-1);
    }
    void selectNext(){
      int nxt=getSelected()+1;
      if(nxt >= ntw){
        nxt=-1;
      }
      setSelected(nxt);
    }
    void selectPrevious(){
      int nxt=getSelected();
      if(nxt < 0){
        nxt=ntw-1;
      }
      else {
        nxt--;
      }
      setSelected(nxt);
    }
    int lenInt(int32_t i){
      int len=1;
      if(i < 0){
        len++;
        i*=-1;
      }
      while(i >= 10){
        len++;
        i/=10;
      }
      return len;
    }
    int lenBase40(uint32_t i){
      int len=0;
      while(i > 0){
        len++;
        i/=40;
      }
      return len;
    }
    void ListFiles(){
      char ascii[16];
      TypeWord tw;
      GO.update();
      if(SD.begin(22)){
        uint8_t cardType = SD.cardType();
        if (cardType == CARD_NONE) {
          return;
        }
        File listFile=SD.open("/", FILE_READ);
        while(true){
          File listing = listFile.openNextFile();
          if(!listing){
            return;
          }
          strncpy(ascii, listing.name(), 16);
          ascii[7]=0;
          for(int i=1;i<7;i++){
            if(ascii[i] == '.'){
              ascii[i]=0;
            }
          }
          if(isBase40(&ascii[1])){
            tw.type.u=toBase40(&ascii[1]);
            push(tw);
          }
          listing.close();
        }
      }
    }
    void ReadFile(Word a){
      char ascii[16],chr;
      GO.update();
      delay(1000);
      Serial.println("ReadFile");
      if(SD.begin(22)){
        Serial.println("SD.begin");
        uint8_t cardType = SD.cardType();
        if (cardType == CARD_NONE) {
          return;
        }
      
        Serial.println("Begin Read");
        ascii[0]='/';
        fromBase40(&ascii[1], a.u);
        strcat(ascii,".4TH");
        File readFile=SD.open(ascii, FILE_READ);
        int i=0,state=0;
        ascii[i]=0;
        TypeWord tw;
        while(readFile.available()){
          chr=(char)readFile.read();
          switch(state){
            /* Not in anything yet */
            case 0:
            if(!isBlank(chr)){
              state=1;
              i=0;
              ascii[i++]=chr;
            }
            break;
            case 1:
            if(isBlank(chr)){
              state=0;
              ascii[i]=0;
              i=0;
              if(isBase40(ascii)){
                tw.type.u=toBase40(ascii);
                push(tw);                
              }
              else if(isBase10(ascii)){
                tw.type.i=CMDPUSH;
                tw.value.i=toBase10(ascii);
                push(tw);                                
              }
            }
            else {
              ascii[i++]=chr;
            }
            break;
          }
        }
        if(state == 1){
          ascii[i]=0;
          if(isBase40(ascii)){
            tw.type.u=toBase40(ascii);
            push(tw);                
          }
          else if(isBase10(ascii)){
            tw.type.i=CMDPUSH;
            tw.value.i=toBase10(ascii);
            push(tw);                                
          }
        }
        readFile.close();
        Serial.println("Closed ");
      }
    }
    void WriteFile(Word a){
      char ascii[16];
      GO.update();
      delay(1000);
      Serial.println("WriteFile");
      if(SD.begin(22)){
        Serial.println("SD.begin");
        uint8_t cardType = SD.cardType();
        if (cardType == CARD_NONE) {
          Serial.println("No SD card attached");
          return;
        }
      
        Serial.println("Begin Write");
        ascii[0]='/';
        fromBase40(&ascii[1], a.u);
        strcat(ascii,".4TH");
        File writeFile=SD.open(ascii, FILE_WRITE);
        // Loop through programStack.ds (0 to nsw)
        for(int k=0;k<ntw;k++){
          TypeWord tw=get(k);
          // This gets an error
          if(tw.type.i == CMDPUSH){
            sprintf(ascii,"%d ",tw.value.i);
            Serial.println(ascii);
            for(int i=0;i<strlen(ascii);i++){
              writeFile.write((unsigned char)ascii[i]);
            }
          }
          else {
            fromBase40(ascii,tw.type.u);
            Serial.println(ascii);
            for(int i=0;i<strlen(ascii);i++){
              writeFile.write((unsigned char)ascii[i]);
            }
            writeFile.write((unsigned char)' ');
          }
        }
        writeFile.close();
        Serial.println("Closed ");
      }
    }
// Display a stack starting on the given line.
// Emphasize the stack if non-zero
    void show(int line, int is, int limitLine){
    int row,column=0,size,notShown=1,above=0;
    char ascii[16];
    uint16_t color=(is == INPUTSTATECLEAR)?NORMALCOLOR:OTHERCOLOR;

    if(ntw == 0){
      eraseSpace(line,0,COLUMNS);
    }
    while(ntw > 0 && notShown){
      row=line-nOffset;
      GO.lcd.setTextColor(color,NORMALBGCOLOR);
      for(int i=0;i<ntw;i++){
        TypeWord tw=get(i);
        if(tw.type.i == CMDPUSH){
          size=lenInt(tw.value.i);
        }
        else {
          size=lenBase40(tw.type.u);
        }
        if(column + size > COLUMNS){
          Serial.println("Should not happen");
          // Erase to the end of this line
          if(row >= line){
            eraseSpace(row,column,COLUMNS-column);
          }
          column=0;
          row++;
          if(row >= limitLine){
            break;
          }
        }

        if(row >= line && row < limitLine){
          // Erase the space after this next word
          eraseSpace(row,column+size,1);
          if(i == selected){
            GO.lcd.setTextColor(NORMALBGCOLOR,color);
          }
          else {
            GO.lcd.setTextColor(color,NORMALBGCOLOR);
          }
          if(i == selected){
            notShown=0;
          }
          else if(selected == -1 && i == ntw-1){
            notShown=0;
          }
          // Now write the word
          GO.lcd.setCharCursor(column, row);
          if(tw.type.i == CMDPUSH){
            GO.lcd.printf("%d",tw.value.i);
          }
          else {
            fromBase40(ascii,tw.type.u);
            GO.lcd.printf("%s",ascii);
          }
        }
        else {
          if(i == selected){
            notShown=1;
            above=(row<line)?1:0;
          }
        }
        // Add one for space delimiter
        column+=size+1;
      }
      eraseSpace(row,column,COLUMNS-column);
      if(row+1 < limitLine){
        eraseLine(row+1);
      }
      if(notShown){
        nOffset=(above)?0:(nOffset+1);
      }
    }
    GO.lcd.setTextColor(color,NORMALBGCOLOR);
  }
} programStack, cutStack;

int32_t accumulator,increment;
int32_t cmdIndex=1;


RowCol an={0,0},anDelete={ANROWS-1,ANCOLS-1};

/* Karplus strong data */
#define NSAMPLE 512

uint8_t voice[NSAMPLE];
uint32_t wavelength=18;


void kpInit(){
  uint32_t sum=0,j=0;
  for(uint32_t i=0;i<NSAMPLE;i++){
#if 1
    voice[i]=random(63);
#else
    if(j & 0x01){
      voice[i]=63;
    }
    else {
      voice[i]=0;
    }
    if(i > sum){
      sum+=j;
      j++;
    }
#endif
  }
}
void kpNote(uint32_t ln){
  wavelength=ln;
}
void kpPlay(uint32_t ln){
  if(wavelength > 0){
    kpInit();
    for(uint32_t i=0;i<ln*1024;i++){
      uint32_t j=i%wavelength;
      uint32_t k=i%NSAMPLE;
      dacWrite(SPEAKER_PIN, (uint8_t)voice[j]);
      delayMicroseconds(32);
      voice[k]=(voice[k]+voice[(k+1)%NSAMPLE])>>1;
    }
    dacWrite(SPEAKER_PIN, (uint8_t)0);
  }
  else {
    dacWrite(SPEAKER_PIN, (uint8_t)0);
    for(uint32_t i=0;i<ln;i++){
      delayMicroseconds(32);
    }
  }
}
void kpWait(uint32_t ln){
  dacWrite(SPEAKER_PIN, (uint8_t)0);
  delayMicroseconds(32*1024*ln);
}
void resetEvent(){
  increment=1;
  accumulator=0;
}
void initKeypress(){
  tk=pk=0;
  resetEvent();
  Base40Buffer.type.u=0;
  Base40Buffer.value.u=0;
}

void showAccumulator(int InputState){
#define ATCOLUMN 7
  int size=11;
  int16_t color;
  int accu=accumulator,incr=increment;
  if(InputState == INPUTSTATENUMERIC){
    color=OTHERCOLOR;
    color=NORMALCOLOR;
  }
  else {
#if 0
    color=NORMALCOLOR;
#else
    return;
#endif
  }
  eraseSpace(ROWS-1,ATCOLUMN,size);
  char space=' ';
  if(accu < 0){
    space='-';
    accu*=-1;
  }
  else if(accu == 0){
    space='0';
  }
  
  for(int i=0;i<size;i++){
    if(incr == 1){
      GO.lcd.setTextColor(NORMALBGCOLOR,color);
    }
    else {
      GO.lcd.setTextColor(color,NORMALBGCOLOR);
    }
    GO.lcd.setCharCursor(ATCOLUMN-1+size-i,ROWS-1);
    if(accu == 0){
      GO.lcd.printf("%c",space);
      space=' ';
    }
    else {
      GO.lcd.printf("%d",accu%10);
    }
    incr/=10;
    accu/=10;
  }
}
uint32_t i40(int i){
  uint32_t v=1;
  while(i){
    v*=40L;
    i--;
  }
  return v;
}
void showBase40(){
  uint32_t b4b=Base40Buffer.type.u;
  int size=lengthBase40(b4b);
  eraseSpace(BASE40ROW,BASE40COL,6);
  GO.lcd.setTextColor(YELLOW,NORMALBGCOLOR);
  for(int i=0;i<size;i++){
    GO.lcd.setCharCursor(BASE40COL-1+size-i, BASE40ROW);
    uint32_t inx=b4b%BASE;
    GO.lcd.printf("%c",Base40[inx]);
    b4b/=BASE;
  }
}

void pushBase40(int i){
  uint32_t power;
  power=0;
  while(i40(power) <= accumulator){
    power++;
  }
  accumulator+=(uint32_t)i*i40(power);
}
void popBase40(){
  uint32_t power;

  power=0;
  while(accumulator/i40(power) > 40L){
    power++;
  }
  power=(power>5)?5:power;
  accumulator=accumulator%i40(power);
}
void cmdPlus(){
  Word a,b,c;
  GFI i,j,k;
  
  a=dataStack.pop();
  i=a.i;
  b=dataStack.pop();
  j=b.i;
  k=j+i;
  c.i=k;
  dataStack.push(c);
}
void cmdMinus(){
  Word a,b,c;
  GFI i,j,k;
  
  a=dataStack.pop();
  i=a.i;
  b=dataStack.pop();
  j=b.i;
  k=j-i;
  c.i=k;
  dataStack.push(c);
}
void cmdTimes(){
  Word a,b,c;
  GFI i,j,k;
  
  a=dataStack.pop();
  i=a.i;
  b=dataStack.pop();
  j=b.i;
  k=j*i;
  c.i=k;
  dataStack.push(c);
}
void cmdDivide(){
  Word a,b,c;
  GFI i,j,k;
  
  a=dataStack.pop();
  i=a.i;
  b=dataStack.pop();
  j=b.i;
  k=j/i;
  c.i=k;
  dataStack.push(c);
}
void cmdModulo(){
  Word a,b,c;
  GFI i,j,k;
  
  a=dataStack.pop();
  i=a.i;
  b=dataStack.pop();
  j=b.i;
  k=j%i;
  c.i=k;
  dataStack.push(c);
}
void cmdAnd(){
  Word a,b,c;
  
  a=dataStack.pop();
  b=dataStack.pop();
  c.u=b.u&a.u;
  dataStack.push(c);
}
void cmdOr(){
  Word a,b,c;
  
  a=dataStack.pop();
  b=dataStack.pop();
  c.u=b.u|a.u;
  dataStack.push(c);
}
void cmdXor(){
  Word a,b,c;
  
  a=dataStack.pop();
  b=dataStack.pop();
  c.u=b.u^a.u;
  dataStack.push(c);
}
void cmdNot(){
  Word a,c;

  a=dataStack.pop();
  c.u=~a.u;
  dataStack.push(c);
}
void cmdLt(){ 
  Word a,b,c;
  a=dataStack.pop();
  b=dataStack.pop();
  c.u=(b.i < a.i)?-1:0;
  dataStack.push(c);
}
void cmdEqual(){
  Word a,b,c;
  a=dataStack.pop();
  b=dataStack.pop();
  c.i=(b.i == a.i)?-1:0;
  dataStack.push(c);
}
void cmdGt(){
  Word a,b,c;
  a=dataStack.pop();
  b=dataStack.pop();
  c.u=(b.i > a.i)?-1:0;
  dataStack.push(c);
}
void cmdPut32(){
  Word a,b;
  a=dataStack.pop();
  b=dataStack.pop();
  heap.put32(a.u,b);
}
void cmdGet32(){
  Word a,b;
  a=dataStack.pop();
  dataStack.push(heap.get32(a.u));
}
void cmdPut8(){
  Word a,b;
  a=dataStack.pop();
  b=dataStack.pop();
  heap.put8(a.u,uint8_t(b.u&0xFF));
}
void cmdGet8(){
  Word a,b,c;
  a=dataStack.pop();
  c.u=(uint32_t)heap.get8(a.u);
  dataStack.push(c);
}
void cmdNoop(){
  return;
}
void cmdBreakOut(){
  BreakOut=TRUE;
  return;
}
void cmdNote(){
  Word a,b;
  a=dataStack.pop();
  b=dataStack.pop();
  kpNote(a.u);
  kpPlay(b.u);
}
void cmdRctngl(){
  Word x,y,w,h,c;
  c=dataStack.pop();
  h=dataStack.pop();
  w=dataStack.pop();
  y=dataStack.pop();
  x=dataStack.pop();
  GO.lcd.fillRect(x.i,y.i,w.i,h.i,c.i);
}
// CUrsor Position
void cmdCup(){
  Word col,row;
  col=dataStack.pop();
  row=dataStack.pop();
  GO.lcd.setCharCursor(col.i, row.i);
  GO.update();
}
void cmdEmit(){
  Word a;
  char xyz[13];
  a=dataStack.pop();
  fromBase40(xyz,a.u);
  GO.lcd.printf("%s",xyz);
  GO.update();
}
void cmdEmitn(){
  Word a;
  GO.lcd.printf("%d",a.i);
  GO.update();
}
void cmdWait(){
  Word a;
  a=dataStack.pop();
  kpWait(a.u);
}
void cmdDelay(){
  Word a;
  a=dataStack.pop();
  delay(a.u);
}
void cmdSlower(){
  Word a;
  a=dataStack.pop();
  slower=a.u;
}
void cmdUdelay(){
  Word a;
  a=dataStack.pop();
  delayMicroseconds(a.u);
}
void cmdPinMode(){
  Word a,b;
  a=dataStack.pop();
  b=dataStack.pop();
  pinMode(a.u, b.u);
}
void cmdPinWrite(){
  Word a,b;
  a=dataStack.pop();
  b=dataStack.pop();
  digitalWrite(a.u, b.u);
}
void cmdIf(){
  Word a;
  
  a=dataStack.pop();
  if(a.i == 0){
    TypeWord tw=programStack.copy();
    while(tw.type.u != toBase40("THEN") && tw.type.u != toBase40("ELSE")){
      programStack.selectNext();
      tw=programStack.copy();
    }
  }
}
void cmdDo(){
#if DEBUG
  Serial.println("cmdDo");
#endif
  Word a,b,c;
  a=dataStack.pop();
  b=dataStack.pop();
  c.i=programStack.getSelected();
  returnStack.push(b);
  returnStack.push(a);
  loopStack.push(c);
}
void cmdBegin(){
  Word c;
  c.i=programStack.getSelected();
  loopStack.push(c);
}
void cmdDrop(){
  Word a;
  a=dataStack.pop();
}
void cmdButton(){
  Word a;
  a.u=(GFU)readkey();
  dataStack.push(a);
}
void cmdFalse(){
  Word a;
  a.i=FALSE;
  dataStack.push(a);
}
void cmdTrue(){
  Word a;
  a.i=TRUE;
  dataStack.push(a);
}
void cmdRed(){
  Word a;
  a.i=RED;
  dataStack.push(a);
}
void cmdGreen(){
  Word a;
  a.i=GREEN;
  dataStack.push(a);
}
void cmdBlue(){
  Word a;
  a.i=BLUE;
  dataStack.push(a);
}
void cmdTime(){
  Word a;
  a.i=millis();
  dataStack.push(a);
}
void cmdDac(){
  Word a;
  a=dataStack.pop();
  dacWrite(SPEAKER_PIN, (uint8_t)(a.u&0x000000FF));
}
void cmdQuote(){
  programStack.selectNext();
  dataStack.push(programStack.copy().type);
  if(dataStack.peek().i == CMDPUSH){
    globalerror=1;
    globalmessage="ERROR: Trying to quote a number";
  }
}
void cmdAgain(){
  Word c=loopStack.pop();
  loopStack.push(c);
  programStack.setSelected(c.i);
}
void cmdUntil(){
  Word a=dataStack.pop();
  if(a.i == FALSE){
    cmdAgain();
  }
  else {
    loopStack.pop();
  }
}
void cmdLoop(){
  Word a,b,c;
  a=returnStack.pop();
  b=returnStack.pop();
  c=loopStack.pop();
  a.i++;
  if(a.i < b.i){
    returnStack.push(b);
    returnStack.push(a);
    loopStack.push(c);
    programStack.setSelected(c.i);
  }
}
void cmdLoopVariable(){
  dataStack.push(returnStack.peek());
}
void cmdElse(){
  TypeWord tw=programStack.copy();
  while(tw.type.u != toBase40("THEN")){
    programStack.selectNext();
    tw=programStack.copy();
  }
}
void cmdPush(){
  TypeWord tw=programStack.copy();
  dataStack.push(tw.value);
}
void cmdDup(){
  Word a=dataStack.pop();
  dataStack.push(a);
  dataStack.push(a);
}
void cmdOver(){
  Word a=dataStack.pop();
  Word b=dataStack.pop();
  dataStack.push(b);
  dataStack.push(a);
  dataStack.push(b);
}
void cmdRot(){
  Word a=dataStack.pop();
  Word b=dataStack.pop();
  Word c=dataStack.pop();
  dataStack.push(b);
  dataStack.push(a);
  dataStack.push(c);
}
void cmdSwap(){
  Word a=dataStack.pop();
  Word b=dataStack.pop();
  dataStack.push(a);
  dataStack.push(b);
}
void cmdSemicolon(){
  // Restore previous value of selected
  if(returnStack.size() > 0){
    programStack.setSelected(returnStack.pop().i);
  }
#if 0
  else {
    // ERROR: Ran into ; without returnStack entry
    Serial.println(" -1 !!!");
    break;
  }
#endif
}
void cmdPread(){
  Serial.println("PREAD");
  programStack.ReadFile(dataStack.pop());
}
void cmdPlist(){
  programStack.ListFiles();
}
void cmdPsave(){
  programStack.WriteFile(dataStack.pop());
}
void commandInit(){
  CmdLookup.addEntry(&cmdPush, CMDPUSH, "(- value)");
  CmdLookup.addEntry(&cmdBreakOut, toBase40(":"), "(-)");
  CmdLookup.addEntry(&cmdPlus, toBase40("+"),     "(r1 r2 - sum)");
  CmdLookup.addEntry(&cmdMinus, toBase40("-"),    "(r1 r2 - difference)");
  CmdLookup.addEntry(&cmdTimes, toBase40("*"),    "(r1 r2 - product)");
  CmdLookup.addEntry(&cmdDivide, toBase40("/"),   "(dividend divisor - value)");
  CmdLookup.addEntry(&cmdAnd, toBase40("&"),      "(r1 r2 - and)");
  CmdLookup.addEntry(&cmdOr, toBase40("|"),       "(r1 r2 - or)");
  CmdLookup.addEntry(&cmdXor, toBase40("XOR"),    "(r1 r2 - xor)");
  CmdLookup.addEntry(&cmdLt, toBase40("<"),       "(r1 r2 - less?)");
  CmdLookup.addEntry(&cmdEqual, toBase40("="),    "(r1 r2 - equal?)");
  CmdLookup.addEntry(&cmdGt, toBase40(">"),       "(r1 r2 - greater)");
  CmdLookup.addEntry(&cmdDac, toBase40("DAC"),    "(r1 -)");
  CmdLookup.addEntry(&cmdGet8, toBase40("GETBYT"),"(r1 - value)");
  CmdLookup.addEntry(&cmdPut8, toBase40("PUTBYT"),"(r1 r2 -)");
  CmdLookup.addEntry(&cmdGet32, toBase40("GET"),  "(r1 - value)");
  CmdLookup.addEntry(&cmdPut32, toBase40("PUT"),  "(r1 r2 -)");
  CmdLookup.addEntry(&cmdModulo, toBase40("MOD"), "(dividend divisor - rem)");
  // Liable to get deleted
  CmdLookup.addEntry(&cmdNote, toBase40("NOTE"),  "(r1 r2 -)");
  // Liable to get deleted
  CmdLookup.addEntry(&cmdWait, toBase40("WAIT"),  "(r1 -)");
  CmdLookup.addEntry(&cmdDelay, toBase40("DELAY"),"(r1 -)");
  CmdLookup.addEntry(&cmdUdelay, toBase40("UDELAY"), "(r1 -)");
  CmdLookup.addEntry(&cmdSlower, toBase40("SLOWER"), "(r1 -)");
  // Liable to get deleted
  CmdLookup.addEntry(&cmdFalse, toBase40("*FALSE"), "(- value)");
  CmdLookup.addEntry(&cmdQuote, toBase40("QUOTE"),"(- value)");
  // Liable to get deleted
  CmdLookup.addEntry(&cmdTrue, toBase40("*TRUE"), "(- value)");
  CmdLookup.addEntry(&cmdRed, toBase40("*RED"),   "(- value)");
  CmdLookup.addEntry(&cmdGreen,toBase40("*GREEN"),"(- value)");
  CmdLookup.addEntry(&cmdBlue, toBase40("*BLUE"), "(- value)");
  CmdLookup.addEntry(&cmdTime, toBase40("TIME"),  "(- value)");
  CmdLookup.addEntry(&cmdDup, toBase40("DUP"),    "(r1 - r1 r1)");
  CmdLookup.addEntry(&cmdOver, toBase40("OVER"),  "(r1 r2 - r1 r2 r1)");
  CmdLookup.addEntry(&cmdRot, toBase40("ROT"),    "(r1 r2 r3 - r2 r3 r1)");
  CmdLookup.addEntry(&cmdSwap, toBase40("SWAP"),  "(r1 r2 - r2 r1)");
  CmdLookup.addEntry(&cmdSemicolon, toBase40(";"), "(-)");
  CmdLookup.addEntry(&cmdPread, toBase40("PREAD"), "(r1 -)");
  CmdLookup.addEntry(&cmdPlist, toBase40("PLIST"), "(-)");
  CmdLookup.addEntry(&cmdPsave, toBase40("PSAVE"), "(r1 -)");
  CmdLookup.addEntry(&cmdIf, toBase40("IF"), "(r1 -)");
  CmdLookup.addEntry(&cmdElse, toBase40("ELSE"), "(-)");
  CmdLookup.addEntry(&cmdNoop, toBase40("THEN"), "(-)");
  CmdLookup.addEntry(&cmdDo, toBase40("DO"),      "(limit initial -)");
  CmdLookup.addEntry(&cmdDrop, toBase40("DROP"),  "(r1 -)");
  CmdLookup.addEntry(&cmdLoop, toBase40("LOOP"),  "(-)");
  CmdLookup.addEntry(&cmdBegin, toBase40("BEGIN"),"(-)");
  CmdLookup.addEntry(&cmdAgain, toBase40("AGAIN"),"(-)");
  CmdLookup.addEntry(&cmdUntil, toBase40("UNTIL"),"(r1 -)");
  // Liable to be replaced with I2C commands
  CmdLookup.addEntry(&cmdPinMode, toBase40("PINMOD"), "(pin mode -)");
  // Liable to be replaced with I2C commands
  CmdLookup.addEntry(&cmdPinWrite, toBase40("PINWRI"), "(pin value -)");
  CmdLookup.addEntry(&cmdLoopVariable, toBase40("I"), "(- value)");
  CmdLookup.addEntry(&cmdRctngl, toBase40("RCTNGL"), "(x y wt ht color -)");
  CmdLookup.addEntry(&cmdCup, toBase40("CUP"),       "(row column -)");
  CmdLookup.addEntry(&cmdEmit, toBase40("EMIT"), "(r1 -)");
  CmdLookup.addEntry(&cmdButton, toBase40("BUTTON"), "(- value)");
  CmdLookup.addEntry(&cmdEmitn, toBase40("."), "(r1 -)");
}
int programScan(){
  int level=0,lastQuote=false;
  definedWordIndex=0;
  programStack.selectFirst();
  while(level >= 0 && programStack.getSelected() != -1){
    TypeWord tw=programStack.copy();
    if(tw.type.u == toBase40(":")){
      lastQuote=false;
      level++;
      programStack.selectNext();
      tw=programStack.copy();
      definedWord[definedWordIndex].word=tw.type.u;
      definedWord[definedWordIndex].index=programStack.getSelected();
      definedWordIndex++;
    }
    else if(tw.type.u == toBase40(";")){
      lastQuote=false;
      level--;
    }
    else if(tw.type.u == toBase40("QUOTE")){
      lastQuote=true;
    }
    else if(lastQuote == true && tw.type.i == CMDPUSH){
      return true;
    }
    else {
      lastQuote=false;
    }
    programStack.selectNext();
  }
  return false;
}
void dumpDefined(){
  for(int i=0;i<definedWordIndex;i++){
    Serial.print("DefinedWord: ");
    Serial.print(definedWord[i].word);
    Serial.print(": ");
    Serial.println(definedWord[i].index);
  }
}
void programExecute(int singleStep){
  TypeWord tw;
  int remember=programStack.getSelected();
  BreakOut=FALSE;
  globalerror=0;
  if(programScan()){
    // Information less return at this point.
    // Eventually will add diagnostic message, in this case
    // number following quote
    return;
  }
  if(singleStep && remember != -1){
    programStack.setSelected(remember);
  }
  else {
    programStack.selectFirst();
  }
  while(programStack.getSelected() != -1){
    tw=programStack.copy();
    // How much does this cost?
    if(slower){
      GO.update();
      if( GO.BtnA.isPressed() && GO.BtnB.isPressed() ){
        singleStep=true;
      }
    }
    if(CmdLookup.callEntry(tw.type.u)){
      /* NOOP - already called function */
    }
    else {
      for(int j=0;j<definedWordIndex;j++){
        if(tw.type.u == definedWord[j].word){
          // Push return address
          Word w;
          w.i=programStack.getSelected();
          returnStack.push(w);
          // Set next address
          programStack.setSelected(definedWord[j].index);
          break;
        }
      }
    }
    if(globalerror){
      return;
    }
    programStack.selectNext();
    if(singleStep || BreakOut){
      return;
    }
  }
}
int AnRC2I(RowCol rc){
  return rc.row*ANCOLS+rc.col+1;
}
int I2Base40(int i){
  // Map < to delete (-1)
  if(i == strlen(Base40)-1){
    return -1;
  }
  return i;
}
void AnApnd(RowCol rc){
  TypeWord tw;
  int inx=AnRC2I(rc);
  // Delete
  if(inx >= BASE){
    Base40Buffer.type.u/=BASE;
  }
  else {
    Base40Buffer.type.u=(Base40Buffer.type.u*BASE)+inx;
  }
  if(Base40Buffer.type.u >= (BASE*BASE*BASE*BASE*BASE)){
    programStack.paste(Base40Buffer);
    Base40Buffer.type.u=0;
  }
}
RowCol AnRNx(RowCol rc){
  rc.row++;
  if(rc.row == ANROWS){
    rc.row=0;
    rc.col^=4;
  }
  return rc;
}
RowCol AnCNx(RowCol rc){
  rc.col=(rc.col+1)%ANCOLS;
  return rc;
}
RowCol AnRPv(RowCol rc){
  rc.row--;
  if(rc.row == -1){
    rc.row=ANROWS-1;
    rc.col^=4;
  }
  return rc;
}
RowCol AnCPv(RowCol rc){
  rc.col=(rc.col+ANCOLS-1)%ANCOLS;
  return rc;
}
int showMessage(int line){
  Serial.println("showMessage");
  if(globalmessage != NULL){
    Serial.print("Printing...");
    Serial.println(globalmessage);
    line++;
    eraseSpace(line,0,COLUMNS);
    GO.lcd.setCharCursor(0, line);
    GO.lcd.printf("%s",globalmessage);
  }
  globalmessage=NULL;
  return line;
}
void showAlphaNumeric(int InputState){
  uint16_t color;
  if(InputState == INPUTSTATEALPHANUMERIC){
    color=OTHERCOLOR;
    color=NORMALCOLOR;
  }
  else {
    return;
  }
  RowCol local;
  for(local.row=0;local.row<ANROWS;local.row++){
    for(local.col=0;local.col<ANCOLS;local.col++){
      if(an.row == local.row && an.col == local.col){
        GO.lcd.setTextColor(NORMALBGCOLOR,color);
      }
      else {
        GO.lcd.setTextColor(color,NORMALBGCOLOR);
      }
      GO.lcd.setCharCursor(local.col, 10+local.row);
      GO.lcd.printf("%c",Base40[AnRC2I(local)]);
    }
  }
  showBase40();
}
void showState(int InputState, int PreviousInputState){
  int line=0,limitLine;
  if(InputState == INPUTSTATECLEAR){
    limitLine=ROWS;
  }
  else if(InputState == INPUTSTATEMENU || InputState == INPUTSTATEALPHANUMERIC){
    limitLine=ROWS-5;
  }
  else if(InputState == INPUTSTATENUMERIC){
    limitLine=ROWS-1;
  }
// Testing cmdWriteFile
  if(InputState != PreviousInputState){
// made a difference when changing from CLEAR to NUMERIC
    for(int i=0;i<ROWS;i++){
      eraseSpace(i,0,COLUMNS);
    }
  }
  GO.update();
  line=dataStack.show(line);
  line=showMessage(line);
  programStack.show(line+1,InputState,limitLine);
  showAlphaNumeric(InputState);
  GO.update();
  showAccumulator(InputState);
  GO.update();
  menuStack.show(InputState);
  GO.update();
}

/* Switching to expect one keypress at a time */
void processEvent(uint16_t pkk, uint16_t tkk){
  int i;
  if(increment < 1){
    increment=1;
  }
  PreviousInputState=InputState;
  if(pkk == KPA){
    switch(tkk){
      case KPLEFT|KPA:
        InputState=INPUTSTATEALPHANUMERIC;
        break;
      case KPRIGHT|KPA:
        InputState=INPUTSTATENUMERIC;
        break;
      case KPUP|KPA:
        InputState=INPUTSTATECLEAR;
        break;
      case KPDOWN|KPA:
        InputState=INPUTSTATEMENU;
        break;
    }
  }
  if(InputState == INPUTSTATEALPHANUMERIC){
      switch(tkk){
        case KPLEFT:
          an=AnCPv(an);
          break;
        case KPRIGHT:
          an=AnCNx(an);
          break;
        case KPUP:
          an=AnRPv(an);
          break;
        case KPDOWN:
          an=AnRNx(an);
          break;
        case KPB:
          AnApnd(an);
          break;
        case KPB|KPA:
        case KPSELECT:
          programStack.paste(Base40Buffer);
          Base40Buffer.type.u=0;
          break;
        case KPSTART:
          AnApnd(anDelete);
          break;
      }
  }
  else if(InputState == INPUTSTATEMENU){
    switch(tkk){
      case KPLEFT:
        menuStack.previous();
        break;
      case KPRIGHT:
        menuStack.next();
        break;
      case KPUP:
        menuStack.previousLine();
        break;
      case KPDOWN:
        menuStack.nextLine();
        break;
      case KPB:
        TypeWord tw;
        tw.type=menuStack.getSelected();
        programStack.paste(tw);
        break;
      case KPMENU:
        Word w;
        w=menuStack.getSelected();
        globalmessage=CmdLookup.getHelpMessage(w.u);
        Serial.print("message is ");
        Serial.println(globalmessage);
        break;
    }
  }
  else if(InputState == INPUTSTATENUMERIC){
      switch(tkk){
        case KPLEFT:
          increment=(increment >= 1000000000)?increment:increment*10;
          break;
        case KPRIGHT:
          increment=(increment == 1)?1:increment/10;
          break;
        case KPUP:
          accumulator+=increment;
          break;
        case KPDOWN:
          accumulator-=increment;
          break;
        case KPB:
          TypeWord tw;
          tw.type.i=CMDPUSH;
          tw.value.i=accumulator;
          programStack.paste(tw);
      }
  }
  else if(InputState == INPUTSTATECLEAR){
      switch(tkk){
        case KPB:
          programExecute(FALSE);
          Serial.println("Completed programExecute(FALSE)");
          break;
        case KPB|KPA:
          programExecute(TRUE);
          Serial.println("Completed programExecute(TRUE)");
          break;
        case KPMENU:
          break;
        case KPVOLUME:
          dataStack.clear();
          break;
        case KPSELECT:
          programStack.paste(cutStack.cut());
          break;
        case KPSTART|KPA:
          programStack.init();
          break;
        case KPSTART:
          cutStack.paste(programStack.cut());
          break;
        case KPLEFT:
          programStack.selectPrevious();
          break;
        case KPRIGHT:
          programStack.selectNext();
          break;
        case KPUP:
          break;
        case KPDOWN:
          break;
      }
  }
  if(pkk < tkk){
    showState(InputState,PreviousInputState);
  }
}
uint16_t readkey(){
  uint16_t tkk=0;

  GO.update();

  tkk |= ((GO.BtnMenu.isPressed()) ? KPMENU : 0);
  tkk |= ((GO.BtnVolume.isPressed()) ? KPVOLUME : 0);
  tkk |= ((GO.BtnSelect.isPressed()) ? KPSELECT : 0);
  tkk |= ((GO.BtnStart.isPressed()) ? KPSTART : 0);
  tkk |= ((GO.BtnA.isPressed()) ? KPA : 0);
  tkk |= ((GO.BtnB.isPressed()) ? KPB : 0);
  tkk |= ((GO.JOY_Y.isAxisPressed() == 2) ? KPUP : 0);
  tkk |= ((GO.JOY_Y.isAxisPressed() == 1) ? KPDOWN : 0);
  tkk |= ((GO.JOY_X.isAxisPressed() == 2) ? KPLEFT : 0);
  tkk |= ((GO.JOY_X.isAxisPressed() == 1) ? KPRIGHT : 0);

  return tkk;
}
void keypress(){
  uint16_t tkk=readkey();
  if(tkk != pk){
    if(tkk != 0){
      digitalWrite(LED_BUILTIN, HIGH);
    }
    else {
      digitalWrite(LED_BUILTIN, LOW);
    }
    processEvent(pk,tkk);
    pk=tkk;
    return;
  }
  // No change
  return;
}
void setup() {
  // Initialize globals
  Word truth;
  truth.i=TRUE;
  globalmessage=NULL;
  globalerror=0;
  
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(25, OUTPUT);
  digitalWrite(25, LOW);
  delay(100);
  GO.begin();
  dacWrite(SPEAKER_PIN, (uint8_t)0);
  GO.lcd.setTextSize(2);
  GO.lcd.setTextColor(NORMALCOLOR);
  CmdLookup.init();
  dataStack.clear();
  commandInit();
  dataStack.push(truth);
  cmdSlower();
  initKeypress();
  heap.init();
  GO.update();
  showState(InputState,PreviousInputState);
}
void loop() {
  // Process keypress
  keypress();
  delay(32);
}

#include <odroid_go.h>
#include <math.h>
// 
// To Do:
// 0) I have a follower - get documentation complete, more frequent commits, release notes
// 1) helpLookup.. in CmdLookup is limited to 64.  At least make 64 a symbol (parameterized)
// 2) Change hash overflow lookup to the simplest binary search lookup
// 3) PREAD doesn't work after program download.  Have to cycle power, then it works.  Maybe not fixable.
// 4) Sometimes left arrow doesn't move to select next (visibly), then left again skips it.
//  - Implement scripted analog computer
//    - requires graphics, text output, inputs
//
// Floating Point
// 1) Entering floating point - DONE
// 2) Showing floting numbers in program - DONE
// 3) No straight forward (without changing structure of stack) method of showing floats in stack
// 4) Floating point methods - DONE
// 4a) Conversion methods - DONE
// 5) ReadFile, WriteFile - DONE
// Bugs: after entering float, increment gets set to 0 or -1 - NOT SEEN
//       I noticed it reboot after doing A-B in numeric and other stuff. - NOT SEEN
//       Numeric entry changes to yellow at some point. - DONE
//       View decimal position when entering float - DONE
//
uint16_t tk, pk, slower;
uint32_t globalerror;
const char *globalmessage;

// #define VERSION "0.0.2" // Completed work on initial bugs, adding some features
#define VERSION "0.0.3" // Added floating point

#define TRUE -1
#define FALSE 0
#define DEBUG 0
#define WAIT 0

#define ERRORNONE 0
#define ERRORUNDERFLOW 1
#define ERROROVERFLOW 2
#define ERRORQUOTENUMBER 3

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
#define CMDFPUSH -2
#define NOOP 0x00

#define INPUTSTATEALPHANUMERIC 0
#define INPUTSTATENUMERIC 1
#define INPUTSTATECLEAR 2
#define INPUTSTATEMENU 3

#define KEEPHASH -1
#define FLOATING -1

int InputState=INPUTSTATEMENU,PreviousInputState=INPUTSTATECLEAR;
int BreakOut;
#if 0
typedef struct {
  uint32_t word,index;
} DefinedWord;
int32_t definedWordIndex=0;
DefinedWord definedWord[128];
#endif
#define BASE 40
char Base40[]=" ABCDEFGHIJKLMNOPQRSTUVWXYZ:;+-*/&|^.<=>\e";
char Base10[]="0123456789-";

#define ROWS 15
#define COLUMNS 26
#define ANCOLS 8
#define ANROWS 5
#define MENUROWS 5
#define BASE40ROW 10
#define BASE40COL 8

#define STACKSIZE 1024
#if FLOATING
typedef float GFF;
typedef int32_t GFI;
typedef uint32_t GFU;
typedef union {GFU u; GFI i;GFF f;} Word;
#else
typedef int32_t GFI;
typedef uint32_t GFU;
typedef union {GFU u; GFI i;} Word;
#endif
typedef struct {Word type, value;} TypeWord;
typedef struct {
  int16_t row,col;
} RowCol;
typedef struct {
  void (*fptr)(void);
  GFU original, defined;
} HashEntry;
typedef struct {
  GFU cmd;
  const char *msg;
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

void globalError(int error){
  switch(error){
    case ERRORNONE:
      globalmessage=NULL;
      break;
    case ERRORUNDERFLOW:
      globalmessage="Stack Underflow";
      break;
    case ERROROVERFLOW:
      globalmessage="Stack Overflow";
      break;
    case ERRORQUOTENUMBER:
      globalmessage="Quote Number";
      break;
    default:
      break;
  }
  return;
}


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
uint32_t toBase40(const char *txt){
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
int nFloat(char c){
  char Float[]="0123456789-.";
  int i=0;
  while(Float[i] != c){
    i++;
    if(i == strlen(Float)){
      return -1;
    }
  }
  return i;
}
int isFloat(char *txt){
  int sign=1,next=0,point=false;
  int i=nFloat(txt[next]);
  if(i == -1){
    return false;
  }
  if(i == 10){
    sign=-1;
    next++;
  }
  while(txt[next] != 0 && txt[next] != ' ' && txt[next] != '\n' && txt[next] != '\r' && txt[next] != '\t'){
    i=nBase10(txt[next++]);
    if(point == false && i == 11){
      point=true;
    }
    else if(i == -1 || i > 9){
      return false;
    }
  }
  return true;  
}
GFF toFloat(char *txt){
  int next=0,point=false;
  GFF sign=1.0,sum=0.0,divisor=1.0;;
  int i=nFloat(txt[next]);
  if(i == -1){
    return 0;
  }
  if(i == 10){
    sign=-1.0;
    next++;
  }
  while(txt[next] != 0 && txt[next] != ' ' && txt[next] != '\n' && txt[next] != '\r' && txt[next] != '\t'){
    i=nFloat(txt[next++]);
    if(point){
      divisor*=10.0;
    }
    if(point == false && i == 11){
      point=true;
    }
    else if(i == -1 || i > 9){
      return 0;
    }
    else {
      if(point){
        sum+=((GFF)i)/divisor;
      }
      else {
        sum=(10.0*sum)+(GFF)i;
      }
    }
  }
  return sum*sign;
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
void eraseLine(int line){
  GO.lcd.fillRect(0,line*16,320,16,0x0000);
}
void eraseSpace(int line,int start, int width){
  GO.lcd.fillRect(start*12,line*16,width*12,16,0x0000);
  GO.update();
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
class Accumulator {
  Word accumulator,increment;
  int floating;
  public:
  void init(){
    accumulator.i=0;
    increment.i=1;
    floating=false;
  }
  void incr(){
    if(floating){
      accumulator.f += increment.f;
    }
    else {
      accumulator.i += increment.i;
    }
  }
  void decr(){
    if(floating){
      accumulator.f -= increment.f;
    }
    else {
      accumulator.i-=increment.i;
    }
  }
  void shiftLeft(){
    if(floating){
      increment.f *= 10.0;
    }
    else {
      increment.i=(increment.i >= 1000000000)?increment.i:increment.i*10;
    }
  }
  void shiftRight(){
    if(floating){
      increment.f /= 10.0;
    }
    else {
#if 1
      if(increment.i == 1){
        floating=true;
        increment.f=1.0/10.0;
        accumulator.f = (GFF)accumulator.i;
      }
      else {
        increment.i /= 10;
      }
#else
      increment.i=(increment.i == 1)?1:increment.i/10;
#endif
    }
  }
  void minimums(){
    if(floating == false && increment.i < 1){
      increment.i=1;
    }
  }
  int isFloating(){
    return floating;
  }
  Word get(){
    return accumulator;
  }
  void show(int InputState){
#define ATCOLUMN 7
    int size=11;
    int16_t color;
    Word accu=accumulator,incr=increment;
    if(InputState == INPUTSTATENUMERIC){
      color=NORMALCOLOR;
    }
    else {
      return;
    }
    eraseSpace(ROWS-1,ATCOLUMN,size);
    char space=' ';
// Not yet
    if(floating){
      if(roundf(incr.f) >= 1.0){
        incr.f = roundf(incr.f);
      }
      // floating point display
      GFF places=10.0,remainder;
      int digit,i=0;
      GO.lcd.setTextColor(color,NORMALBGCOLOR);
      if(accu.f < 0){
        space='-';
        accu.f*=-1;
        GO.lcd.setCharCursor(ATCOLUMN+(i++),ROWS-1);
        GO.lcd.printf("%c",space);
      }
      while(places <= accu.f || places <= incr.f){
        places *= 10.0;
      }
      // Leading digits
      while(places > 1.0){
        places /= 10.0;
        digit = ((int)(accu.f / places)) % 10;
//        if(roundf(incr.f) == places){
        if(incr.f == places){
          GO.lcd.setTextColor(NORMALBGCOLOR,color);
        }
        else {
          GO.lcd.setTextColor(color,NORMALBGCOLOR);
        }
        GO.lcd.setCharCursor(ATCOLUMN+(i++),ROWS-1);
        if(digit == 0 && places > accu.f){
          GO.lcd.printf(" ");
        }
        else {
          GO.lcd.printf("%d",digit);
        }
      }
      // Then the decimal point
      space='.';
      GO.lcd.setCharCursor(ATCOLUMN+(i++),ROWS-1);
      GO.lcd.setTextColor(color,NORMALBGCOLOR);
      GO.lcd.printf("%c",space);

      // Fractional digits
      places = 10.0;
      accu.f -= (float)((int)accu.f);
      while((ATCOLUMN+i) < 26){
        digit = ((int)(accu.f * places))%10;
        if(roundf(1.0/incr.f) == places){
          GO.lcd.setTextColor(NORMALBGCOLOR,color);
        }
        else {
          GO.lcd.setTextColor(color,NORMALBGCOLOR);
        }
        GO.lcd.setCharCursor(ATCOLUMN+(i++),ROWS-1);
        GO.lcd.printf("%d",digit);
        places *= 10.0;
      }
    }
    else {
      // Integer display
      if(accu.i < 0){
        space='-';
        accu.i*=-1;
      }
      else if(accu.i == 0){
        space='0';
      }
      
      for(int i=0;i<size;i++){
        if(incr.i == 1){
          GO.lcd.setTextColor(NORMALBGCOLOR,color);
        }
        else {
          GO.lcd.setTextColor(color,NORMALBGCOLOR);
        }
        GO.lcd.setCharCursor(ATCOLUMN-1+size-i,ROWS-1);
        if(accu.i == 0){
          GO.lcd.printf("%c",space);
          space=' ';
        }
        else {
          GO.lcd.printf("%d",accu.i%10);
        }
        incr.i/=10;
        accu.i/=10;
      }
      eraseSpace(ROWS-1,ATCOLUMN+size,COLUMNS-(ATCOLUMN+size));
    }
  }
} accumulator;
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
    if(i < 0){
      globalError(ERRORUNDERFLOW);
    }
    else if(i >= HEAPLIMITB){
      globalError(ERROROVERFLOW);
    }
    else {
      u.b[i]=b;
    }
  }
  void put32(uint32_t i, Word w){
    if(i < 0){
      globalError(ERRORUNDERFLOW);
    }
    else if(i >= HEAPLIMITW){
      globalError(ERROROVERFLOW);
    }
    else {
      u.w[i].u=w.u;
    }
  }
  uint8_t get8(uint32_t i){
    uint8_t x;
    if(i < 0){
      globalError(ERRORUNDERFLOW);
      x=0;
      return x;
    }
    else if(i >= HEAPLIMITB){
      globalError(ERROROVERFLOW);
      x=0;
      return x;
    }
    else {
      return u.b[i];
    }
  }
  Word get32(uint32_t i){
    Word x;

    if(i < 0){
      globalError(ERRORUNDERFLOW);
      x.u=0;
      return x;
    }
    else if(i >= HEAPLIMITW){
      globalError(ERROROVERFLOW);
      x.u=0;
      return x;
    }
    else {
      return u.w[i];
    }
  }
} heap;

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
        globalError(ERROROVERFLOW);
      }
    };
    Word pop(){
      if(nws > 0){
        return value[--nws];
      }
      globalError(ERRORUNDERFLOW);
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
    void write(File f){
      for(int i=0;i<nws;i++){
        f.write((unsigned char *)&value[i].u,4);
      }
    }
// Display data stack starting on the given line.
    void show(){
      int offset,size,column,line;
      column=line=0;
      for(int i=0;i<nws;i++){
        size=lenInt(value[i].i);
        if(column + size > COLUMNS){
          column=0;
          line++;
        }
        column += size+1;
      }
      offset=(line<2)?0:(line-1);
      GO.lcd.setTextColor(NORMALCOLOR,NORMALBGCOLOR);
      erase(0);
      erase(1);
      column=line=0;
      for(int i=0;i<nws;i++){
        size=lenInt(value[i].i);
        if(column + size > COLUMNS){
          column=0;
          line++;
        }
        if((line-offset) >= 0){
          GO.lcd.setCharCursor(column, line-offset);
          GO.lcd.printf("%d",value[i].i);
        }
        column += size+1;
      }
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
        if(tw.type.i == CMDPUSH || tw.type.i == CMDFPUSH){
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
      init();
    }
    void init(){
      selected=-1;
      lastLine=ntw=nOffset=0;      
      ds.clear();
    }
    void paste(TypeWord tw){
      if(tw.type.i != NOOP){
        int n=twfind(selected);
        ds.paste(tw.type,n);
        if(tw.type.i == CMDPUSH || tw.type.i == CMDFPUSH){
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
        if(tw.type.i == CMDPUSH || tw.type.i == CMDFPUSH){
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
        globalError(ERRORUNDERFLOW);
      }
      return tw;
    }
    TypeWord get(int n){
      TypeWord tw;
      int nt=twfind(n);
      tw.type = ds.copy(nt);
      if(tw.type.i == CMDPUSH || tw.type.i == CMDFPUSH){
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
      if(tw.type.i == CMDPUSH || tw.type.i == CMDFPUSH){
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
      if(tw.type.i == CMDPUSH || tw.type.i == CMDFPUSH){
        ds.push(tw.value);
      }
      ntw++;
    }
    TypeWord pop(){
      TypeWord tw;
      int n=twfind(--ntw);
      tw.type = ds.copy(n);
      if(tw.type.i == CMDPUSH || tw.type.i == CMDFPUSH){
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
        if(w.i == CMDPUSH || w.i == CMDFPUSH){
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

    void nextLine(){
      int size=0;
      int nxt=getSelected()+1;
      while(nxt < ntw){
        TypeWord tw=get(nxt);
        if(tw.type.i == CMDPUSH){
          size+=(lenInt(tw.value.i) + 1);
        }
        else if(tw.type.i == CMDFPUSH){
          size += 10;
        }
        else {
          size+=(lenBase40(tw.type.u) + 1);
        }
        if(size >= COLUMNS){
          setSelected(nxt);
          return;
        }
        nxt++;
      }
      setSelected(0);
    }
    void previousLine(){
      int size=0;
      int nxt=getSelected()-1;
      while(nxt > -1){
        TypeWord tw=get(nxt);
        if(tw.type.i == CMDPUSH){
          size+=(lenInt(tw.value.i) + 1);
        }
        else if(tw.type.i == CMDFPUSH){
          size += 10;
        }
        else {
          size+=(lenBase40(tw.type.u) + 1);
        }
        if(size >= COLUMNS){
          setSelected(nxt);
          return;
        }
        nxt--;
      }
      setSelected(ntw-1);
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
              else if(isFloat(ascii)){
                tw.type.i=CMDFPUSH;
                tw.value.f=toFloat(ascii);
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
          else if(isFloat(ascii)){
            tw.type.i=CMDFPUSH;
            tw.value.f=toFloat(ascii);
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
          // 
          if(tw.type.i == CMDPUSH){
            sprintf(ascii,"%d ",tw.value.i);
            Serial.println(ascii);
            for(int i=0;i<strlen(ascii);i++){
              writeFile.write((unsigned char)ascii[i]);
            }
          }
          else if(tw.type.i == CMDFPUSH){
            sprintf(ascii,"%14.7 ",tw.value.f);
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
// Display a stack 
    void show(int is, int limitLine){
      uint16_t color=(is == INPUTSTATECLEAR)?NORMALCOLOR:OTHERCOLOR;
      char ascii[16];
      int size,changed=1,startLine=2;

      if(ntw == 0){
        for(int i=startLine;i<limitLine;i++){
          erase(i);
        }
        return;
      }
      // Don't change nOffset if nothing is selected
      int startActual=0;
      for(int actual=startActual;actual<2;actual++){
        int screenLine=startLine-nOffset,line=startLine;
        int column=0;

        for(int i=0;i<ntw;i++){
          // Figure out the size of the next word
          TypeWord tw=get(i);
          if(tw.type.i == CMDPUSH){
            size=lenInt(tw.value.i);
          }
          else if(tw.type.i == CMDFPUSH){
            size=10;
          }
          else {
            size=lenBase40(tw.type.u);
          }
          if(column + size > COLUMNS){
            if(actual){
              if(screenLine >= startLine && screenLine < limitLine){
                eraseSpace(screenLine,column,COLUMNS-column);
              }
            }
            line++;
            screenLine++;
            column=0;                        
          }
          if(actual){
            if(screenLine >= startLine && screenLine < limitLine){
              if(i == selected){
                GO.lcd.setTextColor(NORMALBGCOLOR,color);
              }
              else {
                GO.lcd.setTextColor(color,NORMALBGCOLOR);
              }
              GO.lcd.setCharCursor(column, screenLine);
              if(tw.type.i == CMDPUSH){
                GO.lcd.printf("%d",tw.value.i);
              }
              else if(tw.type.i == CMDFPUSH){
                GO.lcd.printf("%10.5e",tw.value.f);
              }
              else {
                fromBase40(ascii,tw.type.u);
                GO.lcd.printf("%s",ascii);
              }
            }
          }
          else if(i == selected){
            if(screenLine >= limitLine){
              nOffset+=1+screenLine-limitLine;
            }
            else if(screenLine < startLine){
              nOffset+=(screenLine-startLine);
            }
          }
          column+=size;
          if(actual){
            if(screenLine >= startLine && screenLine < limitLine){
              if(column <= COLUMNS){
                eraseSpace(screenLine,column,1);              
              }
            }
          }
          column++;
        }
        eraseSpace(screenLine,column,COLUMNS-column);              
        for(int i=line-nOffset+1;i<limitLine;i++){
          if(actual && (i-nOffset) >= startLine && (i-nOffset) < limitLine){
            eraseLine(i);
          }
        }
      }
    }
} programStack, cutStack;
class HashLookup {
/* 256 entries for hash lookup, 64 for overflow */
#define HASHSIZE 256
#define HASHOVERFLOW 64
#define HASHHELPSIZE 80
  HashEntry table[HASHSIZE+HASHOVERFLOW];
  HelpEntry helpLookup[HASHHELPSIZE];
  int overflowSize;
//  int32_t definedWordIndex;

  public:
    void init(){
      initDefined();
      for(int i=0;i<HASHSIZE+HASHOVERFLOW;i++){
        if(table[i].fptr != NULL){
          table[i].fptr=NULL;
          table[i].original=0;
        }
      }
      for(int i=0;i<HASHHELPSIZE;i++){
        helpLookup[i].cmd=0;
        helpLookup[i].msg=NULL;
      }
      overflowSize=HASHOVERFLOW/2;
    }
    void initDefined(){
      // I'll probably still do this, but...
      // I can zero out the hash
//      definedWordIndex=0;
      for(int i=0;i<HASHSIZE;i++){
        if(table[i].defined != 0){
          table[i].defined=0;
          table[i].original=0;
        }
      }      
      // But have to remove from overflow without leaving gaps.
      for(int i=HASHSIZE;i<HASHSIZE+HASHOVERFLOW;i++){
        if(table[i].defined != 0){
          for(int j=i+1;j<HASHSIZE+HASHOVERFLOW;j++){
            table[j-1]=table[j];
          }
          i=i-1;
        }        
      }
    }
#if KEEPHASH
    uint32_t computeHash(uint32_t x){
      uint32_t y=x^(x>>8);
      return 0xFF&(y^(y>>16));
    }
#endif
    const char *getHelpMessage(GFU fname){
      char xyz[13];
      fromBase40(xyz,fname);
      for(int i=0;i<HASHHELPSIZE;i++){
        if(fname == helpLookup[i].cmd){
          return helpLookup[i].msg;
        }
      }
      return NULL;
    }
    void setOverflowSize(){
      overflowSize=HASHOVERFLOW/2;
      while(overflowSize > 1){
        if(table[overflowSize].original){
          return;
        }
        overflowSize>>=1;
      }
    }
    void addEntry(void (*fptr)(), GFU fname, const char *msg){
      // Define Help message
      for(int i=0;i<HASHHELPSIZE;i++){
        if(helpLookup[i].cmd == fname || helpLookup[i].cmd == 0){
          helpLookup[i].cmd=fname;
          helpLookup[i].msg=msg;
          break;
        }
      }
#if KEEPHASH
      // Hash portion
      int i=(int)computeHash(fname);
      if(table[i].original == 0){
        table[i].original=fname;
        table[i].defined=0;
        table[i].fptr=fptr;
      }
      else {
#endif
        // Btree portion
        // Put these in sorted order, largest to smallest (0)
        for(int i=HASHSIZE;i<HASHSIZE+HASHOVERFLOW;i++){
          if(fname > table[i].original){
            for(int j=HASHSIZE+HASHOVERFLOW-1;j>i;j--){
              table[j]=table[j-1];
            }
            table[i].original=fname;
            table[i].fptr=fptr;
            table[i].defined=0;
            setOverflowSize();
            return;
          }
        }
#if KEEPHASH
      }
#endif
    }
    void addDefinedEntry(int selected, GFU fname){
#if KEEPHASH
      // Hash portion, same as above
      int i=(int)computeHash(fname);
      // Allow overwriting
      if(table[i].original == 0 || table[i].original == fname){
        table[i].original=fname;
        table[i].fptr=NULL;
        table[i].defined=selected;
      }
      else {
#endif
        // Btree portion
        // Put these in sorted order, largest to smallest (0)
        // This will allow overwriting native words
        for(int i=HASHSIZE;i<HASHSIZE+HASHOVERFLOW;i++){
          if(fname > table[i].original){
            for(int j=HASHSIZE+HASHOVERFLOW-1;j>i;j--){
              table[j].original=table[j-1].original;
              table[j].fptr=table[j-1].fptr;
              table[j].defined=table[j-1].defined;
            }
          }
          if(fname >= table[i].original){
            table[i].original=fname;
            table[i].fptr=NULL;          
            table[i].defined=selected;
            setOverflowSize();
            return;
          }
        }
#if KEEPHASH
      }
#endif
    }
    GFU getNextEntry(GFU e){
      // In terms of HASHSIZE and HASHOVERFLOW
      if(e == 0){
        for(int i=0;i<HASHSIZE+HASHOVERFLOW;i++){
          if(table[i].original != 0){
            return table[i].original;
          }
        }
      }
      else {
        for(int i=0;i<HASHSIZE+HASHOVERFLOW;i++){
          if(table[i].original == e){
            for(int j=i+1;j<HASHSIZE+HASHOVERFLOW;j++){
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
#if KEEPHASH
      // Hash portion
      GFU i=computeHash(fname);
      if(table[i].original == fname){
        if(table[i].fptr != NULL){
          (*table[i].fptr)();
        }
        else {
          // Push return address
          Word w;
          w.i=programStack.getSelected();
          returnStack.push(w);
          // Set next address
          programStack.setSelected(table[i].defined);
        }
        return TRUE;
      }
      else {
#endif
// Want to replace this with a binary search - which means they have to be in order
        int i=HASHSIZE;
#if DEBUG
        Serial.print("Seeking ");
        Serial.println(fname);
#endif
        for(int bt=overflowSize;bt;bt>>=1){
          i^=bt;
#if DEBUG
          Serial.print(i);
          Serial.print(": ");
          Serial.println(table[i].original);
#endif
          if(table[i].original == fname){
            if(table[i].fptr != NULL){
              (*table[i].fptr)();
            }
            else {
              // Push return address
              Word w;
              w.i=programStack.getSelected();
              returnStack.push(w);
              // Set next address
              programStack.setSelected(table[i].defined);
            }
            return TRUE;
          }
          else if(table[i].original < fname){
            i^=bt;
          }
        }
#if KEEPHASH
      }
#endif
      return FALSE;
    }
#if DEBUG
    void dump(){
#if KEEPHASH
      for(int i=0;i<HASHSIZE;i++){
        if(table[i].original){
          char xyz[7];
          fromBase40(xyz,table[i].original);
          Serial.print(xyz);
          Serial.print(" ");
          Serial.print((table[i].fptr != NULL));
          Serial.print(" ");
          Serial.println(table[i].defined);
        }
      }
      Serial.println("-----------------------------------");
#endif
      for(int i=0;i<HASHOVERFLOW;i++){
        if(table[HASHSIZE+i].original){
          Serial.print(HASHSIZE+i);
          Serial.print(": ");
          Serial.println(table[HASHSIZE+i].original);
        }
      }
    }
#endif
} CmdLookup;
class NameStack {
#define NameStackWIDTH COLUMNS
// Issue 0.1.0.3 - Increased size of Menu from 64 to 256
#define NameStackWORDS 256
  Word Menu[NameStackWORDS];
  int nMenu,nMenuSelected,nMenuOffset;
  private:
    void makeGapAt(int n){
// Issue 0.1.0.3 - Added bounds checking
      if(nMenu < NameStackWORDS){
        for(int j=nMenu;j>n;j--){
          Menu[j]=Menu[j-1];
        }
        nMenu++;
      }
    }
  public:
    NameStack(){
      nMenuSelected=nMenu=nMenuOffset=0;
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
    uint32_t sortValue(uint32_t value){
      while(value < 102400000){
        value *= 40;
      }
      return value;
    }
    void push(uint32_t value){
      uint32_t sv=sortValue(value);
      char xyz[7];
      fromBase40(xyz,value);
      if(nMenu == 0){
        Menu[nMenu++].u=value;
      }
      else if(sv > sortValue(Menu[nMenu-1].u)){
        Menu[nMenu++].u=value;
      }
      else if(sv < sortValue(Menu[0].u)){
        makeGapAt(0);
        Menu[0].u=value;
      }
      else {
        for(int i=0;i<nMenu-1;i++){
          if(sv > sortValue(Menu[i].u) && sv < sortValue(Menu[i+1].u)){
            makeGapAt(i+1);
            Menu[i+1].u=value;
            return;
          }
        }
      }
    }
    void show(int InputState){
      int16_t color;
      char xyz[7];
      if(InputState == INPUTSTATEMENU){
        color=NORMALCOLOR;
      }
      else {
        return;
      }
      // First of all, build the menu from scratch (including user-defined words)
      nMenu=0;
      GFU cmd=0;
      while((cmd=CmdLookup.getNextEntry(cmd)) != 0){
        // Only show valid Base40 strings (not CMDPUSH or CMDFPUSH)
        if(cmd < 4096000000){
          push(cmd);
        }
      }
#if 0
      for(int j=0;j<definedWordIndex;j++){
        push(definedWord[j].word);
      }
#endif
      nMenuSelected%=nMenu;
      // Done
      // First pass - set nMenuOffset
      int col,row;
      col=row=0;
      for(int j=0;j<nMenu;j++){
        int size=lengthBase40(Menu[j].u);
        if(col+size > COLUMNS){
          row++;
          col=0;
        }

        if(nMenuSelected == j){
          if(0 > row-nMenuOffset){
            nMenuOffset=row;
          }
          else if(row-nMenuOffset >= MENUROWS){
            nMenuOffset=row-MENUROWS+1;
          }
          break;
        }
        col+=size+1;
      }
      // Second pass - show words
      col=0;
      row=0-nMenuOffset;

      for(int j=0;j<nMenu;j++){
        int screenRow=row+ROWS-MENUROWS;
        int size=lengthBase40(Menu[j].u);
        // Clear the rest of the line
        // If the next one won't fit on this line
        if(col+size > COLUMNS){
          if(0 <= row && row < MENUROWS){
            eraseSpace(screenRow,col,COLUMNS-col);
          }
          row++;
          screenRow=row+ROWS-MENUROWS;
          col=0;
        }
        // If this is the last one
        if((j+1) == nMenu){
          if(0 <= row && row < MENUROWS){
            eraseSpace(screenRow,col,COLUMNS-col);
          }
        }
        // Write the word
        if(0 <= row && row < MENUROWS){
          if(nMenuSelected == j){
            GO.lcd.setTextColor(NORMALBGCOLOR,color);
          }
          else {
            GO.lcd.setTextColor(color,NORMALBGCOLOR);
          }
          GO.lcd.setCharCursor(col, screenRow);
          char bfr[7];
          fromBase40(bfr,Menu[j].u);
          GO.lcd.printf("%s",bfr);
          if(col+size+1 <= COLUMNS){
            eraseSpace(screenRow,col+size,1);
          }
        }
        col+=size+1;
      }

      return;
    }
} menuStack;
#if FLOATING
#else
int32_t accumulator,increment;
#endif
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
#if WAIT
void kpWait(uint32_t ln){
  dacWrite(SPEAKER_PIN, (uint8_t)0);
  delayMicroseconds(32*1024*ln);
}
#endif
void resetEvent(){
#if FLOATING
  accumulator.init();
#else
  increment=1;
  accumulator=0;
#endif
}
void initKeypress(){
  tk=pk=0;
  resetEvent();
  Base40Buffer.type.u=0;
  Base40Buffer.value.u=0;
}
#if FLOATING
#else
void showAccumulator(int InputState){
#define ATCOLUMN 7
  int size=11;
  int16_t color;
  int accu=accumulator,incr=increment;
  if(InputState == INPUTSTATENUMERIC){
    color=NORMALCOLOR;
  }
  else {
    return;
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
#endif
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
void cmdFToInt(){
  Word a,b;
  a=dataStack.pop();
  b.i=(GFI)a.f;
  dataStack.push(b);
}
void cmdIntToF(){
  Word a,b;
  a=dataStack.pop();
  b.f=(GFF)a.i;
  dataStack.push(b);
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
void cmdFPlus(){
  Word a,b,c;
  GFF x,y,z;
  a=dataStack.pop();
  x=a.f;
  b=dataStack.pop();
  y=b.f;
  z=x+y;
  c.f=z;
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
void cmdFMinus(){
  Word a,b,c;
  GFF x,y,z;
  a=dataStack.pop();
  x=a.f;
  b=dataStack.pop();
  y=b.f;
  z=y-x;
  c.f=z;
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
void cmdFTimes(){
  Word a,b,c;
  GFF x,y,z;
  
  a=dataStack.pop();
  x=a.f;
  b=dataStack.pop();
  y=b.f;
  z=y*x;
  c.f=z;
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
void cmdFDivide(){
  Word a,b,c;
  GFF x,y,z;
  
  a=dataStack.pop();
  x=a.f;
  b=dataStack.pop();
  y=b.f;
  z=y/x;
  c.f=z;
  dataStack.push(c);
}
void cmdPower(){
  Word a,b,c;
  GFI i,j,k;
  
  a=dataStack.pop();
  i=a.u;
  b=dataStack.pop();
  j=b.i;
  k=1;
  for(int n=0;n<i;n++){
    k*=j;
  }
  c.i=k;
  dataStack.push(c);
}
void cmdShiftLeft(){
  Word a,b,c;
  GFI i,j,k;
  
  a=dataStack.pop();
  i=a.u;
  b=dataStack.pop();
  j=b.i;
  k=j<<i;
  c.i=k;
  dataStack.push(c);
}
void cmdShiftRight(){
  Word a,b,c;
  GFI i,j,k;
  
  a=dataStack.pop();
  i=a.u;
  b=dataStack.pop();
  j=b.i;
  k=j>>i;
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
void cmdLt(){ 
  Word a,b,c;
  a=dataStack.pop();
  b=dataStack.pop();
  c.u=(b.i < a.i)?TRUE:FALSE;
  dataStack.push(c);
}
void cmdEqual(){
  Word a,b,c;
  a=dataStack.pop();
  b=dataStack.pop();
  c.i=(b.i == a.i)?TRUE:FALSE;
  dataStack.push(c);
}
void cmdGt(){
  Word a,b,c;
  a=dataStack.pop();
  b=dataStack.pop();
  c.u=(b.i > a.i)?TRUE:FALSE;
  dataStack.push(c);
}
void cmdLtEqual(){ 
  Word a,b,c;
  a=dataStack.pop();
  b=dataStack.pop();
  c.u=(b.i <= a.i)?TRUE:FALSE;
  dataStack.push(c);
}
void cmdNotEqual(){
  Word a,b,c;
  a=dataStack.pop();
  b=dataStack.pop();
  c.i=(b.i != a.i)?TRUE:FALSE;
  dataStack.push(c);
}
void cmdGtEqual(){
  Word a,b,c;
  a=dataStack.pop();
  b=dataStack.pop();
  c.u=(b.i >= a.i)?TRUE:FALSE;
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
  GO.lcd.fillRect(x.u,y.u,w.u,h.u,c.u);
}
void cmdCircle(){
  Word x,y,r,c;
  c=dataStack.pop();
  r=dataStack.pop();
  y=dataStack.pop();
  x=dataStack.pop();
  GO.lcd.fillCircle(x.u,y.u,r.u,c.u);
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
  a=dataStack.pop();
  GO.lcd.printf("%d",a.i);
  GO.update();
}
void cmdFEmit(){
  Word a;
  a=dataStack.pop();
  GO.lcd.printf("%10.5e",a.f);
  GO.update();
}
#if WAIT
void cmdWait(){
  Word a;
  a=dataStack.pop();
  kpWait(a.u);
}
#endif
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
#if 0
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
#endif
#if 0
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
#else
void cmdIf(){
  int level=0;
  Word a;
  
  
  a=dataStack.pop();
  if(a.i == 0){
    while(-1){
      programStack.selectNext();
      TypeWord tw=programStack.copy();
      if(level == 0){
        if(tw.type.u == toBase40("THEN") || tw.type.u == toBase40("ELSE")){
          return;
        }
      }
      if(tw.type.u == toBase40("IF")){
        level++;
      }
      else if(tw.type.u == toBase40("THEN")){
        level--;
      }
    }
  }
}
#endif
void cmdDo(){
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
void cmdRandom(){
  Word a,b;
  
  a=dataStack.pop();
  b.i=random(a.i);
  dataStack.push(b);
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
void cmdUp(){
  Word a;
  a.i=KPUP;
  dataStack.push(a);  
}
void cmdDown(){
  Word a;
  a.i=KPDOWN;
  dataStack.push(a);  
}
void cmdLeft(){
  Word a;
  a.i=KPLEFT;
  dataStack.push(a);  
}
void cmdRight(){
  Word a;
  a.i=KPRIGHT;
  dataStack.push(a);  
}
void cmdA(){
  Word a;
  a.i=KPA;
  dataStack.push(a);  
}
void cmdB(){
  Word a;
  a.i=KPB;
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
  if(programStack.get(programStack.getSelected()).type.i == CMDPUSH){
    globalError(ERRORQUOTENUMBER);
  }
  else if(programStack.get(programStack.getSelected()).type.i == CMDFPUSH){
    globalError(ERRORQUOTENUMBER);
  }
  else {
    dataStack.push(programStack.copy().type);    
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
#if DEBUG
void cmdDump(){
  CmdLookup.dump();
}
#endif
void commandInit(){
  CmdLookup.addEntry(&cmdPush, CMDPUSH, "(- value)");
  CmdLookup.addEntry(&cmdPush, CMDFPUSH, "(- value)");
  CmdLookup.addEntry(&cmdBreakOut, toBase40(":"), "(-)");
  CmdLookup.addEntry(&cmdPlus, toBase40("+"),     "(int int - sum)");
  CmdLookup.addEntry(&cmdFPlus, toBase40(".+"),     "(float float - sum)");
  CmdLookup.addEntry(&cmdMinus, toBase40("-"),    "(int int - difference)");
  CmdLookup.addEntry(&cmdFMinus, toBase40(".-"),    "(float float - difference)");
  CmdLookup.addEntry(&cmdTimes, toBase40("*"),    "(int int - product)");
  CmdLookup.addEntry(&cmdFTimes, toBase40(".*"),    "(float float - product)");
  CmdLookup.addEntry(&cmdDivide, toBase40("/"),   "(dividend divisor - quotient)");
  CmdLookup.addEntry(&cmdFDivide, toBase40("./"),   "(float float - quotient)");
  CmdLookup.addEntry(&cmdFToInt, toBase40(".TOINT"),   "(float - int)");
  CmdLookup.addEntry(&cmdIntToF, toBase40("INTTO."),   "(int - float)");
  CmdLookup.addEntry(&cmdPower, toBase40("**"),   "(base exponent - result)");
  CmdLookup.addEntry(&cmdShiftLeft, toBase40("<<"),   "(number bits - result)");
  CmdLookup.addEntry(&cmdShiftRight, toBase40(">>"),   "(number bits - result)");
  CmdLookup.addEntry(&cmdAnd, toBase40("&"),      "(r1 r2 - and)");
  CmdLookup.addEntry(&cmdOr, toBase40("|"),       "(r1 r2 - or)");
  CmdLookup.addEntry(&cmdXor, toBase40("^"),    "(r1 r2 - xor)");
  CmdLookup.addEntry(&cmdLt, toBase40("<"),       "(r1 r2 - less?)");
  CmdLookup.addEntry(&cmdEqual, toBase40("="),    "(r1 r2 - equal?)");
  CmdLookup.addEntry(&cmdGt, toBase40(">"),       "(r1 r2 - greater)");
  CmdLookup.addEntry(&cmdLtEqual, toBase40("<="),       "(r1 r2 - lessOrEqual?)");
  CmdLookup.addEntry(&cmdNotEqual, toBase40("<>"),    "(r1 r2 - notEqual?)");
  CmdLookup.addEntry(&cmdGtEqual, toBase40(">="),       "(r1 r2 - greaterOrEqual)");
  CmdLookup.addEntry(&cmdDac, toBase40("DAC"),    "(r1 -)");
  CmdLookup.addEntry(&cmdGet8, toBase40("GETBYT"),"(r1 - value)");
  CmdLookup.addEntry(&cmdPut8, toBase40("PUTBYT"),"(r1 r2 -)");
  CmdLookup.addEntry(&cmdGet32, toBase40("GET"),  "(r1 - value)");
  CmdLookup.addEntry(&cmdPut32, toBase40("PUT"),  "(r1 r2 -)");
  CmdLookup.addEntry(&cmdModulo, toBase40("MOD"), "(dividend divisor - rem)");
  CmdLookup.addEntry(&cmdNote, toBase40("NOTE"),  "(r1 r2 -)");
#if WAIT
  // Liable to get deleted
  CmdLookup.addEntry(&cmdWait, toBase40("WAIT"),  "(r1 -)");
#endif
  CmdLookup.addEntry(&cmdDelay, toBase40("DELAY"),"(r1 -)");
  CmdLookup.addEntry(&cmdUdelay, toBase40("UDELAY"), "(r1 -)");
  CmdLookup.addEntry(&cmdSlower, toBase40("SLOWER"), "(r1 -)");
  CmdLookup.addEntry(&cmdFalse, toBase40("*FALSE"), "(- value)");
  CmdLookup.addEntry(&cmdQuote, toBase40("QUOTE"),"(- value)");
  CmdLookup.addEntry(&cmdTrue, toBase40("*TRUE"), "(- value)");
  CmdLookup.addEntry(&cmdRed, toBase40("*RED"),   "(- value)");
  CmdLookup.addEntry(&cmdGreen,toBase40("*GREEN"),"(- value)");
  CmdLookup.addEntry(&cmdBlue, toBase40("*BLUE"), "(- value)");

  CmdLookup.addEntry(&cmdUp, toBase40("*UP"), "(- value)");
  CmdLookup.addEntry(&cmdDown, toBase40("*DOWN"), "(- value)");
  CmdLookup.addEntry(&cmdLeft, toBase40("*LEFT"), "(- value)");
  CmdLookup.addEntry(&cmdRight, toBase40("*RIGHT"), "(- value)");
  CmdLookup.addEntry(&cmdA, toBase40("*A"), "(- value)");
  CmdLookup.addEntry(&cmdB, toBase40("*B"), "(- value)");
  
  CmdLookup.addEntry(&cmdTime, toBase40("TIME"),  "(- value)");
  CmdLookup.addEntry(&cmdDup, toBase40("DUP"),    "(r1 - r1 r1)");
  CmdLookup.addEntry(&cmdOver, toBase40("OVER"),  "(r1 r2 - r1 r2 r1)");
  CmdLookup.addEntry(&cmdRot, toBase40("ROT"),    "(r1 r2 r3 - r2 r3 r1)");
  CmdLookup.addEntry(&cmdSwap, toBase40("SWAP"),  "(r1 r2 - r2 r1)");
  CmdLookup.addEntry(&cmdSemicolon, toBase40(";"), "(-)");
  CmdLookup.addEntry(&cmdPread, toBase40("PREAD"), "(name -)");
  CmdLookup.addEntry(&cmdPlist, toBase40("PLIST"), "(-)");
  CmdLookup.addEntry(&cmdPsave, toBase40("PSAVE"), "(name -)");
  CmdLookup.addEntry(&cmdIf, toBase40("IF"), "(r1 -)");
  CmdLookup.addEntry(&cmdElse, toBase40("ELSE"), "(-)");
  CmdLookup.addEntry(&cmdNoop, toBase40("THEN"), "(-)");
  CmdLookup.addEntry(&cmdDo, toBase40("DO"),      "(limit initial -)");
  CmdLookup.addEntry(&cmdDrop, toBase40("DROP"),  "(r1 -)");
  CmdLookup.addEntry(&cmdLoop, toBase40("LOOP"),  "(-)");
  CmdLookup.addEntry(&cmdBegin, toBase40("BEGIN"),"(-)");
  CmdLookup.addEntry(&cmdAgain, toBase40("AGAIN"),"(-)");
  CmdLookup.addEntry(&cmdUntil, toBase40("UNTIL"),"(r1 -)");
#if 0
  // Liable to be replaced with I2C commands
  CmdLookup.addEntry(&cmdPinMode, toBase40("PINMOD"), "(pin mode -)");
  // Liable to be replaced with I2C commands
  CmdLookup.addEntry(&cmdPinWrite, toBase40("PINWRI"), "(pin value -)");
#endif
  CmdLookup.addEntry(&cmdLoopVariable, toBase40("I"), "(- value)");
  CmdLookup.addEntry(&cmdRandom, toBase40("RANDOM"), "(- value)");
  CmdLookup.addEntry(&cmdRctngl, toBase40("RCTNGL"), "(x y wt ht color -)");
  CmdLookup.addEntry(&cmdCircle, toBase40("CIRCLE"), "(x y radius color -)");
  CmdLookup.addEntry(&cmdCup, toBase40("CUP"),       "(row column -)");
  CmdLookup.addEntry(&cmdEmit, toBase40("EMIT"), "(Base40 -)");
  CmdLookup.addEntry(&cmdButton, toBase40("BUTTON"), "(- value)");
  CmdLookup.addEntry(&cmdEmitn, toBase40("."), "(int -)");
  CmdLookup.addEntry(&cmdFEmit, toBase40(".."), "(float -)");
#if DEBUG
  CmdLookup.addEntry(&cmdDump, toBase40("DUMP"), "(-)");
#endif
}
int programScan(){
  int level=0,lastQuote=false;
 
//  definedWordIndex=0;
  CmdLookup.initDefined();
  programStack.selectFirst();
  while(level >= 0 && programStack.getSelected() != -1){
    TypeWord tw=programStack.copy();
    if(tw.type.u == toBase40(":")){
      lastQuote=false;
      level++;
      programStack.selectNext();
      tw=programStack.copy();
      CmdLookup.addDefinedEntry(programStack.getSelected(), tw.type.u);
    }
    else if(tw.type.u == toBase40(";")){
      lastQuote=false;
      level--;
    }
    else if(tw.type.u == toBase40("QUOTE")){
      lastQuote=true;
    }
    else if(lastQuote == true && (tw.type.i == CMDPUSH || tw.type.i == CMDFPUSH)){
      globalError(ERRORQUOTENUMBER);
      return true;
    }
    else {
      lastQuote=false;
    }
    programStack.selectNext();
  }
  return false;
}
void programExecute(int singleStep){
  TypeWord tw;
  int remember=programStack.getSelected();
  BreakOut=FALSE;
  
  globalError(ERRORNONE);
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
    // This implements a program break by switching to single step mode if both B and A are pressed
    if(slower){
      GO.update();
      if( GO.BtnA.isPressed() && GO.BtnB.isPressed() ){
        singleStep=true;
      }
    }
    if(CmdLookup.callEntry(tw.type.u)){
      /* NOOP - already called function */
    }
#if 0
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
#endif
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
void showMessage(){
  if(globalmessage != NULL){
    int line=1;
    eraseSpace(line,0,COLUMNS);
    GO.lcd.setCharCursor(0, line);
    GO.lcd.printf("%s",globalmessage);
    globalmessage=NULL;
  }
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
  int limitLine;
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
  dataStack.show();
  showMessage();
  programStack.show(InputState,limitLine);
  showAlphaNumeric(InputState);
  GO.update();
#if FLOATING
  accumulator.show(InputState);
#else
  showAccumulator(InputState);
#endif
  GO.update();
  menuStack.show(InputState);
  GO.update();
}

/* Switching to expect one keypress at a time */
void processEvent(uint16_t pkk, uint16_t tkk){
  int i;
#if FLOATING
  accumulator.minimums();
#else
  if(increment.i < 1){
    increment.i=1;
  }
#endif
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
        break;
    }
  }
  else if(InputState == INPUTSTATENUMERIC){
      switch(tkk){
        case KPLEFT:
#if FLOATING
          accumulator.shiftLeft();
#else 
          increment.i=(increment.i >= 1000000000)?increment.i:increment.i*10;
#endif
          break;
        case KPRIGHT:
#if FLOATING
          accumulator.shiftRight();
#else
          increment.i=(increment.i == 1)?1:increment.i/10;
#endif
          break;
        case KPUP:
#if FLOATING
          accumulator.incr();
#else
          accumulator.i+=increment.i;
#endif
          break;
        case KPDOWN:
#if FLOATING
          accumulator.decr();
#else
          accumulator.i-=increment.i;
#endif
          break;
        case KPB:
          TypeWord tw;
          
#if FLOATING
          if(accumulator.isFloating()){
            tw.type.i=CMDFPUSH;
          }
          else {
            tw.type.i=CMDPUSH;
          }
          tw.value=accumulator.get();
#else
          tw.type.i=CMDPUSH;
          tw.value.i=accumulator.i;
#endif
          programStack.paste(tw);
          break;
        case KPB|KPA:
          accumulator.init();
          break;
      }
  }
  else if(InputState == INPUTSTATECLEAR){
      switch(tkk){
        case KPB:
          programExecute(FALSE);
          break;
        case KPB|KPA:
          programExecute(TRUE);
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
          programStack.previousLine();
          break;
        case KPDOWN:
          programStack.nextLine();
          break;
      }
  }
  if(pkk < tkk){
    showState(InputState,PreviousInputState);
  }
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
  globalError(ERRORNONE);
  
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(25, OUTPUT);
  digitalWrite(25, LOW);
  delay(100);
  GO.begin();
  dacWrite(SPEAKER_PIN, (uint8_t)0);
  GO.lcd.setTextSize(2);
  GO.lcd.setTextColor(NORMALCOLOR);
  // Show Version
  GO.lcd.setCharCursor(2, 3);
  GO.lcd.printf("GO Forth, Version %s",VERSION);
  GO.lcd.setCharCursor(2, 4);
  GO.lcd.printf("written by Jay Anderson");
  GO.update();
  delay(2500);
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

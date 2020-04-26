#include <odroid_go.h>
#include <WiFi.h>

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
//
// Implement variables, arrays
// Weatherboard, I2C
// Battery level
// Webpage from current program
// Maybe some cleanup/speedup - goal is to be able to do KPS, Analog Computer from FORTH
//  Currently about 4 seconds to calculate factorials 1 through 16
//  Now taking 83 milliseconds
//  Left,Right arrow in IDE is slow (rewriting screen) which may be selectNext() again
//  Screen is rewriting on down and up button push.
// Port to ESP8266/FeatherWing
// Scrollable data, program, variable area
// Output/Input
// Help?
// programExecute optimization
// - Faster lookup of functions (place in sorted array, find by binary (or hash) search)
// - Possibly make next step a macro
#define DEBUGEXECUTE 0
//
// Bugs:
// Programs:
//  Sieve (need to get into stack or other array), Factor (done)
//
uint16_t tk, pk;

#define TRUE -1
#define FALSE 0

#define SELECTEDCOLOR 0x07FE
#define NORMALCOLOR WHITE
#define NORMALBGCOLOR BLACK

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
int InputState=INPUTSTATEMENU;

typedef struct {
  uint32_t word,index;
} DefinedWord;
int32_t definedWordIndex=0;
DefinedWord definedWord[128];

#define BASE 40
char Base40[]=" ABCDEFGHIJKLMNOPQRSTUVWXYZ:;+-*/&|^.<=>\e";
char Base10[]="0123456789-";

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

class NameStack {
#define NameStackWIDTH (COLUMNS-ANCOLS-1)
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
    void next(){
      nMenuSelected=(nMenuSelected+1)%nMenu;
    }
    void nextLine(){
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
    void show(){
      int16_t color;
      if(InputState == INPUTSTATEMENU){
        color=MAGENTA;
      }
      else {
        color=NORMALCOLOR;
      }
      nMenu=0;
      push(toBase40(":"));
      push(toBase40("+"));
      push(toBase40("-"));
      push(toBase40("*"));
      push(toBase40("/"));
      push(toBase40("&"));
      push(toBase40("|"));
      push(toBase40("^"));
      push(toBase40("."));
      push(toBase40("<"));
      push(toBase40("="));
      push(toBase40(">"));
      push(toBase40(";"));
      push(toBase40("AGAIN"));
      push(toBase40("BEGIN"));
      push(toBase40("DELAY"));
      push(toBase40("DO"));
      push(toBase40("DROP"));
      push(toBase40("DUP"));
      push(toBase40("ELSE"));
      push(toBase40("FALSE"));
      push(toBase40("I"));
      push(toBase40("IF"));
      push(toBase40("LOOP"));
      push(toBase40("MOD"));
      push(toBase40("NOTE"));
      push(toBase40("OVER"));
      push(toBase40("PINMOD"));
      push(toBase40("PINWRI"));
      push(toBase40("PLIST"));
      push(toBase40("PREAD"));
      push(toBase40("PSAVE"));
      push(toBase40("QUOTE"));
      push(toBase40("STACK"));
      push(toBase40("ROT"));
      push(toBase40("SIZE"));
      push(toBase40("SWAP"));
      push(toBase40("THEN"));
      push(toBase40("TIME"));
      push(toBase40("TRUE"));
      push(toBase40("UNTIL"));
      push(toBase40("WAIT"));
      for(int j=0;j<definedWordIndex;j++){
        push(definedWord[j].word);
      }
      nMenuSelected%=nMenu;
      int notShown=1;
      while(notShown){
        RowCol next={BASE40ROW+1-nMenuOffset,ANCOLS+1};
        for(int j=0;j<nMenu;j++){
          int size=lengthBase40(Menu[j].u);
          if(next.col+size > COLUMNS){
            next.row++;
            next.col=ANCOLS+1;
          }
          if(next.row >= BASE40ROW+1 && next.row < 15){
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
              if(next.row >= 15){
                nMenuOffset+=next.row-14;
              }
              else if(next.row < BASE40ROW+1){
                nMenuOffset--;
              }
              for(int i=BASE40ROW+1;i<15;i++){
                eraseSpace(i,ANCOLS+1,COLUMNS-ANCOLS);
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
    void push(Word w){value[nws++]=w;};
    Word pop(){return value[--nws];};
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
} dataStack, returnStack;

class TypeWordStack {
  DataStack ds;
  int selected, ntw, lastLine;
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
      selected=-1;
      lastLine=ntw=0;
    }
    void dump(){
      Serial.println("programStack.dump");
      ds.dump();
    }
    void paste(TypeWord tw){
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
    TypeWord cut(){
      int n;
      if(selected < 0){
        n=twfind(ntw-1);
      }
      else {
        n=twfind(selected);
      }
      TypeWord tw;
      tw.value.i=0;
      tw.type = ds.cut(n);
      if(tw.type.i == CMDPUSH){
        tw.value = ds.cut(n);
      }
      ntw--;
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
            break;
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
            Serial.println(&ascii[1]);
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
#if 1
        ascii[0]='/';
        fromBase40(&ascii[1], a.u);
        strcat(ascii,".4TH");
        File readFile=SD.open(ascii, FILE_READ);
#else
        File readFile=SD.open("/TEMP.4TH", FILE_READ);
#endif
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
#if 1
        ascii[0]='/';
        fromBase40(&ascii[1], a.u);
        strcat(ascii,".4TH");
        File writeFile=SD.open(ascii, FILE_WRITE);
#else
        File writeFile=SD.open("/TEMP.4TH", FILE_WRITE);
#endif
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
    void show(int line, int emphasize){
      int column=0,size;
      uint16_t color=emphasize?MAGENTA:NORMALCOLOR;

      GO.lcd.setTextColor(color,NORMALBGCOLOR);
      erase(line);
      for(int i=0;i<ntw;i++){
        if(i == selected){
          GO.lcd.setTextColor(NORMALBGCOLOR,color);
        }
        else {
          GO.lcd.setTextColor(color,NORMALBGCOLOR);
        }
        TypeWord tw=get(i);
        if(tw.type.i == CMDPUSH){
          size=lenInt(tw.value.i);
          if(column + size > COLUMNS){
            column=0;
            line++;
            erase(line);
          }
          GO.lcd.setCharCursor(column, line);
          column += size+1;
          GO.lcd.printf("%d",tw.value.i);
        }
        else {
          // Should be an internal command without a second longword unlike CMDPUSH
          size=lenBase40(tw.type.u);
          // Copied from above - try to make common
          if(column + size > COLUMNS){
            column=0;
            line++;
            erase(line);
          }
          for(int i=0;i<size;i++){
            GO.lcd.setCharCursor(column+size-i-1, line);
            uint32_t inx=tw.type.u%40;
            GO.lcd.printf("%c",Base40[inx]);
            tw.type.u/=40;
          }
          // Add one for space delimiter
          column+=size+1;
        }
      }
      if(lastLine > line){
        for(int i=lastLine;i>line;i--){
          erase(i);
        }
      }
      lastLine=line;
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

void eraseLine(int line){
  GO.lcd.fillRect(0,line*16,320,16,0x0000);
}
void eraseSpace(int line,int start, int width){
  GO.lcd.fillRect(start*12,line*16,width*12,16,0x0000);
  GO.update();
}
void showAccumulator(){
#define ATCOLUMN 15
  int size=11;
  int16_t color;
  int accu=accumulator,incr=increment;
  if(InputState == INPUTSTATENUMERIC){
    color=MAGENTA;
  }
  else {
    color=NORMALCOLOR;
  }
  eraseSpace(BASE40ROW,ATCOLUMN,size);
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
    GO.lcd.setCharCursor(ATCOLUMN-1+size-i,BASE40ROW);
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
void cmdSize(){
  Word a;
  a.i=dataStack.size();
  dataStack.push(a);  
}
void cmdNote(){
  Word a,b;
  a=dataStack.pop();
  b=dataStack.pop();
  kpNote(a.u);
  kpPlay(b.u);
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
  Word a,b;
  a=dataStack.pop();
  b=dataStack.pop();
  returnStack.push(b);
  returnStack.push(a);
}
void cmdDrop(){
  Word a;
  a=dataStack.pop();
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
void cmdTime(){
  Word a;
  a.i=millis();
  dataStack.push(a);
}
void cmdQuote(){
  programStack.selectNext();
  dataStack.push(programStack.copy().type);
  if(dataStack.peek().i == CMDPUSH){
    Serial.println("ERROR: Trying to quote a push");
  }
}
void cmdStack(){
  Word a=dataStack.pop();
  if(a.u < dataStack.size()){
    dataStack.push(dataStack.copy(a.u));
  }
  else {
    Serial.println("ERROR: STACK trying to copy from beyond the stack");    
  }
}
void cmdAgain(){
  programStack.selectPrevious();
  TypeWord tw=programStack.copy();
  while(tw.type.u != toBase40("BEGIN")){
    programStack.selectPrevious();
    tw=programStack.copy();
  }  
}
void cmdUntil(){
  Word a=dataStack.pop();
  if(a.i == FALSE){
    cmdAgain();
  }
}
void cmdLoop(){
  Word a,b;
  a=returnStack.pop();
  b=returnStack.pop();
  a.i++;
  if(a.i < b.i){
    returnStack.push(b);
    returnStack.push(a);
    programStack.selectPrevious();
    TypeWord tw=programStack.copy();
    while(tw.type.u != toBase40("DO")){
      programStack.selectPrevious();
      tw=programStack.copy();
    }
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
int programScan(){
  int level=0,lastQuote=false;
  definedWordIndex=0;
  programStack.selectFirst();
  while(level >= 0 && programStack.getSelected() != -1){
    TypeWord tw=programStack.copy();
#if DEBUGEXECUTE
    Serial.print("Scan@");
    Serial.print(programStack.getSelected());
    Serial.print(", ");
#endif
    if(tw.type.u == toBase40(":")){
      lastQuote=false;
      level++;
      programStack.selectNext();
#if DEBUGEXECUTE
      Serial.println(":");
      Serial.print("Scan entry@");
      Serial.println(programStack.getSelected());
#endif
      tw=programStack.copy();
      definedWord[definedWordIndex].word=tw.type.u;
      definedWord[definedWordIndex].index=programStack.getSelected();
      definedWordIndex++;
    }
    else if(tw.type.u == toBase40(";")){
#if DEBUGEXECUTE
      Serial.println(";");
#endif
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
#if DEBUGEXECUTE
      Serial.println("Nothing");
#endif
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
#define DEBUGEXECUTE 0
void programExecute(int singleStep){
  TypeWord tw;
  int remember=programStack.getSelected();
#if DEBUGEXECUTE
  Serial.println("programScan");
#endif
  if(programScan()){
    // Information less return at this point.
    // Eventually will add diagnostic message, in this case
    // number following quote
    return;
  }
#if DEBUGEXECUTE
  dumpDefined();
  Serial.println("programExecute");
#endif
  if(singleStep && remember != -1){
    programStack.setSelected(remember);
  }
  else {
    programStack.selectFirst();
  }
  while(programStack.getSelected() != -1){
    tw=programStack.copy();
#if DEBUGEXECUTE
    Serial.print("Executing @");
    Serial.println(programStack.getSelected());
    Serial.print("Next? ");
    Serial.print(tw.type.i);
    Serial.print(", ");
    Serial.println(tw.value.i);
#endif
    GO.update();
    if( GO.BtnA.isPressed() && GO.BtnB.isPressed() ){
      singleStep=true;
    }

    if(tw.type.i == CMDPUSH){
      dataStack.push(tw.value);
#if DEBUGEXECUTE
      Serial.print("pushed ");
      Serial.println(tw.value.i);
#endif
    }
    else if(tw.type.u == toBase40(":")){
      // Nothing to do.  Terminate programExecute if : is encountered.
      // Not necessarily error.  If we ran into :, must be end of main program.
#if DEBUGEXECUTE
      Serial.println("Stopping after finding :");
#endif
      break;
    }
    else if(tw.type.u == toBase40("+")){
      cmdPlus();
    }
    else if(tw.type.u == toBase40("-")){
      cmdMinus();
    }
    else if(tw.type.u == toBase40("*")){
      cmdTimes();
    }
    else if(tw.type.u == toBase40("/")){
      cmdDivide();
    }
    else if(tw.type.u == toBase40("&")){
      cmdAnd();
    }
    else if(tw.type.u == toBase40("|")){
      cmdOr();
    }
    else if(tw.type.u == toBase40("^")){
      cmdXor();
    }
    else if(tw.type.u == toBase40(".")){
      dataStack.pop();
    }
    else if(tw.type.u == toBase40("<")){
      cmdLt();
    }
    else if(tw.type.u == toBase40("=")){
      cmdEqual();
    }
    else if(tw.type.u == toBase40(">")){
      cmdGt();
    }
    else if(tw.type.u == toBase40("MOD")){
      cmdModulo();
    }
    else if(tw.type.u == toBase40("DUP")){
      Word a=dataStack.pop();
      dataStack.push(a);
      dataStack.push(a);
    }
    else if(tw.type.u == toBase40("OVER")){
      Word a=dataStack.pop();
      Word b=dataStack.pop();
      dataStack.push(b);
      dataStack.push(a);
      dataStack.push(b);
    }
    else if(tw.type.u == toBase40("ROT")){
      Word a=dataStack.pop();
      Word b=dataStack.pop();
      Word c=dataStack.pop();
      dataStack.push(b);
      dataStack.push(a);
      dataStack.push(c);
    }
    else if(tw.type.u == toBase40("SWAP")){
      Word a=dataStack.pop();
      Word b=dataStack.pop();
      dataStack.push(a);
      dataStack.push(b);
    }
    else if(tw.type.u == toBase40("IF")){
      cmdIf();
    }
    else if(tw.type.u == toBase40("ELSE")){
      cmdElse();
    }
    else if(tw.type.u == toBase40("DO")){
      cmdDo();
    }
    else if(tw.type.u == toBase40("DROP")){
      cmdDrop();
    }
    else if(tw.type.u == toBase40("LOOP")){
      cmdLoop();
    }
    // BEGIN is a noop
    else if(tw.type.u == toBase40("AGAIN")){
      cmdAgain();
    }
    else if(tw.type.u == toBase40("UNTIL")){
      cmdUntil();
    }
    else if(tw.type.u == toBase40("PINMOD")){
      cmdPinMode();
    }
    else if(tw.type.u == toBase40("PINWRI")){
      cmdPinWrite();
    }
    else if(tw.type.u == toBase40("I")){
      cmdLoopVariable();
    }
    else if(tw.type.u == toBase40(";")){
      // Restore previous value of selected
      if(returnStack.size() > 0){
        programStack.setSelected(returnStack.pop().i);
#if DEBUGEXECUTE
        Serial.print("Return to ");
        Serial.println(programStack.getSelected());
#endif
      }
      else {
        // ERROR: Ran into ; without returnStack entry
        Serial.println(" -1 !!!");
        break;
      }
    }
    else if(tw.type.u == toBase40("NOTE")){
#if DEBUGEXECUTE
      Serial.println("NOTE");
#endif
      cmdNote();
    }
    else if(tw.type.u == toBase40("WAIT")){
#if DEBUGEXECUTE
      Serial.println("WAIT");
#endif
      cmdWait();
    }
    else if(tw.type.u == toBase40("DELAY")){
#if DEBUGEXECUTE
      Serial.println("DELAY");
#endif
      cmdDelay();
    }
    else if(tw.type.u == toBase40("FALSE")){
      cmdFalse();
    }
    else if(tw.type.u == toBase40("PREAD")){
      programStack.ReadFile(dataStack.pop());
      /* We don't want to do anything other than read the file */
      break;
    }
    else if(tw.type.u == toBase40("PLIST")){
      programStack.ListFiles();
      /* We don't want to do anything other than read the file */
      break;
    }
    else if(tw.type.u == toBase40("PSAVE")){
      programStack.WriteFile(dataStack.pop());
    }
    else if(tw.type.u == toBase40("QUOTE")){
      cmdQuote();
    }
    else if(tw.type.u == toBase40("STACK")){
      cmdStack();
    }
    else if(tw.type.u == toBase40("SIZE")){
      cmdSize();
    }
    else if(tw.type.u == toBase40("TRUE")){
      cmdTrue();
    }
    else if(tw.type.u == toBase40("TIME")){
      cmdTime();
    }
    else {
      for(int j=0;j<definedWordIndex;j++){
        if(tw.type.u == definedWord[j].word){
#if DEBUGEXECUTE
          Serial.print("User Defined @");
          Serial.println(definedWord[j].index);
#endif
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
    programStack.selectNext();
    if(singleStep){
#if DEBUGEXECUTE
      Serial.println("Step");
#endif
      return;
    }
  }
#if DEBUGEXECUTE
  Serial.println("programExecute Done");
#endif
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
  Serial.println(inx);
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
void showAlphaNumeric(){
  uint16_t color;
  if(InputState == INPUTSTATEALPHANUMERIC){
    color=MAGENTA;
  }
  else {
    color=NORMALCOLOR;
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
void showState(){
  int line=0;
// Testing cmdWriteFile
  line=dataStack.show(line);
  programStack.show(line+1,(InputState == INPUTSTATECLEAR));
  // Seems to be the problem child
  showAlphaNumeric();
  GO.update();
  showAccumulator();
  GO.update();
  menuStack.show();
  GO.update();
}

/* Switching to expect one keypress at a time */
void processEvent(uint16_t pk, uint16_t tk){
  int i;
  if(increment < 1){
    increment=1;
  }
  if(pk == KPA){
    switch(tk){
      case KPLEFT|KPA:
        InputState=INPUTSTATEALPHANUMERIC;
        Serial.println("INPUTSTATEALPHANUMERIC");
        break;
      case KPRIGHT|KPA:
        InputState=INPUTSTATENUMERIC;
        Serial.println("INPUTSTATENUMERIC");
        break;
      case KPUP|KPA:
        InputState=INPUTSTATECLEAR;
        Serial.println("INPUTSTATECLEAR");
        break;
      case KPDOWN|KPA:
        InputState=INPUTSTATEMENU;
        Serial.println("INPUTSTATEMENU");
        break;
    }
  }
  if(InputState == INPUTSTATEALPHANUMERIC){
      switch(tk){
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
        case KPSELECT:
          programStack.paste(Base40Buffer);
          Base40Buffer.type.u=0;
#if 1
          Serial.print(Base40Buffer.type.u);
          Serial.println("+ ");
          programStack.dump();
#endif
          break;
        case KPSTART:
          AnApnd(anDelete);
          break;
      }
  }
  else if(InputState == INPUTSTATEMENU){
    switch(tk){
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
    }
  }
  else if(InputState == INPUTSTATENUMERIC){
      switch(tk){
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
      switch(tk){
        case KPB:
          programStack.selectFirst();
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
  if(pk < tk){
    showState();
  }
}
void keypress(){
  uint16_t tk=0;

  GO.update();

  tk |= ((GO.BtnMenu.isPressed()) ? KPMENU : 0);
  tk |= ((GO.BtnVolume.isPressed()) ? KPVOLUME : 0);
  tk |= ((GO.BtnSelect.isPressed()) ? KPSELECT : 0);
  tk |= ((GO.BtnStart.isPressed()) ? KPSTART : 0);
  tk |= ((GO.BtnA.isPressed()) ? KPA : 0);
  tk |= ((GO.BtnB.isPressed()) ? KPB : 0);
  tk |= ((GO.JOY_Y.isAxisPressed() == 2) ? KPUP : 0);
  tk |= ((GO.JOY_Y.isAxisPressed() == 1) ? KPDOWN : 0);
  tk |= ((GO.JOY_X.isAxisPressed() == 2) ? KPLEFT : 0);
  tk |= ((GO.JOY_X.isAxisPressed() == 1) ? KPRIGHT : 0);

  if(tk != pk){
    if(tk != 0){
      digitalWrite(LED_BUILTIN, HIGH);
    }
    else {
      digitalWrite(LED_BUILTIN, LOW);
    }
    processEvent(pk,tk);
    pk=tk;
    return;
  }
  // No change
  return;
}
void setup() {
  // Initialize globals
  
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(25, OUTPUT);
  digitalWrite(25, LOW);
  delay(100);
  GO.begin();
  dacWrite(SPEAKER_PIN, (uint8_t)0);
  GO.lcd.setTextSize(2);
  GO.lcd.setTextColor(NORMALCOLOR);
  initKeypress();
  GO.update();
  showState();
}
void loop() {
  // Process keypress
  keypress();
  delay(32);
}

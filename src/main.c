#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#ifdef USESDL
#include <SDL.h>
#include <SDL_audio.h>
#endif
// Approximations of some Windows functions to ease portability
#if defined GNU_LIBRARY || defined GLIBC

#endif

// Define missing macros and variables
#define END 255
#define BREAK 254
#define FLAG_DIPTHONG 0x0010
#define FLAG_VOWEL 0x0080
#define pR 23
#define pT 69
#define pD 57
#define FLAG_DIP_YX 0x0020
#define FLAG_PUNCT 0x0100
#define FLAG_VOICED 0x0004
#define FLAG_CONSONANT 0x0040
#define FLAG_NASAL 0x0800
#define FLAG_LIQUIC 0x1000
#define PHONEME_PERIOD 0x1
#define RISING_INFLECTION 0x1
#define PHONEME_QUESTION 0x2
#define FALLING_INFLECTION 0x2
unsigned char stressOutput[256];
#define FLAG_ALVEOLAR 0x0400

// from render.c
unsigned char phonemeIndexOutput[60];  // tab47296
unsigned char phonemeLengthOutput[60]; // tab47416
unsigned char pitches[256];
unsigned char sampledConsonantFlag[256]; // tab44800
unsigned char frequency1[256];
unsigned char frequency2[256];
unsigned char frequency3[256];
unsigned char amplitude1[256];
unsigned char amplitude2[256];
unsigned char amplitude3[256];
// from sam.c
char input[256]; // tab39445
unsigned char speed = 72;
unsigned char pitch = 64;
unsigned char mouth = 128;
unsigned char throat = 128;
int singmode = 0;
int debug = 0;
unsigned char phonemeLength[256]; // tab40160
unsigned char stress[256];        // numbers from 0 to 8
unsigned char phonemeindex[256];
int bufferpos = 0;
char *buffer = NULL;
// from debug.c
unsigned char signInputTable1[];
unsigned char signInputTable2[];
// from reciter.c
unsigned char A, X;
// from RenderTabs.h
unsigned char blendRank[];
unsigned char outBlendLength[];
unsigned char inBlendLength[];
unsigned char phonemeLengthTable[];
unsigned char phonemeStressedLengthTable[];
unsigned char tab36376[];
unsigned char tab37489[];
unsigned char tab37515[];
// extern unsigned char flags[];
unsigned char multtable[];
unsigned char sinus[];
unsigned char rectangle[];
unsigned char tab47492[];
unsigned char tab48426[];
unsigned char stressInputTable[];
unsigned char signInputTable1[];
unsigned char signInputTable2[];
unsigned char freq1data[];
unsigned char freq2data[];
unsigned char freq3data[];
unsigned char ampl1data[];
unsigned char ampl2data[];
unsigned char ampl3data[];
unsigned char sampledConsonantFlags[];
unsigned char sampleTable[];
unsigned char amplitudeRescale[];

// timetable for more accurate c64 simulation
static const int timetable[5][5] = {{162, 167, 167, 127, 128},
                                    {226, 60, 60, 0, 0},
                                    {225, 60, 59, 0, 0},
                                    {200, 0, 0, 54, 55},
                                    {199, 0, 0, 54, 54}};

unsigned char trans(unsigned char mouth, unsigned char initialFrequency);
static void CombineGlottalAndFormants(unsigned char phase1, unsigned char phase2, unsigned char phase3, unsigned char Y);

void PrintUsage();
void interpolate(unsigned char width, unsigned char table, unsigned char frame, char mem53);
void interpolate_pitch(unsigned char pos, unsigned char mem49, unsigned char phase3);
void fopen_s(FILE **f, const char *filename, const char *mode);
void Insert(unsigned char position, unsigned char mem60_inputMatchPos, unsigned char mem59, unsigned char mem58_variant);
unsigned char trans(unsigned char a, unsigned char b);
void InsertBreath();
void SetInput(unsigned char *_input);
char *GetBuffer();
unsigned char Read(unsigned char p, unsigned char Y);
unsigned char Code37055(unsigned char npos, unsigned char mask);
unsigned int match(const char *str);
void Write(unsigned char p, unsigned char Y, unsigned char value);
int handle_ch2(unsigned char ch, unsigned char mem);
int handle_ch(unsigned char ch, unsigned char mem);
void change(unsigned char pos, unsigned char val, const char *rule);
void drule(const char *str);
void drule_pre(const char *descr, unsigned char X);
void drule_post(unsigned char X);
void rule_alveolar_uw(unsigned char X);
void rule_ch(unsigned char X);
void rule_j(unsigned char X);
void rule_g(unsigned char pos);
void rule_dipthong(unsigned char p, unsigned short pf, unsigned char pos);
void AddInflection(unsigned char inflection, unsigned char pos);
unsigned char CreateTransitions();
void ProcessFrames(unsigned char t);

void PrintPhonemes(unsigned char *phonemeindex, unsigned char *phonemeLength, unsigned char *stress);
void PrintOutput(unsigned char *flag, unsigned char *f1, unsigned char *f2, unsigned char *f3, unsigned char *a1, unsigned char *a2, unsigned char *a3, unsigned char *p);
// from sam.c
void SetInput(unsigned char *_input);
void SetSpeed(unsigned char _speed);
void SetPitch(unsigned char _pitch);
void SetMouth(unsigned char _mouth);
void SetThroat(unsigned char _throat);
void EnableSingmode();
char *GetBuffer();
int GetBufferLength();
void Init();
int SAMMain();
void PrepareOutput();
void SetMouthThroat(unsigned char mouth, unsigned char throat);
// from debug.c
void PrintRule(unsigned short offset);
// from reciter.c
unsigned char GetRuleByte(unsigned short mem62, unsigned char Y);
int TextToPhonemes(unsigned char *input);
// from render.c
void RenderSample(unsigned char *mem66_openBrace, unsigned char consonantFlag, unsigned char mem49);
void Render();

void CopyStress();
void SetPhonemeLength();
void AdjustLengths();
void Code41240();
static void Output(int index, unsigned char A);
void OutputSound();
void WriteWav(char *filename, char *buffer, int bufferlength);
#ifdef USESDL
int pos = 0;
int SDL_pos_sound_buffer = 0;
void MixAudio(void *unused, Uint8 *stream, int len);
#endif

static unsigned char inputtemp[256]; // secure copy of input tab36096

void Output(int index, unsigned char A) {
  static unsigned oldtimetableindex = 0;
  int k;
  bufferpos += timetable[oldtimetableindex][index];
  oldtimetableindex = index;
  // write a little bit in advance
  for (k = 0; k < 5; k++)
    buffer[bufferpos / 50 + k] = (A & 15) * 16;
}

void fopen_s(FILE **f, const char *filename, const char *mode) {
  *f = fopen(filename, mode);
}

static int min(int l, int r) {
    if (l < r) {
        return l;
    } else {
        return r;
    }
}

static void strcat_s(char *dest, int size, char *str) {
  unsigned int dlen = strlen(dest);
  if (dlen >= size - 1)
    return;
  strncat(dest + dlen, str, size - dlen - 1);
}

void Write(unsigned char p, unsigned char Y, unsigned char value) {
  switch (p) {
  case 168:
    pitches[Y] = value;
    return;
  case 169:
    frequency1[Y] = value;
    return;
  case 170:
    frequency2[Y] = value;
    return;
  case 171:
    frequency3[Y] = value;
    return;
  case 172:
    amplitude1[Y] = value;
    return;
  case 173:
    amplitude2[Y] = value;
    return;
  case 174:
    amplitude3[Y] = value;
    return;
  default:
    printf("Error writing to tables\n");
    return;
  }
}

int main(int argc, char **argv) {
  int i;
  int phonetic = 0;

  char *wavfilename = NULL;
  unsigned char input[256];

  memset(input, 0, 256);

  if (argc <= 1) {
    PrintUsage();
    return 1;
  }

  i = 1;
  while (i < argc) {
    if (argv[i][0] != '-') {
      strcat_s((char *)input, 256, argv[i]);
      strcat_s((char *)input, 256, " ");
    } else {
      if (strcmp(&argv[i][1], "wav") == 0) {
        wavfilename = argv[i + 1];
        i++;
      } else if (strcmp(&argv[i][1], "sing") == 0) {
        EnableSingmode();
      } else if (strcmp(&argv[i][1], "phonetic") == 0) {
        phonetic = 1;
      } else if (strcmp(&argv[i][1], "debug") == 0) {
        debug = 1;
      } else if (strcmp(&argv[i][1], "pitch") == 0) {
        SetPitch((unsigned char)min(atoi(argv[i + 1]), 255));
        i++;
      } else if (strcmp(&argv[i][1], "speed") == 0) {
        SetSpeed((unsigned char)min(atoi(argv[i + 1]), 255));
        i++;
      } else if (strcmp(&argv[i][1], "mouth") == 0) {
        SetMouth((unsigned char)min(atoi(argv[i + 1]), 255));
        i++;
      } else if (strcmp(&argv[i][1], "throat") == 0) {
        SetThroat((unsigned char)min(atoi(argv[i + 1]), 255));
        i++;
      } else {
        PrintUsage();
        return 1;
      }
    }

    i++;
  } // while

  for (i = 0; input[i] != 0; i++)
    input[i] = (unsigned char)toupper((int)input[i]);

  if (debug) {
    if (phonetic)
      printf("phonetic input: %s\n", input);
    else
      printf("text input: %s\n", input);
  }

  if (!phonetic) {
    strcat_s((char *)input, 256, "[");
    if (!TextToPhonemes(input))
      return 1;
    if (debug)
      printf("phonetic input: %s\n", input);
  } else
    strcat_s((char *)input, 256, "\x9b");

#ifdef USESDL
  if (SDL_Init(SDL_INIT_AUDIO) < 0) {
    printf("Unable to init SDL: %s\n", SDL_GetError());
    exit(1);
  }
  atexit(SDL_Quit);
#endif

  SetInput(input);
  if (!SAMMain()) {
    PrintUsage();
    return 1;
  }
  if (wavfilename != NULL) {
    WriteWav(wavfilename, GetBuffer(), GetBufferLength() / 50);
  } else {
    OutputSound();
  }

  return 0;
}

void PrintUsage() {
  printf("usage: sam [options] Word1 Word2 ....\n");
  printf("options\n");
  printf("	-phonetic 		enters phonetic mode. (see below)\n");
  printf("	-pitch number		set pitch value (default=64)\n");
  printf("	-speed number		set speed value (default=72)\n");
  printf("	-throat number		set throat value (default=128)\n");
  printf("	-mouth number		set mouth value (default=128)\n");
  printf("	-wav filename		output to wav instead of libsdl\n");
  printf("	-sing			special treatment of pitch\n");
  printf("	-debug			print additional debug messages\n");
  printf("\n");

  printf("     VOWELS                            VOICED CONSONANTS	\n");
  printf("IY           f(ee)t                    R        red		\n");
  printf("IH           p(i)n                     L        allow		\n");
  printf("EH           beg                       W        away		\n");
  printf("AE           Sam                       W        whale		\n");
  printf("AA           pot                       Y        you		\n");
  printf("AH           b(u)dget                  M        Sam		\n");
  printf("AO           t(al)k                    N        man		\n");
  printf("OH           cone                      NX       so(ng)	"
         "	\n");
  printf("UH           book                      B        bad		\n");
  printf("UX           l(oo)t                    D        dog		\n");
  printf("ER           bird                      G        again		\n");
  printf("AX           gall(o)n                  J        judge		\n");
  printf("IX           dig(i)t                   Z        zoo		\n");
  printf("				       ZH       plea(s)ure	\n");
  printf("   DIPHTHONGS                          V        seven		\n");
  printf("EY           m(a)de                    DH       (th)en	"
         "	\n");
  printf("AY           h(igh)						\n");
  printf("OY           boy						\n");
  printf("AW           h(ow)                     UNVOICED CONSONANTS	\n");
  printf("OW           slow                      S         Sam		\n");
  printf("UW           crew                      Sh        fish		\n");
  printf("                                       F         fish		\n");
  printf("                                       TH        thin		\n");
  printf(" SPECIAL PHONEMES                      P         poke		\n");
  printf("UL           sett(le) (=AXL)           T         talk		\n");
  printf("UM           astron(omy) (=AXM)        K         cake		\n");
  printf("UN           functi(on) (=AXN)         CH        speech	"
         "	\n");
  printf("Q            kitt-en (glottal stop)    /H        a(h)ead	\n");
}

// linearly interpolate values
void interpolate(unsigned char width, unsigned char table, unsigned char frame, char mem53) {
  unsigned char sign = (mem53 < 0);
  unsigned char remainder = abs(mem53) % width;
  unsigned char div = mem53 / width;
  unsigned char error = 0;
  unsigned char pos = width;
  unsigned char val = Read(table, frame) + div;

  pos--;
  while (pos > 0) {
    error += remainder;
    if (error >= width) { // accumulated a whole integer error, so adjust output
      error -= width;
      if (sign) {
        val--;
      } else if (val) {
        val++; // if input is 0, we always leave it alone
      }
    }
    frame++;
    Write(table, frame, val); // Write updated value back to next frame.
    val += div;
    pos--;
  }
}

// return = hibyte(mem39212*mem39213) <<  1
unsigned char trans(unsigned char a, unsigned char b) {
  return (((unsigned int)a * b) >> 8) << 1;
}

void interpolate_pitch(unsigned char pos, unsigned char mem49, unsigned char phase3) {
  // unlike the other values, the pitches[] interpolates from
  // the middle of the current phoneme to the middle of the
  // next phoneme

  // half the width of the current and next phoneme
  unsigned char cur_width = phonemeLengthOutput[pos] / 2;
  unsigned char next_width = phonemeLengthOutput[pos + 1] / 2;
  // sum the values
  unsigned char width = cur_width + next_width;
  char pitch = pitches[next_width + mem49] - pitches[mem49 - cur_width];
  interpolate(width, 168, phase3, pitch);
}

unsigned char CreateTransitions() {
  unsigned char mem49 = 0;
  unsigned char pos = 0;
  while (1) {
    unsigned char next_rank;
    unsigned char rank;
    unsigned char speedcounter;
    unsigned char phase1;
    unsigned char phase2;
    unsigned char phase3;
    unsigned char transition;

    unsigned char phoneme = phonemeIndexOutput[pos];
    unsigned char next_phoneme = phonemeIndexOutput[pos + 1];

    if (next_phoneme == 255)
      break; // 255 == end_token

    // get the ranking of each phoneme
    next_rank = blendRank[next_phoneme];
    rank = blendRank[phoneme];

    // compare the rank - lower rank value is stronger
    if (rank == next_rank) {
      // same rank, so use out blend lengths from each phoneme
      phase1 = outBlendLength[phoneme];
      phase2 = outBlendLength[next_phoneme];
    } else if (rank < next_rank) {
      // next phoneme is stronger, so us its blend lengths
      phase1 = inBlendLength[next_phoneme];
      phase2 = outBlendLength[next_phoneme];
    } else {
      // current phoneme is stronger, so use its blend lengths
      // note the out/in are swapped
      phase1 = outBlendLength[phoneme];
      phase2 = inBlendLength[phoneme];
    }

    mem49 += phonemeLengthOutput[pos];

    speedcounter = mem49 + phase2;
    phase3 = mem49 - phase1;
    transition = phase1 + phase2; // total transition?

    if (((transition - 2) & 128) == 0) {
      unsigned char table = 169;
      interpolate_pitch(pos, mem49, phase3);
      while (table < 175) {
        // tables:
        // 168  pitches[]
        // 169  frequency1
        // 170  frequency2
        // 171  frequency3
        // 172  amplitude1
        // 173  amplitude2
        // 174  amplitude3

        char value = Read(table, speedcounter) - Read(table, phase3);
        interpolate(transition, table, phase3, value);
        table++;
      }
    }
    ++pos;
  }

  // add the length of this phoneme
  return mem49 + phonemeLengthOutput[pos];
}

// written by me because of different table positions.
// mem[47] = ...
// 168=pitches
// 169=frequency1
// 170=frequency2
// 171=frequency3
// 172=amplitude1
// 173=amplitude2
// 174=amplitude3
unsigned char Read(unsigned char p, unsigned char Y) {
  switch (p) {
  case 168:
    return pitches[Y];
  case 169:
    return frequency1[Y];
  case 170:
    return frequency2[Y];
  case 171:
    return frequency3[Y];
  case 172:
    return amplitude1[Y];
  case 173:
    return amplitude2[Y];
  case 174:
    return amplitude3[Y];
  default:
    printf("Error reading from tables");
    return 0;
  }
}

void PrintPhonemes(unsigned char *phonemeindex, unsigned char *phonemeLength, unsigned char *stress) {
  int i = 0;
  printf("===========================================\n");
  printf("Internal Phoneme presentation:\n\n");
  printf(" idx    phoneme  length  stress\n");
  printf("------------------------------\n");
  while ((phonemeindex[i] != 255) && (i < 255)) {
    if (phonemeindex[i] < 81) {
      printf(" %3i      %c%c      %3i       %i\n", phonemeindex[i],
             signInputTable1[phonemeindex[i]], signInputTable2[phonemeindex[i]],
             phonemeLength[i], stress[i]);
    } else {
      printf(" %3i      ??      %3i       %i\n", phonemeindex[i],
             phonemeLength[i], stress[i]);
    }
    i++;
  }
  printf("===========================================\n");
  printf("\n");
}

void PrintOutput(unsigned char *flag, unsigned char *f1, unsigned char *f2, unsigned char *f3, unsigned char *a1, unsigned char *a2, unsigned char *a3, unsigned char *p) {
  int i = 0;
  printf("===========================================\n");
  printf("Final data for speech output:\n\n");
  printf(" flags ampl1 freq1 ampl2 freq2 ampl3 freq3 pitch\n");
  printf("------------------------------------------------\n");
  while (i < 255) {
    printf("%5i %5i %5i %5i %5i %5i %5i %5i\n", flag[i], a1[i], f1[i], a2[i],
           f2[i], a3[i], f3[i], p[i]);
    i++;
  }
  printf("===========================================\n");
}

void PrintRule(unsigned short offset) {
  unsigned char i = 1;
  unsigned char A = 0;
  printf("Applying rule: ");
  do {
    A = GetRuleByte(offset, i);
    if ((A & 127) == '=')
      printf(" -> ");
    else
      printf("%c", A & 127);
    i++;
  } while ((A & 128) == 0);
  printf("\n");
}

unsigned char GetRuleByte(unsigned short mem62, unsigned char Y) {
  unsigned int address = mem62;
  if (mem62 >= 37541) {
    address -= 37541;
    return rules2[address + Y];
  }
  address -= 32000;
  return rules[address + Y];
}

int TextToPhonemes(unsigned char *input) {
  unsigned char mem56_phonemeOutpos; // output position for phonemes
  unsigned char mem57_currentFlags;
  unsigned char inputtemp_index = 0;
  unsigned char mem59;
  unsigned char mem60_inputMatchPos;
  unsigned char mem61_inputPos;
  unsigned short mem62 = 0; // memory position of current rule

  unsigned char mem64_equalSignInRule; // position of '=' or current character
  unsigned char mem65_closingBrace; // position of ')'
  unsigned char mem66_openBrace; // position of '('

  unsigned char Y;

  int r;

  int process_rule_flag = 0;

  inputtemp[0] = ' ';

  // secure copy of input
  // because input will be overwritten by phonemes
  X = 0;
  do {
    A = input[X] & 127;
    if (A >= 112) {
      A = A & 95;
    }
    else if (A >= 96) {
      A = A & 79;
    }
    inputtemp[++X] = A;
  } while (X < 255);
  inputtemp[255] = 27;
  mem56_phonemeOutpos = mem61_inputPos = 255;

  while (1) {
    while (1) {
      while (1) {
        X = ++mem61_inputPos;
        mem64_equalSignInRule = inputtemp[X];
        if (mem64_equalSignInRule == '[') {
          X = ++mem56_phonemeOutpos;
          input[X] = 155;
          return 1;
        }

        if (mem64_equalSignInRule != '.') {
          break;
        }
        X++;
        A = tab36376[inputtemp[X]] & 1;
        if (A != 0) {
          break;
        }
        mem56_phonemeOutpos++;
        X = mem56_phonemeOutpos;
        A = '.';
        input[X] = '.';
      }
      mem57_currentFlags = tab36376[mem64_equalSignInRule];
      if ((mem57_currentFlags & 2) != 0) {
        mem62 = 37541;
        break;
      }

      if (mem57_currentFlags != 0) {
        break;
      }
      inputtemp[X] = ' ';
      X = ++mem56_phonemeOutpos;
      if (X > 120) {
        input[X] = 155;
        return 1;
      }
      input[X] = 32;
    }

    if ((mem57_currentFlags & 2) == 0) {
      if (!(mem57_currentFlags & 128)) {
        return 0;
      }

      // go to the right rules for this character.
      X = mem64_equalSignInRule - 'A';
      mem62 = tab37489[X] | (tab37515[X] << 8);
    }
    while (1) {
      // find next rule
      // Look for a byte where the 7th bit is set
      mem62++;  // Start checking from the next position
      while ((GetRuleByte(mem62, 0) & 128) == 0) {
          mem62++;  // Continue searching in the next byte
      }

      // Initialize Y to search for the opening brace '('
      Y = 1;  // Start checking from the next position
      while (GetRuleByte(mem62, Y) != '(') {
          Y++;  // Increment Y until '(' is found
      }
      mem66_openBrace = Y; // Store the position of '('

      // Look for the closing brace ')'
      Y++;  // Start checking from the next position
      while (GetRuleByte(mem62, Y) != ')') {
          Y++;  // Increment Y until ')' is found
      }
      mem65_closingBrace = Y; // Store the position of ')'

      // Look for the '=' sign, taking into account 127 mask
      Y++;  // Start checking from the next position
      while ((GetRuleByte(mem62, Y) & 127) != '=') {
          Y++;  // Increment Y until '=' is found
      }
      mem64_equalSignInRule = Y; // Store the position of '='

      mem60_inputMatchPos = X = mem61_inputPos;
      // compare the string within the bracket
      Y = mem66_openBrace + 1;

      int match_failed = 0;
      while (1) {
        if (GetRuleByte(mem62, Y) != inputtemp[X]) {
          match_failed = 1;
          break;
        }
        if (++Y == mem65_closingBrace) {
          break;
        }
        mem60_inputMatchPos = ++X;
      }

      if (match_failed) {
        continue;
      }

      // the string in the bracket is correct

      mem59 = mem61_inputPos;

      while (1) {
        unsigned char ch;
        while (1) {
          mem66_openBrace--;
          mem57_currentFlags = GetRuleByte(mem62, mem66_openBrace);
          if ((mem57_currentFlags & 128) != 0) {
            inputtemp_index = mem60_inputMatchPos;
            process_rule_flag = 1;
            break;
          }
          X = mem57_currentFlags & 127;
          if ((tab36376[X] & 128) == 0) {
            break;
          }
          if (inputtemp[mem59 - 1] != mem57_currentFlags) {
            match_failed = 1;
            break;
          }
          --mem59;
        }
        if (! process_rule_flag) {
          if (match_failed) {
            break;
          }

          ch = mem57_currentFlags;

          r = handle_ch2(ch, mem59 - 1);
          if (r == -1) {
            switch (ch) {
            case '&':
              if (!Code37055(mem59 - 1, 16)) {
                if (inputtemp[X] != 'H') {
                  r = 1;
                }
                else {
                  A = inputtemp[--X];
                  if ((A != 'C') && (A != 'S')) {
                    r = 1;
                  }
                }
              }
              break;

            case '@':
              if (!Code37055(mem59 - 1, 4)) {
                A = inputtemp[X];
                if (A != 'H') {
                  r = 1;
                }
                if ((A != 'T') && (A != 'C') && (A != 'S')) {
                  r = 1;
                }
              }
              break;
            case '+':
              X = mem59;
              A = inputtemp[--X];
              if ((A != 'E') && (A != 'I') && (A != 'Y')) {
                r = 1;
              }
              break;
            case ':':
              while (Code37055(mem59 - 1, 32))
                --mem59;
              continue;
            default:
              return 0;
            }
          }

          if (r == 1) {
            match_failed = 1;
            break;
          }

          mem59 = X;
        } else {
          break;
        }

        if (match_failed) {
          continue;
        }
      }

      do {

        if (! process_rule_flag) {
          X = inputtemp_index + 1;
          if (inputtemp[X] == 'E') {
            if ((tab36376[inputtemp[X + 1]] & 128) != 0) {
              A = inputtemp[++X];
              if (A == 'L') {
                if (inputtemp[++X] != 'Y') {
                  match_failed = 1;
                  break;
                }
              } else if ((A != 'R') && (A != 'S') && (A != 'D') && !match("FUL")) {
                match_failed = 1;
                break;
              }
            }
          } else {
            if (!match("ING")) {
              match_failed = 1;
              break;
            }
            inputtemp_index = X;
          }

        }
        process_rule_flag = 0;



        r = 0;
        do {
          while (1) {
            Y = mem65_closingBrace + 1;
            if (Y == mem64_equalSignInRule) {
              mem61_inputPos = mem60_inputMatchPos;

              if (debug) {
                PrintRule(mem62);
              }

              while (1) {
                mem57_currentFlags = A = GetRuleByte(mem62, Y);
                A = A & 127;
                if (A != '=') {
                  input[++mem56_phonemeOutpos] = A;
                }
                if ((mem57_currentFlags & 128) != 0) {
                  break; // Break out of the inner while loop
                }
                Y++;
              }
              // If we've reached this point, we need to continue the outermost loop
              break; // Break out of the do-while loop
            }
            mem65_closingBrace = Y;
            mem57_currentFlags = GetRuleByte(mem62, Y);
            if ((tab36376[mem57_currentFlags] & 128) == 0) {
              break;
            }
            if (inputtemp[inputtemp_index + 1] != mem57_currentFlags) {
              r = 1;
              break;
            }
            ++inputtemp_index;
          }

          if (r == 0) {
            A = mem57_currentFlags;
            if (A == '@') {
              if (Code37055(inputtemp_index + 1, 4) == 0) {
                A = inputtemp[X];
                if ((A != 'R') && (A != 'T') && (A != 'C') && (A != 'S')) {
                  r = 1;
                }
              } else {
                r = -2;
              }
            } else if (A == ':') {
              while (Code37055(inputtemp_index + 1, 32))
                inputtemp_index = X;
              r = -2;
            } else {
              r = handle_ch(A, inputtemp_index + 1);
            }
          }

          if (r == 1) {
            match_failed = 1;
            break;
          }
          if (r == -2) {
            r = 0;
            continue;
          }
          if (r == 0) {
            inputtemp_index = X;
          }
        } while (r == 0 && Y != mem64_equalSignInRule);

        if (match_failed) {
          break;
        }

        if (Y == mem64_equalSignInRule) {
          // If we've reached this point, we need to continue the outermost loop
          break; // Break out of the do-while loop
        }

      } while (A == '%');

      if (match_failed) {
        continue;
      }

      // If we've reached this point, we've successfully processed the rule
      break;
    }

    // This is where we continue the outermost loop
    continue;
  }

  return 0;
}

unsigned char Code37055(unsigned char npos, unsigned char mask) {
  X = npos;
  return tab36376[inputtemp[X]] & mask;
}

unsigned int match(const char *str) {
  while (*str) {
    unsigned char ch = *str;
    A = inputtemp[X++];
    if (A != ch)
      return 0;
    ++str;
  }
  return 1;
}

int handle_ch2(unsigned char ch, unsigned char mem) {
  unsigned char tmp;
  X = mem;
  tmp = tab36376[inputtemp[mem]];
  if (ch == ' ') {
    if (tmp & 128)
      return 1;
  } else if (ch == '#') {
    if (!(tmp & 64))
      return 1;
  } else if (ch == '.') {
    if (!(tmp & 8))
      return 1;
  } else if (ch == '^') {
    if (!(tmp & 32))
      return 1;
  } else
    return -1;
  return 0;
}

int handle_ch(unsigned char ch, unsigned char mem) {
  unsigned char tmp;
  X = mem;
  tmp = tab36376[inputtemp[X]];
  if (ch == ' ') {
    if ((tmp & 128) != 0)
      return 1;
  } else if (ch == '#') {
    if ((tmp & 64) == 0)
      return 1;
  } else if (ch == '.') {
    if ((tmp & 8) == 0)
      return 1;
  } else if (ch == '&') {
    if ((tmp & 16) == 0) {
      if (inputtemp[X] != 72)
        return 1;
      ++X;
    }
  } else if (ch == '^') {
    if ((tmp & 32) == 0)
      return 1;
  } else if (ch == '+') {
    X = mem;
    ch = inputtemp[X];
    if ((ch != 69) && (ch != 73) && (ch != 89))
      return 1;
  } else
    return -1;
  return 0;
}

signed int full_match(unsigned char sign1, unsigned char sign2) {
  unsigned char Y = 0;
  do {
    // GET FIRST CHARACTER AT POSITION Y IN signInputTable
    // --> should change name to PhonemeNameTable1
    unsigned char A = signInputTable1[Y];

    if (A == sign1) {
      A = signInputTable2[Y];
      // NOT A SPECIAL AND MATCHES SECOND CHARACTER?
      if ((A != '*') && (A == sign2))
        return Y;
    }
  } while (++Y != 81);
  return -1;
}

signed int wild_match(unsigned char sign1) {
  signed int Y = 0;
  do {
    if (signInputTable2[Y] == '*') {
      if (signInputTable1[Y] == sign1)
        return Y;
    }
  } while (++Y != 81);
  return -1;
}

// The input[] buffer contains a string of phonemes and stress markers along
// the lines of:
//
//     DHAX KAET IHZ AH5GLIY. <0x9B>
//
// The byte 0x9B marks the end of the buffer. Some phonemes are 2 bytes
// long, such as "DH" and "AX". Others are 1 byte long, such as "T" and "Z".
// There are also stress markers, such as "5" and ".".
//
// The first character of the phonemes are stored in the table
// signInputTable1[]. The second character of the phonemes are stored in the
// table signInputTable2[]. The stress characters are arranged in low to high
// stress order in stressInputTable[].
//
// The following process is used to parse the input[] buffer:
//
// Repeat until the <0x9B> character is reached:
//
//        First, a search is made for a 2 character match for phonemes that do
//        not end with the '*' (wildcard) character. On a match, the index of
//        the phoneme is added to phonemeIndex[] and the buffer position is
//        advanced 2 bytes.
//
//        If this fails, a search is made for a 1 character match against all
//        phoneme names ending with a '*' (wildcard). If this succeeds, the
//        phoneme is added to phonemeIndex[] and the buffer position is advanced
//        1 byte.
//
//        If this fails, search for a 1 character match in the
//        stressInputTable[]. If this succeeds, the stress value is placed in
//        the last stress[] table at the same index of the last added phoneme,
//        and the buffer position is advanced by 1 byte.
//
//        If this fails, return a 0.
//
// On success:
//
//    1. phonemeIndex[] will contain the index of all the phonemes.
//    2. The last index in phonemeIndex[] will be 255.
//    3. stress[] will contain the stress value for each phoneme

// input[] holds the string of phonemes, each two bytes wide
// signInputTable1[] holds the first character of each phoneme
// signInputTable2[] holds te second character of each phoneme
// phonemeIndex[] holds the indexes of the phonemes after parsing input[]
//
// The parser scans through the input[], finding the names of the phonemes
// by searching signInputTable1[] and signInputTable2[]. On a match, it
// copies the index of the phoneme into the phonemeIndexTable[].
//
// The character <0x9B> marks the end of text in input[]. When it is reached,
// the index 255 is placed at the end of the phonemeIndexTable[], and the
// function returns with a 1 indicating success.
int Parser1() {
  unsigned char sign1;
  unsigned char position = 0;
  unsigned char srcpos = 0;

  memset(stress, 0, 256); // Clear the stress table.

  while ((sign1 = input[srcpos]) != 155) { // 155 (\233) is end of line marker
    signed int match;
    unsigned char sign2 = input[++srcpos];
    if ((match = full_match(sign1, sign2)) != -1) {
      // Matched both characters (no wildcards)
      phonemeindex[position++] = (unsigned char)match;
      ++srcpos; // Skip the second character of the input as we've matched it
    } else if ((match = wild_match(sign1)) != -1) {
      // Matched just the first character (with second character matching '*'
      phonemeindex[position++] = (unsigned char)match;
    } else {
      // Should be a stress character. Search through the
      // stress table backwards.
      match = 8; // End of stress table. FIXME: Don't hardcode.
      while ((sign1 != stressInputTable[match]) && (match > 0)) {
        --match;
      }

      if (match == 0)
        return 0; // failure

      stress[position - 1] =
          (unsigned char)match; // Set stress for prior phoneme
    }
  } // while

  phonemeindex[position] = END;
  return 1;
}

void ChangeRule(unsigned char position, unsigned char mem60_inputMatchPos, const char *descr) {
  if (debug)
    printf("RULE: %s\n", descr);
  phonemeindex[position] = 13; // rule;
  Insert(position + 1, mem60_inputMatchPos, 0, stress[position]);
}

// change phonemelength depedendent on stress
void SetPhonemeLength() {
  int position = 0;
  while (phonemeindex[position] != 255) {
    unsigned char A = stress[position];
    if ((A == 0) || ((A & 128) != 0)) {
      phonemeLength[position] = phonemeLengthTable[phonemeindex[position]];
    } else {
      phonemeLength[position] = phonemeStressedLengthTable[phonemeindex[position]];
    }
    position++;
  }
}

void Code41240() {
  unsigned char pos = 0;

  while (phonemeindex[pos] != END) {
    unsigned char index = phonemeindex[pos];

    if ((flags[index] & FLAG_STOPCONS)) {
      if ((flags[index] & FLAG_PLOSIVE)) {
        unsigned char A;
        unsigned char X = pos;
        while (!phonemeindex[++X])
          ; /* Skip pause */
        A = phonemeindex[X];
        if (A != END) {
          if ((flags[A] & 8) || (A == 36) || (A == 37)) {
            ++pos;
            continue;
          } // '/H' '/X'
        }
      }
      Insert(pos + 1, index + 1, phonemeLengthTable[index + 1], stress[pos]);
      Insert(pos + 2, index + 2, phonemeLengthTable[index + 2], stress[pos]);
      pos += 2;
    }
    ++pos;
  }
}

void Parser2() {
  unsigned char pos = 0; // mem66_openBrace;
  unsigned char p;

  if (debug)
    printf("Parser2\n");

  while ((p = phonemeindex[pos]) != END) {
    unsigned short pf;
    unsigned char prior;

    if (debug)
      printf("%d: %c%c\n", pos, signInputTable1[p], signInputTable2[p]);

    if (p == 0) { // Is phoneme pause?
      ++pos;
      continue;
    }

    pf = flags[p];
    prior = phonemeindex[pos - 1];

    if ((pf & FLAG_DIPTHONG))
      rule_dipthong(p, pf, pos);
    else if (p == 78)
      ChangeRule(pos, 24, "UL -> AX L"); // Example: MEDDLE
    else if (p == 79)
      ChangeRule(pos, 27, "UM -> AX M"); // Example: ASTRONOMY
    else if (p == 80)
      ChangeRule(pos, 28, "UN -> AX N"); // Example: FUNCTION
    else if ((pf & FLAG_VOWEL) && stress[pos]) {
      // RULE:
      //       <STRESSED VOWEL> <SILENCE> <STRESSED VOWEL> -> <STRESSED VOWEL>
      //       <SILENCE> Q <VOWEL>
      // EXAMPLE: AWAY EIGHT
      if (!phonemeindex[pos + 1]) { // If following phoneme is a pause, get next
        p = phonemeindex[pos + 2];
        if (p != END && (flags[p] & FLAG_VOWEL) && stress[pos + 2]) {
          drule("Insert glottal stop between two stressed vowels with space "
                "between them");
          Insert(pos + 2, 31, 0, 0); // 31 = 'Q'
        }
      }
    } else if (p == pR) { // RULES FOR PHONEMES BEFORE R
      if (prior == pT)
        change(pos - 1, 42, "T R -> CH R"); // Example: TRACK
      else if (prior == pD)
        change(pos - 1, 44, "D R -> J R"); // Example: DRY
      else if (flags[prior] & FLAG_VOWEL)
        change(pos, 18, "<VOWEL> R -> <VOWEL> RX"); // Example: ART
    } else if (p == 24 && (flags[prior] & FLAG_VOWEL))
      change(pos, 19, "<VOWEL> L -> <VOWEL> LX"); // Example: ALL
    else if (prior == 60 && p == 32) {            // 'G' 'S'
      // Can't get to fire -
      //       1. The G -> GX rule intervenes
      //       2. Reciter already replaces GS -> GZ
      change(pos, 38, "G S -> G Z");
    } else if (p == 60)
      rule_g(pos);
    else {
      if (p == 72) { // 'K'
        // K <VOWEL OR DIPTHONG NOT ENDING WITH IY> -> KX <VOWEL OR DIPTHONG NOT
        // ENDING WITH IY> Example: COW
        unsigned char Y = phonemeindex[pos + 1];
        // If at end, replace current phoneme with KX
        if ((flags[Y] & FLAG_DIP_YX) == 0 ||
            Y == END) { // VOWELS AND DIPTHONGS ENDING WITH IY SOUND flag set?
          change(pos, 75,
                 "K <VOWEL OR DIPTHONG NOT ENDING WITH IY> -> KX <VOWEL OR "
                 "DIPTHONG NOT ENDING WITH IY>");
          p = 75;
          pf = flags[p];
        }
      }

      // Replace with softer version?
      if ((flags[p] & FLAG_PLOSIVE) && (prior == 32)) { // 'S'
        // RULE:
        //      S P -> S B
        //      S T -> S D
        //      S K -> S G
        //      S KX -> S GX
        // Examples: SPY, STY, SKY, SCOWL

        if (debug)
          printf("RULE: S* %c%c -> S* %c%c\n", signInputTable1[p],
                 signInputTable2[p], signInputTable1[p - 12],
                 signInputTable2[p - 12]);
        phonemeindex[pos] = p - 12;
      } else if (!(pf & FLAG_PLOSIVE)) {
        p = phonemeindex[pos];
        if (p == 53)
          rule_alveolar_uw(pos); // Example: NEW, DEW, SUE, ZOO, THOO, TOO
        else if (p == 42)
          rule_ch(pos); // Example: CHEW
        else if (p == 44)
          rule_j(pos); // Example: JAY
      }

      if (p == 69 || p == 57) { // 'T', 'D'
        // RULE: Soften T following vowel
        // NOTE: This rule fails for cases such as "ODD"
        //       <UNSTRESSED VOWEL> T <PAUSE> -> <UNSTRESSED VOWEL> DX <PAUSE>
        //       <UNSTRESSED VOWEL> D <PAUSE>  -> <UNSTRESSED VOWEL> DX <PAUSE>
        // Example: PARTY, TARDY
        if (flags[phonemeindex[pos - 1]] & FLAG_VOWEL) {
          p = phonemeindex[pos + 1];
          if (!p)
            p = phonemeindex[pos + 2];
          if ((flags[p] & FLAG_VOWEL) && !stress[pos + 1])
            change(pos, 30,
                   "Soften T or D following vowel or ER and preceding a pause "
                   "-> DX");
        }
      }
    }
    pos++;
  } // while
}

// Applies various rules that adjust the lengths of phonemes
//
//         Lengthen <FRICATIVE> or <VOICED> between <VOWEL> and <PUNCTUATION>
//         by 1.5 <VOWEL> <RX | LX> <CONSONANT> - decrease <VOWEL> length by 1
//         <VOWEL> <UNVOICED PLOSIVE> - decrease vowel by 1/8th
//         <VOWEL> <UNVOICED CONSONANT> - increase vowel by 1/2 + 1
//         <NASAL> <STOP CONSONANT> - set nasal = 5, consonant = 6
//         <VOICED STOP CONSONANT> {optional silence} <STOP CONSONANT> - shorten
//         both to 1/2 + 1 <LIQUID CONSONANT> <DIPTHONG> - decrease by 2
//
void AdjustLengths() {
  // LENGTHEN VOWELS PRECEDING PUNCTUATION
  //
  // Search for punctuation. If found, back up to the first vowel, then
  // process all phonemes between there and up to (but not including) the
  // punctuation. If any phoneme is found that is a either a fricative or
  // voiced, the duration is increased by (length * 1.5) + 1

  // loop index
  {
    unsigned char X = 0;
    unsigned char index;

    while ((index = phonemeindex[X]) != END) {
      unsigned char loopIndex;

      // not punctuation?
      if ((flags[index] & FLAG_PUNCT) == 0) {
        ++X;
        continue;
      }

      loopIndex = X;

      // Back up to the first vowel
      while (X > 0) {
          X--;  // Decrement X
          if (flags[phonemeindex[X]] & FLAG_VOWEL) {
              break;  // Exit the loop if a vowel is found
          }
      }

      if (X == 0)
        break;

      do {
        // test for vowel
        index = phonemeindex[X];

        // test for fricative/unvoiced or not voiced
        if (!(flags[index] & FLAG_FRICATIVE) || (flags[index] & FLAG_VOICED)) { // nochmal �berpr�fen
          unsigned char A = phonemeLength[X];
          // change phoneme length to (length * 1.5) + 1
          drule_pre("Lengthen <FRICATIVE> or <VOICED> between <VOWEL> and <PUNCTUATION> by 1.5", X);
          phonemeLength[X] = (A >> 1) + A + 1;
          drule_post(X);
        }
      } while (++X != loopIndex);
      X++;
    } // while
  }

  // Similar to the above routine, but shorten vowels under some circumstances

  // Loop through all phonemes
  unsigned char loopIndex = 0;
  unsigned char index;

  while ((index = phonemeindex[loopIndex]) != END) {
    unsigned char X = loopIndex;

    if (flags[index] & FLAG_VOWEL) {
      index = phonemeindex[loopIndex + 1];
      if (!(flags[index] & FLAG_CONSONANT)) {
        if ((index == 18) || (index == 19)) { // 'RX', 'LX'
          index = phonemeindex[loopIndex + 2];
          if ((flags[index] & FLAG_CONSONANT)) {
            drule_pre("<VOWEL> <RX | LX> <CONSONANT> - decrease length of "
                      "vowel by 1\n",
                      loopIndex);
            phonemeLength[loopIndex]--;
            drule_post(loopIndex);
          }
        }
      } else { // Got here if not <VOWEL>
        unsigned short flag;
        if (index == END) {
          flag = 65;
        } else {
          flag = flags[index];
        }


        if (!(flag & FLAG_VOICED)) { // Unvoiced
          // *, .*, ?*, ,*, -*, DX, S*, SH, F*, TH, /H, /X, CH, P*, T*, K*, KX
          if ((flag & FLAG_PLOSIVE)) { // unvoiced plosive
            // RULE: <VOWEL> <UNVOICED PLOSIVE>
            // <VOWEL> <P*, T*, K*, KX>
            drule_pre("<VOWEL> <UNVOICED PLOSIVE> - decrease vowel by 1/8th",
                      loopIndex);
            phonemeLength[loopIndex] -= (phonemeLength[loopIndex] >> 3);
            drule_post(loopIndex);
          }
        } else {
          unsigned char A;
          drule_pre("<VOWEL> <VOICED CONSONANT> - increase vowel by 1/2 + 1\n",
                    X - 1);
          // decrease length
          A = phonemeLength[loopIndex];
          phonemeLength[loopIndex] = (A >> 2) + A + 1; // 5/4*A + 1
          drule_post(loopIndex);
        }
      }
    } else if ((flags[index] & FLAG_NASAL) != 0) { // nasal?
      // RULE: <NASAL> <STOP CONSONANT>
      //       Set punctuation length to 6
      //       Set stop consonant length to 5
      index = phonemeindex[++X];
      if (index != END && (flags[index] & FLAG_STOPCONS)) {
        drule("<NASAL> <STOP CONSONANT> - set nasal = 5, consonant = 6");
        phonemeLength[X] = 6;     // set stop consonant length to 6
        phonemeLength[X - 1] = 5; // set nasal length to 5
      }
    } else if ((flags[index] & FLAG_STOPCONS)) { // (voiced) stop consonant?
      // RULE: <VOICED STOP CONSONANT> {optional silence} <STOP CONSONANT>
      //       Shorten both to (length/2 + 1)

      // move past silence
      while ((index = phonemeindex[++X]) == 0)
        ;

      if (index != END && (flags[index] & FLAG_STOPCONS)) {
        // FIXME, this looks wrong?
        // RULE: <UNVOICED STOP CONSONANT> {optional silence} <STOP CONSONANT>
        drule("<UNVOICED STOP CONSONANT> {optional silence} <STOP CONSONANT> - "
              "shorten both to 1/2 + 1");
        phonemeLength[X] = (phonemeLength[X] >> 1) + 1;
        phonemeLength[loopIndex] = (phonemeLength[loopIndex] >> 1) + 1;
        X = loopIndex;
      }
    } else if ((flags[index] & FLAG_LIQUIC)) { // liquic consonant?
      // RULE: <VOICED NON-VOWEL> <DIPTHONG>
      //       Decrease <DIPTHONG> by 2
      index = phonemeindex[X - 1]; // prior phoneme;

      // FIXME: The debug code here breaks the rule.
      // prior phoneme a stop consonant>
      if ((flags[index] & FLAG_STOPCONS) != 0)
        drule_pre("<LIQUID CONSONANT> <DIPTHONG> - decrease by 2", X);

      phonemeLength[X] -= 2; // 20ms
      drule_post(X);
    }

    ++loopIndex;
  }
}

static unsigned char RenderVoicedSample(unsigned short hi, unsigned char off, unsigned char phase1) {
  do {
    unsigned char bit = 8;
    unsigned char sample = sampleTable[hi + off];
    do {
      if ((sample & 128) != 0)
        Output(3, 26);
      else
        Output(4, 6);
      sample <<= 1;
    } while (--bit != 0);
    off++;
  } while (++phase1 != 0);
  return off;
}

static void RenderUnvoicedSample(unsigned short hi, unsigned char off, unsigned char mem53) {
  do {
    unsigned char bit = 8;
    unsigned char sample = sampleTable[hi + off];
    do {
      if ((sample & 128) != 0)
        Output(2, 5);
      else
        Output(1, mem53);
      sample <<= 1;
    } while (--bit != 0);
  } while (++off != 0);
}

// -------------------------------------------------------------------------
// Code48227
// Render a sampled sound from the sampleTable.
//
//   Phoneme   Sample Start   Sample End
//   32: S*    15             255
//   33: SH    257            511
//   34: F*    559            767
//   35: TH    583            767
//   36: /H    903            1023
//   37: /X    1135           1279
//   38: Z*    84             119
//   39: ZH    340            375
//   40: V*    596            639
//   41: DH    596            631
//
//   42: CH
//   43: **    399            511
//
//   44: J*
//   45: **    257            276
//   46: **
//
//   66: P*
//   67: **    743            767
//   68: **
//
//   69: T*
//   70: **    231            255
//   71: **
//
// The SampledPhonemesTable[] holds flags indicating if a phoneme is
// voiced or not. If the upper 5 bits are zero, the sample is voiced.
//
// Samples in the sampleTable are compressed, with bits being converted to
// bytes from high bit to low, as follows:
//
//   unvoiced 0 bit   -> X
//   unvoiced 1 bit   -> 5
//
//   voiced 0 bit     -> 6
//   voiced 1 bit     -> 24
//
// Where X is a value from the table:
//
//   { 0x18, 0x1A, 0x17, 0x17, 0x17 };
//
// The index into this table is determined by masking off the lower
// 3 bits from the SampledPhonemesTable:
//
//        index = (SampledPhonemesTable[i] & 7) - 1;
//
// For voices samples, samples are interleaved between voiced output.

void RenderSample(unsigned char *mem66_openBrace, unsigned char consonantFlag, unsigned char mem49) {
  // mem49 == current phoneme's index

  // mask low three bits and subtract 1 get value to
  // convert 0 bits on unvoiced samples.
  unsigned char hibyte = (consonantFlag & 7) - 1;

  // determine which offset to use from table { 0x18, 0x1A, 0x17, 0x17, 0x17 }
  // T, S, Z                0          0x18
  // CH, J, SH, ZH          1          0x1A
  // P, F*, V, TH, DH       2          0x17
  // /H                     3          0x17
  // /X                     4          0x17

  unsigned short hi = hibyte * 256;
  // voiced sample?
  unsigned char pitchl = consonantFlag & 248;
  if (pitchl == 0) {
    // voiced phoneme: Z*, ZH, V*, DH
    pitchl = pitches[mem49] >> 4;
    *mem66_openBrace = RenderVoicedSample(hi, *mem66_openBrace, pitchl ^ 255);
  } else
    RenderUnvoicedSample(hi, pitchl ^ 255, tab48426[hibyte]);
}

static void CombineGlottalAndFormants(unsigned char phase1, unsigned char phase2, unsigned char phase3, unsigned char Y) {
  unsigned int tmp;

  tmp = multtable[sinus[phase1] | amplitude1[Y]];
  tmp += multtable[sinus[phase2] | amplitude2[Y]];
  if (tmp > 255) {
    tmp += 1;
  } else {
    tmp += 0;
  }
  tmp += multtable[rectangle[phase3] | amplitude3[Y]];
  tmp += 136;
  tmp >>= 4; // Scale down to 0..15 range of C64 audio.

  Output(0, tmp & 0xf);
}

// PROCESS THE FRAMES
//
// In traditional vocal synthesis, the glottal pulse drives filters, which
// are attenuated to the frequencies of the formants.
//
// SAM generates these formants directly with sin and rectangular waves.
// To simulate them being driven by the glottal pulse, the waveforms are
// reset at the beginning of each glottal pulse.
//
void ProcessFrames(unsigned char mem48) {
  unsigned char speedcounter = 72;
  unsigned char phase1 = 0;
  unsigned char phase2 = 0;
  unsigned char phase3 = 0;
  unsigned char mem66_openBrace = 0; //!! was not initialized

  unsigned char Y = 0;

  unsigned char glottal_pulse = pitches[0];
  unsigned char mem38 = glottal_pulse - (glottal_pulse >> 2); // mem44 * 0.75

  while (mem48) {
    unsigned char flags = sampledConsonantFlag[Y];

    // unvoiced sampled phoneme?
    if (flags & 248) {
      RenderSample(&mem66_openBrace, flags, Y);
      // skip ahead two in the phoneme buffer
      Y += 2;
      mem48 -= 2;
      speedcounter = speed;
    } else {
      CombineGlottalAndFormants(phase1, phase2, phase3, Y);

      speedcounter--;
      if (speedcounter == 0) {
        Y++; // go to next amplitude
        // decrement the frame count
        mem48--;
        if (mem48 == 0)
          return;
        speedcounter = speed;
      }

      --glottal_pulse;

      if (glottal_pulse != 0) {
        // not finished with a glottal pulse

        --mem38;
        // within the first 75% of the glottal pulse?
        // is the count non-zero and the sampled flag is zero?
        if ((mem38 != 0) || (flags == 0)) {
          // reset the phase of the formants to match the pulse
          phase1 += frequency1[Y];
          phase2 += frequency2[Y];
          phase3 += frequency3[Y];
          continue;
        }

        // voiced sampled phonemes interleave the sample with the
        // glottal pulse. The sample flag is non-zero, so render
        // the sample for the phoneme.
        RenderSample(&mem66_openBrace, flags, Y);
      }
    }

    glottal_pulse = pitches[Y];
    mem38 = glottal_pulse - (glottal_pulse >> 2); // mem44 * 0.75

    // reset the formant wave generators to keep them in
    // sync with the glottal pulse
    phase1 = 0;
    phase2 = 0;
    phase3 = 0;
  }
}

// CREATE FRAMES
//
// The length parameter in the list corresponds to the number of frames
// to expand the phoneme to. Each frame represents 10 milliseconds of time.
// So a phoneme with a length of 7 = 7 frames = 70 milliseconds duration.
//
// The parameters are copied from the phoneme to the frame verbatim.
//
static void CreateFrames() {
  unsigned char X = 0;
  unsigned int i = 0;
  while (i < 256) {
    // get the phoneme at the index
    unsigned char phoneme = phonemeIndexOutput[i];
    unsigned char phase1;
    unsigned phase2;

    // if terminal phoneme, exit the loop
    if (phoneme == 255) {
      break;
    }

    if (phoneme == PHONEME_PERIOD) {
      AddInflection(RISING_INFLECTION, X);
    } else if (phoneme == PHONEME_QUESTION) {
      AddInflection(FALLING_INFLECTION, X);
    }

    // get the stress amount (more stress = higher pitch)
    phase1 = tab47492[stressOutput[i] + 1];

    // get number of frames to write
    phase2 = phonemeLengthOutput[i];

    // copy from the source to the frames list
    do {
      frequency1[X] = freq1data[phoneme]; // F1 frequency
      frequency2[X] = freq2data[phoneme]; // F2 frequency
      frequency3[X] = freq3data[phoneme]; // F3 frequency
      amplitude1[X] = ampl1data[phoneme]; // F1 amplitude
      amplitude2[X] = ampl2data[phoneme]; // F2 amplitude
      amplitude3[X] = ampl3data[phoneme]; // F3 amplitude
      sampledConsonantFlag[X] =
          sampledConsonantFlags[phoneme]; // phoneme data for sampled consonants
      pitches[X] = pitch + phase1;        // pitch

      ++X;
    } while (--phase2 != 0);

    ++i;
  }
}

// RESCALE AMPLITUDE
//
// Rescale volume from a linear scale to decibels.
//
void RescaleAmplitude() {
  int i;
  for (i = 255; i >= 0; i--) {
    amplitude1[i] = amplitudeRescale[amplitude1[i]];
    amplitude2[i] = amplitudeRescale[amplitude2[i]];
    amplitude3[i] = amplitudeRescale[amplitude3[i]];
  }
}

// ASSIGN PITCH CONTOUR
//
// This subtracts the F1 frequency from the pitch to create a
// pitch contour. Without this, the output would be at a single
// pitch level (monotone).

void AssignPitchContour() {
  int i;
  for (i = 0; i < 256; i++) {
    // subtract half the frequency of the formant 1.
    // this adds variety to the voice
    pitches[i] -= (frequency1[i] >> 1);
  }
}

void PrepareOutput() {
  unsigned char srcpos = 0;  // Position in source
  unsigned char destpos = 0; // Position in output

  while (1) {
    unsigned char A = phonemeindex[srcpos];
    phonemeIndexOutput[destpos] = A;
    switch (A) {
    case END:
      Render();
      return;
    case BREAK:
      phonemeIndexOutput[destpos] = END;
      Render();
      destpos = 0;
      break;
    case 0:
      break;
    default:
      phonemeLengthOutput[destpos] = phonemeLength[srcpos];
      stressOutput[destpos] = stress[srcpos];
      ++destpos;
      break;
    }
    ++srcpos;
  }
}

// Create a rising or falling inflection 30 frames prior to
// index X. A rising inflection is used for questions, and
// a falling inflection is used for statements.

void AddInflection(unsigned char inflection, unsigned char pos) {
  unsigned char A;
  // store the location of the punctuation
  unsigned char end = pos;

  if (pos < 30)
    pos = 0;
  else
    pos -= 30;

  // FIXME: Explain this fix better, it's not obvious
  // ML : A =, fixes a problem with invalid pitch with '.'
  while ((A = pitches[pos]) == 127)
    ++pos;

  while (pos != end) {
    // add the inflection direction
    A += inflection;

    // set the inflection
    pitches[pos] = A;

    while ((++pos != end) && pitches[pos] == 255)
      ;
  }
}

// RENDER THE PHONEMES IN THE LIST
//
// The phoneme list is converted into sound through the steps:
//
// 1. Copy each phoneme <length> number of times into the frames list,
//    where each frame represents 10 milliseconds of sound.
//
// 2. Determine the transitions lengths between phonemes, and linearly
//    interpolate the values across the frames.
//
// 3. Offset the pitches by the fundamental frequency.
//
// 4. Render the each frame.
void Render() {
  unsigned char t;

  if (phonemeIndexOutput[0] == 255)
    return; // exit if no data

  CreateFrames();
  t = CreateTransitions();

  if (!singmode)
    AssignPitchContour();
  RescaleAmplitude();

  if (debug) {
    PrintOutput(sampledConsonantFlag, frequency1, frequency2, frequency3,
                amplitude1, amplitude2, amplitude3, pitches);
  }

  ProcessFrames(t);
}

void SetInput(unsigned char *_input) {
  int i, l;
  l = strlen((char *)_input);
  if (l > 254)
    l = 254;
  for (i = 0; i < l; i++)
    input[i] = _input[i];
  input[l] = 0;
}

void SetSpeed(unsigned char _speed) { speed = _speed; }
void SetPitch(unsigned char _pitch) { pitch = _pitch; }
void SetMouth(unsigned char _mouth) { mouth = _mouth; }
void SetThroat(unsigned char _throat) { throat = _throat; }
void EnableSingmode() { singmode = 1; }
char *GetBuffer() { return buffer; }
int GetBufferLength() { return bufferpos; }

void Init() {
  int i;
  SetMouthThroat(mouth, throat);

  bufferpos = 0;
  // TODO, check for free the memory, 10 seconds of output should be more than
  // enough
  buffer = malloc(22050 * 10);

  for (i = 0; i < 256; i++) {
    stress[i] = 0;
    phonemeLength[i] = 0;
  }

  for (i = 0; i < 60; i++) {
    phonemeIndexOutput[i] = 0;
    stressOutput[i] = 0;
    phonemeLengthOutput[i] = 0;
  }
  phonemeindex[255] = END; // to prevent buffer overflow // ML : changed from 32
                           // to 255 to stop freezing with long inputs
}

int SAMMain() {
  unsigned char X = 0; //!! is this intended like this?
  Init();
  /* FIXME: At odds with assignment in Init() */
  phonemeindex[255] = 32; // to prevent buffer overflow

  if (!Parser1()) {
    return 0;
  }
  if (debug) {
    PrintPhonemes(phonemeindex, phonemeLength, stress);
  }
  Parser2();
  CopyStress();
  SetPhonemeLength();
  AdjustLengths();
  Code41240();
  do {
    if (phonemeindex[X] > 80) {
      phonemeindex[X] = END;
      break; // error: delete all behind it
    }
  } while (++X != 0);
  InsertBreath();

  if (debug)
    PrintPhonemes(phonemeindex, phonemeLength, stress);
  PrepareOutput();
  return 1;
}

// Iterates through the phoneme buffer, copying the stress value from
// the following phoneme under the following circumstance:

//     1. The current phoneme is voiced, excluding plosives and fricatives
//     2. The following phoneme is voiced, excluding plosives and fricatives,
//     and
//     3. The following phoneme is stressed
//
//  In those cases, the stress value+1 from the following phoneme is copied.
//
// For example, the word LOITER is represented as LOY5TER, with as stress
// of 5 on the dipthong OY. This routine will copy the stress value of 6 (5+1)
// to the L that precedes it.

void CopyStress() {
  // loop thought all the phonemes to be output
  unsigned char pos = 0; // mem66_openBrace
  unsigned char Y;
  while ((Y = phonemeindex[pos]) != END) {
    // if CONSONANT_FLAG set, skip - only vowels get stress
    if (flags[Y] & 64) {
      Y = phonemeindex[pos + 1];

      // if the following phoneme is the end, or a vowel, skip
      if (Y != END && (flags[Y] & 128) != 0) {
        // get the stress value at the next position
        Y = stress[pos + 1];
        if (Y && !(Y & 128)) {
          // if next phoneme is stressed, and a VOWEL OR ER
          // copy stress from next phoneme to this one
          stress[pos] = Y + 1;
        }
      }
    }

    ++pos;
  }
}

void Insert(unsigned char position /*var57*/, unsigned char mem60_inputMatchPos, unsigned char mem59, unsigned char mem58_variant) {
  int i;
  for (i = 253; i >= position; i--) // ML : always keep last safe-guarding 255
  {
    phonemeindex[i + 1] = phonemeindex[i];
    phonemeLength[i + 1] = phonemeLength[i];
    stress[i + 1] = stress[i];
  }

  phonemeindex[position] = mem60_inputMatchPos;
  phonemeLength[position] = mem59;
  stress[position] = mem58_variant;
}

void InsertBreath() {
  unsigned char mem54 = 255;
  unsigned char len = 0;
  unsigned char index; // variable Y

  unsigned char pos = 0;

  while ((index = phonemeindex[pos]) != END) {
    len += phonemeLength[pos];
    if (len < 232) {
      if (index == BREAK) {
      } else if (!(flags[index] & FLAG_PUNCT)) {
        if (index == 0)
          mem54 = pos;
      } else {
        len = 0;
        Insert(++pos, BREAK, 0, 0);
      }
    } else {
      pos = mem54;
      phonemeindex[pos] = 31; // 'Q*' glottal stop
      phonemeLength[pos] = 4;
      stress[pos] = 0;

      len = 0;
      Insert(++pos, BREAK, 0, 0);
    }
    ++pos;
  }
}

void drule(const char *str) {
  if (debug)
    printf("RULE: %s\n", str);
}

void drule_pre(const char *descr, unsigned char X) {
  drule(descr);
  if (debug) {
    printf("PRE\n");
    printf("phoneme %d (%c%c) length %d\n", X, signInputTable1[phonemeindex[X]],
           signInputTable2[phonemeindex[X]], phonemeLength[X]);
  }
}

void drule_post(unsigned char X) {
  if (debug) {
    printf("POST\n");
    printf("phoneme %d (%c%c) length %d\n", X, signInputTable1[phonemeindex[X]],
           signInputTable2[phonemeindex[X]], phonemeLength[X]);
  }
}

// Rewrites the phonemes using the following rules:
//
//       <DIPTHONG ENDING WITH WX> -> <DIPTHONG ENDING WITH WX> WX
//       <DIPTHONG NOT ENDING WITH WX> -> <DIPTHONG NOT ENDING WITH WX> YX
//       UL -> AX L
//       UM -> AX M
//       <STRESSED VOWEL> <SILENCE> <STRESSED VOWEL> -> <STRESSED VOWEL>
//       <SILENCE> Q <VOWEL> T R -> CH R D R -> J R <VOWEL> R -> <VOWEL> RX
//       <VOWEL> L -> <VOWEL> LX
//       G S -> G Z
//       K <VOWEL OR DIPTHONG NOT ENDING WITH IY> -> KX <VOWEL OR DIPTHONG NOT
//       ENDING WITH IY> G <VOWEL OR DIPTHONG NOT ENDING WITH IY> -> GX <VOWEL
//       OR DIPTHONG NOT ENDING WITH IY> S P -> S B S T -> S D S K -> S G S KX
//       -> S GX <ALVEOLAR> UW -> <ALVEOLAR> UX CH -> CH CH' (CH requires two
//       phonemes to represent it) J -> J J' (J requires two phonemes to
//       represent it) <UNSTRESSED VOWEL> T <PAUSE> -> <UNSTRESSED VOWEL> DX
//       <PAUSE> <UNSTRESSED VOWEL> D <PAUSE>  -> <UNSTRESSED VOWEL> DX <PAUSE>

void rule_alveolar_uw(unsigned char X) {
  // ALVEOLAR flag set?
  if (flags[phonemeindex[X - 1]] & FLAG_ALVEOLAR) {
    drule("<ALVEOLAR> UW -> <ALVEOLAR> UX");
    phonemeindex[X] = 16;
  }
}

void rule_ch(unsigned char X) {
  drule("CH -> CH CH+1");
  Insert(X + 1, 43, 0, stress[X]);
}

void rule_j(unsigned char X) {
  drule("J -> J J+1");
  Insert(X + 1, 45, 0, stress[X]);
}

void rule_g(unsigned char pos) {
  // G <VOWEL OR DIPTHONG NOT ENDING WITH IY> -> GX <VOWEL OR DIPTHONG NOT
  // ENDING WITH IY> Example: GO

  unsigned char index = phonemeindex[pos + 1];

  // If dipthong ending with YX, move continue processing next phoneme
  if ((index != 255) && ((flags[index] & FLAG_DIP_YX) == 0)) {
    // replace G with GX and continue processing next phoneme
    drule("G <VOWEL OR DIPTHONG NOT ENDING WITH IY> -> GX <VOWEL OR DIPTHONG "
          "NOT ENDING WITH IY>");
    phonemeindex[pos] = 63; // 'GX'
  }
}

void change(unsigned char pos, unsigned char val, const char *rule) {
  drule(rule);
  phonemeindex[pos] = val;
}

void rule_dipthong(unsigned char p, unsigned short pf, unsigned char pos) {
  // <DIPTHONG ENDING WITH WX> -> <DIPTHONG ENDING WITH WX> WX
  // <DIPTHONG NOT ENDING WITH WX> -> <DIPTHONG NOT ENDING WITH WX> YX
  // Example: OIL, COW

  // If ends with IY, use YX, else use WX
  unsigned char A;
  if (pf & FLAG_DIP_YX) {
    A = 21;
  } else {
    A = 20;
  }
  // Insert at WX or YX following, copying the stress
  if (A == 20)
    drule("insert WX following dipthong NOT ending in IY sound");
  else if (A == 21)
    drule("insert YX following dipthong ending in IY sound");
  Insert(pos + 1, A, 0, stress[pos]);

  if (p == 53)
    rule_alveolar_uw(pos); // Example: NEW, DEW, SUE, ZOO, THOO, TOO
  else if (p == 42)
    rule_ch(pos); // Example: CHEW
  else if (p == 44)
    rule_j(pos); // Example: JAY
}

/*
    SAM's voice can be altered by changing the frequencies of the
    mouth formant (F1) and the throat formant (F2). Only the voiced
    phonemes (5-29 and 48-53) are altered.
*/
void SetMouthThroat(unsigned char mouth, unsigned char throat) {
  // mouth formants (F1) 5..29
  static const unsigned char mouthFormants5_29[30] = {
      0,  0,  0,  0,  0,  10, 14, 19, 24, 27, 23, 21, 16, 20, 14,
      18, 14, 18, 18, 16, 13, 15, 11, 18, 14, 11, 9,  6,  6,  6};

  // throat formants (F2) 5..29
  static const unsigned char throatFormants5_29[30] = {
      255, 255, 255, 255, 255, 84, 73, 67, 63, 40, 44, 31, 37, 45, 73,
      49,  36,  30,  51,  37,  29, 69, 24, 50, 30, 24, 83, 46, 54, 86,
  };

  // there must be no zeros in this 2 tables
  // formant 1 frequencies (mouth) 48..53
  static const unsigned char mouthFormants48_53[6] = {19, 27, 21, 27, 18, 13};

  // formant 2 frequencies (throat) 48..53
  static const unsigned char throatFormants48_53[6] = {72, 39, 31, 43, 30, 34};

  unsigned char newFrequency = 0;
  unsigned char pos = 5;

  // recalculate formant frequencies 5..29 for the mouth (F1) and throat (F2)
  while (pos < 30) {
    // recalculate mouth frequency
    unsigned char initialFrequency = mouthFormants5_29[pos];
    if (initialFrequency != 0) {
      newFrequency = trans(mouth, initialFrequency);
    }
    freq1data[pos] = newFrequency;

    // recalculate throat frequency
    initialFrequency = throatFormants5_29[pos];
    if (initialFrequency != 0) {
      newFrequency = trans(throat, initialFrequency);
    }
    freq2data[pos] = newFrequency;
    pos++;
  }

  // recalculate formant frequencies 48..53
  pos = 0;
  while (pos < 6) {
    // recalculate F1 (mouth formant)
    unsigned char initialFrequency = mouthFormants48_53[pos];
    freq1data[pos + 48] = trans(mouth, initialFrequency);

    // recalculate F2 (throat formant)
    initialFrequency = throatFormants48_53[pos];
    freq2data[pos + 48] = trans(throat, initialFrequency);
    pos++;
  }
}

void WriteWav(char *filename, char *buffer, int bufferlength) {
  unsigned int filesize;
  unsigned int fmtlength = 16;
  unsigned short int format = 1; // PCM
  unsigned short int channels = 1;
  unsigned int samplerate = 22050;
  unsigned short int blockalign = 1;
  unsigned short int bitspersample = 8;

  FILE *file;
  fopen_s(&file, filename, "wb");
  if (file == NULL)
    return;
  // RIFF header
  fwrite("RIFF", 4, 1, file);
  filesize = bufferlength + 12 + 16 + 8 - 8;
  fwrite(&filesize, 4, 1, file);
  fwrite("WAVE", 4, 1, file);

  // format chunk
  fwrite("fmt ", 4, 1, file);
  fwrite(&fmtlength, 4, 1, file);
  fwrite(&format, 2, 1, file);
  fwrite(&channels, 2, 1, file);
  fwrite(&samplerate, 4, 1, file);
  fwrite(&samplerate, 4, 1, file); // bytes/second
  fwrite(&blockalign, 2, 1, file);
  fwrite(&bitspersample, 2, 1, file);

  // data chunk
  fwrite("data", 4, 1, file);
  fwrite(&bufferlength, 4, 1, file);
  fwrite(buffer, bufferlength, 1, file);

  fclose(file);
}

#ifdef USESDL

void MixAudio(void *unused, Uint8 *stream, int len) {
  int bufferpos = GetBufferLength();
  char *buffer = GetBuffer();
  int i;
  if (pos >= bufferpos)
    return;
  if ((bufferpos - pos) < len)
    len = (bufferpos - pos);
  for (i = 0; i < len; i++) {
    stream[i] = buffer[pos];
    pos++;
  }
}

void OutputSound() {
  int bufferpos = GetBufferLength();
  bufferpos /= 50;
  SDL_AudioSpec fmt;
  fmt.freq = 22050;
  fmt.format = AUDIO_U8;
  fmt.channels = 1;
  fmt.samples = 2048;
  fmt.callback = MixAudio;
  fmt.userdata = NULL;

  /* Open the audio device and start playing sound! */
  if (SDL_OpenAudio(&fmt, NULL) < 0) {
    printf("Unable to open audio: %s\n", SDL_GetError());
    exit(1);
  }
  SDL_PauseAudio(0);

  while (pos < bufferpos) {
    SDL_Delay(100);
  }

  SDL_CloseAudio();
}
#else
void OutputSound() {}
#endif
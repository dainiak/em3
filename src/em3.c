//(c) Alex Dainiak, http://www.dainiak.com
// This file is stored in OEM866 encoding
// Compiles successfully under Borland C++ 3.1

#include <stdio.h>     // Standard
#include <conio.h>     // libraries
#include <string.h>    //
#include <stdlib.h>    //
#include <ctype.h>     // For isspace() function
#include <dos.h>       // For delay(), sound() and nosound()

#define NUM_OF_OPTS 8  // Number of elements in options[] array

enum en_bools          // Enumeration of boolean values
{
  B_NO,
  B_YES
};

enum en_snd            // Enmeration of sound types
{
  S_CLICK,
  S_BEEP
};

enum en_errors         // Error enumeration
{
  NO_ERROR,            // No error occured during execution
  DIV_BY_ZERO,         // Division by zero
  FLOAT_NAN,           // Trying to perform arithmetic operation with NAN
  INT_OVERFLOW,        // Integer overflow
  CONVERT_ERROR,       // Error occured while converting integer to float
  NO_SUCH_COMMAND,     // An attempt to execute non-existing command
  IO_ERROR,            // An Input/Output error have occured
  IN_ERROR,            // Illegal input format
  REACHED_THE_END,     // RA = 511, and next command is undetermined
  INTERRUPTED_BY_USER, // Program was interrupted by user
  NORMAL_TERMINATION   // Program stopped on command 31 (successfuly)
};

enum en_ch_handlers    // Context help elements enumeration
{
  CH_ERROR,            // Errors block start in file "help.dat"
  CH_OPTIONS = 20,     // Options block start
  CH_EDIT_WINDOW = 40, // Edit window context help handler
  CH_FILE_WINDOW,      // File window context help handler
  CH_DIAG_WINDOW,      // Diag window context help handler
  CH_XTND_WINDOW,      // Additional info window help hendler
  CH_DEFINE_FLOAT = 50,// Float definition in editor help
  CH_DEFINE_INT,       // Integer definition in editor help
  CH_MSG_CANTOPEN = 60,// Couldn't open file
  CH_MSG_CANTCREATE,   // Couldn't create file
  CH_MSG_CANTUPDATE,   // Couldn't update file
  CH_MSG_WRONGINPUT,   // Wrong input
  CH_QUERY_LOAD = 80,  // Query for open file path
  CH_QUERY_SAVE,       // Query for save file path
  CH_QUERY_SAVETXT,    // Query for save txt-file path
  CH_QUERY_FLOAT,      // Query for float number
  CH_QUERY_INT         // Query for integer number
};

enum en_options        // Options enumeration
{
  Z_BEHAVIOUR,         // Should it always be 0 at address [000] or not
  LOOP_MEM,            // Loop memory
  RECOVER_MEM,         // Recover memory after program execution
  SHOW_DESC_IN_EDIT,   // Show command descriptions while editing
  SHOW_EXACT_ADDRESS,  // Show exact addresses in prompts
  USE_SMART2,          // Use smart insert/delete functions
  STEPPED_OUTPUT,      // Always perform step-by-step output
  SOUND
};

long int mWords[512];      // The whole EM3 memory
long int RK, R1, R2, S;    // EM3 registers
int RA;                    //
int omega;                 //
int err;                   //

int ioservice;             // Variable for handling input errors
                           // and formatting output
int stepmode = 0;          // Step-by-step execution: 0-off, 1-on
int cmdmode = 1;           // Command displaying mode: 0-mnemonic, 1-standard
int ismodflag = 0;         // 0 - document wasn't modified, 1 - otherwise

int options[NUM_OF_OPTS] = // Default options boolean array
{
  1, //zbehaviour          // (See descriptions in EN_OPTIONS enum)
  0, //loopmemory          //
  0, //recovermemory       //
  0, //showdescinedit      //
  1, //showexactaddress    //
  1, //usesmart2           //
  0, //steppedoutput       //
  1  //sound               //
};

int curaddress;            // Current first line in Editor window
int ex = 6, ey = 1;        // Cursor coodrinates in Editor window

char inputfile[80];        //
char* em3path;
char visiblepath[27] = ""; // Path shown in Current File window
FILE* helpfile = NULL;     // Context help dat-file pointer
char* descarr[32];         // Array of commnd descriptions
char* aliasarr[32];        // Array of mnemonic command names


enum en_keys               // Control keys enumeration
{
  KEY_UNKNOWN = 300,
  KEY_F1,
  KEY_F2,
  KEY_F3,
  KEY_F4,
  KEY_F5,
  KEY_F6,
  KEY_F7,
  KEY_F8,
  KEY_F9,
  KEY_F10,
  KEY_LEFT,
  KEY_RIGHT,
  KEY_UP,
  KEY_DOWN,
  KEY_HOME,
  KEY_END,
  KEY_PGUP,
  KEY_PGDN,
  KEY_INS,
  KEY_DEL,
  KEY_BKSP,
  KEY_ENTER,
  KEY_ESC,
  KEY_CTRL_INS,
  KEY_CTRL_DEL,
  KEY_ALT_I,
  KEY_ALT_F,
  KEY_CTRL_F2,
  KEY_ALT_F2,
  KEY_CTRL_F9,
  KEY_TAB
};

//////////////////////////////////////////////////////////////////////////////
//Machine words handling functions:///////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
int GetRKOpcode( void )
{
  return ( RK >> 27 ) & 037;
}

int GetOpcode( int address )
{
  return ( mWords[address] >> 27 ) & 037;
}

int GetWordOpcode( long int word )
{
  return ( word >> 27 ) & 037;
}

int GetRKAddress( int n )
{
  return ( RK >> ( 3 - n ) * 9 ) & 0777;
}

int GetAddress( int address, int n )
{
  return ( mWords[address] >> ( 3 - n ) * 9 ) & 0777;
}

int GetWordAddress( long int word, int n )
{
  return ( word >> ( 3 - n ) * 9 ) & 0777;
}

//////////////////////////////////////////////////////////////////////////////
//Functions for direct conversion between int and float://////////////////////
//////////////////////////////////////////////////////////////////////////////
float IntToFloat( long int integer )
{
  float* pfloat;
  pfloat = ( float* )( ( void* )( &integer ) );

  return *pfloat;
}

long int FloatToInt( float floating )
{
  long int* pint;
  pint = ( long int* )( ( void* )( &floating ) );

  return *pint;
}

//////////////////////////////////////////////////////////////////////////////
//Functions for checking overflows in arithmetic operations:  !MAY BE BUGGY!//
//////////////////////////////////////////////////////////////////////////////
int CheckAddIntValidity( long int operand1, long int operand2 )
{
  if( operand1 >= 0 && operand2 <= 0 || operand1 <= 0 && operand2 >= 0 )
    return 1;

  if( operand1 > 0 )
    if( 2147483647 - operand1 >= operand2 )
      return 1;
    else
      return 0;
  else
    if( -2147483648 - operand1 <= operand2 )
      return 1;
    else
      return 0;
}

int CheckSubIntValidity( long int operand1, long int operand2 )
{
  if( operand1 >= 0 && operand2 >= 0 || operand1 <= 0 && operand2 <= 0 )
    return 1;

  if( operand1 > 0 )
    if(( operand2 != -2147483648 ) && ( 2147483647 - operand1 >= -operand2 ))
      return 1;
    else
      return 0;
  else
    if( -2147483648 - operand1 <= -operand2 )
      return 1;
    else
      return 0;
}

int CheckMultIntValidity( long int operand1, long int operand2 )
{
  double tmpdbl1, tmpdbl2;

  if( operand1 < 0 ) operand1 = -operand1;
  if( operand2 < 0 ) operand2 = -operand2;

  if( operand1 < 1 || operand2 < 1 )
    return 1;
  else
  {
    if( operand1 < operand2 )
    {
      tmpdbl1 = operand2;
      tmpdbl2 = operand1;
    }
    else
    {
      tmpdbl1 = operand1;
      tmpdbl2 = operand2;
    }

    if( tmpdbl1 < 2147483647.0 / tmpdbl2 )
      return 1;
  }

  return 0;
}

int IsNAN( long int tmpint )
{
  return ( tmpint == 2143289344 || tmpint == -4194304 || tmpint == -1 );
}

int IsInfinite(  long int tmpint )
{
  return ( tmpint == 2139095040 || tmpint == -8388608 );
}

//////////////////////////////////////////////////////////////////////////////
//Omega register updating:////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void ChangeIntOmega( void )
{
  if( S == 0 )
    omega = 0;
  else if( S < 0 )
    omega = 1;
  else
    omega = 2;
}

void ChangeFloatOmega( void )
{
  float S1 = IntToFloat( S );

  if( S1 == 0 )
    omega = 0;
  else if( S1 < 0 )
    omega = 1;
  else
    omega = 2;
}

//////////////////////////////////////////////////////////////////////////////
//GetChar function:///////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
int GetChar( void )
{
  int keys[] =
  {
    359, 360, 361, 362, 363, 364, 365, 366, 367, 368,
    375, 377, 372, 380,
    371, 379,
    373, 381,
    382, 383,
    8, 13, 27,
    446, 447,
    323, 333,
    395, 405, 402,
    9
  };

  int c, i;

  c = getch();

  if( !c )
    c = 300 + getch();

  for( i = 0; i < 31; i++ )
    if( keys[i] == c )
      return i + 301;

  if( c > 300 )
    return 300;

  return c;
}

//////////////////////////////////////////////////////////////////////////////
//Data loading service functions://///////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
char* FGetString( FILE* tmpfl )
{
  char tmpstr[80];
  char* tmpres;
  int i = 0;

  fgets( tmpstr, 80, tmpfl );

  while( tmpstr[i] && tmpstr[i] != '\n' )
    i++;

  tmpstr[i] = '\0';
  tmpres = ( char* )malloc( i + 1 );

  for( ; i >= 0; i-- )
    if( tmpstr[i] == 'n' && tmpstr[i-1] == '\\' )
    {
      tmpres[i] = '\r';
      i--;
      tmpres[i] = '\n';
    }
    else
      tmpres[i] = tmpstr[i];

  return tmpres;
}

//////////////////////////////////////////////////////////////////////////////
//Additional string handling functions:///////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
int IsEmpty( char* str )
{
  int i = 0;

  while( str[i] && isspace( str[i] ) )
    i++;

  return !str[i];
}

//////////////////////////////////////////////////////////////////////////////
//Initialization functions:///////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void Initialize( void )
{
  R1 = 0;
  R2 = 0;
  S = 0;
  RK = 0;
  RA = 1;
  omega = 0;
  err = NO_ERROR;
  mWords[0] = 0;
  ioservice = 11;
  window( 28, 11, 49, 14 );
  clrscr();
  window( 28, 16, 49, 23 );
  clrscr();
}

void InitFirstTime( void )
{
  int i;

  for( i = 0; i < 512; i++ )
    mWords[i] = 0;

  Initialize();
  curaddress = 1;
}

void PerformExit( int code )
{
  int i;

  if( em3path )
    free( em3path );

  for( i = 0; i < 32; i++ )
  {
    if( aliasarr[i] )
      free( aliasarr[i] );

    if( descarr[i] )
      free( descarr[i] );
  }

  exit( code );
}

void LoadData( void )
{
  int i = 0;
  int pathlen;
  FILE* tmpfl = NULL;

  pathlen = strlen( em3path );
  strcat( em3path, "comdesc.dat" );
  tmpfl = fopen( em3path, "r" );

  if( !tmpfl )
  {
    clrscr();
    puts( "î†©´ ÆØ®c†≠®© ™Æ¨†≠§ ≠• ≠†©§•≠\nÑ´Ô ¢ÎÂÆ§† ≠†¶¨®‚• ´Ó°yÓ ™´†¢®Ëy" );
    getch();
    PerformExit( 1 );
  }

  for( i = 0; i < 32; i++ )
    descarr[i] = FGetString( tmpfl );

  fclose( tmpfl );
  em3path[pathlen] = '\0';
  strcat( em3path, "mnem.dat" );
  tmpfl = fopen( em3path, "r" );

  if( !tmpfl )
  {
    clrscr();
    puts( "î†©´ ¨≠•¨Æ≠®Á•c™®Â ®¨•≠ ≠• ≠†©§•≠\nÑ´Ô ¢ÎÂÆ§† ≠†¶¨®‚• ´Ó°yÓ ™´†¢®Ëy" );
    getch();
    PerformExit( 1 );
  }

  for( i = 0; i < 32; i++ )
    aliasarr[i] = FGetString( tmpfl );

  fclose( tmpfl );
  em3path[pathlen] = '\0';
  strcat( em3path, "options.dat" );
  tmpfl = fopen( em3path, "r" );

  if( !tmpfl )
  {
    clrscr();
    puts( "î†©´ ÆØÊ®© ≠• ≠†©§•≠\nÅ„§„‚ „·‚†≠Æ¢´•≠Î ·‚†≠§†‡‚≠Î• ÆØÊ®®\nÑ´Ô ØpÆ§Æ´¶•≠®Ô ≠†¶¨®‚• ´Ó°yÓ ™´†¢®Ëy" );
    getch();
  }
  else
  {
    for( i = 0; i < NUM_OF_OPTS; i++ )
    {
      while( fgetc( tmpfl ) != '\n' && !feof( tmpfl ));

      switch( fgetc( tmpfl ) )
      {
        case '+': options[i] = 1; break;
        case '-': options[i] = 0; break;
      }

      fgetc( tmpfl );
    }

    fclose( tmpfl );
  }

  em3path[pathlen] = '\0';
  strcat( em3path, "help.dat" );
  helpfile = fopen( em3path, "r" );

  if( !helpfile )
  {
    clrscr();
    puts( "î†©´ ØÆ¨ÆÈ® ≠• ≠†©§•≠\näÆ≠‚•™·‚≠†Ô ·Ø‡†¢™† °„§•‚ ≠•§Æ·‚„Ø≠†\nÑ´Ô ØpÆ§Æ´¶•≠®Ô ≠†¶¨®‚• ´Ó°yÓ ™´†¢®Ëy" );
    getch();
  }

  em3path[pathlen] = '\0';
} //LoadData

//////////////////////////////////////////////////////////////////////////////
//High quality sound:)////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void Sound( int type )
{
  if( options[SOUND] )
  {
    sound( 1000 );

    if( type == S_BEEP )
      delay( 400 );
    else
      delay( 50 );

    nosound();
  }
}

//////////////////////////////////////////////////////////////////////////////
//Functions for working in mnemonic mode://///////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
char* CodeToAlias( int code )
{
  return aliasarr[code];
}

int AliasToCode( char* alias )
{
  int i = 0;

  while( ( aliasarr[i][0] != alias[0] || aliasarr[i][1] != alias[1] ||
                              aliasarr[i][2] != alias[2] ) && i <= 31 ) i++;

  return i;
}

//////////////////////////////////////////////////////////////////////////////
//Screen updating:////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void DrawMainWindow( void )
{
  clrscr();
  textmode(3);
  textattr( 0x10 );
  textcolor( YELLOW );
  cputs("\n\n\
 …ÕÄÑêÕÕäéèÕÄ1ÕÕÄ2ÕÕÄ3ÕÕª …ÕêÖÉÕÕäéèÕÄ1ÕÕÄ2ÕÕÄ3ÕÕª …ÕÕÕÕÕÕÕíÖäìôàâ îÄâãÕÕÕÕÕÕÕª \
 ∫ 001   00 000 000 000 ∫ ∫ R1    00 000 000 000 ∫ ∫                          ∫ \
 ∫ 002   00 000 000 000 ∫ ∫ R2    00 000 000 000 ∫ »ÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕº \
 ∫ 003   00 000 000 000 ∫ ∫ S     00 000 000 000 ∫ …ÕÕÕÕÕÕÕÕÑàÄÉHéCíàäÄÕÕÕÕÕÕÕª \
 ∫ 004   00 000 000 000 ∫ ∫ RK    00 000 000 000 ∫ ∫                          ∫ \
 ∫ 005   00 000 000 000 ∫ ÃÕÕÕÕÕÕÕÕÀÕÕÕÕÕÀÕÕÕÕÕÕÕπ ∫                          ∫ \
 ∫ 006   00 000 000 000 ∫ ∫ RA:000 ∫ w:0 ∫ Err:0 ∫ ∫                          ∫ \
 ∫ 007   00 000 000 000 ∫ »ÕÕÕÕÕÕÕÕ ÕÕÕÕÕ ÕÕÕÕÕÕÕº ∫                          ∫ \
 ∫ 008   00 000 000 000 ∫ …ÕÕÕÕÕÕÕÕÕÇÇéÑÕÕÕÕÕÕÕÕÕª ∫                          ∫ \
 ∫ 009   00 000 000 000 ∫ ∫                      ∫ ∫                          ∫ \
 ∫ 010   00 000 000 000 ∫ ∫                      ∫ ∫                          ∫ \
 ∫ 011   00 000 000 000 ∫ ∫                      ∫ ∫                          ∫ \
 ∫ 012   00 000 000 000 ∫ ∫                      ∫ ∫                          ∫ \
 ∫ 013   00 000 000 000 ∫ ÃÕÕÕÕÕÕÕÕÕÇõÇéÑÕÕÕÕÕÕÕÕπ ÃÕÕÕÕÕÕÕÑéèéãHàíÖãúHéÕÕÕÕÕÕπ \
 ∫ 014   00 000 000 000 ∫ ∫                      ∫ ∫                          ∫ \
 ∫ 015   00 000 000 000 ∫ ∫                      ∫ ∫                          ∫ \
 ∫ 016   00 000 000 000 ∫ ∫                      ∫ ∫                          ∫ \
 ∫ 017   00 000 000 000 ∫ ∫                      ∫ ∫                          ∫ \
 ∫ 018   00 000 000 000 ∫ ∫                      ∫ ∫                          ∫ \
 ∫ 019   00 000 000 000 ∫ ∫                      ∫ ∫                          ∫ \
 ∫ 020   00 000 000 000 ∫ ∫                      ∫ ∫                          ∫ \
 ∫ 021   00 000 000 000 ∫ ∫                      ∫ ∫                          ∫ \
 »ÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕº »ÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕº »ÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕº \
 ÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕ");
}

void StatusMessage( char* message )
{
  window( 1, 1, 80, 25 );
  gotoxy( 2, 25 );
  cprintf( "ÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕ");
  gotoxy( 3, 25 );
  cprintf( "%s ", message );
}

void ShowHelp( char* message )
{
  window( 53, 16, 78, 23 );
  clrscr();
  gotoxy( 1, 1 );
  cputs( message );
}

void GenerateDescription( int command, char* tmpstr )
{
  char* desc = descarr[GetOpcode( command )];
  int tmpaddress, singles, hundreds, tens, i, j;

  for( i = 0, j = 0; i < 80 && desc[i]; i++, j++ )
    if( desc[i] != 'A' )
      tmpstr[j] = desc[i];
    else
    {
      i++;
      tmpaddress = GetAddress( command, desc[i] - 48 );
      singles = tmpaddress % 10;
      tens = ( tmpaddress / 10 ) % 10;
      hundreds = tmpaddress / 100;
      tmpstr[j] = hundreds + 48;
      j++;
      tmpstr[j] = tens + 48;
      j++;
      tmpstr[j] = singles + 48;
    }

  tmpstr[j] = 0;
}

void UpdateRegisters( void )
{
  window( 35, 3, 48, 8 );
  gotoxy( 1, 1 );
  cprintf( "%02d %03d %03d %03d", GetWordOpcode( R1 ),
                                  GetWordAddress( R1, 1 ),
                                  GetWordAddress( R1, 2 ),
                                  GetWordAddress( R1, 3 ));
  gotoxy( 1, 2 );
  cprintf( "%02d %03d %03d %03d", GetWordOpcode( R2 ),
                                  GetWordAddress( R2, 1 ),
                                  GetWordAddress( R2, 2 ),
                                  GetWordAddress( R2, 3 ));
  gotoxy( 1, 3 );
  cprintf( "%02d %03d %03d %03d", GetWordOpcode( S ),
                                  GetWordAddress( S, 1 ),
                                  GetWordAddress( S, 2 ),
                                  GetWordAddress( S, 3 ));
  gotoxy( 1, 4 );
  cprintf( "%02d %03d %03d %03d", GetWordOpcode( RK ),
                                  GetWordAddress( RK, 1 ),
                                  GetWordAddress( RK, 2 ),
                                  GetWordAddress( RK, 3 ));
  window( 1, 1, 80, 25 );
  gotoxy( 32, 8 );
  cprintf( "%03d", RA );
  gotoxy( 40, 8 );
  cprintf( "%01d", omega );
  gotoxy( 48, 8 );
  cprintf( "%01d", ( err != 0 ) );
}

void PrintLine( int address )
{
  if( cmdmode == 1 )
    cprintf( "%03d   %02d %03d %03d %03d", address,
                                           GetOpcode( address ),
                                           GetAddress( address, 1 ),
                                           GetAddress( address, 2 ),
                                           GetAddress( address, 3 ));
  else
    cprintf( "%03d  %s %03d %03d %03d",    address,
                                           CodeToAlias( GetOpcode( address )),
                                           GetAddress( address, 1 ),
                                           GetAddress( address, 2 ),
                                           GetAddress( address, 3 ));
}

void UpdateMemory( void )
{
  int i;

  window( 4, 3, 23, 24 );

  for( i = 0; i < 21; i++ )
  {
    gotoxy( 1, i + 1 );
    PrintLine( curaddress + i );
  }
}

void MarkLine( void )
{
  curaddress = RA - 10;

  if( curaddress < 1 )
    curaddress = 1;

  if( curaddress > 491 )
    curaddress = 491;

  UpdateMemory();
  window( 4, 3, 23, 24 );
  ex = 6 + cmdmode;
  ey = RA - curaddress + 1;
  gotoxy( 1, ey );
  textcolor( BLACK );
  textbackground( LIGHTGRAY );
  PrintLine( RA );
  textattr( 0x10 );
  textcolor( YELLOW );
}

//////////////////////////////////////////////////////////////////////////////
//Functions for context help support://///////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void LoadCHDescription( int dcode, char* dest )
{
  if( helpfile != NULL )
  {
    int tempcode = -2;

    fseek( helpfile, 0, 0 );

    while( !feof( helpfile ) && ( dcode != tempcode ) )
      if( fgetc( helpfile ) == '$' )
        tempcode = fgetc( helpfile ) * 100 + fgetc( helpfile ) * 10
                                           + fgetc( helpfile ) - 5328;

    if( dcode == tempcode )
    {
      int tmpchr = 0;
      int i = 0;

      fgetc( helpfile );

      while( !feof( helpfile ) && tmpchr != '$' )
      {
        tmpchr = fgetc( helpfile );
        dest[i] = tmpchr;

        if( dest[i] == '\n' )
        {
          dest++;
          dest[i] = '\r';
        }

        i++;
      }

      if( dest[i - 2] == '\n' || dest[i - 1] == '$' )
        dest[i - 3] = 0;
      else
        dest[i] = 0;
    }
    else
      strcpy( dest, "éØ®c†≠®• ≠• ≠†©§•≠Æ" );
  }
  else
    strcpy( dest, "H• ≠†©§•≠ ‰†©´ ØÆ¨ÆÈ®" );
} //LoadCHDescription

void ShowContextHelp( int dcode )
{
  char description[512];
  char buffer[1632];
  struct text_info ti;

  gettextinfo( &ti );
  LoadCHDescription( dcode, description );
  gettext( 15, 5, 65, 20, buffer );
  window( 15, 5, 65, 21 );
  cputs( "\
…ÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕäéHíÖäCíHÄü CèPÄÇäÄÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕª\
∫                                                 ∫\
∫                                                 ∫\
∫                                                 ∫\
∫                                                 ∫\
∫                                                 ∫\
∫                                                 ∫\
∫                                                 ∫\
∫                                                 ∫\
∫                                                 ∫\
∫                                                 ∫\
∫                                                 ∫\
∫                                                 ∫\
∫                                                 ∫\
∫                                                 ∫\
»ÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕº" );
  window( 16, 6, 64, 19 );
  gotoxy( 1, 1 );
  cprintf( "%s", description );
  GetChar();
  puttext( 15, 5, 65, 20, buffer );
  window( ti.winleft, ti.wintop, ti.winright, ti.winbottom );
  gotoxy( ti.curx, ti.cury );
}

int CheckForF1( int tmpchr, int dcode )
{
  if( tmpchr == KEY_UNKNOWN )
    tmpchr = GetChar();

  if( tmpchr == KEY_F1 )
    ShowContextHelp( dcode );

  return tmpchr;
}

void ShowMessage( char* str, int ch )
{
  char buffer[372];
  struct text_info ti;

  gettextinfo( &ti );
  gettext( 25, 10, 55, 15, buffer );
  window( 25, 10, 55, 16 );
  cputs( "\
…ÕÕÕÕÕÕÕÕÕÕëééÅôÖçàÖÕÕÕÕÕÕÕÕÕÕª\
∫                             ∫\
∫                             ∫\
∫                             ∫\
∫   ç†¶¨®‚• ´Ó°„Ó ™´†¢®Ë„...  ∫\
»ÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕÕº" );
  window( 26, 11, 54, 14 );
  gotoxy( 1, 1 );
  textcolor( LIGHTRED );
  cprintf( "%s", str );
  textcolor( YELLOW );

  Sound( S_BEEP );

  while( CheckForF1( KEY_UNKNOWN, ch ) == KEY_F1 );

  puttext( 25, 10, 55, 15, buffer );
  window( ti.winleft, ti.wintop, ti.winright, ti.winbottom );
  gotoxy( ti.curx, ti.cury );
}

//////////////////////////////////////////////////////////////////////////////
//Functions for recognising whether string contains a number://///////////////
//////////////////////////////////////////////////////////////////////////////
int IsInt( char* str )
{
  int i = 0;
  unsigned long int t;
  int sig = 0;

  while( str[i] && isspace( str[i] ) )
    i++;

  if( str[i] && str[i] != '+' && str[i] != '-' && ( str[i] > '9' || str[i] < '0' ) )
    return 0;
  else if( str[i] )
    i++;

  while( str[i] && str[i] <= '9' && str[i] >= '0' )
    i++;

  while( str[i] && isspace( str[i] ) )
    i++;

  if( str[i] != '\0' )
    return 0;

  i = 0;

  while( isspace( str[i] ) )
    i++;

  if( str[i] == '-' )
  {
    i++;
    sig = 1;
  }

  if( sscanf( str + i, "%lu", &t ) != 1 )
    return 0;

  if( t <= 2147483647 || ( sig && t == 2147483648 ))
    return 1;

  return 0;
}

int IsINFNAN( char* str )
{
  int i = 0;

  while( str[i] && isspace( str[i] ) )
    i++;

  if( str[i] != '+' && str[i] != '-' )
    return 0;

  i++;

  if( str[i] == 'N' )
  {
    if( str[i+1] != 'A' || str[i+2] != 'N' )
      return 0;
  }
  else if( str[i] == 'I' )
  {
    if( str[i+1] != 'N' || str[i+2] != 'F' )
      return 0;
  }
  else return 0;

  i += 3;

  while( str[i] && isspace( str[i] ) )
    i++;

  return !str[i];
}

int IsFloat( char* str )
{
  int i = 0;
  int sw = 0;

  strupr( str );

  if( IsINFNAN( str ) )
    return 1;

  while( str[i] && isspace( str[i] ) )
    i++;

  if( str[i] == '+' || str[i] == '-' )
    i++;

  while( str[i] >= '0' && str[i] <= '9' )
  {
    i++;

    if( !sw )
      sw = 1;
  }

  if( str[i] == '.' )
  {
     i++;

     while( str[i] >= '0' && str[i] <= '9' )
     {
       i++;

       if( !sw )
         sw = 1;
     }
  }

  if( !sw )
    return 0;

  if( str[i] == 'E' )
  {
    sw = 0;
    i++;

    if( str[i] == '+' || str[i] == '-' )
      i++;

    while( str[i] >= '0' && str[i] <= '9' )
    {
      i++;

      if( !sw )
        sw = 1;
    }

    if( !sw )
      return 0;
  }

  while( str[i] && isspace( str[i] ) )
    i++;

  return ( str[i] == 0 );
} //IsFloat

//////////////////////////////////////////////////////////////////////////////
//Cool and enormous function for string input:////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
int ReadString( char* str, int winlength, int numofchars, int clifempty, int chcode )
{
  char tmpstr[90] = "";
  int i, j, startchr;
  int tmpchr;
  int curx, cury;
  int exitflag = 2;

  i = 0;
  startchr = 0;
  curx = wherex();
  cury = wherey();

  while( exitflag == 2 )
  {
    tmpchr = GetChar();

    switch( tmpchr )
    {
      case KEY_LEFT:
        if( startchr + i )
          i--;
      break;

      case KEY_RIGHT:
        if( tmpstr[i + startchr] )
          i++;
      break;

      case KEY_HOME:
        i = 0;
        startchr = 0;
      break;

      case KEY_END:
        for( startchr = -1; tmpstr[ ++startchr ]; );
          if( startchr < winlength )
          {
            i = startchr;
            startchr = 0;
          }
          else
          {
            startchr -= winlength - 1;
            i = winlength - 1;
          }
      break;

      case KEY_DEL:
        j = startchr + i - 1;

        while( tmpstr[j] )
        {
          j++;
          tmpstr[j] = tmpstr[j+1];
        }
      break;

      case KEY_F1:
        if( chcode != -1 )
          ShowContextHelp( chcode );
      break;

      case KEY_BKSP:
        if( startchr + i )
        {
          i--;
          j = startchr + i;

          while( tmpstr[j] )
          {
            tmpstr[j] = tmpstr[j+1];
            j++;
          }
        }
      break;

      case KEY_ENTER:
        if( IsEmpty( tmpstr ) && clifempty )
          str[0] = '\0';
        else
          for( j = 0; j < numofchars; j++ )
            str[j] = tmpstr[j];

        tmpchr = -1;
        exitflag = 1;
      break;

      case KEY_ESC:
        str[0] = '\0';
        tmpchr = -1;
        exitflag = 0;
      break;

      default:
        j = startchr + i;

        while( tmpstr[j] )
          j++;

        while( j >= startchr + i )
        {
          tmpstr[j + 1] = tmpstr[j];
          j--;
        }

        tmpstr[startchr + i] = tmpchr;
        i++;
      break;
    }

    if( startchr > 0 && !tmpstr[startchr] )
    {
      startchr++;
      i = -1;
    }

    if( startchr + i == numofchars )
      i--;

    if( i == winlength )
    {
      startchr++;
      i--;
    }

    if( i == -1 )
    {
      startchr -= winlength * 2 / 3;
      i = winlength * 2 / 3 - 1;

      if( startchr < 0 )
      {
        i += startchr;
        startchr = 0;
      }
    }

    for( j = 0; j < winlength && tmpstr[startchr + j]; j++ )
    {
      gotoxy( curx + j, cury );
      putch( tmpstr[startchr + j] );
    }

    for( ; j < winlength; j++ )
    {
      gotoxy( curx + j, cury );
      putch( ' ' );
    }

    gotoxy( curx + i, cury );
  }

  return exitflag;
} //ReadString

//////////////////////////////////////////////////////////////////////////////
//Standard EM-3 commands://///////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void c00( void )
{
  R1 = mWords[GetRKAddress( 3 )];
  mWords[GetRKAddress( 1 )] = R1;
}

void c01( void )
{
  R1 = mWords[GetRKAddress( 2 )];
  R2 = mWords[GetRKAddress( 3 )];

  if( IsNAN( R1 ) || IsNAN( R2 ) )
    err = FLOAT_NAN;
  else
  {
    S = FloatToInt( IntToFloat( R1 ) + IntToFloat( R2 ) );
    mWords[GetRKAddress( 1 )] = S;
    ChangeFloatOmega();
  }
}

void c02( void )
{
  R1 = mWords[GetRKAddress( 2 )];
  R2 = mWords[GetRKAddress( 3 )];

  if( IsNAN( R1 ) || IsNAN( R2 ) )
    err = FLOAT_NAN;
  else
  {
    S = FloatToInt( IntToFloat( R1 ) - IntToFloat( R2 ) );
    mWords[GetRKAddress( 1 )] = S;
    ChangeFloatOmega();
  }
}

void c03( void )
{
  R1 = mWords[GetRKAddress( 2 )];
  R2 = mWords[GetRKAddress( 3 )];

  if( IsNAN( R1 ) || IsNAN( R2 ) )
    err = FLOAT_NAN;
  else
  {
    S = FloatToInt( IntToFloat( R1 ) * IntToFloat( R2 ) );
    mWords[GetRKAddress( 1 )] = S;
    ChangeFloatOmega();
  }
}

void c04( void )
{
  R1 = mWords[GetRKAddress( 2 )];
  R2 = mWords[GetRKAddress( 3 )];

  if( R2 == 0 )
    err = DIV_BY_ZERO;
  else if( IsNAN( R1 ) || IsNAN( R2 ) )
    err = FLOAT_NAN;
  else
  {
    S = FloatToInt( IntToFloat( R1 ) / IntToFloat( R2 ) );
    mWords[GetRKAddress( 1 )] = S;
    ChangeFloatOmega();
  }
}

void c05( void )
{
  char tmpstr[40];
  int i;

  UpdateRegisters();
  StatusMessage( "Ç¢Æ§ ¢•È•c‚¢•≠≠ÎÂ Á®c•´" );
  window( 28, 11, 50, 14 );
  _setcursortype( _NORMALCURSOR );

  if( !options[LOOP_MEM] && ( GetRKAddress( 1 ) + GetRKAddress( 2 ) > 512 ) || !GetRKAddress( 2 ) )
    err = IO_ERROR;
  else
  {
    if( ioservice / 10 == 5 )
     gotoxy( 1, 4 );
    else
      gotoxy( 1, ioservice / 10 );

    for( i = 0; i < GetRKAddress( 2 ); i++ )
    {
      if( ioservice / 10 == 4 )
      {
        movetext( 28, 12, 49, 14, 28, 11 );
        gotoxy( 1, 4 );
        cprintf( "                      \r" );
      }
      else if( ioservice / 10 == 5 )
        ioservice -= 10;

      if( !ReadString( tmpstr, 22, 40, B_YES, CH_QUERY_FLOAT ) )
      {
        err = INTERRUPTED_BY_USER;
        break;
      }

      if( !IsFloat( tmpstr ) || sscanf( tmpstr, "%f", ( float* )( mWords + ( GetRKAddress( 1 ) + i ) % 512 )) != 1 )
      {
        err = IN_ERROR;
        ioservice = i + 1;
      }
      else
      {
        cprintf( "\r%f", IntToFloat( mWords[( GetRKAddress( 1 ) + i ) % 512] ));

        while( wherex() <= 22 )
          putch( ' ' );

        if( ioservice / 10 != 4 )
        {
          ioservice += 10;
          cprintf( "\r\n" );

          if( ioservice / 10 == 4 )
            ioservice += 10;
        }
      }
    }
  }

  _setcursortype( _NOCURSOR );
  StatusMessage( "à§•‚ ¢ÎØÆ´≠•≠®• ØpÆ£p†¨¨Î" );
}

void c06( void )
{
  char tmpstr[40];
  int i;

  UpdateRegisters();
  StatusMessage( "Ç¢Æ§ Ê•´ÎÂ Á®c•´" );
  window( 28, 11, 50, 14 );
  _setcursortype( _NORMALCURSOR );

  if( !options[LOOP_MEM] && ( GetRKAddress( 1 ) + GetRKAddress( 2 ) > 512 ) || !GetRKAddress( 2 ) )
    err = IO_ERROR;
  else
  {
    if( ioservice / 10 == 5 )
     gotoxy( 1, 4 );
    else
      gotoxy( 1, ioservice / 10 );

    for( i = 0; i < GetRKAddress( 2 ); i++ )
    {
      if( ioservice / 10 == 4 )
      {
        movetext( 28, 12, 49, 14, 28, 11 );
        gotoxy( 1, 4 );
        cprintf( "                      \r" );
      }
      else if( ioservice / 10 == 5 )
        ioservice -= 10;

      if( !ReadString( tmpstr, 22, 40, B_YES, CH_QUERY_INT ) )
      {
        err = INTERRUPTED_BY_USER;
        break;
      }

      if( !IsInt( tmpstr ) || sscanf( tmpstr, "%D", mWords + ( GetRKAddress( 1 ) + i ) % 512 ) != 1 )
      {
        err = IN_ERROR;
        ioservice = i + 1;
        i = 512;
      }
      else
      {
        cprintf( "\r%ld", mWords[( GetRKAddress( 1 ) + i ) % 512] );

        while( wherex() <= 22 )
          putch( ' ' );

        if( ioservice / 10 != 4 )
        {
          ioservice += 10;
          cprintf( "\r\n" );

          if( ioservice / 10 == 4 )
            ioservice += 10;
        }
      }
    }
  }

  _setcursortype( _NOCURSOR );
  StatusMessage( "à§•‚ ¢ÎØÆ´≠•≠®• ØpÆ£p†¨¨Î" );
}

void c07( void )
{
  err = NO_SUCH_COMMAND;
}

void c08( void )
{
  err = NO_SUCH_COMMAND;
}

void c09( void )
{
  RA = GetRKAddress( 2 ) - 1;
}

void c10( void )
{
  float tmpfloat;
  long int tmpint = mWords[GetRKAddress( 3 )];

  if( IsNAN( tmpint ) || IsInfinite( tmpint ) )
    err = CONVERT_ERROR;
  else
  {
    tmpfloat = IntToFloat( tmpint );

    if( -2147483648.0 <= tmpfloat && tmpfloat <= 2147483647.0 )
      mWords[GetRKAddress( 1 )] = tmpfloat;
    else
      err = CONVERT_ERROR;
  }
}

void c11( void )
{
  R1 = mWords[GetRKAddress( 2 )];
  R2 = mWords[GetRKAddress( 3 )];

  if( !CheckAddIntValidity( R1, R2 ) )
    err = INT_OVERFLOW;
  else
  {
    S = R1 + R2;
    mWords[GetRKAddress( 1 )] = S;
    ChangeIntOmega();
  }
}

void c12( void )
{
  R1 = mWords[GetRKAddress( 2 )];
  R2 = mWords[GetRKAddress( 3 )];

  if( !CheckSubIntValidity( R1, R2 ) )
    err = INT_OVERFLOW;
  else
  {
    S = R1 - R2;
    mWords[GetRKAddress( 1 )] = S;
    ChangeIntOmega();
  }
}

void c13( void )
{
  R1 = mWords[GetRKAddress( 2 )];
  R2 = mWords[GetRKAddress( 3 )];

  if( !CheckMultIntValidity( R1, R2 ) )
    err = INT_OVERFLOW;
  else
  {
    S = R1 * R2;
    mWords[GetRKAddress( 1 )] = S;
    ChangeIntOmega();
  }
}

void c14( void )
{
  R1 = mWords[GetRKAddress( 2 )];
  R2 = mWords[GetRKAddress( 3 )];

  if( R2 == 0 )
    err = DIV_BY_ZERO;
  else
  {
    S = R1 / R2;
    mWords[GetRKAddress( 1 )] = S;
    ChangeIntOmega();
  }
}

void c15( void )
{
  int i;

  if( stepmode || options[STEPPED_OUTPUT] )
    StatusMessage( "ÇÎ¢Æ§ ¢•È•c‚¢•≠≠ÎÂ Á®c•´" );

  window( 28, 16, 49, 23 );
  gotoxy( 1, ioservice % 10 );

  if( !options[LOOP_MEM] && ( GetRKAddress( 1 ) + GetRKAddress( 2 ) > 512 ) || !GetRKAddress( 2 ) )
    err = IO_ERROR;
  else for( i = 0; i < GetRKAddress( 2 ); i++ )
  {
    if( ( stepmode || options[STEPPED_OUTPUT] ) && i )
      if( GetChar() == KEY_ESC )
      {
        err = INTERRUPTED_BY_USER;
        break;
      }

    if( ioservice % 10 == 8 )
      cprintf( "\n\r" );

    cprintf( "%f", IntToFloat( mWords[( GetRKAddress( 1 ) + i ) % 512 ] ));

    if( ioservice % 10 != 8 )
    {
      ioservice++;

      if( ioservice % 10 != 8 )
        cprintf( "\n\r" );
    }
  }

  if( stepmode || options[STEPPED_OUTPUT] )
    StatusMessage( "à§•‚ ¢ÎØÆ´≠•≠®• ØpÆ£p†¨¨Î" );
}

void c16( void )
{
  int i;

  if( stepmode || options[STEPPED_OUTPUT] )
    StatusMessage( "ÇÎ¢Æ§ Ê•´ÎÂ Á®c•´" );

  window( 28, 16, 49, 23 );
  gotoxy( 1, ioservice % 10 );

  if( !options[LOOP_MEM] && ( GetRKAddress( 1 ) + GetRKAddress( 2 ) > 512 ) || !GetRKAddress( 2 ) )
    err = IO_ERROR;
  else for( i = 0; i < GetRKAddress( 2 ); i++ )
  {
    if( ( stepmode || options[STEPPED_OUTPUT] ) && i )
      if( GetChar() == KEY_ESC )
      {
        err = INTERRUPTED_BY_USER;
        break;
      }

    if( ioservice % 10 == 8 )
      cprintf( "\n\r" );

    cprintf( "%ld", mWords[( GetRKAddress( 1 ) + i ) % 512] );

    if( ioservice % 10 != 8 )
    {
      ioservice++;

      if( ioservice % 10 != 8 )
        cprintf( "\n\r" );
    }
  }

  if( stepmode || options[STEPPED_OUTPUT] )
    StatusMessage( "à§•‚ ¢ÎØÆ´≠•≠®• ØpÆ£p†¨¨Î" );
}

void c17( void )
{
  err = NO_SUCH_COMMAND;
}

void c18( void )
{
  err = NO_SUCH_COMMAND;
}

void c19( void )
{
  RA = GetRKAddress( omega + 1 ) - 1;
}

void c20( void )
{
  mWords[GetRKAddress( 1 )] = FloatToInt( mWords[GetRKAddress( 3 )] );
}

void c21( void )
{
  err = NO_SUCH_COMMAND;
}

void c22( void )
{
  err = NO_SUCH_COMMAND;
}

void c23( void )
{
  err = NO_SUCH_COMMAND;
}

void c24( void )
{
  R1 = mWords[GetRKAddress( 2 )];
  R2 = mWords[GetRKAddress( 3 )];

  if( R2 == 0 )
    err = DIV_BY_ZERO;
  else
  {
    S = R1 % R2;
    mWords[GetRKAddress( 1 )] = S;
  }
}

void c25( void )
{
  err = NO_SUCH_COMMAND;
}

void c26( void )
{
  err = NO_SUCH_COMMAND;
}

void c27( void )
{
  err = NO_SUCH_COMMAND;
}

void c28( void )
{
  err = NO_SUCH_COMMAND;
}

void c29( void )
{
  err = NO_SUCH_COMMAND;
}

void c30( void )
{
  err = NO_SUCH_COMMAND;
}

void c31( void )
{
  err = NORMAL_TERMINATION;
  RA--;
}

//////////////////////////////////////////////////////////////////////////////
void ( *command_array[] )( void ) = {
  c00, c01, c02, c03, c04, c05, c06, c07, c08, c09, c10, c11, c12, c13, c14, c15,
  c16, c17, c18, c19, c20, c21, c22, c23, c24, c25, c26, c27, c28, c29, c30, c31 };

//////////////////////////////////////////////////////////////////////////////
//Main program execution functions:///////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void PerformDiagnostics( void )
{
  int tmpy;

  StatusMessage( "èpÆ£p†¨¨† ß†¢•pË•≠†" );
  window( 53, 6, 78, 14 );
  clrscr();
  gotoxy( 1, 1 );

  if( err == NO_ERROR )
  {
    cprintf( "èpÆ£p†¨¨† ¢ÎØÆ´≠•≠† °•ß\n\rÆË®°Æ™.\n\r" );
    cprintf( "H†¶¨®‚• ´Ó°yÓ ™´†¢®Ëy..." );

    while( CheckForF1( KEY_UNKNOWN, CH_ERROR + err ) == KEY_F1 );

    window( 53, 6, 78, 10 );
    clrscr();
  }
  else if( err == INTERRUPTED_BY_USER )
  {
    cprintf( "ÇÎØÆ´≠•≠®• ØpÆ£p†¨¨Î\n\rØ‡•‡¢†≠Æ ØÆ´ÏßÆ¢†‚•´•¨.\n\r" );
    cprintf( "H†¶¨®‚• ´Ó°yÓ ™´†¢®Ëy..." );

    while( CheckForF1( KEY_UNKNOWN, CH_ERROR + err ) == KEY_F1 );

    window( 53, 6, 78, 10 );
    clrscr();
  }
  else
  {
    cprintf( "èpÆ®ßÆË´† ÆË®°™†:\n\r" );

    switch( err )
    {
      case DIV_BY_ZERO:
        cprintf( "Ñ•´•≠®• ≠† ≠Æ´Ï." ); break;

      case INT_OVERFLOW:
        cprintf( "è•p•ØÆ´≠•≠®• Øp® p†°Æ‚•\n\rc Ê•´Î¨® Á®c´†¨®." ); break;

      case FLOAT_NAN:
        cprintf( "èÆØÎ‚™† ØpÆ®ß¢•c‚®\n\r†p®‰¨•‚®Á•c™yÓ ÆØ•p†Ê®Ó\n\r≠†§ +/-NAN." ); break;

      case CONVERT_ERROR:
        cprintf( "è•p•ØÆ´≠•≠®• Øp® Ø•p•¢Æ§•\n\r®ß ¢•È•c‚¢•≠≠Æ£Æ ¢ Ê•´Æ•." ); break;

      case REACHED_THE_END:
        cprintf( "èpÆ£p†¨¨† ß†¢•pË®´†cÏ,\n\r≠• ¢ÎØÆ´≠®¢ Æc‚†≠Æ¢†." ); break;

      case NO_SUCH_COMMAND:
        cprintf( "Ç cØ•Ê®‰®™†Ê®® ìå-3\n\r≠•‚ ™Æ¨†≠§Î c ™Æ§Æ¨ %02d.", GetRKOpcode() ); break;

      case IO_ERROR:
        cprintf( "éË®°™† ¢¢Æ§†/¢Î¢Æ§†." ); break;

      case IN_ERROR:
        cprintf( "Ç≠y‚p•≠≠ÔÔ ÆË®°™†\n\ryc‚pÆ©c‚¢† ¢¢Æ§†\n\rØp® Æ°p†°Æ‚™• Á®c´† #%d.", ioservice ); break;
    }

    cprintf( "\n\rÄ§p•c ™Æ¨†≠§Î: %03d\n\r", RA );
    cprintf( "H†¶¨®‚• ´Ó°yÓ ™´†¢®Ëy..." );
    tmpy = wherey();
    MarkLine();
    Sound( S_BEEP );

    while( CheckForF1( KEY_UNKNOWN, CH_ERROR + err ) == KEY_F1 );

    window( 53, 6, 78, 14 );
    gotoxy( 1, 1 );
    cprintf( "èÆc´•§≠ÔÔ" );
    gotoxy( 1, tmpy );
    cprintf( "                        " );
  }
} //PerformDiagnostics

void ExecuteProgram( void )
{
  char description[80];
  long int* tmpmem = NULL;
  int i;

  Initialize();
  window( 53, 6, 78, 14 );
  clrscr();
  StatusMessage( "à§•‚ ¢ÎØÆ´≠•≠®• ØpÆ£p†¨¨Î" );
  _setcursortype( _NOCURSOR );

  if( options[RECOVER_MEM] )
  {
    tmpmem = ( long int* )malloc( 511 * sizeof( long int ) );

    if( tmpmem != NULL )
      for( i = 1; i < 512; i++ )
        tmpmem[i] = mWords[i];
  }

  while( err == NO_ERROR )
  {
    if( options[Z_BEHAVIOUR] ) mWords[0] = 0;

    RK = mWords[RA];

    if( stepmode )
    {
      if( options[SHOW_EXACT_ADDRESS] )
      {
        GenerateDescription( RA, description );
        ShowHelp( description );
      }
      else
        ShowHelp( descarr[GetRKOpcode()] );

      UpdateRegisters();
      MarkLine();
    }

    if( stepmode || kbhit() )
      if( GetChar() == KEY_ESC )
      {
        err = INTERRUPTED_BY_USER;
        break;
      }

    if( stepmode && kbhit() )
      getch();

    ( *command_array[ GetRKOpcode() ] )();

    if( err == NO_ERROR || err == NORMAL_TERMINATION )
      RA++;

    if( RA == 512 )
      if( options[LOOP_MEM] )
        RA = 0;
      else
      {
        RA = 511;
        err = REACHED_THE_END;
      }

    UpdateRegisters();
  }

  if( err == NORMAL_TERMINATION )
  {
    err = NO_ERROR;
    UpdateRegisters();
  }
  else if( err == INTERRUPTED_BY_USER )
  {
    err = NO_ERROR;
    UpdateRegisters();
    err = INTERRUPTED_BY_USER;
  }


  PerformDiagnostics();

  if( stepmode )
    ShowHelp( "" );

  if( options[RECOVER_MEM] && tmpmem != NULL )
  {
    for( i = 1; i < 512; i++ )
      mWords[i] = tmpmem[i];

    free( tmpmem );
  }
  else
    ismodflag = 1;

  _setcursortype( _NORMALCURSOR );
} //ExecuteProgram

//////////////////////////////////////////////////////////////////////////////
//Functions for advanced code edition:////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void DefineFloat( void )
{
  int tmpchr;
  char tmpstr[40];

  StatusMessage( "èpÆc¨Æ‚p ÔÁ•©™® ™†™ ¢•È•c‚¢•≠≠Æ£Æ Á®c´†" );
  window( 9 + cmdmode, 2 + ey, 24, 2 + ey );
  clrscr();
  gotoxy( 1, 1 );
  cprintf( "%E", IntToFloat( mWords[curaddress + ey - 1] ) );

  while( ( tmpchr = CheckForF1( KEY_UNKNOWN, CH_DEFINE_FLOAT )) == KEY_F1 );

  if( tmpchr == KEY_ENTER )
  {
    StatusMessage( "àß¨•≠•≠®• ß≠†Á•≠®Ô ¢•È•c‚¢•≠≠Æ£Æ Á®c´†" );
label_startinput:;
    window( 9 + cmdmode, 2 + ey, 24, 2 + ey );
    gotoxy( 1, 1 );
    clrscr();
    ReadString( tmpstr, 14, 40, B_YES, CH_QUERY_FLOAT );

    if( IsFloat( tmpstr ) && sscanf( tmpstr, "%f", ( float* )( mWords + curaddress + ey - 1 )) == 1 )
      ismodflag = 1;
    else if( tmpstr[0] )
    {
      ShowMessage( "        ç•¢•‡≠Î© ¢¢Æ§", CH_MSG_WRONGINPUT );
      goto label_startinput;
    }
  }

  StatusMessage( "P•§†™‚®pÆ¢†≠®• ØpÆ£p†¨¨Î" );
  window( 4, 3, 23, 24 );
  gotoxy( 1, ey );
  PrintLine( curaddress + ey - 1 );
}

void DefineInt( void )
{
  int tmpchr;
  char tmpstr[40];

  StatusMessage( "èpÆc¨Æ‚p ÔÁ•©™® ™†™ Ê•´Æ£Æ Á®c´†" );
  window( 9 + cmdmode, 2 + ey, 24, 2 + ey );
  clrscr();
  gotoxy( 1, 1 );
  cprintf( "%ld", mWords[curaddress + ey - 1] );

  while( ( tmpchr = CheckForF1( KEY_UNKNOWN, CH_DEFINE_INT )) == KEY_F1 );

  if( tmpchr == KEY_ENTER )
  {
    StatusMessage( "àß¨•≠•≠®• ß≠†Á•≠®Ô Ê•´Æ£Æ Á®c´†" );
label_startinput:;
    window( 9 + cmdmode, 2 + ey, 24, 2 + ey );
    gotoxy( 1, 1 );
    clrscr();
    ReadString( tmpstr, 14, 40, B_YES, CH_QUERY_INT );

    if( IsInt( tmpstr ) && sscanf( tmpstr, "%D", mWords + curaddress + ey - 1 ) == 1 )
      ismodflag = 1;
    else if( tmpstr[0] )
    {
      ShowMessage( "        ç•¢•‡≠Î© ¢¢Æ§", CH_MSG_WRONGINPUT );
      goto label_startinput;
    }
  }

  StatusMessage( "P•§†™‚®pÆ¢†≠®• ØpÆ£p†¨¨Î" );
  window( 4, 3, 23, 24 );
  gotoxy( 1, ey );
  PrintLine( curaddress + ey - 1 );
}

int RewriteMemoryAddress( int address, int n, int pos, int newchr )
{
  int singles, tens, hundreds;
  long int newvalue = GetAddress( address, n );

  singles = newvalue % 10;
  tens = newvalue % 100 - singles;
  hundreds = newvalue - tens - singles;

  switch( pos )
  {
    case 1: newvalue = newchr * 100 + tens + singles; break;
    case 2: newvalue = newchr * 10 + hundreds + singles; break;
    case 3: newvalue = newchr + hundreds + tens; break;
  }

  if( newvalue > 511 )
  {
    newvalue = 511;
    pos = 0;
  }

  mWords[address] &= ~( 0777L << ( 3 - n ) * 9 );
  mWords[address] |= newvalue << ( 3 - n ) * 9;
  ismodflag = 1;

  return pos;
}

void RewriteMemoryOpcode( int address, long int newvalue )
{
  mWords[address] &= ~( 037L << 27 );
  mWords[address] |= newvalue << 27;
  ismodflag = 1;
}

int RewriteMemoryOpcodeChar( int address, int pos, int newchr )
{
  int singles, tens;
  long int newvalue = GetOpcode( address );

  singles = newvalue % 10;
  tens = newvalue - singles;

  switch( pos )
  {
    case 1: newvalue = newchr * 10 + singles; break;
    case 2: newvalue = newchr + tens; break;
  }

  if( newvalue > 31 )
  {
    newvalue = 31;
    pos = 0;
  }

  mWords[address] &= ~( 037L << 27 );
  mWords[address] |= newvalue << 27;
  ismodflag = 1;

  return pos;
}

//////////////////////////////////////////////////////////////////////////////
//Subfunctions for Edit()/////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void RewriteMemoryDigit( char tmpchr )
{
  int success = 0;

  tmpchr -= 48;

  switch( ex )
  {
    case 7:
    case 8:
      success = RewriteMemoryOpcodeChar( ey + curaddress - 1, ex - 6, tmpchr );
    break;

    case 10:
    case 11:
    case 12:
      success = RewriteMemoryAddress( ey + curaddress - 1, 1, ex - 9, tmpchr );
    break;

    case 14:
    case 15:
    case 16:
      success = RewriteMemoryAddress( ey + curaddress - 1, 2, ex - 13, tmpchr );
    break;

    case 18:
    case 19:
    case 20:
      success = RewriteMemoryAddress( ey + curaddress - 1, 3, ex - 17, tmpchr );
    break;
  }

  if( success )
    putch( tmpchr + 48 );
  else
  {
    switch( ex )
    {
      case 7:
      case 8:
        gotoxy( 7, ey );
        cputs( "31" );
      break;

      case 10:
      case 11:
      case 12:
        gotoxy( 10, ey );
        cputs( "511" );
      break;

      case 14:
      case 15:
      case 16:
        gotoxy( 14, ey );
        cputs( "511" );
      break;

      case 18:
      case 19:
      case 20:
        gotoxy( 18, ey );
        cputs( "511" );
      break;
    }

    Sound( S_CLICK );
  }

  ex++;

  if( ex == 9 || ex == 13 || ex == 17 ) ex++;
} //RewriteMemoryDigit

char EngToRus( char chr )
{
  int i = 0;
  char cyrarr[] = "ÄÅÇÉÑÖÜáàäãåHéèPCíìîïñóòôõùûü";
  char latarr[] = "f,dult;pbrkvyjghcnea[wxios'.z";

  while( i < 29 && latarr[i] != chr )
    i++;

  if( i < 29 )
    return cyrarr[i];
  else
    return '?';
}

void RewriteMnemonicCommand( int pos, char newchar )
{
  int tmpcode;
  char tmpcmd[3];
  char* oldcmd;

  oldcmd = CodeToAlias( GetOpcode( ey - 1 + curaddress ) );

  switch( pos )
  {
    case 1:
      tmpcmd[0] = newchar;
      tmpcmd[1] = oldcmd[1];
      tmpcmd[2] = oldcmd[2];
      tmpcode = AliasToCode( tmpcmd );

      if( tmpcode <= 31 )
      {
        gotoxy( 6, ey );
        putch( newchar );
        RewriteMemoryOpcode( ey - 1 + curaddress, tmpcode );
        ex++;
      }
      else
      {
        for( tmpcode = 0; tmpcode <= 31 && aliasarr[tmpcode][0] != newchar;
                                                                  tmpcode++ );

        if( tmpcode <= 31 )
        {
          RewriteMemoryOpcode( ey - 1 + curaddress, tmpcode );
          gotoxy( 6, ey );
          cputs( aliasarr[tmpcode] );
          ex++;
        }
        else
          Sound( S_CLICK );
      }
    break;

    case 2:
      tmpcmd[0] = oldcmd[0];
      tmpcmd[1] = newchar;
      tmpcmd[2] = oldcmd[2];
      tmpcode = AliasToCode( tmpcmd );

      if( tmpcode <= 31 )
      {
        gotoxy( 7, ey );
        putch( newchar );
        RewriteMemoryOpcode( ey - 1 + curaddress, tmpcode );
        ex++;
      }
      else
      {
        for( tmpcode = 0;
             tmpcode <= 31 && !( aliasarr[tmpcode][0] == tmpcmd[0]
                           && aliasarr[tmpcode][1] == newchar );
             tmpcode++ );

        if( tmpcode <= 31 )
        {
          RewriteMemoryOpcode( ey - 1 + curaddress, tmpcode );
          gotoxy( 6, ey );
          cputs( aliasarr[tmpcode] );
          ex++;
        }
        else
          Sound( S_CLICK );
      }
    break;

    case 3:
      tmpcmd[0] = oldcmd[0];
      tmpcmd[1] = oldcmd[1];
      tmpcmd[2] = newchar;
      tmpcode = AliasToCode( tmpcmd );

      if( tmpcode <= 31 )
      {
        gotoxy( 8, ey );
        putch( newchar );
        RewriteMemoryOpcode( ey - 1 + curaddress, tmpcode );
        ex += 2;
      }
      else
        Sound( S_CLICK );
    break;
  }
} //RewriteMnemonicCommand

int ChangePosition( int tmpchr )
{
  switch( tmpchr )
  {
    case KEY_LEFT:
      ex--;

      if( ex == 9 || ex == 13 || ex == 17 )
        ex--;

    break;

    case KEY_RIGHT:
      ex++;

      if( ex == 9 || ex == 13 || ex == 17 )
        ex++;

    break;

    case KEY_UP:   ey--; break;
    case KEY_DOWN: ey++; break;

    case KEY_HOME: ex = 6 + cmdmode; break;
    case KEY_END:  ex = 20; break;

    case KEY_PGUP:
      curaddress -= 21;

      if( curaddress < 1 )
      {
        curaddress = 1;
        ey = 1;
      }

      UpdateMemory();
    break;

    case KEY_PGDN:
      curaddress += 21;

      if( curaddress > 491 )
      {
        curaddress = 491;
        ey = 21;
      }

      UpdateMemory();
    break;

    case KEY_TAB:
      switch( ex )
      {
        case 6:
        case 7:
        case 8:  ex = 10; break;
        case 10:
        case 11:
        case 12: ex = 14; break;
        case 14:
        case 15:
        case 16: ex = 18; break;
        case 18:
        case 19:
        case 20: ex = 21; break;
      }
    break;

    case KEY_ENTER: ex = 21; break;

    case KEY_F6:
      cmdmode  = 1 - cmdmode;
      UpdateMemory();

      if( cmdmode && ex == 6 )
        ex = 7;
    break;

    default:
      return 0;
  }

  return 1;
} //ChangePosition

void CorrectPosition( void )
{
  if( ex <= 5 + cmdmode )
  {
    ey--;
    ex = 20;
  }

  if( ex >= 21 )
  {
    ey++;
    ex = 6 + cmdmode;
  }

  if( ey == 0 )
  {
    if( curaddress != 1 )
    {
      curaddress--;
      movetext( 4, 3, 23, 22, 4, 4 );
      gotoxy( 1, 1 );
      PrintLine( curaddress );
    }

    ey = 1;
  }

  if( ey == 22 )
  {
    if( curaddress != 491 )
    {
      curaddress++;
      movetext( 4, 4, 23, 23, 4, 3 );
      gotoxy( 1, 21 );
      PrintLine( curaddress + 20 );
    }

    ey = 21;
  }
}

int IsFunctionKey( int chr )
{
  int keys[9] = { KEY_F2, KEY_F3, KEY_F7, KEY_F8, KEY_F9, KEY_ALT_F2, KEY_CTRL_F2, KEY_CTRL_F9, KEY_F10 };
  int i;

  for( i = 0; i < 9; i++ )
    if( keys[i] == chr )
      return 1;

  return 0;
}

void ShowCurrentDescription( void )
{
  if( options[SHOW_EXACT_ADDRESS] )
  {
    char description[80];
    GenerateDescription( ey - 1 + curaddress, description );
    ShowHelp( description );
  }
  else
    ShowHelp( descarr[GetOpcode( ey - 1 + curaddress )] );

  window( 4, 3, 23, 24 );
}

//////////////////////////////////////////////////////////////////////////////
//Smart command insertion and deletion:///////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void DelLine( void )
{
  int address = ey - 1 + curaddress;
  int i;

  for( i = address; i < 511; i++ )
    mWords[i] = mWords[i+1];

  mWords[511] = 0;
  UpdateMemory();
  ismodflag = 1;
}

void SmartDelLine( void )
{
  int address = ey - 1 + curaddress;
  long int opcode, a1, a2, a3;
  int i;

  for( i = address; i < 511; i++ )
    mWords[i] = mWords[i+1];

  mWords[511] = 0;

  for( i = 1; i < 511; i++ )
  {
    opcode = ( mWords[i] >> 27 ) & 037;
    a1 = ( mWords[i] >> 18 ) & 0777;
    a2 = ( mWords[i] >> 9 ) & 0777;
    a3 = mWords[i] & 0777;

    if( a1 > address )
      a1--;

    if( a2 > address )
      a2--;

    if( a3 > address )
      a3--;

    mWords[i] = a3 | ( a2 << 9 ) | ( a1 << 18 ) | ( opcode << 27 );
  }

  UpdateMemory();
  ismodflag = 1;
}

void InsLine( void )
{
  int address = ey - 1 + curaddress;
  int i;

  for( i = 510; i >= address; i-- )
    mWords[i+1] = mWords[i];

  mWords[address] = 0;
  UpdateMemory();
  ismodflag = 1;
}

void SmartInsLine( void )
{
  int address = ey - 1 + curaddress;
  long int opcode, a1, a2, a3;
  int i;

  for( i = 510; i >= address; i-- )
    mWords[i+1] = mWords[i];

  mWords[address] = 0;

  for( i = 1; i < 512; i++ )
  {
    opcode = ( mWords[i] >> 27 ) & 037;
    a1 = ( mWords[i] >> 18 ) & 0777;
    a2 = ( mWords[i] >> 9 ) & 0777;
    a3 = mWords[i] & 0777;

    if( a1 >= address && a1 < 511 )
      a1++;

    if( a2 >= address && a2 < 511 )
      a2++;

    if( a3 >= address && a3 < 511 )
      a3++;

    mWords[i] = a3 | ( a2 << 9 ) | ( a1 << 18 ) | ( opcode << 27 );
  }

  UpdateMemory();
  ismodflag = 1;
}

//////////////////////////////////////////////////////////////////////////////
//Smart2 functions:///////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
int IsSmartExcluded( int opcode, int address )
{
  int excom[32] =
  {
    2, 0, 0, 0, 0, 6, 6, 7,
    7, 5, 2, 0, 0, 0, 0, 6,
    6, 7, 7, 0, 2, 7, 7, 7,
    0, 7, 7, 7, 7, 7, 7, 7
  };

  switch( excom[opcode] )
  {
    case 1:
      if( address == 1 )
        return 1;
    break;

    case 2:
      if( address == 2 )
        return 1;
    break;

    case 3:
      if( address == 3 )
        return 1;
    break;

    case 4:
      if( address == 1 || address == 2 )
        return 1;
    break;

    case 5:
      if( address == 1 || address == 3 )
        return 1;
    break;

    case 6:
      if( address == 2 || address == 3 )
        return 1;
    break;

    case 7:
      return 1;
  }

  return 0;
}

void SmartDelLine2( void )
{
  int address = ey - 1 + curaddress;
  long int opcode, a1, a2, a3;
  int j = 0, k, i;

  while( (( mWords[j] >> 27 ) & 037 ) != 31 && j < 511 ) j++;

  k = j;

  while( mWords[k] && k < 511 ) k++;

  for( i = address; i < 511; i++ )
    mWords[i] = mWords[i+1];

  mWords[511] = 0;

  for( i = 1; i <= j; i++ )
  {
    opcode = ( mWords[i] >> 27 ) & 037;
    a1 = ( mWords[i] >> 18 ) & 0777;
    a2 = ( mWords[i] >> 9 ) & 0777;
    a3 = mWords[i] & 0777;

    if( a1 > address && a1 < k && !IsSmartExcluded( opcode, 1 ) )
      a1--;

    if( a2 > address && a2 < k && !IsSmartExcluded( opcode, 2 ) )
      a2--;

    if( a3 > address && a3 < k && !IsSmartExcluded( opcode, 3 ) )
      a3--;

    mWords[i] = a3 | ( a2 << 9 ) | ( a1 << 18 ) | ( opcode << 27 );
  }

  UpdateMemory();
  ismodflag = 1;
}

void SmartInsLine2( void )
{
  int address = ey - 1 + curaddress;
  long int opcode, a1, a2, a3;
  int j = 0, k, i;

  while( (( mWords[j] >> 27 ) & 037 ) != 31 && j < 511 ) j++;

  k = j;

  while( mWords[k] && k < 511 ) k++;

  for( i = 510; i >= address; i-- )
    mWords[i+1] = mWords[i];

  mWords[address] = 0;

  for( i = 1; i <= j; i++ )
  {
    opcode = ( mWords[i] >> 27 ) & 037;
    a1 = ( mWords[i] >> 18 ) & 0777;
    a2 = ( mWords[i] >> 9 ) & 0777;
    a3 = mWords[i] & 0777;

    if( a1 >= address && a1 < k && !IsSmartExcluded( opcode, 1 ) )
      a1++;

    if( a2 >= address && a2 < k && !IsSmartExcluded( opcode, 2 ) )
      a2++;

    if( a3 >= address && a3 < k && !IsSmartExcluded( opcode, 3 ) )
      a3++;

    mWords[i] = a3 | ( a2 << 9 ) | ( a1 << 18 ) | ( opcode << 27 );
  }

  UpdateMemory();
  ismodflag = 1;
}

//////////////////////////////////////////////////////////////////////////////
//Main function for code editing://///////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
int Edit( void )
{
  int tmpchr = KEY_UNKNOWN;

  UpdateMemory();
  StatusMessage( "P•§†™‚®pÆ¢†≠®• ØpÆ£p†¨¨Î" );

  if( options[SHOW_DESC_IN_EDIT] )
    ShowCurrentDescription();

  window( 4, 3, 23, 24 );
  gotoxy( ex, ey );

  while( !IsFunctionKey( tmpchr ) )
  {
    ex = wherex();
    ey = wherey();
    tmpchr = GetChar();

    switch( tmpchr )
    {
      case KEY_ALT_I:
        DefineInt();
      break;

      case KEY_ALT_F:
        DefineFloat();
      break;

      case KEY_F1:
        ShowContextHelp( CH_EDIT_WINDOW );
      break;

      case KEY_DEL:
        DelLine();
      break;

      case KEY_CTRL_DEL:
        if( options[USE_SMART2] )
          SmartDelLine2();
        else
          SmartDelLine();
      break;

      case KEY_INS:
        InsLine();
      break;

      case KEY_CTRL_INS:
        if( options[USE_SMART2] )
          SmartInsLine2();
        else
          SmartInsLine();
      break;
    }

    if( tmpchr >= '0' && tmpchr <= '9' )
      if( ex >= 10 || cmdmode )
        RewriteMemoryDigit( tmpchr );
      else
        Sound( S_CLICK );

    if( !cmdmode && ex < 9 && EngToRus( tmpchr ) != '?' )
      RewriteMnemonicCommand( ex - 5, EngToRus( tmpchr ) );

    ChangePosition( tmpchr );
    CorrectPosition();

    if( options[SHOW_DESC_IN_EDIT] )
      ShowCurrentDescription();

    gotoxy( ex, ey );
  }

  return tmpchr;
} //Edit

//////////////////////////////////////////////////////////////////////////////
//Functions for loading and saving programs to disk://////////////////////////
//////////////////////////////////////////////////////////////////////////////
void SetEM3Path( char* argv0 )
{
  int i = 0;

  while( argv0[i] )
    i++;

  while( i > 0 && argv0[i] != '\\' )
    i--;

  em3path = ( char* )malloc( i + 80 );
  em3path[i+1] = '\0';
  em3path[i]   = '\\';

  while( i > 0 )
  {
    i--;
    em3path[i] = argv0[i];
  }
}

int AddExtension( char* str, char* ext )
{
  int i = 0;

  while( str[i] ) i++;

  if( strlen( str ) > strlen( ext ) && strcmp( str + i - strlen( ext ), ext ) )
  {
    strcat( str, ext );
    return 1;
  }

  return 0;
}

void CreateVisiblePath( char* str )
{
  int k, i = 0;

  while( str[i] ) i++;

  if( i < 26 )
    for( ; i >= 0; i-- )
      visiblepath[i] = str[i];
  else
  {
    while( str[i] != '\\' ) i--;

    i++;

    for( k = 0; str[i]; i++, k++  )
      visiblepath[k] = str[i];

    visiblepath[k+1] = 0;
  }
}

void LoadProgram( void )
{
  FILE* fl;
  char tmppath[80];
  int i = 0;
  int tmpchr;

  StatusMessage( "ó‚•≠®• ØpÆ£p†¨¨Î ®ß ‰†©´†" );

  if( ismodflag )
  {
    _setcursortype( _NOCURSOR );
    window( 53, 16, 78, 23 );
    gotoxy( 1, 1 );
    clrscr();
    cprintf( "í•™yÈ†Ô ØpÆ£p†¨¨†\n\r≠• cÆÂp†≠•≠†.\n\rèpÆ§Æ´¶®‚Ï?\n\r" );
    cprintf( "Enter - OK\n\rEsc - é‚¨•≠†" );

    while( tmpchr != KEY_ESC && tmpchr != KEY_ENTER )
      tmpchr = GetChar();

    clrscr();
    _setcursortype( _NORMALCURSOR );
  }

  if( tmpchr == KEY_ESC )
  {
    clrscr();
    return;
  }

label_startinput:;
  window( 53, 3, 79, 3 );
  gotoxy( 1, 1 );
  cprintf( "                          " );
  gotoxy( 1, 1 );
  ReadString( tmppath, 26, 80, B_YES, CH_QUERY_LOAD );
  strlwr( tmppath );
  fl = fopen( tmppath, "rb" );

  if( fl == NULL && tmppath[0] )
    if( AddExtension( tmppath, ".em3" ) )
      fl = fopen( tmppath, "rb" );

  if( fl != NULL )
  {
    for( i = 0; i < 80; i++ )
      inputfile[i] = tmppath[i];

    fseek( fl, 0, 2 );
    i = ftell( fl );
    fseek( fl, 0, 0 );
    InitFirstTime();
    fread( ( void* )( mWords + 1 ), 1, i, fl );
    fclose( fl );
    CreateVisiblePath( tmppath );
    ismodflag = 0;
  }
  else if( tmppath[0] )
  {
    ShowMessage( "   ç•¢Æß¨Æ¶≠Æ Æ‚™‡Î‚Ï ‰†©´", CH_MSG_CANTOPEN );
    goto label_startinput;
  }

  window( 53, 3, 79, 3 );
  gotoxy( 1, 1 );
  cprintf( "                          " );
  gotoxy( 1, 1 );
  cprintf( "%s", visiblepath );
}

void LoadAtStart( char* tmppath )
{
  FILE* fl;
  int i;

  fl = fopen( tmppath, "rb" );

  if( fl == NULL )
    if( AddExtension( tmppath, ".em3" ) )
      fl = fopen( tmppath, "rb" );

  if( fl != NULL )
  {
    for( i = 0; i < 80; i++ )
      inputfile[i] = tmppath[i];

    InitFirstTime();
    fread( ( void* )( mWords + 1 ), 1, 2044, fl );
    fclose( fl );
    CreateVisiblePath( strlwr( tmppath ) );
  }
}

void WritePrgToFile( FILE* fl )
{
  char* tmpptr;
  int i = 2043;

  tmpptr = ( char* )( mWords + 1 );

  while( i >= 0 && !tmpptr[i] ) i--;

  fwrite( ( void* )( mWords + 1 ), 1, i + 1, fl );
  fclose( fl );
  ismodflag = 0;
}

int SaveProgram( int specifynew )
{
  FILE* fl;
  int i;

  StatusMessage( "CÆÂp†≠•≠®• ØpÆ£p†¨¨Î ¢ ‰†©´" );

  if( inputfile[0] == 0 || specifynew )
  {
    char tmppath[80];

label_startinput:;
    window( 53, 3, 79, 3 );
    gotoxy( 1, 1 );
    cprintf( "                          " );
    gotoxy( 1, 1 );
    ReadString( tmppath, 26, 80, B_YES, CH_QUERY_SAVE );
    strlwr( tmppath );

    if( tmppath[0] )
      AddExtension( tmppath, ".em3" );

    fl = fopen( tmppath, "wb" );

    if( fl != NULL )
    {
      for( i = 0; i < 80; i++ )
        inputfile[i] = tmppath[i];

      CreateVisiblePath( tmppath );
      WritePrgToFile( fl );
      return 1;
    }
    else if( tmppath[0] )
    {
      ShowMessage( "   ç•¢Æß¨Æ¶≠Æ ·Æß§†‚Ï ‰†©´", CH_MSG_CANTCREATE );
      goto label_startinput;
    }

    window( 53, 3, 79, 3 );
    gotoxy( 1, 1 );
    cprintf( "                          " );
    gotoxy( 1, 1 );
    cprintf( "%s", visiblepath );
  }
  else
  {
    fl = fopen( inputfile, "wb" );
    WritePrgToFile( fl );
    return 1;
  }

  return 0;
}

int SaveAsText( void )
{
  FILE* stream = NULL;
  char tmppath[80];
  int startline = 1, endline = 511;

  StatusMessage( "CÆÂp†≠•≠®• ‚•™c‚† ØpÆ£p†¨¨Î ¢ ‰†©´" );

label_inputstline:;
  window( 53, 16, 78, 23 );
  clrscr();
  gotoxy( 1, 1 );
  cprintf( "H†Á†´Ï≠†Ô c‚pÆ™†:" );
  gotoxy( 1, 2 );
  ReadString( tmppath, 25, 80, B_YES, -1 );

  if( !tmppath[0] )
  {
    clrscr();
    return 0;
  }

  if( !IsInt( tmppath ) || sscanf( tmppath, "%d", &startline ) != 1 )
    startline = -1;

  if( startline < 1 || startline > 511 )
  {
    ShowMessage( "        ç•¢•‡≠Î© ¢¢Æ§", CH_MSG_WRONGINPUT );
    goto label_inputstline;
  }

  cprintf( "\n\räÆ≠•Á≠†Ô c‚pÆ™†:" );

label_inputenline:;
  gotoxy( 1, 4 );
  ReadString( tmppath, 25, 80, B_YES, -1 );

  if( !tmppath[0] )
  {
    clrscr();
    return 0;
  }

  if( !IsInt( tmppath ) || sscanf( tmppath, "%d", &endline ) != 1 )
    endline = -1;

  if( endline < startline || endline > 511 )
  {
    ShowMessage( "        ç•¢•‡≠Î© ¢¢Æ§", CH_MSG_WRONGINPUT );
    window( 53, 16, 78, 23 );
    gotoxy( 1, 4 );
    cprintf( "                          " );
    goto label_inputenline;
  }

label_inputfilename:;
  clrscr();
  gotoxy( 1, 1 );
  cprintf( "ì™†¶®‚• Øy‚Ï ™ ‚•™c‚Æ¢Æ¨y ‰†©´y:" );
  gotoxy( 1, 3 );
  ReadString( tmppath, 25, 80, B_YES, CH_QUERY_SAVETXT );

  if( !tmppath[0] )
  {
    clrscr();
    return 0;
  }

  stream = fopen( tmppath, "w" );

  if( stream == NULL )
  {
    ShowMessage( "   ç•¢Æß¨Æ¶≠Æ ·Æß§†‚Ï ‰†©´", CH_MSG_CANTCREATE );
    window( 53, 16, 78, 23 );
    goto label_inputfilename;
  }

  fprintf( stream, "ÄÑê  äéè A1  A2  A3\n" );

  if( cmdmode == 1 )
    for( ; startline <= endline; startline++ )
      fprintf( stream, "%03d   %02d %03d %03d %03d\n", startline,
                                            GetOpcode( startline ),
                                            GetAddress( startline, 1 ),
                                            GetAddress( startline, 2 ),
                                            GetAddress( startline, 3 ));
  else
    for( ; startline <= endline; startline++ )
      fprintf( stream, "%03d  %s %03d %03d %03d\n", startline,
                                            aliasarr[GetOpcode( startline )],
                                            GetAddress( startline, 1 ),
                                            GetAddress( startline, 2 ),
                                            GetAddress( startline, 3 ));

  fclose( stream );
  clrscr();

  return 1;
} //SaveAsText

//////////////////////////////////////////////////////////////////////////////
//Options changing and exit confirming functions://///////////////////////////
//////////////////////////////////////////////////////////////////////////////
void ChangeOptions( void )
{
  int tmpopts[NUM_OF_OPTS];
  int currentoption = -1;
  int tmpchr = KEY_RIGHT;
  int i;

  char* headings[NUM_OF_OPTS] =
  {
    "á≠†Á•≠®• ÔÁ•©™® 000\n\r¢c•£§† p†¢≠Æ ≠y´Ó",
    "á†¨Î™†‚Ï Ø†¨Ô‚Ï\n\r",
    "ÇÆcc‚†≠†¢´®¢†‚Ï Ø†¨Ô‚Ï\n\rØÆ ß†¢•pË•≠®® ØpÆ£p†¨¨Î",
    "èÆ™†ßÎ¢†‚Ï ÆØ®c†≠®Ô\n\r™Æ¨†≠§ ¢ p•§†™‚Æp•",
    "èÆ™†ßÎ¢†‚Ï ¢ ÆØ®c†≠®ÔÂ\n\r™Æ¨†≠§ ™Æ≠™p•‚≠Î• †§p•c†",
    "àcØÆ´ÏßÆ¢†‚Ï \"y¨≠yÓ\"\n\r¢c‚†¢™y/y§†´•≠®• ™Æ¨†≠§",
    "Ç·•£§† ØÆË†£Æ¢Î©\n\r¢Î¢Æ§ Á®·•´",
    "Ç™´ÓÁ®‚Ï ß¢„™Æ¢Î•\n\rÌ‰‰•™‚Î"
  };

  for( i = 0; i < NUM_OF_OPTS; i++ )
    tmpopts[i] = options[i];

  StatusMessage( "àß¨•≠•≠®• ≠†c‚pÆ•™ ØpÆ£p†¨¨Î" );
  window( 53, 16, 78, 23 );
  _setcursortype( _NOCURSOR );

  while( tmpchr != KEY_ENTER && tmpchr != KEY_ESC )
  {
    switch( tmpchr )
    {
      case KEY_LEFT:
        if( currentoption == 0 )
          currentoption = NUM_OF_OPTS - 2;
        else
          currentoption -= 2;

      case KEY_RIGHT:
        currentoption = ( currentoption + 1 ) % NUM_OF_OPTS;
        clrscr();
        gotoxy( 1, 1 );
        cprintf( "%s\n\r[ ] Ñ†\n\r[ ] H•‚", headings[currentoption] );
      break;

      case KEY_UP:
      case KEY_DOWN:
        tmpopts[currentoption] = 1 - tmpopts[currentoption];
      break;
    }

    if( tmpopts[currentoption] )
    {
      gotoxy( 2, 3 );
      putch( '*' );
      gotoxy( 2, 4 );
      putch( ' ' );
    }
    else
    {
      gotoxy( 2, 3 );
      putch( ' ' );
      gotoxy( 2, 4 );
      putch( '*' );
    }

    if( tmpchr == KEY_F2 )
    {
      StatusMessage( "ëÆÂ‡†≠•≠®• ≠†·‚‡Æ•™ ØÆ „¨Æ´Á†≠®Ó" );
      window( 53, 16, 78, 23 );
      clrscr();
      cprintf( "èÆ§‚¢•‡¶§†•‚• ·ÆÂ‡†≠•≠®•?\n\rEnter - OK\n\rEsc   - é‚¨•≠†" );

      while( tmpchr != KEY_ESC && tmpchr != KEY_ENTER )
        tmpchr = CheckForF1( KEY_UNKNOWN, -1 );

      if( tmpchr == KEY_ENTER )
      {
        FILE* tmpfl;
        int j;

        i = strlen( em3path );
        strcat( em3path, "options.dat" );
        tmpfl = fopen( em3path, "w" );
        em3path[i] = '\0';


        if( !tmpfl )
          ShowMessage( "  ç•¢Æß¨Æ¶≠Æ ®ß¨•≠®‚Ï ‰†©´", CH_MSG_CANTUPDATE );
        else
        {
          for( i = 0; i < NUM_OF_OPTS; i++ )
          {
            j = 0;

            while( headings[i][j] )
            {
              if( headings[i][j] != '\n' && headings[i][j] != '\r' )
                fputc( headings[i][j], tmpfl );
              else if( headings[i][j] == '\r' )
                fputc( ' ', tmpfl );

              j++;
            }

            fputc( '\n', tmpfl );

            if( tmpopts[i] )
              fputc( '+', tmpfl );
            else
              fputc( '-', tmpfl );

            fputc( '\n', tmpfl );
          }

          fclose( tmpfl );
          i = -1;
        }
      }

      if( i != -1 || tmpchr != KEY_ENTER )
      {
        clrscr();
        cprintf( "%s\n\r[ ] Ñ†\n\r[ ] H•‚", headings[currentoption] );
        StatusMessage( "àß¨•≠•≠®• ≠†c‚pÆ•™ ØpÆ£p†¨¨Î" );
        window( 53, 16, 78, 23 );
        tmpchr = -1;
      }
    }
    else
      tmpchr = CheckForF1( KEY_UNKNOWN, currentoption + CH_OPTIONS );
  }

  clrscr();
  _setcursortype( _NORMALCURSOR );

  if( tmpchr == KEY_ENTER || tmpchr == KEY_F2 )
    for( i = 0; i < NUM_OF_OPTS; i++ )
      options[i] = tmpopts[i];
} //ChangeOptions

int ConfirmExit( void )
{
  int tmpchr = KEY_UNKNOWN;

  if( !ismodflag )
    return KEY_F10;

  _setcursortype( _NOCURSOR );
  StatusMessage( "èÆ§‚¢•p¶§•≠®• ¢ÎÂÆ§† ®ß ØpÆ£p†¨¨Î" );
  window( 53, 16, 78, 23 );
  gotoxy( 1, 1 );
  clrscr();
  cprintf( "í•™yÈ†Ô ØpÆ£p†¨¨†\n\r≠• cÆÂp†≠•≠†.\n\rÇÎ §•©c‚¢®‚•´Ï≠Æ\n\rÂÆ‚®‚• ß†¢•pË®‚Ï\n\rp†°Æ‚y c Ì¨y´Ô‚ÆpÆ¨?\n\r" );
  cprintf( "F10 - ÇÎÂÆ§\n\r\F2  - ëÆÂ‡†≠®‚Ï ® ¢Î©‚®\n\rEsc - é‚¨•≠†" );

  while( tmpchr != KEY_ESC && tmpchr != KEY_F10 && tmpchr != KEY_F2 )
    tmpchr = CheckForF1( KEY_UNKNOWN, -1 );

  clrscr();
  _setcursortype( _NORMALCURSOR );

  if( tmpchr == KEY_F2 && SaveProgram( 0 ) )
    tmpchr = KEY_F10;

  return tmpchr;
}

//////////////////////////////////////////////////////////////////////////////
//Main function://////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
int main( int argc, char* argv[] )
{
  int tmpchr = KEY_UNKNOWN;
  int i;

  SetEM3Path( argv[0] );
  LoadData();
  inputfile[0] = 0;
  directvideo = 1;
  DrawMainWindow();
  _setcursortype( _NORMALCURSOR );
  InitFirstTime();
  ex += cmdmode;

  if( argc > 1 )
  {
    LoadAtStart( argv[1] );
    window( 53, 3, 78, 3 );
    gotoxy( 1, 1 );
    cprintf( "%s", visiblepath );
  }

  while( tmpchr != KEY_F10 )
  {
    tmpchr = Edit();

    switch( tmpchr )
    {
      case KEY_F2:      SaveProgram( 0 ); break;
      case KEY_ALT_F2:  SaveProgram( 1 ); break;
      case KEY_CTRL_F2: SaveAsText(); break;

      case KEY_F3:      LoadProgram(); break;

      case KEY_F7:      ChangeOptions(); break;

      case KEY_F9:      ExecuteProgram(); break;
      case KEY_CTRL_F9: stepmode = 1; ExecuteProgram(); stepmode = 0; break;

      case KEY_F10:     tmpchr = ConfirmExit(); break;
    }
  }

  window( 1, 1, 80, 25 );
  gotoxy( 1, 1 );
  textbackground( 0 );
  textcolor( 7 );
  clrscr();

  if( helpfile != NULL )
    fclose( helpfile );

  for( i = 0; i < 32; i++ )
  {
    free( aliasarr[i] );
    free( descarr[i] );
  }

  free( em3path );
  return 0;
}
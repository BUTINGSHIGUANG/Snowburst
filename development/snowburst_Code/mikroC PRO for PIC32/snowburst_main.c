/*
* Project name:
    Snowburst  V1.2
      
* Description:
    Tap to melt the snow before you are buried alive!
    
* Created By:
    Andrew Hazelden

* Email:
    andrewhazelden@gmail.com

* Blog:
    http://www.andrewhazelden.com


* Version 1.2 Released
    09/22/2012

* Version 1.1 Released
    12/31/2011
    
* Version 1 Released
    12/27/2011

* Generated by:
    Visual TFT

* Test configuration:
    MCU:             P32MX460F512L
    Dev.Board:       MikroMMB_for_PIC32_hw_rev_1.10
                    http://www.mikroe.com/eng/products/view/595/mikrommb-for-pic32-board/
  
    Optional Accessories:
    
                    Mikromedia WorkStation v7 
                    http://www.mikroe.com/eng/products/view/881/mikromedia-workstation/
  
                    Mikromedia Gaming Shield
                    http://www.mikroe.com/mikromedia/shields/gaming/
                      
    Oscillator:      80000000 Hz
    SW:              mikroC PRO for PIC32
                    http://www.mikroe.com/eng/products/view/623/mikroc-pro-for-pic32/


                    Mikromedia Workstation Board Support package (for MP3 playback WS_SPI_Init_Advanced function)
                    http://www.libstock.com/projects/view/368/mikromedia-workstation-v7-bsp
*/

#include "snowburst_objects.h"
#include "snowburst_resources.h"


//Hard
//#define NUMBER_OF_FLAKES 4

//Medium
#define NUMBER_OF_FLAKES 3

//Simple test
//#define NUMBER_OF_FLAKES 1


#define SCREEN_HEIGHT 240
#define SCREEN_WIDTH 320

#define MAX_SNOW_HEIGHT 200

#define STATUS_TEXT_HEIGHT 25

//How large the tapping "finger" is
#define TAP_RADIUS 15

extern void Set_Index(unsigned short index);
extern void Write_Command(unsigned short cmd);
extern void Write_Data(unsigned int _data);
extern void MP3_Start();
extern void Play_MP3();
extern void Play_MP3_Chunk();
extern void Load_MP3_File(char *filename);
extern void Set_Volume(char left,char right);
extern void UART1_Write_Line(char *uart_text);
extern void UART1_Write_Label_Var(char *uart_text, int var );
void UART1_Write_Label_Long_Var(char *uart_text, long var);
void UART1_Write_Label_Float_Var(char *uart_text, float var);
extern char IsCollision (unsigned int Shape_X, unsigned int Shape_Y,  unsigned int Shape_Width, unsigned int Shape_Height, unsigned int Button_Left, unsigned int Button_Top, unsigned int Button_Width, unsigned int Button_Height);

void Start_UART();

void GetNextSong();
void PreRollSong();
//void CheckVolume();


extern char file_loaded;
extern long current_pos;
extern unsigned long file_size;
extern int play_next_song = 0;

//Count how many songs have played
extern int song_count;

//previous sound file size and position
int prev_sound_pos = 0;
int prev_file_size = 0;

//Mute sound flag
int muteSound = 0;
int prev_muteSound = 0;

unsigned long frame_counter = 0;

//flake movement direction
int x_direction = 0;

//Track the player score
long score = 0;
long prev_score = 0;

//Track the high score since power on
//Future - Update with eeprom highscore save / load
long high_score = 0;

char level_text[11];
char score_text[11];

//The final status display string
char score_display_text[80];

//The final level display string
char level_display_text[80];

//Temp debug value printing string
char temp_txt[12];

long snow_height = 0;
long prev_snow_height = 0;

//Check for the game over status
int game_over = 0;

unsigned int X_Coord, Y_Coord, Prev_X_Coord, Prev_Y_Coord;

char Pen_Down = 0;
int Starting_Pen_Down_X_Coord = 0;
int Starting_Pen_Down_Y_Coord = 0;

int X_Drag_Distance = 0;


void flakeMissed(int flake_number);
void DrawFlake(int frame, int x, int y, int scale);
void Init_Sprites();
int GetRandom(int range);
void GetInput();
void TappedFlake();
void FlakeMissed(int flake_number);
void FlakeReset(int flake_number);
void DrawSnowFall();
void ShowGameOver();
void ShowTitles();
void InitGame();
void SavePrevValues();
void RenderScore();
void ClearFlake();
void RenderScreen();
void MoveFlakes();
void ToggleMute();

struct sprite {
  int x;
  int y;
  int prev_x;
  int prev_y;
  int frame;
  int scale;
  int width;
  int height;
  int speed;
};

//clear rect for old sprites
struct clear_region{
  int top;
  int left;
  int bottom;
  int right;
};

//struct clear_region clear_right_left_flake;
struct clear_region clear_up_down_flake;

struct sprite flakes[NUMBER_OF_FLAKES];


//background night sky color
int sky_color = 0x0928;


void main() {

  int i = 0;
  int flake_size = 0;

  
  //Set up the microcontroller
  Start_TP();
  
  //Start the UART
  Start_UART();
  
  //Set up the snow flake sprites
  Init_Sprites();

  //Clear the screen with a blue sky color
  TFT_Fill_Screen(sky_color);

  //Set up VLSI VS1011E chip sound playback
  file_loaded = 0;
  MP3_Start();
  
  //Set the font color to white
  TFT_Set_Font(TFT_defaultFont, CL_WHITE, FO_HORIZONTAL);

  //Load bg music
  //GetNextSong();
  
  //Write out startup text on the USB serial port.
  UART1_Write_Line("Starting Snowburst.");
  
  TP_TFT_Press_Detect();
  TP_TFT_Press_Detect();

  while (1){
    //Clear the screen with a blue sky color
    TFT_Fill_Screen(sky_color);
    
    //Show the main Snowburst titles
    ShowTitles();
    
    //Reset the game
    InitGame();
    
    while (!game_over) {

      //print the current frame
      if( (frame_counter % 20) == 0){
        UART1_Write_Label_Long_Var("Frame: ", frame_counter);
      }
      
      
      //Play the background music
      if(play_next_song){
        //Switch the current song if it is done playing
        GetNextSong();
      }
      else if( (frame_counter % 2) == 0){
        //Play a chunk of the current song
        Play_MP3_Chunk();
      }


      //Check for user input
      GetInput();

      //Move the snowflakes
      MoveFlakes();

      //Check if a snowflake has been tapped
      TappedFlake();

      //Clear the old snowflakes
      ClearFlake();

      //Update the screen
      RenderScreen();

      //Slow down the loop if the sound is muted
      if(muteSound){
        Delay_ms(25);
      }

      //Save the previous values
      SavePrevValues();
    }

    //Show the game over screen
    ShowGameOver();
  }
  
}


void MoveFlakes(){
  int i = 0;

  for(i=0;i<=(NUMBER_OF_FLAKES-1); i++){
    //Animate the snow flake falling
    flakes[i].y += flakes[i].speed;

    //Check if a snow flake landed on the ground
    if( (long)(flakes[i].y+flakes[i].height+flakes[i].speed) > (long)(240-snow_height-STATUS_TEXT_HEIGHT)){
      //The player missed a flake
      flakeMissed(i);

    }

  }
}


void Start_UART(){
  //WS_UART_Init(256000);

  UART1_Init(256000);             // INITIALIZE UART MODULE AT 256000 BAUD
  Delay_ms(100);                  // Wait for UART module to stabilize
}


void RenderScreen(){
  int i = 0;

  for(i=0;i<=(NUMBER_OF_FLAKES-1); i++){
    //DrawFlake(frame, x, y);
    DrawFlake(flakes[i].frame, flakes[i].x, flakes[i].y, flakes[i].scale);

    //Increment the frame counter
    flakes[i].frame++;

    if(flakes[i].frame > 13){
      flakes[i].frame = 1;
    }
  }

  //Draw the white snowbank
  DrawSnowFall();

  //Update the onscreen score
  RenderScore();

}

void InitGame(){
  //Reset the score
  score = 0;
  
  prev_score = 0;

  //Reset the snowbank height
  snow_height = 0;

  //Reset the game
  game_over = 0;
}

void FlakeReset(int flake_number){
  int offset = 0;  
  int top_clear_flake_pos = 0;
  
  int small_speed_boost = 0;
  int large_speed_boost = 0;
  
  
  //Speed up the game play if the score is over 40000 or 20000
  if(score > 20000){
    small_speed_boost = 2;
    large_speed_boost = 1;
  }
  else if(score > 10000){
    small_speed_boost = 2;
  }
  else if(score > 5000){
    small_speed_boost = 1;
  }
  else{
    small_speed_boost = 0;
    large_speed_boost = 0;
  }
  


  //Set the pen to 1px and blue
  TFT_Set_Pen(sky_color, 1);

  //Set the fill brush to a blue color
  TFT_Set_Brush(1, sky_color, 0, TOP_TO_BOTTOM, sky_color, sky_color);

  //Debug clear region
  //TFT_Set_Brush(1, CL_BLACK, 0, TOP_TO_BOTTOM, sky_color, sky_color);
  
  //Calculate the top of the clear snowflake area
  top_clear_flake_pos = flakes[flake_number].y-flakes[flake_number].speed;
  if(top_clear_flake_pos < 0){
    top_clear_flake_pos = 0;
  }
  
  //Clear the old snowflake
  TFT_Rectangle(flakes[flake_number].x,top_clear_flake_pos, flakes[flake_number].x+flakes[flake_number].width, flakes[flake_number].y+flakes[flake_number].height);

  //Snowflake position
  flakes[flake_number].x = ((320 + 32)/ NUMBER_OF_FLAKES) * flake_number + ( getRandom(120) - 60);
  
  //Keep the snowflakes onscreen
  if(  flakes[flake_number].x < 0){
    flakes[flake_number].x = 0;
  }
  if(  flakes[flake_number].x > 290){
    flakes[flake_number].x = 290;
  }

  
  flakes[flake_number].y = 0;
  
  flakes[flake_number].prev_x = flakes[flake_number].x;
  flakes[flake_number].prev_y = flakes[flake_number].y;

  //Randomize the snowflake animation frame number
  flakes[flake_number].frame = getRandom(13);

  //Randomize the flake size
  //flakes[flake_number].scale = 1;
  flakes[flake_number].scale = getRandom(2);
  
  //define the snow flake dimensions based upon the scale
  if(flakes[flake_number].scale == 1){
    flakes[flake_number].width = 32;
    flakes[flake_number].height = 32;
  }
  else{
    flakes[flake_number].width = 64;
    flakes[flake_number].height = 64;
  }


  //Randomize the flake falling speed
  //flakes[flake_number].speed = 1;
  //flakes[flake_number].speed = getRandom(3);
  //flakes[flake_number].speed = 3;
  
  //large flakes fall slowly and small ones fall quickly
  if (flakes[flake_number].scale  == 1){
    //small flake
    flakes[flake_number].speed = getRandom(4)+small_speed_boost;
  }
  else{
    //large flake
    flakes[flake_number].speed = getRandom(3)+large_speed_boost;
  }

}


void SavePrevValues(){
  int i;
  
  //Increment the frame counter
  frame_counter++;

  //Save snowbank height value
  prev_snow_height = snow_height;

  //Save the previous score
  prev_score = score;

  
  //previous sound on / off setting
  prev_muteSound = muteSound;
  
  //Save the previous snowflake position
  for(i=0;i<=(NUMBER_OF_FLAKES-1);i++){
    flakes[i].prev_x = flakes[i].x;
    flakes[i].prev_y = flakes[i].y;
  }
}

void FlakeMissed(int flake_number)
{

  //The player missed a small snowflake
  if(flakes[flake_number].scale == 1){

    //Slow down the snowbank fill rate near the top of the screen
    if(snow_height > 120){
      //A small flake was missed
      snow_height+=3;
    }
    else{ 
      //A small flake was missed
      snow_height+=7;
    }
    
  }
  //The player missed a large snowflake
  else if(flakes[flake_number].scale ==2){
    //Slow down the snowbank fill rate near the top of the screen
    if(snow_height > 120){
      //A large flake was missed
      snow_height+=7;
    }
    else{
      //A large flake was missed
      snow_height+=15;
    }
    
  }

  FlakeReset(flake_number);

}


void TappedFlake(){
  int i;
  
  int tapped_flake_size = 0;
  

  //Check if the pen is down and the drag distance is short
  if( Pen_Down && (abs(X_Drag_Distance) < 10)  ){

    //Scan for the position of the snowflakes
    for(i=0;i<NUMBER_OF_FLAKES;i++){

      //Check if the finger has tapped on a snowflake
      if(IsCollision (X_Coord-TAP_RADIUS, Y_Coord-TAP_RADIUS, TAP_RADIUS*2, TAP_RADIUS*2, flakes[i].x, flakes[i].y, flakes[i].width, flakes[i].height) ){


        if(flakes[i].scale == 1){
          //A small snowflake melted
          score+= 50;
        }
        else {
          //A big snowflake melted
          score+= 25;
        }

        //UART1_Write_Label_Var("You melted a snowflake:", i);

        //Rest the snowflake position
        FlakeReset(i);

      }

    }

  }
  
}

//Draw the snowflakes over the blue background
void DrawFlake(int frame, int x, int y, int scale){

  switch (frame) {
  case 0:
    TFT_Image(x, y, snowflake1_bmp, scale);
    break;

  case 1:
    TFT_Image(x, y, snowflake1_bmp, scale);
    break;

  case 2:
    TFT_Image(x, y, snowflake2_bmp, scale);
    break;

  case 3:
    TFT_Image(x, y, snowflake3_bmp, scale);
    break;

  case 4:
    TFT_Image(x, y, snowflake4_bmp, scale);
    break;

  case 5:
    TFT_Image(x, y, snowflake5_bmp, scale);
    break;

  case 6:
    TFT_Image(x, y, snowflake6_bmp, scale);
    break;

  case 7:
    TFT_Image(x, y, snowflake7_bmp, scale);
    break;

  case 8:
    TFT_Image(x, y, snowflake8_bmp, scale);
    break;

  case 9:
    TFT_Image(x, y, snowflake9_bmp, scale);
    break;

  case 10:
    TFT_Image(x, y, snowflake10_bmp, scale);
    break;

  case 11:
    TFT_Image(x, y, snowflake11_bmp, scale);
    break;

  case 12:
    TFT_Image(x, y, snowflake12_bmp, scale);
    break;

  case 13:
    TFT_Image(x, y, snowflake13_bmp, scale);
    break;

  case 14:
    TFT_Image(x, y, snowflake14_bmp, scale);
    break;

  default:
    TFT_Image(x, y, snowflake1_bmp, scale);
    
  }
  
  //Delay_ms(50);
}




//Set up the snowflakes
void Init_Sprites(){
  int random_x = 0;
  int i = 0;

  for(i=0;i<=(NUMBER_OF_FLAKES-1);i++){

    //Randomize the flake positions
    //flakes[i].x = getRandom(6) * 45;
    flakes[i].x = ((320 + 32)/ NUMBER_OF_FLAKES) * i + getRandom(32);
    flakes[i].y = getRandom(5) * 48;
    
    //save the previous flake position
    flakes[i].prev_x = flakes[i].x;
    flakes[i].prev_y = flakes[i].y;

    //Randomize the flake frame number
    //flakes[i].frame = 12;
    flakes[i].frame = getRandom(13);

    //Randomize the flake size
    //flakes[i].scale = 1;
    flakes[i].scale = getRandom(2);
    
    //define the snow flake dimensions based upon the scale
    if(flakes[i].scale == 1){
      flakes[i].width = 32;
      flakes[i].height = 32;
    }
    else{
      flakes[i].width = 64;
      flakes[i].height = 64;
    }

    //Randomize the flake falling speed
    //flakes[i].speed = 1;
    flakes[i].speed = getRandom(4);


  }

}

//Draw the snowbank with a gradient
void DrawSnowFall(){

  long screen_snow_height = 0;
  long prev_screen_snow_height = 0;

  //Check if the game has ended
  if(snow_height > (long)(240-64-STATUS_TEXT_HEIGHT)){
    game_over = 1;
  }

  if( (prev_muteSound != muteSound) || (snow_height != prev_snow_height) || (score != prev_score) || (frame_counter < 1) ){

    //Lower the snowbank height by 7 pixels every 500 points
    if(  ((score % (long)500) == 0) && (score>(long)50)){
      
      
      
      //Lower the bank faster near the top of the screen
      if (snow_height>64){
        //lower the snowbank
        snow_height-=50;
      }
      else{
        //lower the snowbank
        snow_height-=7;
      }

      //Fill the screen with the blue skycolor
      //TFT_Fill_Screen(sky_color);

      UART1_Write_Line("You lowered the snowbank.");
      
      //Keep the snow_height value positive
      if(snow_height<0){
        snow_height=0;
      }
      
      screen_snow_height = (long)240-snow_height-STATUS_TEXT_HEIGHT;
      prev_screen_snow_height = (long)240-prev_snow_height-STATUS_TEXT_HEIGHT;

      

      //Keep the snow_height value positive
      if(screen_snow_height<0){
        screen_snow_height=0;
      }
      
      //Keep the previous snow_height value positive
      if(prev_screen_snow_height<0){
        prev_screen_snow_height=0;
      }
      
      //Set the pen to 1px and blue
      TFT_Set_Pen(sky_color, 1);

      //Set the fill brush to a blue color
      TFT_Set_Brush(1, sky_color, 0, TOP_TO_BOTTOM, sky_color, sky_color);

      //clear the old snow height area
      TFT_Rectangle(0, prev_screen_snow_height, SCREEN_WIDTH, screen_snow_height);

      //UART1_Write_Label_Long_Var("Lowering the snow height", screen_snow_height);
      
    }



    //Turn off the pen
    TFT_Set_Pen(0, 0);

    //Set the fill brush to a solid white color
    //TFT_Set_Brush(1, CL_WHITE, 0, TOP_TO_BOTTOM, CL_WHITE, CL_WHITE);
    
    //Set up a white to gray gradient fill
    TFT_Set_Brush(1, 0, 1, TOP_TO_BOTTOM, CL_WHITE, CL_GRAY);
    
    //Debug clear region
    //TFT_Set_Brush(1, CL_BLACK, 0, TOP_TO_BOTTOM, CL_BLACK, CL_BLACK);

    //Draw the shaded snowbank
    TFT_Rectangle(0, 240-snow_height-STATUS_TEXT_HEIGHT, 319, 240);
    
  }
}


//Get a random number between 1 and the range
int GetRandom(int range){
  return (rand() % range) + 1;
}



void GetInput(){
  //Check if you tapped the screen
  if(TP_TFT_Press_Detect()){

    if (TP_TFT_Get_Coordinates(&X_Coord, &Y_Coord) == 0) {

      //Capture the starting press location
      if(Pen_Down == 0){
        Starting_Pen_Down_X_Coord = X_Coord;
        Starting_Pen_Down_Y_Coord = Y_Coord;
        //UART1_Write_Label_Var("Starting pen X:", Starting_Pen_Down_X_Coord);
        //UART1_Write_Label_Var("Starting pen Y:", Starting_Pen_Down_Y_Coord);
      }

      X_Drag_Distance =  Starting_Pen_Down_X_Coord - X_Coord;

      //The pen has been pressed onscreen
      Pen_Down = 1;

    }  //end of tap x / y location checking
    
  }
  else{
    //the pen was released
    Pen_Down = 0;
  }

  //Check if the sound mute bar has been pressed
  if( (Pen_Down == 1 )&& (Y_coord > (SCREEN_HEIGHT-STATUS_TEXT_HEIGHT)) ) {
    //Toggle the sound ON / OFF (Mute)
    ToggleMute();
    Delay_ms(500);
  }

}



void ClearFlake(){
  int i = 0;

  for(i=0;i<=(NUMBER_OF_FLAKES-1);i++){

    //flake moving left to right
    //clear_right_left_flake.left = 0;
    //clear_right_left_flake.top = 0;
    //clear_right_left_flake.right = 0;
    //clear_right_left_flake.bottom = 0;


    //temp disable right / left movement clearing
    //clear_up_down_flake.left = flakes[i].prev_x;
    clear_up_down_flake.left = flakes[i].x;
    clear_up_down_flake.top = flakes[i].prev_y;
    clear_up_down_flake.right = flakes[i].x + flakes[i].width;
    //clear_up_down_flake.right = flakes[i].prev_x + flakes[i].width;
    clear_up_down_flake.bottom = flakes[i].y;


    //keep the snowflake clear rect onscreen
    if (clear_up_down_flake.left < 0) {
      clear_up_down_flake.left = 0;
    }
    if (clear_up_down_flake.top < 0) {
      clear_up_down_flake.top = 0;
    }
    if (clear_up_down_flake.right < 0) {
      clear_up_down_flake.right = 0;
    }
    if (clear_up_down_flake.bottom < 0) {
      clear_up_down_flake.bottom = 0;
    }
    
    //Set the pen to 1px and blue
    TFT_Set_Pen(sky_color, 1);
    
    //Set the fill brush to a blue color
    TFT_Set_Brush(1, sky_color, 0, TOP_TO_BOTTOM, sky_color, sky_color);

    //Debug clear region
    //TFT_Set_Brush(1, CL_BLACK, 0, TOP_TO_BOTTOM, sky_color, sky_color);
    //UART1_Write_Label_Var("Clear flake rect:", i);
    //UART1_Write_Label_Var("Flake width:", flakes[i].width);
    //UART1_Write_Label_Var("left:", clear_up_down_flake.left);
    //UART1_Write_Label_Var("top:", clear_up_down_flake.top);
    //UART1_Write_Label_Var("right:", clear_up_down_flake.right);
    //UART1_Write_Label_Var("bottom:", clear_up_down_flake.bottom);

    //Selective flake region clear
    //clear the rect if the top of the clear rect is bigger than the bottom
    if( clear_up_down_flake.bottom > clear_up_down_flake.top){
      //TFT_Rectangle(clear_right_left_flake.left, clear_right_left_flake.top, clear_right_left_flake.right, clear_right_left_flake.bottom);
      TFT_Rectangle(clear_up_down_flake.left, clear_up_down_flake.top, clear_up_down_flake.right, clear_up_down_flake.bottom);
    }
  }
}





void ToggleMute(){
  //Turn on sound
  if(muteSound){
    muteSound = 0;
    UART1_Write_Line("Sound On");
  }
  else if(!muteSound){
    //Mute the sound
    muteSound = 1;
    UART1_Write_Line("Muting Sound");
  }

}


//Preroll the first few bytes of the song at the startup to load the MP3 header data
void PreRollSong(){
  int roll = 0;
  int roll_max = 100;   //100

  //Start playing the sound
  for(roll = 0; roll<=roll_max;roll++){
    Play_MP3_Chunk();
  }
}

//Load the next song
void GetNextSong(){
  UART1_Write_Line("Switching to the next song.");

  Load_MP3_File("tff.mp3");

  //Get the MP3 header data loaded before the game continues
  if(current_pos < 5){
    PreRollSong();
  }

}



//Display the Score in the status bar
void RenderScore(){

  //Display the sound status
  if(muteSound){
    //Sound off (muted) icon
    //TFT_Image(80, 2, soundMute_bmp, 1);
    TFT_Write_Text("Tap bar to enable music.", 10, 240-STATUS_TEXT_HEIGHT+3);
  }
  else if(!muteSound){
    //Sound on icon
    //TFT_Image(80, 2, sound_bmp, 1);
    TFT_Write_Text("Tap bar to mute music.", 10, 240-STATUS_TEXT_HEIGHT+3);
  }
  
  
  //Convert the level number from an int to string
  //IntToStr(level, level_text);

  //Convert the score from an int to string
  LongToStr(score, score_text);

  //Set up the font and font color
  TFT_Set_Font(TFT_defaultFont, sky_color, FO_HORIZONTAL);

  //Create the score string
  strcpy(score_display_text, "Score: ");
  strcat(score_display_text, score_text);
  
  //Write the score onscreen
  //TFT_Write_Text(score_display_text, 120, 240-STATUS_TEXT_HEIGHT+3);
  TFT_Write_Text(score_display_text, 200, 240-STATUS_TEXT_HEIGHT+3);
}


void ShowTitles(){
  int loop = 0;
  
  //Set the font color
  TFT_Set_Font(TFT_defaultFont, CL_WHITE, FO_HORIZONTAL);

  //Fill the screen with the blue sky color
  TFT_Fill_Screen(sky_color);

  //Display the title
  TFT_Image(32, 50, title_bmp, 1);
  
  TFT_Write_Text("Tap    to    melt    the    snowflakes.", 60, 140);
  
  TFT_Write_Text("Created   By   Andrew   Hazelden   (c) 2011-2012", 20, 220);
  
  GetNextSong();

  //Keep the sound playing
  //for(loop = 0; loop<=20;loop++){
  //   Play_MP3_Chunk();
  //   Delay_ms(50);
  // }

  //Slow down the title card if the sound is muted
  if(muteSound){
    Delay_ms(4000);
  }
  
  //Fill the screen with the blue sky color
  TFT_Fill_Screen(sky_color);

}

void ShowGameOver(){
  int loop = 0;

  UART1_Write_Line("Game Over");

  //Set the font color
  TFT_Set_Font(TFT_defaultFont, CL_WHITE, FO_HORIZONTAL);

  //Fill the screen with the blue sky color
  TFT_Fill_Screen(sky_color);
  
  //Display the game over title
  TFT_Image(32, 50, GameOver_bmp, 1);
  //TFT_Write_Text("GAME OVER", 130, 60);
  
  //Check if you set a high score:
  if(score > high_score){
    //You have a high score
    high_score = score;
    TFT_Write_Text("You  set  a  High  Score!", 95, 140);
  }
  else{
    //Show the previous high score
    
    //Convert the high_score from an int to string
    LongWordToStr(high_score, score_text);
    strcpy(score_display_text, "High  Score: ");
    strcat(score_display_text, score_text);

    //Write the high score
    TFT_Write_Text(score_display_text, 110, 140);
  }
  
  //Convert the score from an int to a string
  LongWordToStr(score, score_text);
  strcpy(score_display_text, "Your  Score: ");
  strcat(score_display_text, score_text);

  //Write the score
  TFT_Write_Text(score_display_text, 110, 170);

  //Print the score out on the UART 2
  UART1_Write_Line(score_display_text);

  TFT_Write_Text("Tip:   Melt   the   small   snowflakes   first!", 45, 215);

  //Keep the sound playing
  for(loop = 0; loop<=60;loop++){
    Play_MP3_Chunk();
    Delay_ms(50);
    
    if(muteSound){
      Delay_ms(20);
    }

  }

  //Delay_ms(3000);

  //Fill the screen with the blue sky color
  TFT_Fill_Screen(sky_color);

}
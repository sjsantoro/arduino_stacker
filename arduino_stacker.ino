/*
  Arduino Stacker
  Copyright (C) 2016 Santiago Santoro <sjsantoro@gmail.com>
  
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*----------------[Port Constants]----------*/
const int DIN = 8;
const int CS = 9;
const int CLK = 10;
const int BUTTON = 12;

/*----------------[Global Vars]----------*/
int status = 0;
/* ----------------[Game vars]------------*/
int pos = 0;
int dir = 0;
int current_row = 7;

byte dots[8] =
{
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x03
};

/* Set current dots this will then have to be compared to
 * the rows of the previous row and checked if the player missed by one and then assign the current dots to this.
 */
byte current_dots = dots[7];

int one_dot = false;
int game_speed = 150;
/* -------------------------------------- */
/* ------------[Heart Vars]---------------*/
byte heart_dots[8] =
{
  0x24,
  0x5A,
  0x81,
  0x81,
  0x42,
  0x42,
  0x24,
  0x18
};
/* -------------------------------------- */

void writeByte(byte data)
{
  byte i = 8;
  byte mask;
  
  // Loop while we have bits to write.
  while(i > 0)
  {
    mask = 0x01 << (i - 1);
    
    // if i = 8 - 1 -> i is 7
     //mask = 0b0000 0001;
     // after bitwise shift
     //mask = 0b0000 0010;
    
    digitalWrite(CLK, LOW);
    digitalWrite(13, HIGH);
    
    if(data & mask)
    {
      digitalWrite(DIN, HIGH);
    }
    else
    {
      digitalWrite(DIN, LOW);
    }
    
    digitalWrite(CLK, HIGH);
    digitalWrite(13, LOW);
    
    i--;
  }
}

void clearAll()
{
  for(int i = 0; i <= 8; i++)
  {
    // Write blank bits
     maxWrite(i, 0x00);
  }
}

void maxWrite(byte reg, byte col)
{
  digitalWrite(CS, LOW);
  
  writeByte(reg);
  writeByte(col);
  
  digitalWrite(CS, LOW);
  digitalWrite(CS, HIGH);
}

void setup()
{
  pinMode(DIN, OUTPUT);
  pinMode(CS, OUTPUT);
  pinMode(CLK, OUTPUT);
  pinMode(BUTTON, INPUT);
  pinMode(13, OUTPUT); // LED Test
  
  // Wait for MAX to start
  delay(200);
 
  maxWrite(0x0B, 0x07);// scanLimit
  maxWrite(0x09, 0x00); // No decode mode
  maxWrite(0x0C, 0x01); // Not in shutdown
  maxWrite(0x0F, 0x00); // Not in display test
  maxWrite(0x0A,0x0F & 0x0F); // First value that can be set
  
  Serial.begin(9600);
  Serial.write("All systems are go!\n");
}

void loop()
{
  if(status == 0) /* in-game */
  {
    gameLoop();
  }
  else if(status == 1) /* Game won! */
  {
    wonLoop();
  }
}

void countDots()
{
   // Check how many dots we have.
   int count = 0;
   int mask;
   for(int i = 0; i <= 7; i++)
    {
      mask = 0x01 << i;
      if(mask & current_dots)
      {
        count++;
      }
    }
    if(count > 1)
    {
      one_dot = false;
    }
    else
    { 
      // This fixes a bug
      if(one_dot == false)
      {
        current_dots = 0x01;
        pos = 0;
        dir = 0;
      }
              
      one_dot = true;
    }
}

void resetGame()
{
  // Reset game.
            for(int i = 0; i <= 6; i++)
            {
              dots[i] = 0x00;
            }
            dots[7] = 0x03;
            current_row = 7;
            current_dots = dots[7];
            pos = 0;
            dir = 0;
            one_dot = false;
            game_speed = 150;
}

void gameLoop()
{
  // Check for direction and change pos.
  if(dir == 0)
  {
    pos++;
    if(one_dot == false)
    {
      if(pos == 6)
      {
        dir = 1;
      }
    }
    else
    {
      if(pos == 7)
      {
        dir = 1;
      }
    }
  }
  else if(dir == 1)
  {
    pos--;
    if(pos == 0)
    {
      dir = 0;
    }
  }
  
  // Change pos of dots
  dots[current_row] = (current_dots << pos);
  
  // Draw all dots to LED Matrix
  for(int i = 0; i <= 7; i++)
  {
    maxWrite(i + 1, dots[i]);
  }
    
    if(digitalRead(BUTTON) == HIGH)
    {
      delay(100);
      if(current_row >= 0)
      {
        if(current_row < 7)
        {
          // Check if the dots coincide useing the AND bitwise operator.
          if((dots[current_row] & dots[current_row + 1]) > 0)
          {
            // Player wins!
            if(current_row == 0)
            {
              status = 1;
              return;
            }
            // Set the dots of the row we are on to match the last row.
            dots[current_row] = dots[current_row + 1] & dots[current_row];
            // Shift the bits back to where they were in the first place after comparing the last dots and the current ones.
            current_dots = ((dots[current_row] & dots[current_row + 1]) >> pos);
            
            countDots();
            
            if(current_row == 3 && one_dot == false)
            {
              current_dots = 0x01;
              pos = 0;
              dir = 0;
              one_dot = true;
            }
            
            // Increase game speed
            game_speed -= 20;
            
            // Move down a row.
            current_row--;
          }
          else
          {
            resetGame();
          }
        }
        else
        {
          current_row--;
        }
      }
    }
  
  // Game delay
  delay(game_speed);
}

void wonLoop()
{
  for(int i = 0; i <= 7; i++)
  {
    maxWrite(i + 1, heart_dots[i]);
  }
  
  delay(500);
  
  clearAll();
  
  delay(500);
  
  if(digitalRead(BUTTON) == HIGH)
  {
    resetGame();
    status = 0;
    return;
  }
}


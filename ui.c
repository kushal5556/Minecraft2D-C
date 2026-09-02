#include "raylib.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

// ----- TODO ----
// -> 

#define WIDTH 800
#define HEIGHT 600

// --- global variables --------
const Color SILVER = {192, 192, 192, 255};

#define PRESSED_COUNTER 0.3f //seconds
#define DT 1.0f/60.0f


// --- structs ------
// --- Core structs -----
typedef struct{
    Color normal;
    Color hover;
    Color active;
}DColor;

typedef enum{
    NORMAL, HOVER, PRESSED
}State;

typedef struct{
    Rectangle rec;
    Color color;
    State state;
    char label[100];
    Color textColor;

    float pressCounter;
}ActionButton_D; //Detailed Action Button

/// ----- function declaration-----------
ActionButton_D getActionButton_D(float x, float y, float width, float height, char* label); //return the default button_D

void updateActionButton_D(ActionButton_D* button, Vector2 mouse);

void drawActionButton_D(ActionButton_D button); 
void drawLabelText(Rectangle rec, char* label, int textOffsetX, int textOffsetY, Color color);

// --- collision function -----------
bool rect_point_collision(Rectangle rec, Vector2 point);

// int main(){return 1;}

// ---- function definition -------
bool rect_point_collision(Rectangle rec, Vector2 point)
{
    if(point.x >= rec.x && point.x <= rec.x + rec.width &&
       point.y >= rec.y && point.y <= rec.y + rec.height){ 
            return true;
    }
    return false;
}

//------ get panel/buttons ---------


ActionButton_D getActionButton_D(float x, float y, float width, float height, char* label)
{
    ActionButton_D button = {
        .rec = (Rectangle){x,y,width,height},
        .color = SILVER,
        .textColor = BLACK,
        .state = NORMAL,
        .pressCounter = PRESSED_COUNTER
    };
    strcpy(button.label, label);

    return button; 
}

void updateActionButton_D(ActionButton_D* button, Vector2 mouse)
{
    if(button->pressCounter != PRESSED_COUNTER || button->pressCounter < PRESSED_COUNTER){
        button->pressCounter -= DT;
        if(button->pressCounter <= 0.0f) button->pressCounter = PRESSED_COUNTER;
    } 

    button->state = NORMAL;//reset

    if(rect_point_collision(button->rec, mouse)){
        button->state = HOVER;
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
            button->state = PRESSED;
            button->pressCounter -= DT;
        }
    }
}

// --------- draw panels/buttons --------------
void drawActionButton_D(ActionButton_D button)
{
    //base
    DrawRectangleRec(button.rec, button.color);
    if(button.pressCounter < PRESSED_COUNTER){
          //outer 
            DrawLine(button.rec.x, button.rec.y, button.rec.x + button.rec.width, button.rec.y, BLACK); 
            DrawLine(button.rec.x, button.rec.y, button.rec.x, button.rec.y+button.rec.height, BLACK); 

            //inner
            DrawLine(button.rec.x, button.rec.y+1, button.rec.x + button.rec.width, button.rec.y+1, DARKGRAY); 
            DrawLine(button.rec.x+1, button.rec.y, button.rec.x+1, button.rec.y+button.rec.height, DARKGRAY); 

            //outer
            DrawLine(button.rec.x-1, button.rec.y+button.rec.height, button.rec.x + button.rec.width, button.rec.y +button.rec.height, WHITE); 
            DrawLine(button.rec.x + button.rec.width, button.rec.y, button.rec.x + button.rec.width, button.rec.y +button.rec.height, WHITE); 

            //inner
            DrawLine(button.rec.x, button.rec.y+button.rec.height-1, button.rec.x + button.rec.width, button.rec.y +button.rec.height-1, WHITE); 
            DrawLine(button.rec.x + button.rec.width-1, button.rec.y, button.rec.x + button.rec.width-1, button.rec.y +button.rec.height, WHITE); 
    }else{
        if(button.state == NORMAL){
            //top/left border (highlight) (double layer)
            //outer 
            DrawLine(button.rec.x, button.rec.y, button.rec.x + button.rec.width, button.rec.y, WHITE); 
            DrawLine(button.rec.x, button.rec.y, button.rec.x, button.rec.y+button.rec.height, WHITE); 

            //inner
            DrawLine(button.rec.x, button.rec.y+1, button.rec.x + button.rec.width, button.rec.y+1, WHITE); 
            DrawLine(button.rec.x+1, button.rec.y, button.rec.x+1, button.rec.y+button.rec.height, WHITE); 

            //bottom/right border (shadow)
            //outer
            DrawLine(button.rec.x-1, button.rec.y+button.rec.height, button.rec.x + button.rec.width, button.rec.y +button.rec.height, BLACK); 
            DrawLine(button.rec.x + button.rec.width, button.rec.y-1, button.rec.x + button.rec.width, button.rec.y +button.rec.height, BLACK); 

            //inner
            DrawLine(button.rec.x, button.rec.y+button.rec.height-1, button.rec.x + button.rec.width, button.rec.y +button.rec.height-1, DARKGRAY); 
            DrawLine(button.rec.x + button.rec.width-1, button.rec.y, button.rec.x + button.rec.width-1, button.rec.y +button.rec.height, DARKGRAY); 
        }else if(button.state == HOVER){
            //extra outer
            DrawLine(button.rec.x-2, button.rec.y-1, button.rec.x + button.rec.width+1, button.rec.y-1, WHITE); 
            DrawLine(button.rec.x-2, button.rec.y-1, button.rec.x-1, button.rec.y+button.rec.height, WHITE); 

            //outer 
            DrawLine(button.rec.x, button.rec.y, button.rec.x + button.rec.width, button.rec.y, WHITE); 
            DrawLine(button.rec.x, button.rec.y, button.rec.x, button.rec.y+button.rec.height, WHITE); 

            //inner
            DrawLine(button.rec.x, button.rec.y+1, button.rec.x + button.rec.width, button.rec.y+1, WHITE); 
            DrawLine(button.rec.x+1, button.rec.y, button.rec.x+1, button.rec.y+button.rec.height, WHITE); 

            //extra outer
            DrawLine(button.rec.x-2, button.rec.y+button.rec.height+1, button.rec.x + button.rec.width+1, button.rec.y +button.rec.height+1, BLACK); 
            DrawLine(button.rec.x + button.rec.width+1, button.rec.y-2, button.rec.x + button.rec.width+1, button.rec.y +button.rec.height+1, BLACK); 

            //outer
            DrawLine(button.rec.x-1, button.rec.y+button.rec.height, button.rec.x + button.rec.width, button.rec.y +button.rec.height, BLACK); 
            DrawLine(button.rec.x + button.rec.width, button.rec.y-1, button.rec.x + button.rec.width, button.rec.y +button.rec.height, BLACK); 

            //inner
            DrawLine(button.rec.x, button.rec.y+button.rec.height-1, button.rec.x + button.rec.width, button.rec.y +button.rec.height-1, DARKGRAY); 
            DrawLine(button.rec.x + button.rec.width-1, button.rec.y, button.rec.x + button.rec.width-1, button.rec.y +button.rec.height, DARKGRAY); 
        }else if(button.state == PRESSED){
            //outer 
            DrawLine(button.rec.x, button.rec.y, button.rec.x + button.rec.width, button.rec.y, BLACK); 
            DrawLine(button.rec.x, button.rec.y, button.rec.x, button.rec.y+button.rec.height, BLACK); 

            //inner
            DrawLine(button.rec.x, button.rec.y+1, button.rec.x + button.rec.width, button.rec.y+1, DARKGRAY); 
            DrawLine(button.rec.x+1, button.rec.y, button.rec.x+1, button.rec.y+button.rec.height, DARKGRAY); 

            //outer
            DrawLine(button.rec.x-1, button.rec.y+button.rec.height, button.rec.x + button.rec.width, button.rec.y +button.rec.height, WHITE); 
            DrawLine(button.rec.x + button.rec.width, button.rec.y, button.rec.x + button.rec.width, button.rec.y +button.rec.height, WHITE); 

            //inner
            DrawLine(button.rec.x, button.rec.y+button.rec.height-1, button.rec.x + button.rec.width, button.rec.y +button.rec.height-1, WHITE); 
            DrawLine(button.rec.x + button.rec.width-1, button.rec.y, button.rec.x + button.rec.width-1, button.rec.y +button.rec.height, WHITE); 
        }
    }

    int textOffsetX = 0; 
    int textOffsetY = 0;

    if(button.state == PRESSED || button.pressCounter < PRESSED_COUNTER){
        textOffsetX = 1;
        textOffsetY = 1;
    }else if(button.state == HOVER){
        textOffsetX = -1;
        textOffsetY = -1;
    }

    drawLabelText(button.rec, button.label, textOffsetX, textOffsetY, button.textColor);
}


void drawLabelText(Rectangle rec, char* label, int textOffsetX, int textOffsetY, Color color)
{
    //minimum padding
    int padX = 12;
    int padY = 8;

    int availableWidth  = rec.width - padX;
    int availableHeight = rec.height - padY;
    int textLength = strlen(label);

    if(textLength == 0) textLength = 1;

    int sizeBasedOnWidth = availableWidth / textLength;
    int sizeBasedOnHeight = availableHeight; 

    int textSize = (sizeBasedOnWidth < sizeBasedOnHeight) ? sizeBasedOnWidth:sizeBasedOnHeight;

    textSize *= 1.5; //scale up

    //calculate centering coordinates
    int measuredWidth = MeasureText(label, textSize);
    int  x = rec.x + (rec.width - measuredWidth)/2;
    int  y = rec.y + (rec.height - textSize)/2;
    
    DrawText(label, x + textOffsetX, y + textOffsetY,textSize, color);
}


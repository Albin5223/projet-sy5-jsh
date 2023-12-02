#include "colors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Add the color to the string, and put the color back to white at the end of the string
 * @param string The string to color
 * @param c The color to add
*/
void color_switch(char **string,enum color c){

    char *new_string = calloc((strlen(*string) + 2*strlen("\033[37m") + 1), sizeof(char));  // Allocating the new string

    const char *color;  // The color to add
    switch(c){  // Choosing the color
        case red:
            color = "\033[31m";
            break;
        case green:
            color = "\033[32m";
            break;
        case blue:
            color = "\033[34m";
            break;
        case yellow:
            color = "\033[33m";
            break;
        case cyan:
            color = "\033[36m";
            break;
        case magenta:
            color = "\033[35m";
            break;
        default:    // white
            color = "\033[37m";
            break;
    }

    strcat(new_string,color);   // Adding the color
    strcat(new_string,*string); // Adding the string
    strcat(new_string,"\033[37m");  // Adding the white color

    free(*string);  // Freeing the old string
    *string = new_string;   // Changing the pointer to the new string
}
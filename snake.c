#include <CoreFoundation/CoreFoundation.h>
#include "curses.h"

WINDOW * g_mainwin;
int g_oldcur, g_score = 0, g_width, g_height;
// Gold finger
bool gold_finger = false;
typedef struct {
    int x;
    int y;
} pos;
pos fruit;

// 2D array of all spaces on the board.
bool *spaces;

// --------------------------------------------------------------------------
// Queue stuff

// Queue implemented as a doubly linked list
struct s_node
{
    pos *position; // **TODO: make this a void pointer for generality.
    struct s_node *prev;
    struct s_node *next;
} *front=NULL, *back=NULL;
typedef struct s_node node;

// Returns the position at the front w/o dequeing
pos* peek( )
{
    return front == NULL ? NULL : front->position;
}

// Returns the position at the front and dequeues
pos* dequeue( )
{
    node *oldfront = front;
    front = front->next;
    return oldfront->position;
}

// Queues a position at the back
void enqueue( pos position )
{
   pos *newpos   = (pos*)  malloc( sizeof( position ) );
   node *newnode = (node*) malloc( sizeof( node ) );

   newpos->x = position.x;
   newpos->y = position.y;
   newnode->position = newpos;

   if( front == NULL && back == NULL )
       front = back = newnode;
   else
   {
       back->next = newnode;
       newnode->prev = back;
       back = newnode;
   }
}
// --------------------------------------------------------------------------
// End Queue stuff

// --------------------------------------------------------------------------
// Snake stuff

// Writes text to a coordinate
void snake_write_text( int y, int x, char* str )
{
    mvwaddstr( g_mainwin, y , x, str );
}

// Draws the borders
void snake_draw_board( )
{
    int i;
    for( i=0; i<g_height; i++ )
    {
        snake_write_text( i, 0,         "X" );
        snake_write_text( i, g_width-1, "X" );
    }
    for( i=1; i<g_width-1; i++ )
    {
        snake_write_text( 0, i,          "X" );
        snake_write_text( g_height-1, i, "X" );
    }
    snake_write_text( g_height+1, 2, "Score:" );
}

// Resets the terminal window and exit
void snake_game_over( )
{
    endwin();
    printf("Final score: %d\n", g_score);
    exit(0);
}

// Is the current position in bounds?
bool snake_in_bounds( pos position )
{
    return position.y < g_height - 1 && position.y > 0 && position.x < g_width - 1 && position.x > 0;
}

// 2D matrix of possible positions implemented with a 1D array. This maps
// the x,y coordinates to an index in the array.
int snake_cooridinate_to_index( pos position )
{
    return g_width * position.y + position.x;
}

// Similarly this functions maps an index back to a position
pos snake_index_to_coordinate( int index )
{
    int x = index % g_width;
    int y = index / g_width;
    return (pos) { x, y };
}

// Draw the fruit somewhere randomly on the board
void snake_draw_fruit( )
{
    attrset( COLOR_PAIR( 3 ) );
    int idx;
    do
    {
        idx = rand( ) % ( g_width * g_height );
        fruit = snake_index_to_coordinate( idx );
    }
    while( spaces[idx] || !snake_in_bounds( fruit ) );
    snake_write_text( fruit.y, fruit.x, "F" );
}

// Handles moving the snake for each iteration
void snake_move_player( pos head )
{
    attrset( COLOR_PAIR( 1 ) ) ;

    // Check if we ran into ourself
    int idx = snake_cooridinate_to_index( head );
    if( !gold_finger && spaces[idx] )
        snake_game_over( );
    spaces[idx] = true; // Mark the space as occupied
    enqueue( head );
    g_score += 10;

    // Check if we're eating the fruit
    if( head.x == fruit.x && head.y == fruit.y )
    {
        snake_draw_fruit( );
        g_score += 1000;
    }
    else
    {
        // Handle the tail
        pos *tail = dequeue( );
        spaces[snake_cooridinate_to_index( *tail )] = false;
        if ( tail->y == 0 || tail->y == (g_height-1) || tail->x == 0 || tail->x == (g_width-1) ) {
            attrset( COLOR_PAIR( 0 ) );
            snake_write_text( tail->y, tail->x, "X" );
            attrset( COLOR_PAIR( 1 ) ) ;
        }
        else
            snake_write_text( tail->y, tail->x, " " );
    }

    // Draw the new head
    snake_write_text( head.y, head.x, "S" );

    // Update scoreboard
    char buffer[25];
    sprintf( buffer, "%d", g_score );
    attrset( COLOR_PAIR( 2 ) );
    snake_write_text( g_height+1, 9, buffer );

}

bool is_number( const char *num )
{
    if ( strcmp( num, "0" ) == 0 )
        return true;
    const char* p = num;
    if ( *p < '1' || *p > '9' )
        return false;
    else
        p++;
    while ( *p ) {
        if( *p < '0' || *p > '9' )
            return false;
        else
            p++;
    }
    return true;
}

bool valid( int in, int key )
{
    switch( in )
    {
        case KEY_DOWN:
        case 'j':
        case 'J':
        case 's':
        case 'S':
            if ( key != KEY_UP && key != 'k' && key != 'K' && key != 'w' && key != 'W' )
                return true;
            break;
        case KEY_RIGHT:
        case 'l':
        case 'L':
        case 'd':
        case 'D':
            if ( key != KEY_LEFT && key != 'h' && key != 'H' && key != 'a' && key != 'A' )
                return true;
            break;
        case KEY_UP:
        case 'k':
        case 'K':
        case 'w':
        case 'W':
            if ( key != KEY_DOWN && key != 'j' && key != 'J' && key != 's' && key != 'S' )
                return true;
            break;
        case KEY_LEFT:
        case 'h':
        case 'H':
        case 'a':
        case 'A':
            if ( key != KEY_RIGHT && key != 'l' && key != 'L' && key != 'd' && key != 'D' )
                return true;
            break;
    }
    return false;
}

int main( int argc, char *argv[] )
{
    int key = KEY_RIGHT;
    if( ( g_mainwin = initscr() ) == NULL ) {
        perror( "error initialising ncurses" );
        return( EXIT_FAILURE );
    }

    // Set up
    srand( ( unsigned int )time( NULL ) );
    noecho( );
    curs_set( 2 );
    halfdelay( 1 );
    keypad( g_mainwin, TRUE );
    g_oldcur = curs_set( 0 );
    start_color( );
    init_pair( 1, COLOR_RED,     COLOR_BLACK );
    init_pair( 2, COLOR_GREEN,   COLOR_BLACK );
    init_pair( 3, COLOR_YELLOW,  COLOR_BLACK );
    init_pair( 4, COLOR_BLUE,    COLOR_BLACK );
    init_pair( 5, COLOR_CYAN,    COLOR_BLACK );
    init_pair( 6, COLOR_MAGENTA, COLOR_BLACK );
    init_pair( 7, COLOR_WHITE,   COLOR_BLACK );
    getmaxyx( g_mainwin, g_height, g_width );

    if ( g_width < 10 || g_height < 14 ) {
        printf("Terminal screen too small.\n");
        return 1;
    }

    g_height -= 4;

    if ( argc == 3 ) {
        if ( is_number( argv[1] ) && is_number( argv[2] ) ) {
            int width = atoi( argv[1] ), height = atoi( argv[2] );
            if ( width < 10 )
                width = 10;
            if ( height < 10 )
                height = 10;
            if ( width < g_width )
                g_width = width;
            if ( height < g_height )
                g_height = height;
        }
    }

    // Set up the 2D array of all spaces
    spaces = (bool*) malloc( sizeof( bool ) * g_height * g_width );

    snake_draw_board( );
    snake_draw_fruit( );
    pos head = { 5,5 };
    enqueue( head );

    // Event loop
    while( 1 )
    {
        int in = getch( );
        // Press the esc key to exit the game
        if( in == 0x1b )
            snake_game_over( );
        // Press the tab key to enable/disable gold finger
        if( in == 0x9 )
            gold_finger = !gold_finger;
        if( in != ERR && valid( in, key ) )
            key = in;
        switch( key )
        {
            case KEY_DOWN:
            case 'j':
            case 'J':
            case 's':
            case 'S':
                head.y++;
                break;
            case KEY_RIGHT:
            case 'l':
            case 'L':
            case 'd':
            case 'D':
                head.x++;
                break;
            case KEY_UP:
            case 'k':
            case 'K':
            case 'w':
            case 'W':
                head.y--;
                break;
            case KEY_LEFT:
            case 'h':
            case 'H':
            case 'a':
            case 'A':
                head.x--;
                break;

        }
        if( !gold_finger && !snake_in_bounds( head ) )
            snake_game_over( );
        else
            snake_move_player( head );
    }
}

/****************************************************************************
 * IPL_FONT HEADER
 ****************************************************************************/

#define MARGIN 42

void init_font(void);
void write_font(int x, int y, const char *string);
void writex(int x, int y, int sx, int sy, const unsigned char *string, int lookup);
void scroller(int y, unsigned char text[][512], int nlines);

int scrollerx;

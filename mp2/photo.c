/*									tab:8
 *
 * photo.c - photo display functions
 *
 * "Copyright (c) 2011 by Steven S. Lumetta."
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 * 
 * IN NO EVENT SHALL THE AUTHOR OR THE UNIVERSITY OF ILLINOIS BE LIABLE TO 
 * ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL 
 * DAMAGES ARISING OUT  OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, 
 * EVEN IF THE AUTHOR AND/OR THE UNIVERSITY OF ILLINOIS HAS BEEN ADVISED 
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * THE AUTHOR AND THE UNIVERSITY OF ILLINOIS SPECIFICALLY DISCLAIM ANY 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE 
 * PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND NEITHER THE AUTHOR NOR
 * THE UNIVERSITY OF ILLINOIS HAS ANY OBLIGATION TO PROVIDE MAINTENANCE, 
 * SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS."
 *
 * Author:	    Steve Lumetta
 * Version:	    3
 * Creation Date:   Fri Sep  9 21:44:10 2011
 * Filename:	    photo.c
 * History:
 *	SL	1	Fri Sep  9 21:44:10 2011
 *		First written (based on mazegame code).
 *	SL	2	Sun Sep 11 14:57:59 2011
 *		Completed initial implementation of functions.
 *	SL	3	Wed Sep 14 21:49:44 2011
 *		Cleaned up code for distribution.
 */


#include <string.h>

#include "assert.h"
#include "modex.h"
#include "photo.h"
#include "photo_headers.h"
#include "world.h"


/* types local to this file (declared in types.h) */

/* 
 * A room photo.  Note that you must write the code that selects the
 * optimized palette colors and fills in the pixel data using them as 
 * well as the code that sets up the VGA to make use of these colors.
 * Pixel data are stored as one-byte values starting from the upper
 * left and traversing the top row before returning to the left of
 * the second row, and so forth.  No padding should be used.
 */
struct photo_t {
    photo_header_t hdr;			/* defines height and width */
    uint8_t        palette[192][3];     /* optimized palette colors */
    uint8_t*       img;                 /* pixel data               */
};

/* 
 * An object image.  The code for managing these images has been given
 * to you.  The data are simply loaded from a file, where they have 
 * been stored as 2:2:2-bit RGB values (one byte each), including 
 * transparent pixels (value OBJ_CLR_TRANSP).  As with the room photos, 
 * pixel data are stored as one-byte values starting from the upper 
 * left and traversing the top row before returning to the left of the 
 * second row, and so forth.  No padding is used.
 */
struct image_t {
    photo_header_t hdr;			/* defines height and width */
    uint8_t*       img;                 /* pixel data               */
};


/* 
 * octree_node_t 
 */
typedef struct octree_node_t 
{
	// used to store sum of rgb respectively-> calculate average
	unsigned r_sum;
	unsigned g_sum;
	unsigned b_sum;
	unsigned count;	//number of pixels in this node
	unsigned index;	//index of the node in the octree

	unsigned r_avg;
	unsigned g_avg;
	unsigned b_avg;
} octree_node_t;


/* 
 * remember_node_t: store necessary information for each pixel in the first loop to avoid loop again
 */
typedef struct remember_node_t 
{
	//int offset;	//offset of the pixel in the image file
	int match_node;	//the match node in the second/fourth level octree, 0-4098
	uint16_t original_pixel;	//original 5:6:5 RGB values
	int find_in_fourth_level;	//whether the match node is found in the fourth level octree 0/1
} remember_node_t;

/* 
 * compare_octree_node_t
 *   DESCRIPTION: helper function for qsort
 *   INPUTS: a, b -- two octree nodes
 *   OUTPUTS: none
 *   RETURN VALUE: second node's count - first node's count
 *   SIDE EFFECTS: none
 *                 
 */
int compare_octree_node_t(const void *a, const void *b)
{
	octree_node_t *node_a = (octree_node_t *)a;
	octree_node_t *node_b = (octree_node_t *)b;
	return node_b->count - node_a->count;
}


/* 
 * get_r
 *   DESCRIPTION: get 6 bits value of red
 * 	 INPUTS: pixel -- 16 bits pixel value
 *   OUTPUTS: none
 *   RETURN VALUE: 6 bits value of red
 *   SIDE EFFECTS: none
 *                 
 */
uint8_t get_r(uint16_t pixel)
{
	return (pixel >> 11) & 0x1F;	//right shift 11 bits totally contain red value, then mask higher bits
}
/* 
 * get_g
 *   DESCRIPTION: get 6 bits value of green
 * 	 INPUTS: pixel -- 16 bits pixel value
 *   OUTPUTS: none
 *   RETURN VALUE: 6 bits value of green
 *   SIDE EFFECTS: none
 *                 
 */
uint8_t get_g(uint16_t pixel)
{
	return (pixel >> 5) & 0x3F;		//right shift 5 bits to obtain green value, then mask higher bits
}
/* 
 * get_b
 *   DESCRIPTION: get 6 bits value of blue
 * 	 INPUTS: pixel -- 16 bits pixel value
 *   OUTPUTS: none
 *   RETURN VALUE: 6 bits value of blue
 *   SIDE EFFECTS: none
 *                 
 */
uint8_t get_b(uint16_t pixel)
{
	return pixel & 0x1F;			//mask higher 11 bits
}
/* 
 * get_rgb12
 *   DESCRIPTION: get 4 bits value of red, 4 bits value of green, 4 bits value of blue
 * 	 INPUTS: pixel -- 16 bits pixel value
 *   OUTPUTS: none
 *   RETURN VALUE: 4 bits value of red, 4 bits value of green, 4 bits value of blue
 *   SIDE EFFECTS: none
 *                 
 */
uint16_t get_rgb12(uint16_t pixel)
{
	uint8_t r = get_r(pixel);
	uint8_t g = get_g(pixel);
	uint8_t b = get_b(pixel);
	return ((r >> 1) << 8) | ((g >> 2) << 4) | (b >> 1);	//get 12-bit RGB value: rrrrr->rrrr->rrrrxxxxxxxx ; gggggg->gggg->xxxxggggxxxx ; bbbbb->bbbb->xxxxxxxxbbbb
}
/* 
 * get_rgb6
 *   DESCRIPTION: get 2 bits value of red, 2 bits value of green, 2 bits value of blue
 * 	 INPUTS: pixel -- 16 bits pixel value
 *   OUTPUTS: none
 *   RETURN VALUE: 2 bits value of red, 2 bits value of green, 2 bits value of blue
 *   SIDE EFFECTS: none
 *                 
 */
uint8_t get_rgb6(uint16_t pixel)
{
	uint8_t r = get_r(pixel);
	uint8_t g = get_g(pixel);
	uint8_t b = get_b(pixel);
	return (r >> 3) << 4 | (g >> 4) << 2 | (b >> 3);			//get 6-bit RGB value: rrrrr->rr->rrxxxx ; gggggg->gg->xxggxx ; bbbbb->bb->xxxxbb
}
/* 
 * rgb12torgb6
 *   DESCRIPTION: translate 12 bits RGB value to 6 bits RGB value
 * 	 INPUTS: pixel 
 *   OUTPUTS: none
 *   RETURN VALUE: 6 bits RGB value
 *   SIDE EFFECTS: none
 *                 
 */
uint8_t rgb12torgb6(uint16_t pixel)
{												//bit operation: 12 bits RGB value -> 6 bits RGB value
	uint8_t r = (pixel >> 10) & 0x3;			//get 2 bits value of red and mask higher bits     rrrrxxxxxxxxxx -> rr
	uint8_t g = (pixel >> 6) & 0x3;				//get 2 bits value of green and mask higher bits   ggggxxxx       -> gg
	uint8_t b = (pixel >>2) & 0x3;				//get 2 bits value of blue and mask higher bits    bbbbxx         -> bb
	return (r << 4) | (g << 2) | b;				//combine r, g, b to 6 bits RGB value    rrggbb
}
/* file-scope variables */

/* 
 * The room currently shown on the screen.  This value is not known to 
 * the mode X code, but is needed when filling buffers in callbacks from 
 * that code (fill_horiz_buffer/fill_vert_buffer).  The value is set 
 * by calling prep_room.
 */
static const room_t* cur_room = NULL; 


/* 
 * fill_horiz_buffer
 *   DESCRIPTION: Given the (x,y) map pixel coordinate of the leftmost 
 *                pixel of a line to be drawn on the screen, this routine 
 *                produces an image of the line.  Each pixel on the line
 *                is represented as a single byte in the image.
 *
 *                Note that this routine draws both the room photo and
 *                the objects in the room.
 *
 *   INPUTS: (x,y) -- leftmost pixel of line to be drawn 
 *   OUTPUTS: buf -- buffer holding image data for the line
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void
fill_horiz_buffer (int x, int y, unsigned char buf[SCROLL_X_DIM])
{
    int            idx;   /* loop index over pixels in the line          */ 
    object_t*      obj;   /* loop index over objects in the current room */
    int            imgx;  /* loop index over pixels in object image      */ 
    int            yoff;  /* y offset into object image                  */ 
    uint8_t        pixel; /* pixel from object image                     */
    const photo_t* view;  /* room photo                                  */
    int32_t        obj_x; /* object x position                           */
    int32_t        obj_y; /* object y position                           */
    const image_t* img;   /* object image                                */

    /* Get pointer to current photo of current room. */
    view = room_photo (cur_room);

    /* Loop over pixels in line. */
    for (idx = 0; idx < SCROLL_X_DIM; idx++) {
        buf[idx] = (0 <= x + idx && view->hdr.width > x + idx ?
		    view->img[view->hdr.width * y + x + idx] : 0);
    }

    /* Loop over objects in the current room. */
    for (obj = room_contents_iterate (cur_room); NULL != obj;
    	 obj = obj_next (obj)) {
	obj_x = obj_get_x (obj);
	obj_y = obj_get_y (obj);
	img = obj_image (obj);

        /* Is object outside of the line we're drawing? */
	if (y < obj_y || y >= obj_y + img->hdr.height ||
	    x + SCROLL_X_DIM <= obj_x || x >= obj_x + img->hdr.width) {
	    continue;
	}

	/* The y offset of drawing is fixed. */
	yoff = (y - obj_y) * img->hdr.width;

	/* 
	 * The x offsets depend on whether the object starts to the left
	 * or to the right of the starting point for the line being drawn.
	 */
	if (x <= obj_x) {
	    idx = obj_x - x;
	    imgx = 0;
	} else {
	    idx = 0;
	    imgx = x - obj_x;
	}

	/* Copy the object's pixel data. */
	for (; SCROLL_X_DIM > idx && img->hdr.width > imgx; idx++, imgx++) {
	    pixel = img->img[yoff + imgx];

	    /* Don't copy transparent pixels. */
	    if (OBJ_CLR_TRANSP != pixel) {
		buf[idx] = pixel;
	    }
	}
    }
}


/* 
 * fill_vert_buffer
 *   DESCRIPTION: Given the (x,y) map pixel coordinate of the top pixel of 
 *                a vertical line to be drawn on the screen, this routine 
 *                produces an image of the line.  Each pixel on the line
 *                is represented as a single byte in the image.
 *
 *                Note that this routine draws both the room photo and
 *                the objects in the room.
 *
 *   INPUTS: (x,y) -- top pixel of line to be drawn 
 *   OUTPUTS: buf -- buffer holding image data for the line
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void
fill_vert_buffer (int x, int y, unsigned char buf[SCROLL_Y_DIM])
{
    int            idx;   /* loop index over pixels in the line          */ 
    object_t*      obj;   /* loop index over objects in the current room */
    int            imgy;  /* loop index over pixels in object image      */ 
    int            xoff;  /* x offset into object image                  */ 
    uint8_t        pixel; /* pixel from object image                     */
    const photo_t* view;  /* room photo                                  */
    int32_t        obj_x; /* object x position                           */
    int32_t        obj_y; /* object y position                           */
    const image_t* img;   /* object image                                */

    /* Get pointer to current photo of current room. */
    view = room_photo (cur_room);

    /* Loop over pixels in line. */
    for (idx = 0; idx < SCROLL_Y_DIM; idx++) {
        buf[idx] = (0 <= y + idx && view->hdr.height > y + idx ?
		    view->img[view->hdr.width * (y + idx) + x] : 0);
    }

    /* Loop over objects in the current room. */
    for (obj = room_contents_iterate (cur_room); NULL != obj;
    	 obj = obj_next (obj)) {
	obj_x = obj_get_x (obj);
	obj_y = obj_get_y (obj);
	img = obj_image (obj);

        /* Is object outside of the line we're drawing? */
	if (x < obj_x || x >= obj_x + img->hdr.width ||
	    y + SCROLL_Y_DIM <= obj_y || y >= obj_y + img->hdr.height) {
	    continue;
	}

	/* The x offset of drawing is fixed. */
	xoff = x - obj_x;

	/* 
	 * The y offsets depend on whether the object starts below or 
	 * above the starting point for the line being drawn.
	 */
	if (y <= obj_y) {
	    idx = obj_y - y;
	    imgy = 0;
	} else {
	    idx = 0;
	    imgy = y - obj_y;
	}

	/* Copy the object's pixel data. */
	for (; SCROLL_Y_DIM > idx && img->hdr.height > imgy; idx++, imgy++) {
	    pixel = img->img[xoff + img->hdr.width * imgy];

	    /* Don't copy transparent pixels. */
	    if (OBJ_CLR_TRANSP != pixel) {
		buf[idx] = pixel;
	    }
	}
    }
}


/* 
 * image_height
 *   DESCRIPTION: Get height of object image in pixels.
 *   INPUTS: im -- object image pointer
 *   OUTPUTS: none
 *   RETURN VALUE: height of object image im in pixels
 *   SIDE EFFECTS: none
 */
uint32_t 
image_height (const image_t* im)
{
    return im->hdr.height;
}


/* 
 * image_width
 *   DESCRIPTION: Get width of object image in pixels.
 *   INPUTS: im -- object image pointer
 *   OUTPUTS: none
 *   RETURN VALUE: width of object image im in pixels
 *   SIDE EFFECTS: none
 */
uint32_t 
image_width (const image_t* im)
{
    return im->hdr.width;
}

/* 
 * photo_height
 *   DESCRIPTION: Get height of room photo in pixels.
 *   INPUTS: p -- room photo pointer
 *   OUTPUTS: none
 *   RETURN VALUE: height of room photo p in pixels
 *   SIDE EFFECTS: none
 */
uint32_t 
photo_height (const photo_t* p)
{
    return p->hdr.height;
}


/* 
 * photo_width
 *   DESCRIPTION: Get width of room photo in pixels.
 *   INPUTS: p -- room photo pointer
 *   OUTPUTS: none
 *   RETURN VALUE: width of room photo p in pixels
 *   SIDE EFFECTS: none
 */
uint32_t 
photo_width (const photo_t* p)
{
    return p->hdr.width;
}


/* 
 * prep_room
 *   DESCRIPTION: Prepare a new room for display.  You might want to set
 *                up the VGA palette registers according to the color
 *                palette that you chose for this room.
 *   INPUTS: r -- pointer to the new room
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: changes recorded cur_room for this file
 */
void
prep_room (const room_t* r)
{
    /* Record the current room. */
    cur_room = r;

	//mp2.2
	//get current phot of the room
	photo_t* cur_photo_ptr = room_photo(r);
	//use optimized palette
	unsigned char* ptr_ =  (unsigned char*) cur_photo_ptr->palette;	//change the type so no warning	
	fill_palette_optimized_colors (ptr_);
}


/* 
 * read_obj_image
 *   DESCRIPTION: Read size and pixel data in 2:2:2 RGB format from a
 *                photo file and create an image structure from it.
 *   INPUTS: fname -- file name for input
 *   OUTPUTS: none
 *   RETURN VALUE: pointer to newly allocated photo on success, or NULL
 *                 on failure
 *   SIDE EFFECTS: dynamically allocates memory for the image
 */
image_t*
read_obj_image (const char* fname)
{
    FILE*    in;		/* input file               */
    image_t* img = NULL;	/* image structure          */
    uint16_t x;			/* index over image columns */
    uint16_t y;			/* index over image rows    */
    uint8_t  pixel;		/* one pixel from the file  */

    /* 
     * Open the file, allocate the structure, read the header, do some
     * sanity checks on it, and allocate space to hold the image pixels.
     * If anything fails, clean up as necessary and return NULL.
     */
    if (NULL == (in = fopen (fname, "r+b")) ||
	NULL == (img = malloc (sizeof (*img))) ||
	NULL != (img->img = NULL) || /* false clause for initialization */
	1 != fread (&img->hdr, sizeof (img->hdr), 1, in) ||
	MAX_OBJECT_WIDTH < img->hdr.width ||
	MAX_OBJECT_HEIGHT < img->hdr.height ||
	NULL == (img->img = malloc 
		 (img->hdr.width * img->hdr.height * sizeof (img->img[0])))) {
	if (NULL != img) {
	    if (NULL != img->img) {
	        free (img->img);
	    }
	    free (img);
	}
	if (NULL != in) {
	    (void)fclose (in);
	}
	return NULL;
    }

    /* 
     * Loop over rows from bottom to top.  Note that the file is stored
     * in this order, whereas in memory we store the data in the reverse
     * order (top to bottom).
     */
    for (y = img->hdr.height; y-- > 0; ) {

	/* Loop over columns from left to right. */
	for (x = 0; img->hdr.width > x; x++) {

	    /* 
	     * Try to read one 8-bit pixel.  On failure, clean up and 
	     * return NULL.
	     */
	    if (1 != fread (&pixel, sizeof (pixel), 1, in)) {
		free (img->img);
		free (img);
	        (void)fclose (in);
		return NULL;
	    }

	    /* Store the pixel in the image data. */
	    img->img[img->hdr.width * y + x] = pixel;
	}
    }

    /* All done.  Return success. */
    (void)fclose (in);
    return img;
}


/* 
 * read_photo
 *   DESCRIPTION: Read size and pixel data in 5:6:5 RGB format from a
 *                photo file and create a photo structure from it.
 *                Code provided simply maps to 2:2:2 RGB.  You must
 *                replace this code with palette color selection, and
 *                must map the image pixels into the palette colors that
 *                you have defined.
 *   INPUTS: fname -- file name for input
 *   OUTPUTS: none
 *   RETURN VALUE: pointer to newly allocated photo on success, or NULL
 *                 on failure
 *   SIDE EFFECTS: dynamically allocates memory for the photo
 */
photo_t*
read_photo (const char* fname)
{
    FILE*    in;	/* input file               */
    photo_t* p = NULL;	/* photo structure          */
    uint16_t x;		/* index over image columns */
    uint16_t y;		/* index over image rows    */
    uint16_t pixel;	/* one pixel from the file  */

    /* 
     * Open the file, allocate the structure, read the header, do some
     * sanity checks on it, and allocate space to hold the photo pixels.
     * If anything fails, clean up as necessary and return NULL.
     */
    if (NULL == (in = fopen (fname, "r+b")) ||
	NULL == (p = malloc (sizeof (*p))) ||
	NULL != (p->img = NULL) || /* false clause for initialization */
	1 != fread (&p->hdr, sizeof (p->hdr), 1, in) ||
	MAX_PHOTO_WIDTH < p->hdr.width ||
	MAX_PHOTO_HEIGHT < p->hdr.height ||
	NULL == (p->img = malloc 
		 (p->hdr.width * p->hdr.height * sizeof (p->img[0])))) {
	if (NULL != p) {
	    if (NULL != p->img) {
	        free (p->img);
	    }
	    free (p);
	}
	if (NULL != in) {
	    (void)fclose (in);
	}
	return NULL;
    }

	//mp2.2
	int fourth_level_num = 4096;	//octree fourth level: 4096 = 8**4; total nodes in the fourth level
	int second_level_num = 64;		//octree second level: 64	= 8**2; total nodes in the second level
	int total_pixels = p->hdr.width * p->hdr.height;	//total pixels in the image file
	octree_node_t octree_fourth_level[fourth_level_num]; 			//octree fourth level: 4096 = 8**4; total nodes in the fourth level
	octree_node_t octree_second_level[second_level_num]; 				//octree second level: 64	= 8**2; total nodes in the second level
	remember_node_t remember_array[total_pixels];		//store necessary information for each pixel in the first loop to avoid loop again
	int i;
	int match_sort[fourth_level_num];						//match the correct index of the sorted octree nodes
	//initialization for second level octree and fourth level octree
	for(i = 0; i < fourth_level_num; i++)			//octree fourth level: 4096 = 8**4; total nodes in the fourth level
	{
		octree_fourth_level[i].r_sum = 0;
		octree_fourth_level[i].g_sum = 0;
		octree_fourth_level[i].b_sum = 0;
		octree_fourth_level[i].count = 0;
		octree_fourth_level[i].r_avg = 0;
		octree_fourth_level[i].g_avg = 0;
		octree_fourth_level[i].b_avg = 0;
		octree_fourth_level[i].index = i;
	}	
	for(i = 0; i < second_level_num; i++)				//octree second level: 64	= 8**2; total nodes in the second level
	{
		octree_second_level[i].r_sum = 0;
		octree_second_level[i].g_sum = 0;
		octree_second_level[i].b_sum = 0;
		octree_second_level[i].count = 0;
		octree_second_level[i].r_avg = 0;
		octree_second_level[i].g_avg = 0;
		octree_second_level[i].b_avg = 0;
		octree_second_level[i].index = i;
	}
	for(i = 0; i < total_pixels; i++)	//remember array
	{
		remember_array[i].match_node = 0;
		remember_array[i].original_pixel = 0;
		remember_array[i].find_in_fourth_level=0;					
	}
    /* 
     * Loop over rows from bottom to top.  Note that the file is stored
     * in this order, whereas in memory we store the data in the reverse
     * order (top to bottom).
     */
    for (y = p->hdr.height; y-- > 0; ) {

	/* Loop over columns from left to right. */
	for (x = 0; p->hdr.width > x; x++) {

	    /* 
	     * Try to read one 16-bit pixel.  On failure, clean up and 
	     * return NULL.
	     */
	    if (1 != fread (&pixel, sizeof (pixel), 1, in)) {
		free (p->img);
		free (p);
	        (void)fclose (in);
		return NULL;

	    }
	    /* 
	     * 16-bit pixel is coded as 5:6:5 RGB (5 bits red, 6 bits green,
	     * and 6 bits blue).  We change to 2:2:2, which we've set for the
	     * game objects.  You need to use the other 192 palette colors
	     * to specialize the appearance of each photo.
	     *
	     * In this code, you need to calculate the p->palette values,
	     * which encode 6-bit RGB as arrays of three uint8_t's.  When
	     * the game puts up a photo, you should then change the palette 
	     * to match the colors needed for that photo.
	     */
	    // p->img[p->hdr.width * y + x] = (((pixel >> 14) << 4) |
		// 			    (((pixel >> 9) & 0x3) << 2) |
		// 			    ((pixel >> 3) & 0x3));

		//mp2.2
		//step1: count the number of pixels in each node at level four of an octree, map them into the appropriate octree node, and count them up
		int cur_offset = p->hdr.width * y + x;		//current offset
		uint8_t r = get_r(pixel);					//get red value
		uint8_t g = get_g(pixel);					//get green value
		uint8_t b = get_b(pixel);					//get blue value
		uint16_t rgb_12 = get_rgb12(pixel);			

		octree_fourth_level[rgb_12].r_sum += r<<1;	//add up red value; left shift because green has 6 bits
		octree_fourth_level[rgb_12].g_sum += g;		//add up green value
		octree_fourth_level[rgb_12].b_sum += b<<1;	//add up blue value; left shift because green has 6 bits
		octree_fourth_level[rgb_12].count++;		//add up count
		remember_array[cur_offset].match_node = rgb_12;	//store the match node in the fourth level octree
	}
    }
		//step2: sort the octree nodes based on the number of pixels appearing in each and select the most frequent 128 for specific palette colors

		qsort(octree_fourth_level, fourth_level_num, sizeof(octree_node_t), compare_octree_node_t);	//sort 4096 octree nodes based on count, from largest to smallest

		//step3: calculate average of the 5:6:5 RGB values for most frequent 128 nodes
		for(i=0; i<fourth_level_num; i++)
		{
			match_sort[octree_fourth_level[i].index] = i;	//match previous rgb_12 to current index
			if(octree_fourth_level[i].count == 0) continue;	//skip the nodes that have no pixels
			if(i<128) 	//only need to calculate the first 128 nodes
			{
			octree_fourth_level[i].r_avg = octree_fourth_level[i].r_sum / octree_fourth_level[i].count;
			octree_fourth_level[i].g_avg = octree_fourth_level[i].g_sum / octree_fourth_level[i].count;
			octree_fourth_level[i].b_avg = octree_fourth_level[i].b_sum / octree_fourth_level[i].count;
			p->palette[i][0] = octree_fourth_level[i].r_avg;
			p->palette[i][1] = octree_fourth_level[i].g_avg;
			p->palette[i][2] = octree_fourth_level[i].b_avg;
			}
			else		//the rest 4096-128 nodes' sum could be used to calculate the average of the second level octree
			{
				int rgb12 = octree_fourth_level[i].index;	//previous rgb_12
				int rgb6 = rgb12torgb6(rgb12);				//index in the second level octree
				octree_second_level[rgb6].r_sum += octree_fourth_level[i].r_sum;	//add up red value
				octree_second_level[rgb6].g_sum += octree_fourth_level[i].g_sum;	//add up green value
				octree_second_level[rgb6].b_sum += octree_fourth_level[i].b_sum;	//add up blue value
				octree_second_level[rgb6].count += octree_fourth_level[i].count;	//add up count
			}
		}
		//step4: map these 5:6:5 RGB color values into the 8-bit VGA colors that you assign
		for(i=0; i<total_pixels;i++)
		{
			if(match_sort[remember_array[i].match_node]<128)	//find match in fourth level, only be mapped to the first 128 colors
			{
				p->img[i] = match_sort[remember_array[i].match_node]+64;  //after first 64 colors provided 
			}
			else
			{
				int rgb6 = rgb12torgb6(remember_array[i].match_node);	//match in second level
				p->img[i] = rgb6 + 192;	//map to the last 64 colors(192=256-64-128)
			}
		}
		//step5: The next step in the algorithm is to assign the remaining 64 palette colors to the 64 level-two octree nodes
	for(i=0; i<second_level_num; i++)
	{
		if(octree_second_level[i].count == 0) continue;	//skip the nodes that have no pixels
		//calculate average of the rgb values
		octree_second_level[i].r_avg = octree_second_level[i].r_sum / octree_second_level[i].count;
		octree_second_level[i].g_avg = octree_second_level[i].g_sum / octree_second_level[i].count;
		octree_second_level[i].b_avg = octree_second_level[i].b_sum / octree_second_level[i].count;
		//last 64 colors;192-128
		p->palette[i+128][0] = octree_second_level[i].r_avg;
		p->palette[i+128][1] = octree_second_level[i].g_avg;
		p->palette[i+128][2] = octree_second_level[i].b_avg;
	}


    /* All done.  Return success. */
    (void)fclose (in);
    return p;
}



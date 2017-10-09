/*
   i2ctest.c

   Copyright 2008-2011 Michel Pollet <buserror@gmail.com>

   This file is part of simavr.

   simavr is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   simavr is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with simavr.  If not, see <http://www.gnu.org/licenses/>.
   */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <pthread.h>

#include "sim_avr.h"
#include "sim_elf.h"
#include "sim_gdb.h"
#include "sim_vcd_file.h"
// EEPROM
#include "avr_twi.h"
#include "i2c_eeprom.h"
// LED/PORT IO
#if __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include "avr_ioport.h"

avr_t * avr = NULL;
avr_vcd_t vcd_file;

i2c_eeprom_t ee;

uint8_t	pinb_state = 0;	// 
uint8_t	pinc_state = 0;	// 
uint8_t	pind_state = 0;	// 
float pixsize = 64;
int window;
int row = 3, col = 8;

/*
 * called when the AVR change any of the pins on port B, C? D?
 * so lets update our buffer
 */
void pinb_changed_hook(struct avr_irq_t * irq, uint32_t value, void * param)
{
	pinb_state = (pinb_state & ~(1 << irq->irq)) | (value << irq->irq);
}
void pinc_changed_hook(struct avr_irq_t * irq, uint32_t value, void * param)
{
	pinc_state = (pinc_state & ~(1 << irq->irq)) | (value << irq->irq);
}
void pind_changed_hook(struct avr_irq_t * irq, uint32_t value, void * param)
{
	pind_state = (pind_state & ~(1 << irq->irq)) | (value << irq->irq);
}

/*
 * Sample Fonts:
 * GLUT_BITMAP_8_BY_13
 * GLUT_BITMAP_9_BY_15
 * GLUT_BITMAP_TIMES_ROMAN_10
 * GLUT_BITMAP_TIMES_ROMAN_24
 * GLUT_BITMAP_HELVETICA_10
 * GLUT_BITMAP_HELVETICA_12
 * GLUT_BITMAP_HELVETICA_18 
 */
void displayText2D( float x, float y/*, float z*/, int r, int g, int b, void *font, const char *string ) {
	int j = strlen( string );

	glColor3f( r, g, b );
	glRasterPos2f( x, y );
	//glRasterPos3f( x, y, z );
	for( int i = 0; i < j; i++ ) {
		glutBitmapCharacter( font, string[i] );
	}
}

void displayText3D( float x, float y, float z, int r, int g, int b, void *font, const char *string ) {
	int j = strlen( string );

	glColor3f( r, g, b );
	//glRasterPos2f( x, y );
	glRasterPos3f( x, y, z );
	for( int i = 0; i < j; i++ ) {
		glutBitmapCharacter( font, string[i] );
	}
}
void displayChar2D( float x, float y, int r, int g, int b, void *font, char stuff ) {
	glColor3f( r, g, b );
	//glRasterPos2f( x, y );
	glRasterPos2f( x, y );
	glutBitmapCharacter( font, stuff );
}

void displayCB(void)		/* function called whenever redisplay needed */
{
	// OpenGL rendering goes here...
	glClear(GL_COLOR_BUFFER_BIT);

	// Set up modelview matrix
	glMatrixMode(GL_MODELVIEW); // Select modelview matrix
	glLoadIdentity(); // Start with an identity matrix

	float grid = pixsize;
	float size = grid * 0.8;
	int padding = 12;
	//glBegin(GL_QUADS);
	//glColor3f(1,0,0);

	// From Atmega168 timer example
	// float color_on = (float)(0xff - display_pwm) / 15.0f;
	// float color_off = 0.0;

	int halfFontsize = 24 / 2
		, charOffsetX = -1.0f * ( (size - halfFontsize) / 2)
		, charOffsetY = -1.0f * ( (size + halfFontsize) / 2)
		;

	for (int di = 0; di < 8; di++) {
		char b_on = (pinb_state & (1 << di)) != 0;
		char c_on = (pinc_state & (1 << di)) != 0;
		char d_on = (pind_state & (1 << di)) != 0;

		float x = (di) * grid;		
		if (b_on) {
			float y = 0; //(si * grid * 8) + (di * grid);

			glBegin(GL_QUADS);			
			glColor3f(1,0,0);
			//glColor3f(1,0, digit & (1 << i) ? color_on : color_off);
			glVertex2f(x + size, y + size);
			glVertex2f(x, y + size);
			glVertex2f(x, y);
			glVertex2f(x + size, y);
			glEnd();

			// Text, PIN NUMBER
			displayChar2D( x-charOffsetX
					, y-charOffsetY
					, 0
					, 255
					, 0
					, GLUT_BITMAP_TIMES_ROMAN_24
					, di + '0'
				     );
		}		
		if (c_on) {
			float y =  -1 * (size + padding); //(si * grid * 8) + (di * grid);

			glBegin(GL_QUADS);			
			glColor3f(1,0,0);
			glVertex2f(x + size, y + size);
			glVertex2f(x, y + size);
			glVertex2f(x, y);
			glVertex2f(x + size, y);
			glEnd();

			// Text, PIN NUMBER
			displayChar2D( x-charOffsetX
					, y-charOffsetY
					, 0
					, 255
					, 0
					, GLUT_BITMAP_TIMES_ROMAN_24
					, di + '0' 
				     );
		}
		if (d_on) {
			float y = -2 * (size + padding); //(si * grid * 8) + (di * grid);

			glBegin(GL_QUADS);			
			glColor3f(1,0,0);
			glVertex2f(x + size, y + size);
			glVertex2f(x, y + size);
			glVertex2f(x, y);
			glVertex2f(x + size, y);
			glEnd();

			// Text, PIN NUMBER
			displayChar2D( x-charOffsetX
					, y-charOffsetY
					, 0
					, 255
					, 0
					, GLUT_BITMAP_TIMES_ROMAN_24
					, di + '0' 
				     );
		}          
	}

	// Text, PORT GROUP
	displayChar2D( 0.0f, (size) / 2, 0, 255, 0, GLUT_BITMAP_TIMES_ROMAN_24, 'B' );
	displayChar2D( 0.0f, ((-1 * size) + padding) / 2 - halfFontsize, 0, 255, 0, GLUT_BITMAP_TIMES_ROMAN_24, 'C' );
	displayChar2D( 0.0f, (-2 * size) + padding / 2, 0, 255, 0, GLUT_BITMAP_TIMES_ROMAN_24, 'D' ); // Need to be corrected !!!

	//glEnd();
	glutSwapBuffers();
	//glFlush();				/* Complete any pending operations */
}

// gl timer. if the pin have changed states, refresh display
void timerCB(int i)
{
	static uint8_t oldstateb = 0xff, oldstatec = 0xff, oldstated = 0xff;
	// restart timer
	glutTimerFunc(1000/64, timerCB, 0);

	if (oldstateb != pinb_state) {
		oldstateb = pinb_state;
		glutPostRedisplay();
	} 
	if (oldstatec != pinc_state) {
		oldstatec = pinc_state;
		glutPostRedisplay();
	}
	if (oldstated != pind_state) {
		oldstated = pind_state;
		glutPostRedisplay();
	}
}

static void * avr_run_thread(void * oaram)
{
	//	int b_press = do_button_press;

	while (1) {
		avr_run(avr);
		//		if (do_button_press != b_press) {
		//			b_press = do_button_press;
		//			printf("Button pressed\n");
		//			button_press(&button, 1000000);
		//		}
	}
	return NULL;
}
//


int main(int argc, char *argv[])
{
	elf_firmware_t f;
	const char * fname =  "atmega32_ledpwm_i2c.axf";

	printf("Firmware pathname is %s\n", fname);
	elf_read_firmware(fname, &f);

	printf("firmware %s f=%d mmcu=%s\n", fname, (int)f.frequency, f.mmcu);

	avr = avr_make_mcu_by_name(f.mmcu);
	if (!avr) {
		fprintf(stderr, "%s: AVR '%s' not known\n", argv[0], f.mmcu);
		exit(1);
	}
	avr_init(avr);
	avr_load_firmware(avr, &f);

	// initialize our 'peripheral', setting the mask to allow read and write
	i2c_eeprom_init(avr, &ee, 0xa0, 0x01, NULL, 1024);

	i2c_eeprom_attach(avr, &ee, AVR_IOCTL_TWI_GETIRQ(0));
	ee.verbose = 1;

	// connect all the pins to callback
	for (int i = 0; i < 8; i++) {
		avr_irq_register_notify(
				avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('B'), i),
				pinb_changed_hook, 
				NULL);
		avr_irq_register_notify(
				avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('C'), i),
				pinc_changed_hook, 
				NULL);
		avr_irq_register_notify(
				avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('D'), i),
				pind_changed_hook, 
				NULL);
	}

	// even if not setup at startup, activate gdb if crashing
	avr->gdb_port = 1234;
	if (0) {
		//avr->state = cpu_Stopped;
		avr_gdb_init(avr);
	}

	/*
	 *	VCD file initialization
	 *	
	 *	This will allow you to create a "wave" file and display it in gtkwave
	 *	Pressing "r" and "s" during the demo will start and stop recording
	 *	the pin changes
	 */
	avr_vcd_init(avr, "gtkwave_output.vcd", &vcd_file, 100000 /* usec */);
	avr_vcd_add_signal(&vcd_file, 
			avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('B'), IOPORT_IRQ_PIN_ALL), 8 /* bits */ ,
			"portb" );
	avr_vcd_add_signal(&vcd_file, 
			avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('C'), IOPORT_IRQ_PIN_ALL), 8 /* bits */ ,
			"portc" );
	avr_vcd_add_signal(&vcd_file, 
			avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('D'), IOPORT_IRQ_PIN_ALL), 8 /* bits */ ,
			"portd" );


	avr_vcd_start(&vcd_file); // Start trace

	/*
	 * OpenGL init, can be ignored
	 */
	glutInit(&argc, argv);		/* initialize GLUT system */

	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutInitWindowSize(col * pixsize + 24, row * pixsize + 24);		
	window = glutCreateWindow("GL 2D Test");	/* create window */

	// Set up projection matrix
	glMatrixMode(GL_PROJECTION); // Select projection matrix
	glLoadIdentity(); // Start with an identity matrix
	//glOrtho(0, col * pixsize, 0, row * pixsize, 0, 10);
	/* flipped the Y values because OpenGL coordinates start from the bottom left corner of the window. So by flipping, I get a more conventional (0,0) starting at the top left corner of the window 
Explained: https://stackoverflow.com/questions/2571402/explain-the-usage-of-glortho
*/
	//glOrtho(0.0f, col * pixsize,row * pixsize, 0.0f, 0.0f, 1.0f); 
	glOrtho(-1.0f, (float)col * pixsize, 0.0f, (float)row * pixsize, 0.0f, 1.0f); 
	glScalef(1,-1,1);
	glTranslatef(0, -1 * pixsize, 0);

	glutDisplayFunc(displayCB);		/* set window's display callback */
	glutTimerFunc(1000 / 24, timerCB, 0);

	// the AVR run on it's own thread. it even allows for debugging!
	pthread_t run;
	pthread_create(&run, NULL, avr_run_thread, NULL);

	glutMainLoop();
}



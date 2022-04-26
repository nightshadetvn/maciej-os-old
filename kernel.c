//import mojej biblioteki do klawoatiry
#include "keyboard_map.h"


/*jest 25 lini po 80 kolumn; każdy element zajmuje 2 bajty */
#define LINES 25
#define COLUMNS_IN_LINE 80
#define BYTES_FOR_EACH_ELEMENT 2
#define SCREENSIZE BYTES_FOR_EACH_ELEMENT * COLUMNS_IN_LINE * LINES

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define IDT_SIZE 256
#define INTERRUPT_GATE 0x8e
#define KERNEL_CODE_SEGMENT_OFFSET 0x08

#define ENTER_KEY_CODE 0x1C

extern unsigned char keyboard_map[128];
extern void keyboard_handler(void);
extern char read_port(unsigned short port);
extern void write_port(unsigned short port, unsigned char data);
extern void load_idt(unsigned long *idt_ptr);

/* polozenie kursora */
unsigned int current_row = 0;
unsigned int current_line = 0;
/* pamiec video zaczyna sie na adresie 0xb8000 */
char *vidptr = (char*)0xb8000;

struct IDT_entry {
	unsigned short int offset_lowerbits;
	unsigned short int selector;
	unsigned char zero;
	unsigned char type_attr;
	unsigned short int offset_higherbits;
};

struct IDT_entry IDT[IDT_SIZE];


void idt_init(void)
{
	unsigned long keyboard_address;
	unsigned long idt_address;
	unsigned long idt_ptr[2];

	/* populate IDT entry of keyboard's interrupt */
	keyboard_address = (unsigned long)keyboard_handler;
	IDT[0x21].offset_lowerbits = keyboard_address & 0xffff;
	IDT[0x21].selector = KERNEL_CODE_SEGMENT_OFFSET;
	IDT[0x21].zero = 0;
	IDT[0x21].type_attr = INTERRUPT_GATE;
	IDT[0x21].offset_higherbits = (keyboard_address & 0xffff0000) >> 16;

	/*     porty
	*	 PIC1	PIC2
	*komenda 0x20	0xA0
	*dabe	 0x21	0xA1
	*/

	/* ICW1 - initialization */
	write_port(0x20 , 0x11);
	write_port(0xA0 , 0x11);

	/* ICW2 - remap offset addres  IDT */
	/*
	* W x86 trybie chornionym, musimy zrempaowac PIC powyzej 0x20 poniewaz
	* Intel dupa desing zrobil 32 interupty jako zarezerwowane dla wypadkow CPU
	*/
	write_port(0x21 , 0x20);
	write_port(0xA1 , 0x28);

	/* ICW3 - cascading */
	write_port(0x21 , 0x00);
	write_port(0xA1 , 0x00);

	/* ICW4 - envt info */
	write_port(0x21 , 0x01);
	write_port(0xA1 , 0x01);
	/* Initialization koniec */

	/* mask interrupts */
	write_port(0x21 , 0xff);
	write_port(0xA1 , 0xff);

	/* napelnij IDT descriptor */
	idt_address = (unsigned long)IDT ;
	idt_ptr[0] = (sizeof (struct IDT_entry) * IDT_SIZE) + ((idt_address & 0xffff) << 16);
	idt_ptr[1] = idt_address >> 16 ;

	load_idt(idt_ptr);
}

void kb_init(void)
{
	/* 0xFD is 11111101 - wlacza tylko IRQ1 (klawa)*/
	write_port(0x21 , 0xFD);
}

void kprint_newline(void)
{
	current_line++;
	current_row = 0;
}

void kprint_char(const char ch)
{
	if (ch == '\n')
	{
		kprint_newline();
	}
	else
	{
		if (current_row >= COLUMNS_IN_LINE)
		{
			kprint_newline();
		}
		
		vidptr[current_line*COLUMNS_IN_LINE*BYTES_FOR_EACH_ELEMENT + current_row*BYTES_FOR_EACH_ELEMENT] = ch;
		vidptr[current_row*BYTES_FOR_EACH_ELEMENT + 1] = 0x07;
		current_row++;
	}
}

void kprint(const char *str)
{	
	unsigned int i = 0;
	while (str[i] != '\0') {
		if ( (str[i] >= 0x20 && str[i] <= 0x7e) || str[i]=='\n')
		{
			kprint_char(str[i++]);
		}
		else
		{
			kprint_char(0xfe);
			i++;
		}
	}
}

void clear_screen(void)
{
	unsigned int i = 0;
	while (i < SCREENSIZE) {
		vidptr[i++] = ' ';
		vidptr[i++] = 0x07;
	}
}

void keyboard_handler_main(void)
{
	unsigned char status;
	char keycode;

	/* write EOI */
	write_port(0x20, 0x20);

	status = read_port(KEYBOARD_STATUS_PORT);
	/* najmnieszy bit statusu bedzie ustawiony jezeli buffer nie jest pusty*/
	if (status & 0x01) {
		keycode = read_port(KEYBOARD_DATA_PORT);
		if(keycode < 0)
			return;

		if(keycode == ENTER_KEY_CODE) {
			kprint_newline();
			return;
		}
		
		kprint_char(keyboard_map[(unsigned char) keycode]);
	}
}

void kmain(void)
{

	const char *test1 = "##     ##    ###     ######  #### ########       ##     #######   ######    \n";
	const char *test2 = "###   ###   ## ##   ##    ##  ##  ##             ##    ##     ## ##    ##   \n";
	const char *test3 = "#### ####  ##   ##  ##        ##  ##             ##    ##     ## ##     	 \n";
	const char *test4 = "## ### ## ##     ## ##        ##  ######         ##    ##     ##  ######  	 \n";
	const char *test5 = "##     ## ######### ##        ##  ##       ##    ##    ##     ##       ## 	 \n";
	const char *test6 = "##     ## ##     ## ##    ##  ##  ##       ##    ##    ##     ## ##    ##   \n";
	const char *test7 = "##     ## ##     ##  ######  #### ########  ######      #######   ######    \n";

	clear_screen();
	kprint(test1);
	kprint(test2);
	//kprint("1234567890");
	kprint(test3);
	kprint(test4);
	kprint(test5);
	kprint(test6);
	kprint(test7);
	kprint_newline();
	kprint_newline();

	idt_init();
	kb_init();

	while(1);
}

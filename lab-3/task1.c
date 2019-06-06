#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <alloca.h>
#include <math.h>

// konstanter från uppgiften
#define FRAME_SIZE 256  // frame size är 256 bytes (2^8)
#define FRAMES 256		// antalet frames
#define PAGE_SIZE 256   // page size är 256 bytes (2^8)
#define TLB_SIZE 16		// antal TLB entries
#define ADDRESS_SIZE 10 // storleken på varje address

// input filerna
FILE *address_file;
FILE *backing_store;

// struct för page table och TLB
struct page_frame
{
	int page_number;
	int frame_number;
};

// globala variabler
int physical_memory[FRAMES][FRAME_SIZE];
struct page_frame TLB[TLB_SIZE];
struct page_frame PAGE_TABLE[FRAMES]; // storleken på page_table är antalet tillgängliga frames
int TLB_hits = 0;
int page_faults = 0;
signed char buffer[256]; // används när addresser ska läsas från backing_store
int first_avail_frame = 0;
int first_avail_page_tbl_index = 0;
signed char value; // används för att spara ner värdet i page table längre fram
int TLB_full_entries = 0;

// funktiondeklaration
void get_page(int logical_address);
int read_from_backing_store(int page_number);
void insert_into_TLB(int page_number, int frame_number);

// hämta page med hjälp av logiska addressen tagen från address.txt
void get_page(int logical_address)
{
	int page_number = ((logical_address & 0xFFFF) >> 8);
	int offset = (logical_address & 0xFF);

	int frame_number = -1; // initialiseras till -1 för att se om det blev ett giltigt värde senare

	// koller igenom TLB efter en matchning med page_numret från filen
	for (int i = 0; i < TLB_SIZE; i++)
	{
		if (TLB[i].page_number == page_number)
		{
			frame_number = TLB[i].frame_number;
			TLB_hits++;
		}
	}

	// om frame_number inte var i TLB:n
	if (frame_number == -1)
	{
		for (int i = 0; i < first_avail_page_tbl_index; i++)
		{
			if (PAGE_TABLE[i].page_number == page_number)
			{
				frame_number = PAGE_TABLE[i].frame_number;
			}
		}

		// om frame_number fortfarande inte har hittats, läs från backing_store
		if (frame_number == -1)
		{
			frame_number = read_from_backing_store(page_number);
			page_faults++;
		}
	}

	insert_into_TLB(page_number, frame_number);
	value = physical_memory[frame_number][offset];

	// Tester
	// printf("frame number: %d\n", frame_number);
	// printf("offset: %d\n", offset);
	// printf("Virtual address: %d Physical address %d Value: %d\n", logical_address, (frame_number << 8) | offset, value);

	// skriv ut de slutgiltiga värdena till filen "results.txt"
	FILE *result = fopen("results.txt", "a");
	fprintf(result, "Virtual address: %d Physical address %d Value: %d\n", logical_address, (frame_number << 8) | offset, value);
	fclose(result);
}

int read_from_backing_store(int page_number)
{
	if (fseek(backing_store, page_number * 256, SEEK_SET) != 0)
	{
		fprintf(stderr, "Error seeking in backing store\n");
	}

	// läs 256 bytes from the backing store till buffern
	if (fread(buffer, sizeof(signed char), 256, backing_store) == 0)
	{
		fprintf(stderr, "Error reading from backing store\n");
	}

	// ladda in bits in i den första tillgängliga framen i fysiska minnet
	for (int i = 0; i < 256; i++)
	{
		physical_memory[first_avail_frame][i] = buffer[i];
	}

	// sen skrivs frame number in i page table på den första tillgängliga framen
	PAGE_TABLE[first_avail_page_tbl_index].page_number = page_number;
	PAGE_TABLE[first_avail_page_tbl_index].frame_number = first_avail_frame;

	first_avail_frame++;
	first_avail_page_tbl_index++;

	return PAGE_TABLE[first_avail_page_tbl_index - 1].frame_number;
}

void insert_into_TLB(int page_number, int frame_number)
{
	/*
		Algoritmen för att sätta in en page i TLB är en sorts FIFO procedur
		- Först kollas det om pagen redan finns i TLB.
			* Finns den i TLB så:
				~ flytta den längst bak i TLB för att följa "FIFO reglementet".

			* Om pagen inte finns i TLB:
				~ Om det finns en fri cell så lägg till pagen på den fria cellen i slutet av kön
				  annars flytta allting ett steg till vänster för att frigöra den sista cellen och lägg pagen i den sista cellen
	*/
	int i;
	for (i = 0; i < TLB_full_entries; i++)
	{
		if (TLB[i].page_number == page_number)
		{
			for (i = i; i < TLB_full_entries - 1; i++)
				TLB[i] = TLB[i + 1];
			break;
		}
	}

	// om den förra loopen inte "break:ade" så finns inte pagen i TLB och de måste läggas till
	if (i == TLB_full_entries)
	{
		for (int j = 0; j < i; j++)
		{
			TLB[j] = TLB[j + 1];
		}
	}
	TLB[i].page_number = page_number;
	TLB[i].frame_number = frame_number;

	// om det fortfarande finns plats i TLB inkrementera antalet element i TLB
	if (TLB_full_entries < TLB_SIZE - 1)
	{
		TLB_full_entries++;
	}
}

int main(int argc, char *argv[])
{
	printf("Resultaten hittas i \"results.txt\" filen\n");

	if (argc != 2)
	{
		fprintf(stderr, "Usage: ./task1 [input file]\n");
		return -1;
	}

	// läs addresser från input filen
	address_file = fopen(argv[1], "r");

	backing_store = fopen("BACKING_STORE.bin", "rb");

	if (address_file == NULL)
	{
		fprintf(stderr, "Error opening addresses from file: %s\n", argv[1]);
		return -1;
	}

	if (backing_store == NULL)
	{
		fprintf(stderr, "Error opening BACKING_STORE from file: %s\n", "BACKING_STORE.bin");
		return -1;
	}

	int trans_addresses = 0;
	char address[ADDRESS_SIZE];
	int logical_address;

	// läs de logiska addresserna från input filen och kör get_page på dem
	while (fgets(address, ADDRESS_SIZE, address_file) != NULL)
	{
		logical_address = atoi(address); // gör en numerisk string till en integer typ

		get_page(logical_address);
		trans_addresses++;
	}

	printf("Number of translated addresses = %d\n", trans_addresses);
	double page_fault_rate = page_faults / (double)trans_addresses;
	double TLB_hit_rate = TLB_hits / (double)trans_addresses;

	printf("Page faults: %d\n", page_faults);
	printf("Page fault rate: %.3f\n", page_fault_rate);
	printf("TLB hits: %d\n", TLB_hits);
	printf("TLB hit rate: %.3f\n", TLB_hit_rate);

	fclose(address_file);
	fclose(backing_store);

	return 0;
}
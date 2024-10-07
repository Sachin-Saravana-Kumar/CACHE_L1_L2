#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "sim.h"
#include <math.h>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <sstream>
#include <cstdint>
#include <bitset>
/*--------------------------------------------------------------------------------------------------------------------------------------------------*/


class cache {
public:
    int rows;
    int cols;
    int L1_read_misses;
    int L1_write_misses;
    int L1_write_back;
    int L2_read_demand;
    int L2_read_misses;
    int L2_write_misses;
    int L2_write_back;
    int L2_write;
    int L2_read_hit;
    int main_traf;
    int blocksize;
    int no_of_cache;
    uint32_t** data;
    int** LRU;
    bool** valid;
    bool** dirty;

    cache(int r, int c, int b) : rows(r), cols(c), blocksize(b) {
        L1_read_misses = 0;
        L1_write_misses = 0;
        L1_write_back = 0;
        L2_read_misses = 0;
        L2_write_misses = 0;
        L2_write = 0;
        L2_write_back = 0;
        L2_read_demand = 0;
        L2_read_hit = 0;
        main_traf = 0;
        valid = new bool*[rows];
        dirty = new bool*[rows];
        LRU   = new int*[rows];
        data = new uint32_t*[rows];
        for (int i = 0; i < rows; ++i) {
            data[i] = new uint32_t[cols];
            valid[i] = new bool[cols];
            dirty[i] = new bool[cols];
            LRU[i]   = new int[cols];
            for (int j = 0; j < cols; ++j) {
                data[i][j] = 0;
                valid[i][j] = 0;
                dirty[i][j] = 0;
                LRU[i][j] = j;
            }
        }
    }

    ~cache() {
        for (int i = 0; i < rows; ++i) {
            delete[] data[i];
            delete[] valid[i];
            delete[] dirty[i];
            delete[] LRU[i];
        }
        delete[] data;
        delete[] valid;
        delete[] dirty;
        delete[] LRU;

    }

    void getData(const uint32_t address,const char rw, cache& L2) {
        uint32_t set = set_add(address);
        uint32_t tag = tag_add(address);

        if (check_cache_hit(set, tag,rw, true)) {
            //printf("l1 hit\n");
        } 
        else {
            //printf("L1 miss\n");
            
            L2_read_demand++;

            int placed = 0;

            //int placed = handle_cache_miss(address,set, tag, rw,L2);
            if(L2.check_cache_hit(L2.set_add(address),L2.tag_add(address),rw, false)){
                L2_read_hit++;
                placed = handle_cache_miss(set, tag, rw, true,L2);
            }
            else{
                L2_read_misses++;  // Increment L2 read miss (demand) counter
                main_traf++; 
                int l2_placed = L2.handle_cache_miss(L2.set_add(address),L2.tag_add(address), rw, false,L2);
                if(l2_placed){
                    placed = handle_cache_miss(set, tag, rw, true,L2);
                }   
            }
        }
    }

    void display() const {
        for (int i = 0; i < rows; ++i) {
            printf("\n %d",i);
            for (int j = 0; j < cols; ++j) {
                printf(" %d %x %d ", LRU[i][j], data[i][j],dirty[i][j]);
            }
        }
    }

    void display1() const {
        for (int i = 0; i < rows; ++i) {
            printf("\n");
            for (int j = 0; j < cols; ++j) {
                printf("%x ", LRU[i][j]);
            }
        }
    }



private:
    uint32_t set_add(const uint32_t address) const {
        int no_index_bit  = log2(rows);
        int no_offset_bit = log2(blocksize);
        int tag_index = address >> no_offset_bit;
        return tag_index & ((1ULL << no_index_bit) - 1); // Calculate set.
    }

    uint32_t tag_add(const uint32_t address) const {
        int no_index_bit = log2(rows);
        int no_offset_bit = log2(blocksize);
        return address >> (no_offset_bit + no_index_bit); // Calculate tag.
    }

    bool check_cache_hit(uint32_t set, uint32_t tag,const char rw,bool l1)  { //const
        for (int j = 0; j < cols; j++) {
            if (data[set][j] == tag) {
                if(l1){
                write(set,j,rw,tag);
                }
                LRU_miss_change(true,set,j);
                return true; // Hit found.
            }
        }
        return false; // No hit.
    }

    bool handle_cache_miss(uint32_t set, uint32_t tag, const char rw,bool l1,cache& L2) {
        bool placed = false;
        for (int j = 0; j < cols; j++) {
            if (LRU_miss_no(set, j)) {
                if(l1){
                miss_count(rw);
                writeback(set, tag, j,L2);
                write(set,j,rw,tag); // write only at w cmd
                }
                else{
                    if(dirty[set][j]== 1){
                    L2.L2_write_back++;
                    main_traf++;
                    dirty[set][j] = 0;
                    }
                    //miss_count(rw);
                }
                
                data[set][j] = tag; // Place tag in the Least recently used block
                valid[set][j] = 1;
                placed = true;
                LRU_miss_change(false,set,j);
                break;

            }
        }
        return placed;
        /*if (!placed) {
            data[set][0] = tag; // Replace the first block if no empty slot was found.
        }*/
    }

    bool LRU_miss_no(uint32_t set,int j){
        return LRU[set][j] == (cols - 1);
        
    }


    void LRU_miss_change(bool hit,uint32_t set,int i){
        int old_value;
        old_value = LRU[set][i];
            for (int j = 0; j < cols; j++) {
            if (LRU[set][j] < old_value) {
                LRU[set][j]++;
            }
        }
        LRU[set][i] = 0; // Most recently used


        /*for(int j=0;j<cols;j++){
            if(j == i){
                LRU[set][j] = 0;
            }
            else{
                LRU[set][j]++;
            }
        }
        }
        else{
        old_value = LRU[set][i];
        LRU[set][i] = 0;
        for(int j=0;j<cols;j++){
            if(j != i && LRU[set][j] < old_value){
                LRU[set][j]++;
            }
        }
        }*/
       
        for(int j=0;j<cols;j++){
            for(int i=0;i<cols;i++){
            if(LRU[set][i] == j){
            int d = data[set][i];
            int l = LRU[set][i];
            int dir = dirty[set][i];
            data[set][i] = data[set][j];
            data[set][j] = d;
            LRU[set][i] = LRU[set][j];
            LRU[set][j] = l;
            dirty[set][i] = dirty[set][j];
            dirty[set][j] = dir;}  }}
    }
    
    

    void write(uint32_t set, int j,const char rw, uint32_t tag){
        if(rw == 'w'){
        dirty[set][j] = 1;}
        else if(rw == 'r' && dirty[set][j] == 1 && data[set][j] == tag){
        dirty[set][j] = 1;
        }
        else {
        
            dirty[set][j] = 0;
        }
    }

    uint32_t find_address(uint32_t oldTag, uint32_t set){
        uint32_t tag_set = (oldTag << static_cast<uint32_t>(log2(rows))) | set;
        uint32_t address = tag_set <<  static_cast<uint32_t>(log2(blocksize));
        return address;
    }

    void writeback(uint32_t set, uint32_t tag, int j,cache& L2){

        if(dirty[set][j] == 1){
        L1_write_back++;
        L2_write++;
        L2.handlewriteback(find_address(data[set][j], set));
        dirty[set][j] = 0;
        //printf("handlewriteback");
        }
    }

    void handlewriteback(uint32_t address){
        uint32_t set = set_add(address);
        uint32_t tag = tag_add(address);
        bool placed = false;

        for (int j = 0; j < cols; j++){
            if(data[set][j] == tag){
            dirty[set][j] = true;
            LRU_miss_change(true,set,j);
            //printf("handlewriteback hit");
            placed = true;
            break;

        }
        }
        if(!placed){
            for (int j = 0; j < cols; j++){
            if (LRU_miss_no(set, j)) {
                L2_write_misses++;
                main_traf++;
                data[set][j] = tag; // Place tag in the Least recently used block
                dirty[set][j] = 1;
                LRU_miss_change(false,set,j);
                //printf("handlewriteback miss");
                valid[set][j] = 1;
                placed = true;
                break;

            }
        }
        //else{

        //}
    }
}

void miss_count(const char rw){
    if(rw == 'r'){
        L1_read_misses++;
    }
    else if (rw == 'w'){
        L1_write_misses++;
    }
}
};



/*--------------------------------------------------------------------------------------------------------------------------------------------------*/
/*  "argc" holds the number of command-line arguments.
    "argv[]" holds the arguments themselves.

    Example:
    ./sim 32 8192 4 262144 8 3 10 gcc_trace.txt
    argc = 9
    argv[0] = "./sim"
    argv[1] = "32"
    argv[2] = "8192"
    ... and so on
*/


int main (int argc, char *argv[]) {
   int l1_col, l2_col, read , write ,t1_no_sets, t2_no_sets, blocksize;
   FILE *fp;			// File pointer.
   char *trace_file;		// This variable holds the trace file name.
   cache_params_t params;	// Look at the sim.h header file for the definition of struct cache_params_t.
   char rw;			// This variable holds the request's type (read or write) obtained from the trace.
   uint32_t addr;		// This variable holds the request's address obtained from the trace.
				// The header file <inttypes.hit> above defines signed and unsigned integers of various sizes in a machine-agnostic way.  "uint32_t" is an unsigned integer of 32 bits.

   // Exit with an error if the number of command-line arguments is incorrect.
   
   if (argc != 9) {
      printf("Error: Expected 8 command-line arguments but was provided %d.\n", (argc - 1));
      exit(EXIT_FAILURE);
    }
    
   // "atoi()" (included by <stdlib.h>) converts a string (char *) to an integer (int).

                                                                                                                                                                                                                                                                                                           

   params.BLOCKSIZE = (uint32_t) atoi(argv[1]);
   params.L1_SIZE   = (uint32_t) atoi(argv[2]);
   params.L1_ASSOC  = (uint32_t) atoi(argv[3]);
   params.L2_SIZE   = (uint32_t) atoi(argv[4]);
   params.L2_ASSOC  = (uint32_t) atoi(argv[5]);
   params.PREF_N    = (uint32_t) atoi(argv[6]);
   params.PREF_M    = (uint32_t) atoi(argv[7]);
   trace_file       = argv[8];


   // Open the trace file for reading.


   fp = fopen(trace_file, "r");
   if (fp == (FILE *) NULL) {
      // Exit with an error if file open failed.
      printf("Error: Unable to open file %s\n", trace_file);
      exit(EXIT_FAILURE);
   }


   // Print simulator configuration.
   printf("===== Simulator configuration =====\n");
   printf("BLOCKSIZE:  %u\n", params.BLOCKSIZE);
   printf("L1_SIZE:    %u\n", params.L1_SIZE  );
   printf("L1_ASSOC:   %u\n", params.L1_ASSOC );
   printf("L2_SIZE:    %u\n", params.L2_SIZE  );
   printf("L2_ASSOC:   %u\n", params.L2_ASSOC );
   printf("PREF_N:     %u\n", params.PREF_N );
   printf("PREF_M:     %u\n", params.PREF_M);
   printf("trace_file: %s\n", trace_file);
   printf("\n");

    t1_no_sets = params.L1_SIZE/((params.L1_ASSOC)*(params.BLOCKSIZE));
    t2_no_sets=params.L2_SIZE/((params.L2_ASSOC)*(params.BLOCKSIZE));
    blocksize = params.BLOCKSIZE;
    
    l1_col=params.L1_ASSOC ;
    l2_col=params.L2_ASSOC ;

    cache L1(t1_no_sets,l1_col,blocksize);
    cache L2(t2_no_sets,l2_col,blocksize);
    read=0;
    write=0;
   // Read requests from the trace file and echo them back.
   while (fscanf(fp, "%c %x\n", &rw, &addr) == 2) {	// Stay in the loop if fscanf() successfully parsed two tokens as specified.
      if (rw == 'r'){
        read++;
         // printf("r %x\n", addr);
         L1.getData(addr,rw,L2);
         }
      else if (rw == 'w'){
        write++;
         // printf("w %x\n", addr);
         L1.getData(addr,rw,L2);
         }
      else {
         printf("Error: Unknown request type %c.\n", rw);
	 exit(EXIT_FAILURE);
      }
    }

    L1.display();
    //L1.display1();

    //printf("\nL1_L2_TRAFFIC, %d",L1.);
    L2.display();
    printf("read : %d",read);
    printf("write : %d",write);
    printf("\n L1 read misses, %d",L1.L1_read_misses);
    printf("\n L1 write misses, %d",L1.L1_write_misses);
    printf("\n L1 write back, %d",L1.L1_write_back);
    printf("\n L2 read misses, %d",(L1.L2_read_misses+L2.L2_read_misses));
    printf("\n L2 read demand, %d",L1.L2_read_demand);
    printf("\n L2 read hit, %d",L1.L2_read_hit);
    printf("\n L2 write demand, %d",L1.L2_write);
    printf("\n L2 write misses, %d",L2.L2_write_misses);
    printf("\n L2 write back, %d",(L2.L2_write_back));
    printf("\n main traf, %d",(L2.main_traf+L1.main_traf));

    //L1.displaylru();
    //L2.displaylru();
    return(0);
}

/*
r 1004bb18
r 10048f96
w 1000f972
w 1004bb18
w 1000f972*/
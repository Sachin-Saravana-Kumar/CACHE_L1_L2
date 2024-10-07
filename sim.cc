#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "sim.h"
//#include <math.h>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <sstream>
#include <cstdint>
#include <bitset>

class Cache{
    public:
    int rows;
    int cols;
    uint32_t** data;
    int** LRU;
    bool** valid;
    bool** dirty;
    int read;
    int write;
    int read_misses;
    int write_misses;
    int write_back;
    int read_demand;
    int main_traf;
    int blocksize;

    //virtual void Check_Cache(uint32_t address, char rw, Cache* nextLevelCache) = 0;
    //virtual void handleWriteback(uint32_t address) = 0;

    Cache(int r, int c, int b) : rows(r), cols(c), blocksize(b) {
        read_misses = 0;
        write_misses = 0;
        write_back = 0;
        read = 0;
        write = 0;
        main_traf = 0;
        read_demand=0;
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

    virtual ~Cache() {
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


    int LRU_value(uint32_t set){
        for(int j = 0; j< cols; j++){    
            if (LRU[set][j] == cols - 1) {
                return j;  // Least recently used
            }
        }
        return -1;
    }

    void LRU_update(bool hit,uint32_t set,int i){
        int old_value;
        //old_value = LRU[set][i];
        old_value = LRU[set][i];
        for (int j = 0; j < cols; j++) {
            if(LRU[set][j] < old_value){
                LRU[set][j]++;
                
            }   
        }
        LRU[set][i] = 0; // Most recently used
        rearrange(set);
    }

    void rearrange(uint32_t set){

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
            dirty[set][j] = dir;}  
            }
            }
    }
    

   void miss_count(const char rw){
    if(rw == 'r'){
        read_misses++;
    }
    else if (rw == 'w'){
        write_misses++;
    }
}
    void count_rw(const char rw){
        if(rw == 'r'){
            read++;
        }
        else if (rw =='w'){
            write++;
        }
    }

    virtual void Check_Cache(uint32_t address, char rw, Cache* nextLevelCache,Cache* MainMemory)  {
        bool hit = false;
        int lru_index;
        uint32_t set = set_add(address);
        uint32_t tag = tag_add(address);
        count_rw(rw);
        for (int j = 0; j < cols; j++) {
            if (data[set][j] == tag) {
                hit = true;
                if (rw == 'w') {
                    dirty[set][j] = 1;  // Mark block as dirty on write
                }
                data[set][j] = tag;
                LRU_update(hit,set, j);
                return;  // Cache hit
            }
        }
        if(!hit){
            miss_count(rw);
            main_traf++;
            lru_index = LRU_value(set);
            if(dirty[set][lru_index]){
                write_back++;
                if (nextLevelCache) {
                uint32_t oldaddress = find_address(data[set][lru_index],set);
                nextLevelCache->Check_Cache(oldaddress,'w',MainMemory,nullptr); //handlewrite 
            }
                dirty[set][lru_index] = 0;
            }
            if(nextLevelCache){
                nextLevelCache->Check_Cache(address, 'r',MainMemory,nullptr);}
            }

            data[set][lru_index] = tag;
            if (rw == 'w') {
                dirty[set][lru_index] = 1;  // Mark block as dirty on write
                }
            valid[set][lru_index] = true;
            LRU_update(false,set,lru_index);
        }
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

    uint32_t find_address(uint32_t oldTag, uint32_t set){
        uint32_t tag_set = (oldTag << static_cast<uint32_t>(log2(rows))) | set;
        uint32_t address = tag_set <<  static_cast<uint32_t>(log2(blocksize));
        return address;
    }

    void display(const char* cacheName) const {
        printf("===== %s contents =====",cacheName);

        for (int i = 0; i < rows; ++i) {
            printf("\nset     %2d: ", i);

            for (int j = 0; j < cols; ++j) {
                printf("  %8x",data[i][j]);

                if(dirty[i][j]){
                    printf(" D");
                }
                else{
                    printf("  ");
                }
            }
        }
        printf("\n");
            
    }

    
};

    void printMeasurements(Cache* L1,Cache* L2,Cache* MM) {
        //double L2_missrate;
        //double L1_missrate =(L1->read_misses+L1->write_misses)/(L1->read+L1->write);
        printf("\n");
    printf("===== Measurements =====\n");
    printf("a. L1 reads:                   %d\n", L1->read);
    printf("b. L1 read misses:             %d\n", L1->read_misses);
    printf("c. L1 writes:                  %d\n", L1->write);
    printf("d. L1 write misses:            %d\n", L1->write_misses);
    printf("e. L1 miss rate:               %.4f\n",(float)(L1->read_misses+L1->write_misses)/(L1->read+L1->write));
    printf("f. L1 writebacks:              %d\n", L1->write_back);
    printf("g. L1 prefetches:              %d\n", 0);
    if(L2){
        //L2_missrate = (L2->read_misses/L2->read);
    printf("h. L2 reads (demand):          %d\n", L2->read);
    printf("i. L2 read misses (demand):    %d\n", L2->read_misses);
    printf("j. L2 reads (prefetch):        %d\n", 0);
    printf("k. L2 read misses (prefetch):  %d\n", 0);
    printf("l. L2 writes:                  %d\n", L2->write);
    printf("m. L2 write misses:            %d\n", L2->write_misses);
    printf("n. L2 miss rate:               %.4f\n",(float)(L2->read_misses)/(L2->read));
    printf("o. L2 writebacks:              %d\n", L2->write_back);
    printf("p. L2 prefetches:              %d\n", 0);}
    else{
    printf("h. L2 reads (demand):          %d\n", 0);
    printf("i. L2 read misses (demand):    %d\n", 0);
    printf("j. L2 reads (prefetch):        %d\n", 0);
    printf("k. L2 read misses (prefetch):  %d\n", 0);
    printf("l. L2 writes:                  %d\n", 0);
    printf("m. L2 write misses:            %d\n", 0);
    printf("n. L2 miss rate:               %.4f\n",0.000);
    printf("o. L2 writebacks:              %d\n", 0);
    printf("p. L2 prefetches:              %d\n", 0);
    }
    printf("q. memory traffic:             %d\n", MM->main_traf);
}



class L1Cache : public Cache {
public:
    L1Cache(int r, int c, int b) : Cache(r, c, b) {}

    // L1 specific behavior can be added here
};

// Derived class for L2 Cache
class L2Cache : public Cache {
public:
    L2Cache(int r, int c, int b) : Cache(r, c, b) {}


  
};

class MainMemory : public Cache {
public:
    MainMemory(int r, int c, int b) : Cache(r, c, b) {}

    void Check_Cache(uint32_t address, char rw, Cache* nextLevelCache, Cache* MainMemory) override {
        main_traf++;
    }



};

int main (int argc, char *argv[]) {
   int l1_col, l2_col,L1_no_sets, L2_no_sets, blocksize;
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

    L1_no_sets = params.L1_SIZE/((params.L1_ASSOC)*(params.BLOCKSIZE));
    blocksize = params.BLOCKSIZE;
    
    l1_col=params.L1_ASSOC ;
    l2_col=params.L2_ASSOC ;
    
    L1Cache L1(L1_no_sets,l1_col,blocksize);
    MainMemory MM(1,1,blocksize);
     
    if(l2_col > 0 &&   params.L2_SIZE > 0){
    L2_no_sets=params.L2_SIZE/((params.L2_ASSOC)*(params.BLOCKSIZE));
    L2Cache L2(L2_no_sets,l2_col,blocksize);   
    while (fscanf(fp, "%c %x\n", &rw, &addr) == 2) {	// Stay in the loop if fscanf() successfully parsed two tokens as specified.
      if (rw == 'r' || rw == 'w'){
         L1.Check_Cache(addr,rw,&L2,&MM);}
      else {
         printf("Error: Unknown request type %c.\n", rw);
	 exit(EXIT_FAILURE);
      }
    }
    L1.display("L1");
    printf("\n");
    L2.display("L2");
    printMeasurements(&L1,&L2,&MM); 
    }
    else{
    while (fscanf(fp, "%c %x\n", &rw, &addr) == 2) {	// Stay in the loop if fscanf() successfully parsed two tokens as specified.
      if (rw == 'r' || rw == 'w'){
         L1.Check_Cache(addr,rw,&MM,nullptr);}
      else {
         printf("Error: Unknown request type %c.\n", rw);
	 exit(EXIT_FAILURE);
      }
    }
    L1.display("L1");
    printMeasurements(&L1,nullptr,&MM); 
    }
    
    
    //L1.display1();
    //printf("\n main traf, %d",(MM.main_traf));
    //L1.displaylru();
    //L2.displaylru();
    return(0);
}


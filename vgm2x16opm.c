#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define DELAY_REG 2
#define DONE_REG  4
#define VGM_DO    0x34

int loadmusic(FILE *ofp, const char *vgm_fn) {
   FILE *ifp;

   int delay;
   uint8_t idata[4];
   uint8_t odata[2];
   int data_offset;
   int done = 0;
   double clock_factor;

   ifp = fopen(vgm_fn, "rb");
   if (ifp == NULL) {
      printf("Error opening %s for reading\n", vgm_fn);
      return -1;
   }

   // roll up to beginning of data
   if (fread(idata,1,4,ifp) < 4) {
      // empty file passed, just add DONE instruction and return quietly
      odata[0] = DONE_REG;
      odata[1] = 0;
      fwrite(&odata,1,2,ofp);
      fclose(ifp);
      return 0;
   }

   if (strncmp(idata,"Vgm ",4) != 0) {
      printf("%s is not a valid VGM file.\n", vgm_fn);
      fclose(ifp);
      return -1;
   }

   fseek(ifp,VGM_DO,SEEK_SET);
   fread(idata,1,4,ifp);
   data_offset = VGM_DO + (int)idata[0] + (((int)idata[1]) << 8) +
                  (((int)idata[2]) << 16) + (((int)idata[3]) << 24);
   fseek(ifp,data_offset,SEEK_SET);

   while (!feof(ifp) && !done) {
      fread(idata,1,1,ifp);
      switch (idata[0]) {
         case 0x54: // YM2151 - just dump right to output
            fread(odata,1,2,ifp);
            fwrite(odata,1,2,ofp);
            break;
         case 0x61: // Wait # samples
            fread(idata,1,2,ifp);
            odata[0] = DELAY_REG;
            delay = ((int)idata[0] + (((int)idata[1]) << 8))/735;
            // write as delay of VSCAN ticks
            odata[1] = (uint8_t)delay;
            fwrite(odata,1,2,ofp);
            break;
         case 0x62: // Wait 735 samples (1 NTSC VSYNC tick)
            odata[0] = DELAY_REG;
            odata[1] = 1;
            fwrite(odata,1,2,ofp);
            break;
         case 0x66: // end of sound data
            odata[0] = DONE_REG;
            odata[1] = 0;
            fwrite(odata,1,2,ofp);
            done = 1;
            break;
         case 0xc0: // Sega PCM (expected from Deflemask)
            fread(idata,1,3,ifp); // ignore data
            break;
         case 0x67: // data block - just skip over it
            fread(idata, 1, 2, ifp); // ignore fake end byte, data type
            fread(idata, 1, 4, ifp); // read block length
            data_offset = (int)idata[0] + (((int)idata[1]) << 8) +
            	(((int)idata[2]) << 16) + (((int)idata[3]) << 24);
            printf("Skipping %d byte data block\n",data_offset);
            fseek(ifp, data_offset, SEEK_CUR);
            break;
         default:
            // TODO: support other sounds chips to include or ignore
            // assume any other data code to be single-byte
            printf("Unexpected code: 0x%02X\n", idata[0]);
            break;
      }
   }

   fclose(ifp);

   return 0;
}

int main(int argc, char **argv) {
   FILE *ofp;
   uint8_t odata[2];
   int address = 0x0000;

   if (argc < 4) {
      printf("Usage: %s [intro VGM file] [loop VGM file] [converted binary file]\n", argv[0]);
      return 0;
   }

   ofp = fopen(argv[3], "wb");
   if (ofp == NULL) {
      printf("Error opening %s for writing\n", argv[2]);
      return -1;
   }

   odata[0] = (uint8_t) (address & 0x00FF);
   odata[1] = (uint8_t) ((address & 0xFF00) >> 8);
   fwrite(&odata,1,2,ofp);

   loadmusic(ofp, argv[1]); // Intro
   loadmusic(ofp, argv[2]); // Loop

   fclose(ofp);

   return 0;
}
